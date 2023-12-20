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
#include "ReBar.h"

#include "SetupNvStraps.h"

static uint_least32_t const
    TARGET_GPU_STRAPS_BASE_OFFSET = 0x0010'1000u,
    TARGET_GPU_STRAPS_SET0_OFFSET = 0x0000'0000u,
    TARGET_GPU_STRAPS_SET1_OFFSET = 0x0000'000Cu;

static bool isBridgeEnumerated = false;

void NvStraps_EnumDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo)
{
    if (!isBridgeEnumerated)
    {
        if (vendorId == TARGET_BRIDGE_PCI_VENDOR_ID && deviceId == TARGET_BRIDGE_PCI_DEVICE_ID)
        {
            isBridgeEnumerated =
                addrInfo->Bus == TARGET_BRIDGE_PCI_BUS &&
                addrInfo->Device == TARGET_BRIDGE_PCI_DEVICE &&
                addrInfo->Function == TARGET_BRIDGE_PCI_FUNCTION;

            if (isBridgeEnumerated)
                SetStatusVar(StatusVar_BridgeFound);
        }
    }
}

bool NvStraps_CheckDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo)
{
    return
           isBridgeEnumerated
        && vendorId == TARGET_GPU_PCI_VENDOR_ID && deviceId == TARGET_GPU_PCI_DEVICE_ID
        && addrInfo->Bus == TARGET_GPU_PCI_BUS && addrInfo->Device == TARGET_GPU_PCI_DEVICE
        && addrInfo->Function == TARGET_GPU_PCI_FUNCTION;
}

static void ConfigureNvStrapsBAR1Size(EFI_PHYSICAL_ADDRESS baseAddress0)
{
    UINT32
        *pSTRAPS = (UINT32 *)(baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET1_OFFSET),
        STRAPS;

    CopyMem(&STRAPS, pSTRAPS, sizeof STRAPS);

    STRAPS = STRAPS & UINT32_C(0xff8fffff) | (UINT32)TARGET_GPU_BAR1_SIZE_SELECTOR << 20u | UINT32_C(0x80000000);

    CopyMem(pSTRAPS, &STRAPS, sizeof STRAPS);
}

void NvStraps_Setup(EFI_HANDLE pciBridgeHandle, UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo, uint_fast8_t reBarState)
{
    UINTN
        bridgePciAddress = EFI_PCI_ADDRESS(TARGET_BRIDGE_PCI_BUS, TARGET_BRIDGE_PCI_DEVICE, TARGET_BRIDGE_PCI_FUNCTION, 0u),
        gpuPciAddress = EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0u);

    UINT32 bridgeSaveArea[4u], gpuSaveArea[2u];

//    EFI_PHYSICAL_ADDRESS baseAddress0 = BASE_4GB - 1u, bridgeIoPortRangeBegin = BASE_64KB - 1u;

    // Pretend the allocated MMIO memory is for the PCI root bridge device
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

                pciSaveAndRemapBridgeConfig(bridgePciAddress, bridgeSaveArea, TARGET_GPU_BAR0_ADDRESS, TARGET_BRIDGE_IO_BASE_LIMIT, TARGET_GPU_PCI_BUS);
                pciSaveAndRemapDeviceBAR0(gpuPciAddress, gpuSaveArea, TARGET_GPU_BAR0_ADDRESS);

                ConfigureNvStrapsBAR1Size(TARGET_GPU_BAR0_ADDRESS);     // mask the flag bits from the address

                pciRestoreDeviceConfig(gpuPciAddress, gpuSaveArea);
                pciRestoreBridgeConfig(bridgePciAddress, bridgeSaveArea);

                SetStatusVar(StatusVar_GpuStrapsConfigured);

                if (reBarState == REBAR_STATE_NV_GPU_ONLY)
                {
                    uint_least16_t capabilityOffset = pciFindExtCapability(gpuPciAddress, PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID);

                    if (capabilityOffset)
                    {
                        pciRebarSetSize(gpuPciAddress, capabilityOffset, PCI_BAR_IDX1, TARGET_GPU_BAR1_SIZE_SELECTOR + 8u);
                        SetStatusVar(StatusVar_GpuReBarConfigured);
                    }
                    else
                        SetStatusVar(StatusVar_GpuNoReBarCapability);
                }
                else
                    if (reBarState == REBAR_STATE_NV_GPU_STRAPS_ONLY)
                    {
                        EFI_EVENT eventTimer = NULL;

                        if (EFI_ERROR(gBS->CreateEvent(EVT_TIMER, TPL_APPLICATION, NULL, NULL, &eventTimer)))
                            SetStatusVar(StatusVar_EFIError);
                        else
                        {
                            if (EFI_ERROR(gBS->SetTimer(eventTimer, TimerRelative, 1'000'000u)))
                                SetStatusVar(StatusVar_EFIError);
                            else
                            {
                                UINTN eventIndex = 0u;

                                if (EFI_ERROR(gBS->WaitForEvent(1, &eventTimer, &eventIndex)))
                                    SetStatusVar(StatusVar_EFIError);
                                else
                                    SetStatusVar(StatusVar_GpuDelayElapsed);
                            }

                            if (EFI_ERROR(gBS->CloseEvent(eventTimer)))
                                SetStatusVar(StatusVar_EFIError);
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

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 barIndex)
{
    SetStatusVar(StatusVar_GpuFound);

    return isBridgeEnumerated && vid == TARGET_GPU_PCI_VENDOR_ID && did == TARGET_GPU_PCI_DEVICE_ID && barIndex == PCI_BAR_IDX1
        && pciAddress == EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0x00u);
}

UINT32 NvStraps_AdjustBARSizeList(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 barIndex, UINT32 barSizeMask)
{
    if (barSizeMask)
        SetStatusVar(StatusVar_GpuReBarConfigured);

    return barSizeMask | UINT32_C(0x00000001) << (8u + (unsigned)TARGET_GPU_BAR1_SIZE_SELECTOR);
}
