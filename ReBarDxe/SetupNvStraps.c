#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

#include "include/pciRegs.h"
#include "include/ReBar.h"
#include "include/LocalPciGPU.h"
#include "include/SetupNvStraps.h"

#define TARGET_GPU_STRAPS_BASE_OFFSET UINT32_C(0x0010'1000)
#define TARGET_GPU_STRAPS_SET0_OFFSET 0x0000u
#define TARGET_GPU_STRAPS_SET1_OFFSET 0x000Cu

static bool isBridgeEnumerated = false;

void NvStraps_EnumDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo, UINT8 reBarState)
{
    if (reBarState && !isBridgeEnumerated)
    {
        if (vendorId == TARGET_BRIDGE_PCI_VENDOR_ID && deviceId == TARGET_BRIDGE_PCI_DEVICE_ID)
        {
            isBridgeEnumerated = 
                addrInfo->Bus == TARGET_BRIDGE_PCI_BUS &&
                addrInfo->Device == TARGET_BRIDGE_PCI_DEVICE &&
                addrInfo->Function == TARGET_BRIDGE_PCI_FUNCTION;
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

static void MapBridgeMemory(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u])
{
    pciReadConfigDword(bridgePciAddress, PCI_COMMAND,     bridgeSaveArea + 0u);
    pciReadConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, bridgeSaveArea + 1u);
    pciReadConfigDword(bridgePciAddress, PCI_IO_BASE,     bridgeSaveArea + 2u);
    pciReadConfigDword(bridgePciAddress, PCI_MEMORY_BASE, bridgeSaveArea + 3u);

    UINT32
        bridgeMemoryBaseLimit = TARGET_BRIDGE_MEM_BASE_LIMIT,
        bridgeIoBaseLimit = bridgeSaveArea[2u] & UINT32_C(0xFFFF0000) | TARGET_BRIDGE_IO_BASE_LIMIT & UINT32_C(0x0000FFFF),
        bridgePciBusNumber = bridgeSaveArea[1u] & UINT32_C(0xFF0000FF) | (TARGET_GPU_PCI_BUS & UINT8_C(0xFF)) << 8u | (TARGET_GPU_PCI_BUS & UINT8_C(0xFF)) << 16u,
        bridgeCommand = bridgeSaveArea[0u] | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;

    pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE, &bridgeMemoryBaseLimit);
    pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,     &bridgeIoBaseLimit);
    pciWriteConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, &bridgePciBusNumber);
    pciWriteConfigDword(bridgePciAddress, PCI_COMMAND,     &bridgeCommand);
}

static void RestoreBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u])
{
    pciWriteConfigDword(bridgePciAddress, PCI_COMMAND,     bridgeSaveArea + 0u);
    pciWriteConfigDword(bridgePciAddress, PCI_PRIMARY_BUS, bridgeSaveArea + 1u);
    pciWriteConfigDword(bridgePciAddress, PCI_IO_BASE,     bridgeSaveArea + 2u);
    pciWriteConfigDword(bridgePciAddress, PCI_MEMORY_BASE, bridgeSaveArea + 3u);
}

static void MapGpuBAR0(UINTN gpuPciAddress, UINT32 gpuSaveArea[2u])
{
     pciReadConfigDword(gpuPciAddress, PCI_COMMAND,        gpuSaveArea + 0u);
     pciReadConfigDword(gpuPciAddress, PCI_BASE_ADDRESS_0, gpuSaveArea + 1u);

     UINT32
        gpuBaseAddress = TARGET_GPU_BAR0_ADDRESS,
        gpuCommand = gpuSaveArea[0u] | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;

    pciWriteConfigDword(gpuPciAddress, PCI_BASE_ADDRESS_0, &gpuBaseAddress);
    pciWriteConfigDword(gpuPciAddress, PCI_COMMAND,        &gpuCommand);
}

static void RestoreGpuConfig(UINTN gpuPciAddress, UINT32 gpuSaveArea[2u])
{
    pciWriteConfigDword(gpuPciAddress, PCI_COMMAND,        gpuSaveArea + 0u);
    pciWriteConfigDword(gpuPciAddress, PCI_BASE_ADDRESS_0, gpuSaveArea + 1u);
}

static void ConfigureNvStrapsBAR1Size(UINT32 baseAddress0)
{
    UINT32
        *pSTRAPS = (UINT32 *)((UINTN)baseAddress0 + TARGET_GPU_STRAPS_BASE_OFFSET + TARGET_GPU_STRAPS_SET1_OFFSET),
        STRAPS;

    CopyMem(&STRAPS, pSTRAPS, sizeof STRAPS);

    STRAPS = STRAPS & UINT32_C(0xff8fffff) | (UINT32)TARGET_GPU_BAR1_SIZE_SELECTOR << 20u | UINT32_C(0x80000000);

    CopyMem(pSTRAPS, &STRAPS, sizeof STRAPS);
}

void NvStraps_Setup(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo)
{
    UINTN
        bridgePciAddress = EFI_PCI_ADDRESS(TARGET_BRIDGE_PCI_BUS, TARGET_BRIDGE_PCI_DEVICE, TARGET_BRIDGE_PCI_FUNCTION, 0u),
        gpuPciAddress = EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0u);

    UINT32 bridgeSaveArea[4u], gpuSaveArea[2u];

    MapBridgeMemory(bridgePciAddress, bridgeSaveArea);
    MapGpuBAR0(gpuPciAddress, gpuSaveArea);

    ConfigureNvStrapsBAR1Size(TARGET_GPU_BAR0_ADDRESS & PCI_BASE_ADDRESS_MEM_MASK);     // mask the flag bits from the address

    RestoreGpuConfig(gpuPciAddress, gpuSaveArea);
    RestoreBridgeConfig(bridgePciAddress, bridgeSaveArea);
}

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 bar)
{
    return isBridgeEnumerated && vid == TARGET_GPU_PCI_VENDOR_ID && did == TARGET_GPU_PCI_DEVICE_ID && bar == 0x01u
        && pciAddress == EFI_PCI_ADDRESS(TARGET_GPU_PCI_BUS, TARGET_GPU_PCI_DEVICE, TARGET_GPU_PCI_FUNCTION, 0x00u);
}

UINT32 NvStraps_AdjustBARSizeList(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 bar, UINT32 cap)
{
    return cap | UINT32_C(0x00000001) << (12u + (unsigned)TARGET_GPU_BAR1_SIZE_SELECTOR);
}
