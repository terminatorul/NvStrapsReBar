#if !defined(NV_STRAPS_REBAR_PCI_CONFIG_H)
#define NV_STRAPS_REBAR_PCI_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <Protocol/PciRootBridgeIo.h>

UINTN pciLocatedDevice(EFI_HANDLE RootBridgeHandle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addressInfo, UINT16 *venID, UINT16 *devID);
uint_least16_t pciFindExtCapability(UINTN pciAddress, uint_least32_t cap);
uint_least32_t pciRebarGetPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, UINT16 vid, UINT16 did, uint_least8_t barIndex);
uint_least32_t pciRebarPollPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least32_t barSizeMask);
bool pciRebarSetSize(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least8_t barSizeBitIndex);

void pciSaveAndRemapBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u], EFI_PHYSICAL_ADDRESS baseAddress0, EFI_PHYSICAL_ADDRESS bridgeBaseIo, UINT8 busNo);
void pciRestoreBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[4u]);

void pciSaveAndRemapDeviceBAR0(UINTN pciAddress, UINT32 deviceSaveArea[2u], EFI_PHYSICAL_ADDRESS baseAddress0);
void pciRestoreDeviceConfig(UINTN pciAddress, UINT32 deviceSaveArea[2u]);

#endif          // !defined(NV_STRAPS_REBAR_PCI_CONFIG_H)
