#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <Protocol/S3SaveState.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <IndustryStandard/Pci.h>

#include "StatusVar.h"
#include "PciConfig.h"
#include "S3ResumeScript.h"
#include "DeviceRegistry.h"
#include "NvStrapsConfig.h"
#include "ReBar.h"

#include "SetupNvStraps.h"

// From envytools documentation at:
// https://envytools.readthedocs.io/en/latest/hw/io/pstraps.html

static uint_least32_t const
    TARGET_GPU_STRAPS_BASE_OFFSET = 0x0010'1000u,
    TARGET_GPU_STRAPS_SET0_OFFSET = 0x0000'0000u,
    TARGET_GPU_STRAPS_SET1_OFFSET = 0x0000'000Cu,

    BAR1_SIZE_PART1_SHIFT = 14u,
    BAR1_SIZE_PART1_BITSIZE = 2u,

    BAR1_SIZE_PART2_SHIFT = 20u,
    BAR1_SIZE_PART2_BITSIZE = 3u;

static uint_least16_t enumeratedBridges[ARRAY_SIZE(config->bridge)] = { 0, };
static uint_least8_t enumeratedBridgeCount = 0u;

static bool isBridgeEnumerated(uint_least16_t pciLocation)
{
    for (unsigned index = 0u; index < enumeratedBridgeCount; index++)
    if (enumeratedBridges[index] == pciLocation)
        return true;

    return false;
}

void NvStraps_EnumDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least8_t headerType)
{
    if (pciIsPciBridge(headerType) && (enumeratedBridgeCount < ARRAY_SIZE(enumeratedBridges)))
    {
    uint_least8_t bus, dev, fun;
    pciUnpackAddress(pciAddress, &bus, &dev, &fun);

    if (NvStrapsConfig_HasBridgeDevice(config, bus, dev, fun) != ((uint_least32_t)WORD_BITMASK << WORD_BITSIZE | WORD_BITMASK))
    {
        enumeratedBridges[enumeratedBridgeCount++] = pciPackLocation(bus, dev, fun);
        SetStatusVar(StatusVar_BridgeFound);
    }
    }
}

bool NvStraps_CheckDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t *subsysVenID, uint_least16_t *subsysDevID)
{
    if (vendorId == TARGET_GPU_VENDOR_ID && NvStrapsConfig_IsGpuConfigured(config) && pciIsVgaController(pciDeviceClass(pciAddress)))
    {
    EFI_STATUS status = pciReadDeviceSubsystem(pciAddress, subsysVenID, subsysDevID);

    if (EFI_ERROR(status))
        return SetEFIError(EFIError_PCI_DeviceSubsystem, status), false;

        uint_least8_t bus, device, fun;

    pciUnpackAddress(pciAddress, &bus, &device, &fun);

        NvStraps_BarSize barSizeSelector =
            NvStrapsConfig_LookupBarSize(config, deviceId, *subsysVenID, *subsysDevID, bus, device, fun);

        if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        {
            SetDeviceStatusVar(pciAddress, barSizeSelector.barSizeSelector == BarSizeSelector_Excluded ? StatusVar_GpuExcluded : StatusVar_GPU_Unconfigured);
            return false;
        }

    SetDeviceStatusVar(pciAddress, StatusVar_GpuFound);

        return true;
    }

    return false;
}

