#include <stdint.h>

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "LocalAppConfig.h"
#include "EfiVariable.h"
#include "NvStrapsConfig.h"
#include "StatusVar.h"
#include "CheckSetupVar.h"

static CHAR16 const SETUP_VAR_NAME[] = L"Setup";
static CHAR16 const CUSTOM_VAR_NAME[] = L"Custom";

static uint_least64_t const ECMA_128_CRC_POLY = UINT64_C(0xC96C'5795'D787'0F42);

static uint_least64_t ecma128_crc64(BYTE const *buffer, BYTE const *bufferEnd, uint_least64_t crcValue)
{
    crcValue = ~crcValue & (uint_least64_t)UINT64_C(0xFFFF'FFFF'FFFF'FFFF);

    while (buffer != bufferEnd)
    {
    crcValue ^= unpack_QWORD(buffer), buffer += QWORD_SIZE;

    for (unsigned char bitIndex = 0u; bitIndex < QWORD_BITSIZE; bitIndex++)
        if (crcValue & UINT64_C(1) << (QWORD_BITSIZE - 1u))
        crcValue <<= 1u, crcValue ^= ECMA_128_CRC_POLY;
        else
        crcValue <<= 1u;
    }

    return ~crcValue & (uint_least64_t)UINT64_C(0xFFFF'FFFF'FFFF'FFFF);
}

static BYTE *LoadSetupVariable(CHAR16 const *name, EFI_GUID *guid, UINTN *dataLength)
{
    UINT32 attributes = 0u;
    *dataLength = 0u;
    BYTE *data = NULL;

    EFI_STATUS status = gRT->GetVariable((CHAR16 *)name, guid, &attributes, dataLength, NULL);

    if (status != EFI_BUFFER_TOO_SMALL)
    {
    SetEFIError(EFIError_ReadSetupVarSize, status);
    return NULL;
    }

    uint_least8_t paddingLength = 8u - *dataLength & 0b0000'0111u;

    status = gBS->AllocatePool(EfiBootServicesData, *dataLength + paddingLength, (VOID **)&data);

    if (EFI_ERROR(status))
    {
    SetEFIError(EFIError_AllocateSetupVarData, status);

    return NULL;
    }

    for (unsigned i = 0u; i < paddingLength; i++)
    data[*dataLength + i] = paddingLength;

    status = gRT->GetVariable((CHAR16 *)name, guid, &attributes, dataLength, data);

    if (EFI_ERROR(status))
    {
    gBS->FreePool(data), data = NULL;
    SetEFIError(EFIError_ReadSetupVar, status);

    return NULL;
    }

    if (   (attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)) != (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
    || (attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD))
    {
    gBS->FreePool(data), data = NULL;
    SetStatusVar(StatusVar_BadSetupVarAttributes);

    return NULL;
    }

    *dataLength += paddingLength;

    return data;
}

static bool FreeSetupVariable(BYTE *data)
{
    if (data)
    {
    EFI_STATUS status = gBS->FreePool(data);

    if (EFI_ERROR(status))
    {
        SetEFIError(EFIError_AllocateSetupVarData, status);

        return false;
    }
    }

    return true;
}

static void LStringCopy(CHAR16 *newVarName, CHAR16 const *varName, size_t len)
{
    CHAR16 const *varNameLimit = varName + len;

    while (varName < varNameLimit)
    *newVarName++ = *varName++;

    *newVarName = L'\0';
}

static UINTN LStringLength(CHAR16 const *str, UINTN stringCapacity)
{
    if (stringCapacity)
    {
    CHAR16 const *ptr = str, *endPtr = str + stringCapacity - 1u;

    while (ptr != endPtr && *ptr)
        ptr++;

    return *ptr ? 0u : ptr - str;
    }

    return 0u;
}

typedef struct UefiString
{
    CHAR16 *ptr;
    UINTN capacity;
    UINTN length;
}
    UefiString;

static bool AllocateUefiString(UefiString *str, size_t stringCapacity)
{
    EFI_STATUS status = gBS->AllocatePool(EfiBootServicesData, stringCapacity * sizeof *str->ptr, (VOID **)&str->ptr);

    if (EFI_ERROR(status))
    {
    SetEFIError(EFIError_AllocateSetupVarName, status);
    return false;
    }

    str->capacity = stringCapacity;
    str->length = 0u;
    str->ptr[0u] = L'\0';

    return true;
}

static bool DeallocateUefiString(UefiString *str)
{
    if (str->ptr)
    {
    EFI_STATUS status = gBS->FreePool(str->ptr);

    if (EFI_ERROR(status))
    {
        SetEFIError(EFIError_AllocateSetupVarName, status);
        return false;
    }
    else
    {
        str->length = 0u;
        str->capacity = 0u;
        str->ptr = NULL;
    }
    }

    return true;
}

static bool ReallocateUefiString(UefiString *str, size_t stringCapacity)
{
    CHAR16 *newStr = NULL;
    EFI_STATUS status = gBS->AllocatePool(EfiBootServicesData, stringCapacity * sizeof *newStr, (VOID **)&newStr);

    if (EFI_ERROR(status))
    {
    SetEFIError(EFIError_AllocateSetupVarName, status);
    return false;
    }

    UINTN length = str->length < stringCapacity ? str->length : stringCapacity ? stringCapacity - 1u : 0u;

    LStringCopy(newStr, str->ptr, str->length);

    if (DeallocateUefiString(str))
    {
    str->ptr = newStr;
    str->capacity = stringCapacity;
    str->length = length;

    return true;
    }
    else
    gBS->FreePool(newStr);

    return false;
}

static char CompareUefiString(UefiString *str, CHAR16 const *val)
{
    CHAR16 const *ptr = str->ptr;

    while (*ptr && *ptr == *val)
    ptr++, val++;

    return *ptr < *val ? -1 : *ptr == *val ? 0 : 1;
}

static bool NextUefiVariableName(UefiString *str, EFI_GUID *efiGUID)
{
    UINTN length = str->capacity;
    EFI_STATUS status = gRT->GetNextVariableName(&length, str->ptr, efiGUID);

    if (EFI_ERROR(status) && status == EFI_BUFFER_TOO_SMALL)
    if (ReallocateUefiString(str, ++length))
        status = gRT->GetNextVariableName(&length, str->ptr, efiGUID);
    else
        return false;

    if (EFI_ERROR(status))
    if (status == EFI_NOT_FOUND)
        length = 0u;
    else
    {
        SetEFIError(EFIError_EnumVar, status);
        return false;
    }

    str->length = LStringLength(str->ptr, str->capacity);

    return true;
}

static CHAR16 const *FindSetupVariable(EFI_GUID *efiGUID)
{
    enum SetupVarName
    {
    SetupVar_None,
    SetupVar_Custom,
    SetupVar_Setup
    }
    setupVarName = SetupVar_None;

    bool multipleCustomVariables = false;

    UefiString varName = { .ptr = NULL, .capacity = 0u, .length = 0u };
    EFI_GUID varGuid = { };

    if (!AllocateUefiString(&varName, 64u))
    return NULL;

    bool enumerationCompleted = false;

    while (NextUefiVariableName(&varName, efiGUID))
    {
    if (varName.length)
    {
        if (varName.length == ARRAY_SIZE(CUSTOM_VAR_NAME) - 1u && CompareUefiString(&varName, CUSTOM_VAR_NAME) == 0)
        if (setupVarName < SetupVar_Custom)
        {
            setupVarName = SetupVar_Custom;
            varGuid = *efiGUID;
        }
        else
            if (setupVarName == SetupVar_Custom)
            multipleCustomVariables = true;		// "Custom" variable found twice
            else
            ;					// "Setup" variable has precedence over "Custom"
        else
        if (varName.length == ARRAY_SIZE(SETUP_VAR_NAME) - 1u && CompareUefiString(&varName, SETUP_VAR_NAME) == 0)
        {
            UINTN varSize = 0u;

            EFI_STATUS status = gRT->GetVariable(varName.ptr, efiGUID, NULL, &varSize, NULL);

            if (status != EFI_BUFFER_TOO_SMALL)
            {
            SetEFIError(EFIError_EnumSetupVarSize, status);
            return NULL;
            }

            if (varSize >= 16u /* && !!(attributes & EFI_VARIABLE_NON_VOLATILE) && !!(attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS) */)
            if (setupVarName < SetupVar_Setup)
            {
                setupVarName = SetupVar_Setup;
                varGuid = *efiGUID;
            }
            else
            {
                // "Setup" variable found twice
                setupVarName = SetupVar_None;
                multipleCustomVariables = true;
                enumerationCompleted = true;
                break;
            }
        }
    }
    else
    {
        enumerationCompleted = true;
        break;
    }
    }

    if (DeallocateUefiString(&varName))
    {
    if (enumerationCompleted)
        switch (setupVarName)
        {
        case SetupVar_Custom:
        if (multipleCustomVariables)
        {
            SetStatusVar(StatusVar_AmbiguousSetupVariable);
            return NULL;
        }

        *efiGUID = varGuid;
        return CUSTOM_VAR_NAME;

        case SetupVar_Setup:
        *efiGUID = varGuid;
        return SETUP_VAR_NAME;

        default:
        SetStatusVar(multipleCustomVariables ? StatusVar_AmbiguousSetupVariable : StatusVar_MissingSetupVariable);
        return NULL;
        }
    }

    return NULL;
}

bool IsSetupVariableChanged()
{
    ERROR_CODE errorCode;
    NvStrapsConfig *config = GetNvStrapsConfig(false, &errorCode);

    if (errorCode)
    return true;

    EFI_GUID setupVarGuid = { .Data1 = 0u, .Data2 = 0u, .Data3 = 0u, };
    CHAR16 const *varName = FindSetupVariable(&setupVarGuid);

    if (!varName)
    return true;

    UINTN length;
    BYTE *data = LoadSetupVariable(varName, &setupVarGuid, &length);

    if (!data)
    return true;

    uint_least64_t crc64 = ecma128_crc64(data, data + length, 0u);

    if (FreeSetupVariable(data))
    {
    data = NULL;

    if (NvStrapsConfig_HasSetupVarCRC(config))
        return NvStrapsConfig_SetupVarCRC(config) != crc64;

    NvStrapsConfig_SetSetupVarCRC(config, crc64);
    NvStrapsConfig_SetHasSetupVarCRC(config, true);

    SaveNvStrapsConfig(&errorCode);

    if (EFI_ERROR(errorCode))
        SetEFIError(EFIError_WriteConfigVar, errorCode);

    return false;
    }

    return true;
}

// vim:ft=cpp
