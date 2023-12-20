#if !defined(REBAR_UEFI_REBAR_H)
#define REBAR_UEFI_REBAR_H

#include <Uefi.h>

extern EFI_HANDLE reBarImageHandle;

EFI_STATUS pciReadConfigDword(UINTN pciAddress, INTN pos, UINT32 *buf);
EFI_STATUS pciWriteConfigDword(UINTN pciAddress, INTN pos, UINT32 *buf);

#endif          // !defined(REBAR_UEFI_REBAR_H)
