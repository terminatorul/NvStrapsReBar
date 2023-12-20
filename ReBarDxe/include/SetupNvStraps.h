#if !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
#define REBAR_UEFI_SETUP_NV_STRAPS_H

#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <Protocol/PciRootBridgeIo.h>

// Special value for ReBarState NVAR variable, to indicate ReBAR should be
// enabled only for selected GPU
static uint_fast8_t const
    REBAR_STATE_NV_GPU_ONLY = 64u,
    REBAR_STATE_NV_GPU_STRAPS_ONLY = 65u;

void NvStraps_EnumDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo);
bool NvStraps_CheckDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo);
void NvStraps_Setup(EFI_HANDLE pciBridgeHandle, UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo, uint_fast8_t reBarState);

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 barIndex);
UINT32 NvStraps_AdjustBARSizeList(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 barIndex, UINT32 barSizeMask);

#endif          // !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
