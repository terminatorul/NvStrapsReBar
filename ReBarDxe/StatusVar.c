
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
# include <Library/UefiRuntimeServicesTableLib.h>

# include "LocalPciGpu.h"
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#  if defined(_M_AMD64) && !defined(_AMD64_)
#   define _AMD64_
#  endif
# endif
# include <windef.h>
# include <winbase.h>
# include <errhandlingapi.h>
# include <winerror.h>
#endif

#include "StatusVar.h"

#if defined(UEFI_SOURCE) || defined(EFIAPI)
static CHAR16 statusVarName[] = u"NvStrapsReBarStatus";
// 5a725dfd-f1f7-4abf-8f50-0439796f0ab5
static GUID statusVarGUID = { 0x5a725dfdU, 0xf1f7U, 0x4abfU, { 0x8fU, 0x50U, 0x04U, 0x39U, 0x79U, 0x6fU, 0x0aU, 0xb5U } };
#else
static TCHAR const statusVarName[] = TEXT("NvStrapsReBarStatus");
static TCHAR const statusVarGUID[] = TEXT("{5a725dfd-f1f7-4abf-8f50-0439796f0ab5}");
#endif

#if defined(UEFI_SOURCE) || defined(EFIAPI)
static StatusVar statusVar = StatusVar_NotLoaded;

void SetStatusVar(StatusVar val)
{
    if (val > statusVar)
    {
        statusVar = val;
        WriteStatusVar();
    }
}

EFI_STATUS WriteStatusVar()
{
   UINT64 var = (UINT64)TARGET_GPU_PCI_BUS << (24u + 32u) | (UINT64)TARGET_GPU_PCI_DEVICE << (19u + 32u) | (UINT64)TARGET_GPU_PCI_FUNCTION << (16u + 32u) | (UINT64)statusVar;

   return gRT->SetVariable(statusVarName, &statusVarGUID, EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, sizeof var, &var);
};
#else
uint_least64_t ReadStatusVar(uint_least32_t *dwLastError)
{
   UINT64 var = StatusVar_NotLoaded;
   DWORD varSize = (SetLastError(0u), GetFirmwareEnvironmentVariable(statusVarName, statusVarGUID, &var, sizeof var));

   if (varSize)
   {
        *dwLastError = ERROR_SUCCESS;

        if (varSize != sizeof var)
            var = StatusVar_ParseError;
        else
            ;
   }
   else
   {
        *dwLastError = GetLastError();

        if (*dwLastError == ERROR_ENVVAR_NOT_FOUND)
            *dwLastError = ERROR_SUCCESS;

        var = (*dwLastError) ? StatusVar_NVAR_API_Error : StatusVar_NotLoaded;
   }

   return var;
}
#endif
