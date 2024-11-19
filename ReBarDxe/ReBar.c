/*
Copyright (c) 2022-2023 xCuri0 <zkqri0@gmail.com>
SPDX-License-Identifier: MIT
*/

#include <stdint.h>

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/PciExpress21.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Library/MemoryAllocationLib.h>

#if defined(_ASSERT)
# undef _ASSERT
#endif

#include <Library/DebugLib.h>

#include "LocalAppConfig.h"
#include "StatusVar.h"
#include "PciConfig.h"
#include "S3ResumeScript.h"
#include "NvStrapsConfig.h"
#include "SetupNvStraps.h"
#include "CheckSetupVar.h"

#include "ReBar.h"

// if system time is before this year then CMOS reset will be detected and rebar will be disabled.
static unsigned const BUILD_YEAR = 2024u;

// for quirk
static uint_least16_t const
    PCI_VENDOR_ID_AMD			 = 0x1002u,
    PCI_DEVICE_Sapphire_RX_5600_XT_Pulse = 0x731Fu;

// 0: disabled
// >0: maximum BAR size (2^x) set to value. 32 for unlimited, 64 for selected GPU only
static uint_least8_t nPciBarSizeSelector = TARGET_PCI_BAR_SIZE_DISABLED;

static EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *pciResAlloc;

static EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_PREPROCESS_CONTROLLER o_PreprocessController;

EFI_HANDLE reBarImageHandle = NULL;
NvStrapsConfig *config = NULL;

// find highest-order bit set and return the bit index
static inline uint_least8_t highestBitIndex(uint_least32_t val)
{
    uint_least8_t bitIndex = (uint_least8_t)(sizeof val * BYTE_BITSIZE - 1u);
    uint_least32_t checkBit = (uint_least32_t)1u  << bitIndex;

    while (bitIndex && (val & checkBit) == 0u)
        bitIndex--, checkBit >>= 1u;

    return bitIndex;
}

static inline uint_least8_t min(uint_least8_t val1, uint_least8_t val2)
{
    return val1 < val2 ? val1 : val2;
}

uint_least32_t getReBarSizeMask(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t barIndex)
{
    uint_least32_t barSizeMask = pciRebarGetPossibleSizes(pciAddress, capabilityOffset, vid, did, barIndex);

    /* Sapphire RX 5600 XT Pulse has an invalid cap dword for BAR 0 */
    if (vid == PCI_VENDOR_ID_AMD && did == PCI_DEVICE_Sapphire_RX_5600_XT_Pulse && barIndex == PCI_BAR_IDX0 && barSizeMask == 0x7000u)
        barSizeMask = 0x3'F000u;
    else
        if (NvStraps_CheckBARSizeListAdjust(pciAddress, vid, did, subsysVenID, subsysDevID, barIndex))
            barSizeMask = NvStraps_AdjustBARSizeList(pciAddress, vid, did, subsysVenID, subsysDevID, barIndex, barSizeMask);

    return barSizeMask;
}

static void reBarSetupDevice(EFI_HANDLE handle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addrInfo)
{
    uint_least16_t vid, did;
    uint_least8_t headerType;
    UINTN pciAddress = pciLocateDevice(handle, addrInfo, &vid, &did, &headerType);

    if (vid == WORD_BITMASK)
        return;

    DEBUG((DEBUG_INFO, "ReBarDXE: Device vid:%x did:%x\n", vid, did));

    NvStraps_EnumDevice(pciAddress, vid, did, headerType);

    uint_least16_t subsysVenID = WORD_BITMASK, subsysDevID = WORD_BITMASK;
    bool isSelectedGpu = NvStraps_CheckDevice(pciAddress, vid, did, &subsysVenID, &subsysDevID);

    if (isSelectedGpu)
        NvStraps_Setup(pciAddress, vid, did, subsysVenID, subsysDevID, nPciBarSizeSelector);

    if (TARGET_PCI_BAR_SIZE_MIN <= nPciBarSizeSelector && nPciBarSizeSelector <= TARGET_PCI_BAR_SIZE_MAX)
    {
        uint_least16_t const capOffset = pciFindExtCapability(pciAddress, PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID);

        if (capOffset)
            for (uint_least8_t barIndex = 0u; barIndex < PCI_MAX_BAR; barIndex++)
            {
                uint_least32_t nBarSizeMask = getReBarSizeMask(pciAddress, capOffset, vid, did, subsysVenID, subsysDevID, barIndex);

                if (nBarSizeMask)
                    for (uint_least8_t barSizeBitIndex = min(highestBitIndex(nBarSizeMask), nPciBarSizeSelector); barSizeBitIndex > 0u; barSizeBitIndex--)
                        if (nBarSizeMask & 1u << barSizeBitIndex)
                        {
                            bool resized = pciRebarSetSize(pciAddress, capOffset, barIndex, barSizeBitIndex);

                            if (isSelectedGpu && resized)
                                SetDeviceStatusVar(pciAddress, StatusVar_GpuReBarConfigured);

                            break;
                        }
            }
    }
}

