#if !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
#define REBAR_UEFI_SETUP_NV_STRAPS_H

#include <stdbool.h>
#include <Uefi.h>
#include <Protocol/PciRootBridgeIo.h>

void NvStraps_EnumDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo, UINT8 reBarState);
bool NvStraps_CheckDevice(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo);
void NvStraps_Setup(UINT16 vendorId, UINT16 deviceId, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS const *addrInfo);

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 bar);
UINT32 NvStraps_AdjustBARSizeList(UINTN pciAddress, UINT16 vid, UINT16 did, UINT8 bar, UINT32 cap);

#endif          // !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