static bool ConfigureNvStrapsBAR1Size(EFI_PHYSICAL_ADDRESS baseAddress0, UINT8 barSize)
{
    UINT32
        *pSTRAPS0 = (UINT32 *)(baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET0_OFFSET),
        *pSTRAPS1 = (UINT32 *)(baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET1_OFFSET),
        STRAPS0, STRAPS1;

    CopyMem(&STRAPS0, pSTRAPS0, sizeof STRAPS0);
    CopyMem(&STRAPS1, pSTRAPS1, sizeof STRAPS1);

    UINT8
        barSize_Part1 = STRAPS0 >> BAR1_SIZE_PART1_SHIFT & (UINT32_C(1) << BAR1_SIZE_PART1_BITSIZE) - 1u,
        barSize_Part2 = STRAPS1 >> BAR1_SIZE_PART2_SHIFT & (UINT32_C(1) << BAR1_SIZE_PART2_BITSIZE) - 1u;

    UINT8
        targetBarSize_Part1 = barSize < 3u ? barSize : barSize < 10u ? 2u           : 3u,
        targetBarSize_Part2 = barSize < 3u ? 0u      : barSize < 10u ? barSize - 2u : 7u;

    if (barSize_Part1 != targetBarSize_Part1)
    {
        STRAPS0 &= ~(UINT32)(((UINT32_C(1) << BAR1_SIZE_PART1_BITSIZE) - 1u) << BAR1_SIZE_PART1_SHIFT);
        STRAPS0 |= (UINT32)targetBarSize_Part1 << BAR1_SIZE_PART1_SHIFT;
        STRAPS0 |= UINT32_C(1) << (DWORD_SIZE * BYTE_BITSIZE - 1u);

        CopyMem(pSTRAPS0, &STRAPS0, sizeof STRAPS0);

    EFI_STATUS status = S3ResumeScript_MemReadWrite_DWORD
        (
        (UINT64)baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET0_OFFSET,
        (UINT32)targetBarSize_Part1 << BAR1_SIZE_PART1_SHIFT | UINT32_C(1) << (DWORD_SIZE * BYTE_BITSIZE - 1u),
        (UINT32) ~(UINT32)(((UINT32_C(1) << BAR1_SIZE_PART1_BITSIZE) - 1u) << BAR1_SIZE_PART1_SHIFT)
        );

    if (EFI_ERROR(status))
        SetEFIError(EFIError_WriteS3SaveStateProtocol, status);
    }

    if (barSize_Part2 != targetBarSize_Part2)
    {
        STRAPS1 &= ~(UINT32)(((UINT32_C(1) << BAR1_SIZE_PART2_BITSIZE) - 1u) << BAR1_SIZE_PART2_SHIFT);
        STRAPS1 |= (UINT32)targetBarSize_Part2 << BAR1_SIZE_PART2_SHIFT;
        STRAPS1 |= UINT32_C(1) << (DWORD_SIZE * BYTE_BITSIZE - 1u);

        CopyMem(pSTRAPS1, &STRAPS1, sizeof STRAPS1);

    EFI_STATUS status = S3ResumeScript_MemReadWrite_DWORD
        (
        (UINT64)baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET1_OFFSET,
        (UINT32)targetBarSize_Part2 << BAR1_SIZE_PART2_SHIFT | UINT32_C(1) << (DWORD_SIZE * BYTE_BITSIZE - 1u),
        (UINT32) ~(UINT32)(((UINT32_C(1) << BAR1_SIZE_PART2_BITSIZE) - 1u) << BAR1_SIZE_PART2_SHIFT)
        );

    if (EFI_ERROR(status))
        SetEFIError(EFIError_WriteS3SaveStateProtocol, status);
    }

    return barSize_Part1 + barSize_Part2 != targetBarSize_Part1 + targetBarSize_Part2;
}

