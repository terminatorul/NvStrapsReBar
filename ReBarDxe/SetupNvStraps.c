#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <IndustryStandard/Pci.h>

#include "LocalPciGPU.h"
#include "StatusVar.h"
#include "PciConfig.h"
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

static bool isBridgeEnumerated = false;

void NvStraps_EnumDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId)
{
    if (!isBridgeEnumerated)
    {
        if (vendorId == TARGET_BRIDGE_PCI_VENDOR_ID && deviceId == TARGET_BRIDGE_PCI_DEVICE_ID && NvStrapsConfig_IsGpuConfigured(config))
        {
            isBridgeEnumerated = pciAddress == EFI_PCI_ADDRESS(TARGET_BRIDGE_PCI_BUS, TARGET_BRIDGE_PCI_DEVICE, TARGET_BRIDGE_PCI_FUNCTION, 0x00u);

            if (isBridgeEnumerated)
                SetStatusVar(StatusVar_BridgeFound);
        }
    }
}

bool NvStraps_CheckDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t *subsysVenID, uint_least16_t *subsysDevID)
{
    if (vendorId == TARGET_GPU_VENDOR_ID && NvStrapsConfig_IsGpuConfigured(config))
    {
        bool deviceFound = isBridgeEnumerated
                && vendorId == TARGET_GPU_PCI_VENDOR_ID && deviceId == TARGET_GPU_PCI_DEVICE_ID
                && pciAddress == EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0x00u);

        if (deviceFound)
        {
            EFI_STATUS status = pciReadDeviceSubsystem(pciAddress, subsysVenID, subsysDevID);

            if (EFI_ERROR(status))
                return SetEFIError(EFIError_PCI_DeviceSubsystem, status), false;

            SetStatusVar(StatusVar_GpuFound);
        }

        uint_least8_t
            bus      = pciAddress >> 24u & BYTE_BITMASK,
            device   = pciAddress >> 16u & BYTE_BITMASK,
            function = pciAddress >>  8u & BYTE_BITMASK;

        NvStraps_BarSize barSizeSelector =
            NvStrapsConfig_LookupBarSize(config, deviceId, *subsysVenID, *subsysDevID, bus, device, function);

        if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        {
            SetDeviceStatusVar(pciAddress, barSizeSelector.barSizeSelector == BarSizeSelector_Excluded ? StatusVar_GpuExcluded : StatusVar_GPU_Unconfigured);
            return false;
        }

        return deviceFound;
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
    }

    if (barSize_Part2 != targetBarSize_Part2)
    {
        STRAPS1 &= ~(UINT32)(((UINT32_C(1) << BAR1_SIZE_PART2_BITSIZE) - 1u) << BAR1_SIZE_PART2_SHIFT);
        STRAPS1 |= (UINT32)targetBarSize_Part2 << BAR1_SIZE_PART2_SHIFT;
        STRAPS1 |= UINT32_C(1) << (DWORD_SIZE * BYTE_BITSIZE - 1u);

        CopyMem(pSTRAPS1, &STRAPS1, sizeof STRAPS1);
    }

    return barSize_Part1 + barSize_Part2 != targetBarSize_Part1 + targetBarSize_Part2;
}

void NvStraps_Setup(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_fast8_t nPciBarSizeSelector)
{
    uint_least8_t
        bus      = pciAddress >> 24u & BYTE_BITMASK,
        device   = pciAddress >> 16u & BYTE_BITMASK,
        function = pciAddress >>  8u & BYTE_BITMASK;

    NvStraps_BarSize barSizeSelector =
        NvStrapsConfig_LookupBarSize(config, deviceId, subsysVenID, subsysDevID, bus, device, function);

    if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        return;

    UINTN bridgePciAddress = EFI_PCI_ADDRESS(TARGET_BRIDGE_PCI_BUS, TARGET_BRIDGE_PCI_DEVICE, TARGET_BRIDGE_PCI_FUNCTION, 0u);
    UINT32 bridgeSaveArea[4u], gpuSaveArea[2u];

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

                pciSaveAndRemapBridgeConfig(bridgePciAddress, bridgeSaveArea, TARGET_GPU_BAR0_ADDRESS & ~UINT32_C(0x000F), TARGET_BRIDGE_IO_BASE_LIMIT, TARGET_GPU_PCI_BUS);
                pciSaveAndRemapDeviceBAR0(pciAddress, gpuSaveArea, TARGET_GPU_BAR0_ADDRESS);

                bool configUpdated = ConfigureNvStrapsBAR1Size(TARGET_GPU_BAR0_ADDRESS & ~UINT32_C(0x000F), barSizeSelector.barSizeSelector);     // mask the flag bits from the address

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
                    SetStatusVar(StatusVar_GpuNoReBarCapability);

                if (nPciBarSizeSelector == TARGET_PCI_BAR_SIZE_GPU_ONLY && capabilityOffset)
                {
                    if (pciRebarSetSize(pciAddress, capabilityOffset, PCI_BAR_IDX1, (uint_least8_t)(barSizeSelector.barSizeSelector + 6u)))
                        SetDeviceStatusVar(pciAddress, StatusVar_GpuReBarConfigured);
                }
                else
                    if (nPciBarSizeSelector == TARGET_PCI_BAR_SIZE_GPU_STRAPS_ONLY)
                    {
                        EFI_EVENT eventTimer = NULL;
                        EFI_STATUS status;

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
    return vid == TARGET_GPU_VENDOR_ID && isBridgeEnumerated && subsysVenID != WORD_BITMASK && subsysDevID != WORD_BITMASK && did == TARGET_GPU_PCI_DEVICE_ID && barIndex == PCI_BAR_IDX1
        && pciAddress == EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0x00u);
}

uint_least32_t NvStraps_AdjustBARSizeList(UINTN pciAddress, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t barIndex, uint_least32_t barSizeMask)
{
    uint_least8_t
        bus      = pciAddress >> 24u & BYTE_BITMASK,
        device   = pciAddress >> 16u & BYTE_BITMASK,
        function = pciAddress >>  8u & BYTE_BITMASK;

    NvStraps_BarSize
        barSizeSelector = NvStrapsConfig_LookupBarSize(config, did, subsysVenID, subsysDevID, bus, device, function);

    if (barSizeSelector.priority == UNCONFIGURED || barSizeSelector.barSizeSelector == BarSizeSelector_None || barSizeSelector.barSizeSelector == BarSizeSelector_Excluded)
        return barSizeMask;
    else
        if (barSizeMask >> 9u)  // any BAR sizes over 256 MB
            SetStatusVar(StatusVar_GpuReBarConfigured);


    return barSizeMask | UINT32_C(0x00000001) << (6u + (unsigned)barSizeSelector.barSizeSelector);
}

// vim: ft=cpp
