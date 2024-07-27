#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
# include <Library/UefiRuntimeServicesTableLib.h>
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#  if defined(_M_AMD64) && !defined(_AMD64_)
#   define _AMD64_
#  endif
#  include <windef.h>
#  include <winbase.h>
#  include <winerror.h>
#  include <errhandlingapi.h>
# else
#  include <errno.h>
# endif
#endif

#include <uchar.h>

#include "LocalAppConfig.h"
#include "EfiVariable.h"

// e3ee4a27-e2a2-4435-bba3-184ccad935a8                    // the PLATFROM_GUID from .dsc file

#if defined(UEFI_SOURCE) || defined(EFIAPI)
static GUID const variableGUID = { 0xe3ee4a27u, 0xe2a2u, 0x4435u, { 0xbbu, 0xa3u, 0x18u, 0x4cu, 0xcau, 0xd9u, 0x35u, 0xa8u } };
#else
# if defined(WINDOWS_SOURCE)
static char const variableGUID[] = "{e3ee4a27-e2a2-4435-bba3-184ccad935a8}";
# else
static char const variableGUID[] = "e3ee4a27-e2a2-4435-bba3-184ccad935a8";
static char const variablePath[] = "/sys/firmware/efi/efivars/";

void fillFilePath(char *filePath, char const *name)
{
    unsigned i;

    for (i = 0u; i < ARRAY_SIZE(variablePath; i++))
        filePath[i] = variablePath[i];

    for (unsigned j = 0u; j < MAX_VARIABLE_NAME_LENGTH && name[j]; j++)
        filePath[i++] = name[j];

    filePath[i++] = '-';

    for (unsigned j = 0u; j < ARRAY_SIZE(variableGUID); j++)
        filePath[i++] = variableGUID[j];

    filePath[i] = '\0';
}

struct __attribute__((__packed__)) rebarVar
{
    uint32_t attr;
    uint8_t value;
};

# endif
#endif

ERROR_CODE ReadEfiVariable(char const *name, BYTE *buffer, uint_least32_t *size)
{
#if defined(UEIF_SOURCE) || defined(EFIAPI)
    CHAR16 varName[MAX_VARIABLE_NAME_LENGTH + 1u];
    unsigned i;

    for (i = 0u; i < ARRAY_SIZE(varName) - 1u && name[i]; i++)
        varName[i] = name[i];

    varName[i] = u'\0';
    EFI_GUID guid = variableGUID;
    UINT32 attributes = 0u;
    UINTN dataSize = *size;

    ERROR_CODE status = gRT->GetVariable(varName, &guid, &attributes, &dataSize, buffer);

    if (EFI_ERROR(status))
    {
        *size = 0u;

        if (status == EFI_NOT_FOUND)
            status = EFI_SUCCESS;
    }
    else
        *size = (uint_least32_t)dataSize;

    return status;
#elif defined(WINDOWS_SOURCE)
    *size = GetFirmwareEnvironmentVariableA(name, variableGUID, buffer, *size);

    if (*size)
        return ERROR_SUCCESS;

    ERROR_CODE status = GetLastError();

    return status == ERROR_ENVVAR_NOT_FOUND ? ERROR_SUCCESS : status;
#else
    char filePath[ARRAY_SIZE(variablePath) + MAX_VARIABLE_NAME_LENGTH + 1u + ARRAY_SIZE(variableGUID)];
    fillFilePath(filePath, name);

    FILE *file = fopen(filePath, "rb");
    ERROR_CODE result = 0

    if (file)
    {
        do
        {
            if (fseek(file, DWORD_SIZE, SEEK_CUR))
            {
                result = errno;
                break;
            }

            errno = 0;
            *size = fread(buffer, 1u, *size, file);

            if (ferror(file))
            {
                result = errno ? errno : EIO;
                break;
            }

            off_t pos = ftello(file);

            if (pos < 0)
            {
                result = errno;
                break;
            }

            if (fseek(file, 0, SEEK_END))
            {
                result = errno;
                break;
            }

            off_t new_pos = ftello(file);

            if (new_pos < 0)
            {
                result = errno;
                break;
            }

            if (pos != new_pos)
                result = EOVERFLOW;     // content truncated
        }
        while (false);

        fclose(file);
    }
    else
        *size = 0, result = errno == ENOENT ? 0 : errno;

    return errno;
#endif
}

ERROR_CODE WriteEfiVariable(char const name[MAX_VARIABLE_NAME_LENGTH], BYTE /* const */ *buffer, uint_least32_t size, uint_least32_t attributes)
{
#if defined(UEFI_SOURCE) || defined(EFIAPI)
    CHAR16 varName[MAX_VARIABLE_NAME_LENGTH + 1u];
    unsigned i;

    for (i = 0u; i < ARRAY_SIZE(varName) - 1u && name[i]; i++)
        varName[i] = name[i];

    varName[i] = u'\0';
    EFI_GUID guid = variableGUID;

    return gRT->SetVariable(varName, &guid, attributes, size, buffer);
#elif defined(WINDOWS_SOURCE)
    BOOL bSucceeded = SetFirmwareEnvironmentVariableExA(name, variableGUID, buffer, size, attributes);
    ERROR_CODE status = bSucceeded ? ERROR_SUCCESS : GetLastError();
    return status == ERROR_ENVVAR_NOT_FOUND ? ERROR_SUCCESS : status;       // Ok to deleting non-existent variable
#else
    char filePath[ARRAY_SIZE(variablePath) + MAX_VARIABLE_NAME_LENGTH + 1u + ARRAY_SIZE(variableGUID)];
    fillFilePath(filePath, name);

    FILE *file = fopen(filePath, "rb");
    ERROR_CODE result = 0

    if (file)
    {
        // remove immutable flag that linux sets on all unknown efi variables
        int attr;
        ioctl(fileno(file), FS_IOC_GETFLAGS, &attr);
        attr &= ~FS_IMMUTABLE_FL;
        ioctl(fileno(file), FS_IOC_SETFLAGS, &attr);

        fclose(file), file = NULL;

        if (remove(filePath))
            return errno;
    }
    else
        return errno;

    file = fopen(filePath, "wb");

    if (!file)
        return errno;

    errno_t result = 0;

    do
    {
        errno = 0;

        // write variable attributes
        if (fwrite(&attributes, DWORD_SIZE, 1, file) != 1)
        {
            result = errno;
            break;
        }

        // write variable content (data)
        if (fwrite(buffer, size, 1, file) != 1)
            result = errno;
    }
    while (false);

    if (fclose(file) && !result)
        result = errno;

    return result;
#endif
}