void NvStraps_Setup(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_fast8_t nPciBarSizeSelector)
{
    uint_least8_t bus, device, func;

    pciUnpackAddress(pciAddress, &bus, &device, &func);

    NvStraps_BarSize barSizeSelector =
        NvStrapsConfig_LookupBarSize(config, deviceId, subsysVenID, subsysDevID, bus, device, func);

    if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        return;

    NvStraps_BarSizeMaskOverride sizeMaskOverride = NvStrapsConfig_LookupBarSizeMaskOverride(config, deviceId, subsysVenID, subsysDevID, bus, device, func);

    NvStraps_GPUConfig const *gpuConfig = NvStrapsConfig_LookupGPUConfig(config, bus, device, func);

    if (!gpuConfig)
    {
    SetDeviceStatusVar(pciAddress, StatusVar_NoGpuConfig);
    return;
    }

    if (gpuConfig->bar0.base >= UINT32_MAX || gpuConfig->bar0.top >= UINT32_MAX || gpuConfig->bar0.base & UINT32_C(0x0000'000F)
        || gpuConfig->bar0.base % (gpuConfig->bar0.top - gpuConfig->bar0.base + 1u))
    {
    SetDeviceStatusVar(pciAddress, StatusVar_BadGpuConfig);
    return;
    }

    NvStraps_BridgeConfig const *bridgeConfig = NvStrapsConfig_LookupBridgeConfig(config, bus);

    if (!bridgeConfig)
    {
    SetDeviceStatusVar(pciAddress, StatusVar_NoBridgeConfig);
    return;
    }
    else
    if (!isBridgeEnumerated(pciPackLocation(bridgeConfig->bridgeBus, bridgeConfig->bridgeDevice, bridgeConfig->bridgeFunction)))
    {
        SetDeviceStatusVar(pciAddress, StatusVar_BridgeNotEnumerated);
        return;
    }

    UINTN bridgePciAddress = EFI_PCI_ADDRESS(bridgeConfig->bridgeBus, bridgeConfig->bridgeDevice, bridgeConfig->bridgeFunction, 0u);
    uint_least8_t bridgeSecondaryBus;
    EFI_STATUS status = pciBridgeSecondaryBus(bridgePciAddress, &bridgeSecondaryBus);

    if (EFI_ERROR(status))
    {
    SetDeviceEFIError(pciAddress, EFIError_PCI_BridgeSecondaryBus, status);
    return;
    }

    if (bridgeSecondaryBus != bus)
    {
    SetDeviceStatusVar(pciAddress, StatusVar_BadBridgeConfig);
    return;
    }

    UINT32 bridgeSaveArea[3u], gpuSaveArea[2u];

//    EFI_PHYSICAL_ADDRESS baseAddress0 = BASE_4GB - 1u, bridgeIoPortRangeBegin = BASE_64KB - 1u;

//    if (EFI_SUCCESS == gDS->AllocateMemorySpace(EfiGcdAllocateMaxAddressSearchTopDown, EfiGcdMemoryTypeMemoryMappedIo, 25u, SIZE_32MB, &baseAddress0, reBarImageHandle, NULL))
//    {
//        if (EFI_SUCCESS == gDS->AllocateIoSpace(EfiGcdAllocateMaxAddressSearchTopDown, EfiGcdIoTypeIo, 12u, SIZE_1KB / 2, &bridgeIoPortRangeBegin, reBarImageHandle, NULL))
//        {
//                EFI_GCD_MEMORY_SPACE_DESCRIPTOR memoryDescriptor;
//
//                if (EFI_ERROR(gDS->GetMemorySpaceDescriptor(baseAddress0, &memoryDescriptor)))
//                    SetStatusVar(StatusVar_EFIError);
//                else
//                    if (EFI_ERROR(gDS->SetMemorySpaceAttributes(baseAddress0, SIZE_16MB, memoryDescriptor.Attributes | EFI_MEMORY_UC)))
//                        SetStatusVar(StatusVar_EFIError);

                pciSaveAndRemapBridgeConfig(bridgePciAddress, bridgeSaveArea, gpuConfig->bar0.base, gpuConfig->bar0.top, TARGET_BRIDGE_IO_BASE_LIMIT);
                pciSaveAndRemapDeviceBAR0(pciAddress, gpuSaveArea, gpuConfig->bar0.base);

                bool configUpdated = ConfigureNvStrapsBAR1Size(gpuConfig->bar0.base & UINT32_C(0xFFFF'FFF0), barSizeSelector.barSizeSelector);     // mask the flag bits from the address

        // RecordUpdateGPU(bus, device, func, barSizeSelector.barSizeSelector);

                pciRestoreDeviceConfig(pciAddress, gpuSaveArea);
                pciRestoreBridgeConfig(bridgePciAddress, bridgeSaveArea);

                SetDeviceStatusVar(pciAddress, configUpdated ? StatusVar_GpuStrapsConfigured : StatusVar_GpuStrapsPreConfigured);

                uint_least16_t capabilityOffset = pciFindExtCapability(pciAddress, PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID);
                uint_least32_t barSizeMask = capabilityOffset ? pciRebarGetPossibleSizes(pciAddress, capabilityOffset, vendorId, deviceId, PCI_BAR_IDX1) : 0u;

                if (barSizeMask)
                {
                    if (barSizeMask & UINT32_C(1) << (barSizeSelector.barSizeSelector + 6u))
                        SetDeviceStatusVar(pciAddress, StatusVar_GpuStrapsConfirm);
                    else
                        SetDeviceStatusVar(pciAddress, StatusVar_GpuStrapsNoConfirm);
                }
                else
            if (isTuringGPU(deviceId))
            SetDeviceStatusVar(pciAddress, StatusVar_GpuNoReBarCapability);

                if (nPciBarSizeSelector == TARGET_PCI_BAR_SIZE_GPU_ONLY)
                {
            if (capabilityOffset && (barSizeMask & UINT32_C(0x0000'0001) << (barSizeSelector.barSizeSelector + 6u) || sizeMaskOverride.sizeMaskOverride))
            {
            if ((barSizeMask & UINT32_C(0x0000'0001) << (barSizeSelector.barSizeSelector + 6u)) == 0)
                SetDeviceStatusVar(pciAddress, StatusVar_GpuReBarSizeOverride);

            if (pciRebarSetSize(pciAddress, capabilityOffset, PCI_BAR_IDX1, (uint_least8_t)(barSizeSelector.barSizeSelector + 6u)))
                SetDeviceStatusVar(pciAddress, StatusVar_GpuReBarConfigured);
            }
                }
                else
                    if (nPciBarSizeSelector == TARGET_PCI_BAR_SIZE_GPU_STRAPS_ONLY)
                    {
                        EFI_EVENT eventTimer = NULL;

                        if (EFI_ERROR((status = gBS->CreateEvent(EVT_TIMER, TPL_APPLICATION, NULL, NULL, &eventTimer))))
                            SetDeviceEFIError(pciAddress, EFIError_CreateTimer, status);
                        else
                        {
                            if (EFI_ERROR((status = gBS->SetTimer(eventTimer, TimerRelative, 1'000'000u))))
                                SetDeviceEFIError(pciAddress, EFIError_SetupTimer, status);
                            else
                            {
                                UINTN eventIndex = 0u;

                                if (EFI_ERROR((status = gBS->WaitForEvent(1, &eventTimer, &eventIndex))))
                                    SetDeviceEFIError(pciAddress, EFIError_WaitTimer, status);
                            }

                            if (EFI_ERROR((status = gBS->CloseEvent(eventTimer))))
                                SetDeviceEFIError(pciAddress, EFIError_CloseTimer, status);
                        }
                    }

//            gDS->FreeIoSpace(bridgeIoPortRangeBegin, SIZE_1KB / 2u);
//        }
//        else
//            SetStatusVar(StatusVar_EFIAllocationError);
//
//        gDS->FreeMemorySpace(baseAddress0, SIZE_32MB);
//    }
//    else
//        SetStatusVar(StatusVar_EFIAllocationError);
}

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t barIndex)
{
    if (vid == TARGET_GPU_VENDOR_ID && subsysVenID != WORD_BITMASK && subsysDevID != WORD_BITMASK && barIndex == PCI_BAR_IDX1)
    {
    uint_least8_t bus, device, func;
    pciUnpackAddress(pciAddress, &bus, &device, &func);

    NvStraps_BarSize barSizeSelector = NvStrapsConfig_LookupBarSize(config, did, subsysVenID, subsysDevID, bus, device, func);

    if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        return false;

    NvStraps_BarSizeMaskOverride sizeMaskOverride = NvStrapsConfig_LookupBarSizeMaskOverride(config, did, subsysVenID, subsysDevID, bus, device, func);

    if (sizeMaskOverride.sizeMaskOverride)
    {
        NvStraps_BridgeConfig const *bridgeConfig = NvStrapsConfig_LookupBridgeConfig(config, bus);

        return isBridgeEnumerated(pciPackLocation(bridgeConfig->bridgeBus, bridgeConfig->bridgeDevice, bridgeConfig->bridgeFunction));
    }

    return false;
    }

    return false;
}

uint_least32_t NvStraps_AdjustBARSizeList(UINTN pciAddress, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t barIndex, uint_least32_t barSizeMask)
{
    uint_least8_t bus, device, func;

    pciUnpackAddress(pciAddress, &bus, &device, &func);

    NvStraps_BarSize barSizeSelector = NvStrapsConfig_LookupBarSize(config, did, subsysVenID, subsysDevID, bus, device, func);

    if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        return barSizeMask;

    if ((barSizeMask & UINT32_C(0x00000001) << (6u + (unsigned)barSizeSelector.barSizeSelector)) == 0u)
    SetDeviceStatusVar(pciAddress, StatusVar_GpuReBarSizeOverride);

    return barSizeMask | UINT32_C(0x00000001) << (6u + (unsigned)barSizeSelector.barSizeSelector);
}

// vim: ft=cpp
