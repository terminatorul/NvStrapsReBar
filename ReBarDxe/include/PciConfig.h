#if !defined(NV_STRAPS_REBAR_PCI_CONFIG_H)
#define NV_STRAPS_REBAR_PCI_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#if defined(UEFI_SOURCE)
# include <Uefi.h>
# include <Protocol/PciRootBridgeIo.h>
#endif

#include "LocalAppConfig.h"

#if defined(UEFI_SOURCE)
UINTN pciLocateDevice(EFI_HANDLE RootBridgeHandle, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS addressInfo, uint_least16_t *venID, uint_least16_t *devID, uint_least8_t *headerType);
uint_least16_t pciFindExtCapability(UINTN pciAddress, uint_least32_t cap);
uint_least32_t pciRebarGetPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, UINT16 vid, UINT16 did, uint_least8_t barIndex);
uint_least32_t pciRebarPollPossibleSizes(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least32_t barSizeMask);

EFI_STATUS pciReadDeviceSubsystem(UINTN pciAddress, uint_least16_t *subsysVenID, uint_least16_t *subsysDevID);
EFI_STATUS pciBridgeSecondaryBus(UINTN pciAddress, uint_least8_t *secondaryBus);
uint_least32_t pciDeviceClass(UINTN pciAddress);
uint_least32_t pciDeviceBAR0(UINTN pciAddress, EFI_STATUS *status);
bool pciRebarSetSize(UINTN pciAddress, uint_least16_t capabilityOffset, uint_least8_t barIndex, uint_least8_t barSizeBitIndex);

void pciSaveAndRemapBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[3u], EFI_PHYSICAL_ADDRESS baseAddress0, EFI_PHYSICAL_ADDRESS topAddress0, EFI_PHYSICAL_ADDRESS bridgeIoBaseLimit);
void pciRestoreBridgeConfig(UINTN bridgePciAddress, UINT32 bridgeSaveArea[3u]);

void pciSaveAndRemapDeviceBAR0(UINTN pciAddress, UINT32 deviceSaveArea[2u], EFI_PHYSICAL_ADDRESS baseAddress0);
void pciRestoreDeviceConfig(UINTN pciAddress, UINT32 deviceSaveArea[2u]);

bool pciIsPciBridge(uint_least8_t headerType);
bool pciIsVgaController(uint_least32_t pciClassReg);

#endif	    // defined(UEFI_SOURCE)

inline void pciUnpackAddress(UINTN pciAddress, uint_least8_t *bus, uint_least8_t *dev, uint_least8_t *fun)
{
    *bus = pciAddress >> 24u & BYTE_BITMASK;
    *dev = pciAddress >> 16u & BYTE_BITMASK;
    *fun = pciAddress >>  8u & BYTE_BITMASK;
}

inline uint_least16_t pciPackLocation(uint_least8_t bus, uint_least8_t dev, uint_least8_t fun)
{
    return (uint_least16_t) bus << BYTE_BITSIZE | dev << 3u & 0b1111'1000u | fun & 0b0111u;
}


#endif          // !defined(NV_STRAPS_REBAR_PCI_CONFIG_H)
