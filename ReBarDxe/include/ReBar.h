#if !defined(REBAR_UEFI_REBAR_H)
#define REBAR_UEFI_REBAR_H

#include <Uefi.h>
#include <Protocol/S3SaveState.h>

#include "NvStrapsConfig.h"

extern EFI_HANDLE reBarImageHandle;
extern NvStrapsConfig *config;
extern EFI_S3_SAVE_STATE_PROTOCOL *S3SaveState;

#endif          // !defined(REBAR_UEFI_REBAR_H)
