#if !defined(NV_STRAPS_REBAR_S3_RESUME_SCRIPT_H)
#define NV_STRAPS_REBAR_S3_RESUME_SCRIPT_H

#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>

void S3ResumeScript_Init(bool enabled);
// EFI_STATUS S3ResumeScript_MemWrite_DWORD(uintptr_t address, uint_least32_t data);
EFI_STATUS S3ResumeScript_MemReadWrite_DWORD(uintptr_t address, uint_least32_t data, uint_least32_t dataMask);
EFI_STATUS S3ResumeScript_PciConfigWrite_DWORD(UINTN pciAddress, uint_least16_t offset, uint_least32_t data);
EFI_STATUS S3ResumeScript_PciConfigReadWrite_DWORD(UINTN pciAddress, uint_least16_t offset, uint_least32_t data, uint_least32_t dataMask);

#endif    // !defined(NV_STRAPS_REBAR_S3_RESUME_SCRIPT_H)