static EFI_STATUS EFIAPI PreprocessControllerOverride
    (
        IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
        IN  EFI_HANDLE                                        RootBridgeHandle,
        IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS       PciAddress,
        IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE      Phase
    )
{
    // call the original method
    EFI_STATUS status = o_PreprocessController(This, RootBridgeHandle, PciAddress, Phase);

    DEBUG((DEBUG_INFO, "ReBarDXE: Hooked PreprocessController called %d\n", Phase));

    // EDK2 PciBusDxe setups Resizable BAR twice so we will do same
    if (Phase <= EfiPciBeforeResourceCollection)
        reBarSetupDevice(RootBridgeHandle, PciAddress);

    return status;
}

static void pciHostBridgeResourceAllocationProtocolHook()
{
    EFI_STATUS status;
    UINTN handleCount;
    EFI_HANDLE *handleBuffer = NULL;

    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiPciHostBridgeResourceAllocationProtocolGuid,
        NULL,
        &handleCount,
        &handleBuffer);

    if (EFI_ERROR(status))
    {
        SetEFIError(EFIError_LocateBridgeProtocol, status);
        goto free;
    }

    status = gBS->OpenProtocol
    (
        handleBuffer[0],
        &gEfiPciHostBridgeResourceAllocationProtocolGuid,
        (VOID **)&pciResAlloc,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(status))
    {
        SetEFIError(EFIError_LoadBridgeProtocol, status);
        goto free;
    }

    DEBUG((DEBUG_INFO, "ReBarDXE: Hooking EfiPciHostBridgeResourceAllocationProtocol->PreprocessController\n"));

    // Hook PreprocessController
    o_PreprocessController = pciResAlloc->PreprocessController;
    pciResAlloc->PreprocessController = &PreprocessControllerOverride;

free:
    if (handleBuffer)
        FreePool(handleBuffer), handleBuffer = NULL;
}

static bool IsCMOSClear()
{
    // Detect CMOS reset by checking if year before BUILD_YEAR
    EFI_STATUS status;
    EFI_TIME time = { .Year = 0u };

    if (EFI_ERROR((status = gRT->GetTime(&time, NULL))))
        SetEFIError(EFIError_CMOSTime, status);

    return time.Year < BUILD_YEAR;
}

EFI_STATUS EFIAPI rebarInit(IN EFI_HANDLE imageHandle, IN EFI_SYSTEM_TABLE *systemTable)
{
    DEBUG((DEBUG_INFO, "ReBarDXE: Loaded\n"));

    reBarImageHandle = imageHandle;
    config = GetNvStrapsConfig(false, NULL);    // attempts to overflow EFI variable data should result in EFI_BUFFER_TOO_SMALL
    nPciBarSizeSelector = NvStrapsConfig_TargetPciBarSizeSelector(config);

    if (nPciBarSizeSelector == TARGET_PCI_BAR_SIZE_DISABLED && NvStrapsConfig_IsGpuConfigured(config))
        nPciBarSizeSelector = TARGET_PCI_BAR_SIZE_GPU_STRAPS_ONLY;
    else
        if (NvStrapsConfig_IsDriverConfigured(config) && !NvStrapsConfig_IsGpuConfigured(config))
            SetStatusVar(StatusVar_GPU_Unconfigured);

    if (nPciBarSizeSelector != TARGET_PCI_BAR_SIZE_DISABLED)
    {
        DEBUG((DEBUG_INFO, "ReBarDXE: Enabled, maximum BAR size 2^%u MiB\n", nPciBarSizeSelector));

    bool isSetupVarChanged = NvStrapsConfig_EnableSetupVarCRC(config) && IsSetupVariableChanged();

        if (isSetupVarChanged || IsCMOSClear())
        {
            NvStrapsConfig_Clear(config);
        NvStrapsConfig_SetIsDirty(config, true);

        if (!isSetupVarChanged)
        SaveNvStrapsConfig(NULL);

            SetStatusVar(StatusVar_Cleared);

            return EFI_SUCCESS;
        }

        SetStatusVar(StatusVar_Configured);

    S3ResumeScript_Init(NvStrapsConfig_IsGpuConfigured(config));
        pciHostBridgeResourceAllocationProtocolHook();          // For overriding PciHostBridgeResourceAllocationProtocol
    }
    else
        SetStatusVar(StatusVar_Unconfigured);

    return EFI_SUCCESS;
}

// vim:ft=cpp
