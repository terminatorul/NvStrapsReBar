#if !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
#define REBAR_UEFI_SETUP_NV_STRAPS_H

#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>

void NvStraps_EnumDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least8_t headerType);
bool NvStraps_CheckDevice(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t *subsysVenID, uint_least16_t *subsysDevID);
void NvStraps_Setup(UINTN pciAddress, uint_least16_t vendorId, uint_least16_t deviceId, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_fast8_t reBarState);

bool NvStraps_CheckBARSizeListAdjust(UINTN pciAddress, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, UINT8 barIndex);
uint_least32_t NvStraps_AdjustBARSizeList(UINTN pciAddress, uint_least16_t vid, uint_least16_t did, uint_least16_t subsysVenID, uint_least16_t subsysDevID, UINT8 barIndex, uint_least32_t barSizeMask);

#endif          // !defined(REBAR_UEFI_SETUP_NV_STRAPS_H)
