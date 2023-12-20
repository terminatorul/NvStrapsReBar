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
#include "SetupNvStraps.h"

#include "ReBar.h"

// if system time is before this year then CMOS reset will be detected and rebar will be disabled.
static unsigned const BUILD_YEAR = 2023u;
CHAR16 reBarStateVarName[] = u"NvStrapsReBar";

// 481893f5-2436-4fd5-9d5a-69b121c3f0ba
static GUID reBarStateGuid = { 0x481893f5, 0x2436, 0x4fd5, { 0x9d, 0x5a, 0x69, 0xb1, 0x21, 0xc3, 0xf0, 0xba } };

// for quirk
static UINT16 const PCI_VENDOR_ID_ATI = 0x1002u;

// 0: disabled
// >0: maximum BAR size (2^x) set to value. 32 for unlimited, 64 for selected GPU only
static UINT8 reBarState = 0;

static EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *pciResAlloc;

static EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_PREPROCESS_CONTROLLER o_PreprocessController;

EFI_HANDLE reBarImageHandle = NULL;

// find last set bit and return the index of it
uint_least8_t highestBitIndex(uint_least32_t val)
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

static uint_least32_t getReBarSizeMask(UINTN pciAddress, uint_least16_t capabilityOffset, UINT16 vid, UINT16 did, uint_least8_t barIndex)
{
    uint_least32_t barSizeMask = pciRebarGetPossibleSizes(pciAddress, capabilityOffset, vid, did, barIndex);

    /* Sapphire RX 5600 XT Pulse has an invalid cap dword for BAR 0 */
    if (vid == PCI_VENDOR_ID_ATI && did == 0x731fu && barIndex == PCI_BAR_IDX0 && barSizeMask == 0x7000u)
        barSizeMask = 0x3f000u;
    else
        if (NvStraps_CheckBARSizeListAdjust(pciAddress, vid, did, barIndex))
            barSizeMask = NvStraps_AdjustBARSizeList(pciAddress, vid, did, barIndex, barSizeMask);

    return barSizeMask;
}

void reBarSetupDevice(EFI_HANDLE handle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addrInfo)
{
    UINT16 vid, did;
    UINTN pciAddress = pciLocatedDevice(handle, addrInfo, &vid, &did);

    if (vid == MAX_UINT16)
        return;

    DEBUG((DEBUG_INFO, "ReBarDXE: Device vid:%x did:%x\n", vid, did));

    NvStraps_EnumDevice(vid, did, &addrInfo);

    if (NvStraps_CheckDevice(vid, did, &addrInfo))
    {
        SetStatusVar(StatusVar_GpuFound);
        NvStraps_Setup(handle, vid, did, &addrInfo, reBarState);
    }

    if (1u <= reBarState && reBarState <= 32u)
    {
        uint_least16_t capOffset = pciFindExtCapability(pciAddress, PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID);

        if (capOffset)
            for (uint_least8_t barIndex = 0u; barIndex < PCI_MAX_BAR; barIndex++)
            {
                uint_least32_t nBarSizeMask = getReBarSizeMask(pciAddress, capOffset, vid, did, barIndex);

                if (nBarSizeMask)
                    for (uint_least8_t barSizeBitIndex = min(highestBitIndex(nBarSizeMask), reBarState); barSizeBitIndex > 0u; barSizeBitIndex--)
                        if (nBarSizeMask & 1u << barSizeBitIndex)
                        {
                            pciRebarSetSize(pciAddress, capOffset, barIndex, barSizeBitIndex);
                            break;
                        }
            }
    }
}

EFI_STATUS EFIAPI PreprocessControllerOverride
    (
        IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
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

void pciHostBridgeResourceAllocationProtocolHook()
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
        SetStatusVar(StatusVar_EFIError);
        goto free;
    }

    status = gBS->OpenProtocol(
        handleBuffer[0],
        &gEfiPciHostBridgeResourceAllocationProtocolGuid,
        (VOID **)&pciResAlloc,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(status))
    {
        SetStatusVar(StatusVar_EFIError);
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

EFI_STATUS EFIAPI rebarInit(IN EFI_HANDLE imageHandle, IN EFI_SYSTEM_TABLE *systemTable)
{
    UINTN bufferSize = 1u;
    EFI_STATUS status;
    UINT32 attributes;
    EFI_TIME time;

    DEBUG((DEBUG_INFO, "ReBarDXE: Loaded\n"));

    reBarImageHandle = imageHandle;
    status = gRT->GetVariable(reBarStateVarName, &reBarStateGuid, &attributes, &bufferSize, &reBarState);

    // any attempts to overflow reBarState should result in EFI_BUFFER_TOO_SMALL
    if (status != EFI_SUCCESS)
    {
        SetStatusVar(status == EFI_NOT_FOUND ? StatusVar_Unconfigured : StatusVar_EFIError);
        reBarState = 0u;
    }

    if (reBarState)
    {
        DEBUG((DEBUG_INFO, "ReBarDXE: Enabled, maximum BAR size 2^%u MB\n", reBarState));

        // Detect CMOS reset by checking if year before BUILD_YEAR
        status = gRT->GetTime (&time, NULL);

        if (time.Year < BUILD_YEAR)
        {
            reBarState = 0u;
            bufferSize = 1u;
            attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

            status = gRT->SetVariable(reBarStateVarName, &reBarStateGuid, attributes, bufferSize, &reBarState);

            SetStatusVar(StatusVar_Cleared);
            return EFI_SUCCESS;
        }

        SetStatusVar(StatusVar_Configured);
        // For overriding PciHostBridgeResourceAllocationProtocol
        pciHostBridgeResourceAllocationProtocolHook();
    }
    else
        SetStatusVar(StatusVar_Unconfigured);

    return EFI_SUCCESS;
}
