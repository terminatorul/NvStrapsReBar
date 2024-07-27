
#include <stdbool.h>
#include <stdint.h>

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/EventGroup.h>
#include <Protocol/S3SaveState.h>

#include "ReBar.h"
#include "PciConfig.h"
#include "StatusVar.h"
#include "S3ResumeScript.h"

EFI_S3_SAVE_STATE_PROTOCOL *S3SaveState = NULL;

static void LoadS3SaveStateProtocol()
{
    EFI_STATUS status = gBS->LocateProtocol(&gEfiS3SaveStateProtocolGuid, NULL, (void **)&S3SaveState);

    if (EFI_ERROR(status))
    SetEFIError(EFIError_LocateS3SaveStateProtocol, status);
}

void S3ResumeScript_Init(bool enabled)
{
    if (enabled && !NvStrapsConfig_SkipS3Resume(config))
    LoadS3SaveStateProtocol();
}

// EFI_STATUS S3ResumeScript_MemWrite_DWORD(uintptr_t address, uint_least32_t data);

EFI_STATUS S3ResumeScript_MemReadWrite_DWORD(uintptr_t address, uint_least32_t data, uint_least32_t dataMask)
{
    if (S3SaveState)
    return S3SaveState->Write
        (
        S3SaveState,
        (UINT16)EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE,
        (EFI_BOOT_SCRIPT_WIDTH)EfiBootScriptWidthUint32,
        (UINT64)address,
        (void *)&data,
        (void *)&dataMask
        );

    return EFI_SUCCESS;
}

EFI_STATUS S3ResumeScript_PciConfigWrite_DWORD(UINTN pciAddress, uint_least16_t offset, uint_least32_t data)
{
    if (S3SaveState)
    return S3SaveState->Write
        (
        S3SaveState,
        (UINT16)EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE,
        (EFI_BOOT_SCRIPT_WIDTH)EfiBootScriptWidthUint32,
        (UINT64)pciAddrOffset(pciAddress, offset),
        (UINTN)1u,
        (void *)&data
        );

    return EFI_SUCCESS;
}

EFI_STATUS S3ResumeScript_PciConfigReadWrite_DWORD(UINTN pciAddress, uint_least16_t offset, uint_least32_t data, uint_least32_t dataMask)
{
    if (S3SaveState)
    return S3SaveState->Write
        (
        S3SaveState,
        (UINT16)EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE,
        (EFI_BOOT_SCRIPT_WIDTH)EfiBootScriptWidthUint32,
        (UINT64)pciAddrOffset(pciAddress, offset),
        (void *)&data,
        (void *)&dataMask
        );

    return EFI_SUCCESS;
}
