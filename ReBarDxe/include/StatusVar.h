#if !defined(NV_STRAPS_REBAR_STATUS_VAR_H)
#define NV_STRAPS_REBAR_STATUS_VAR_H

#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <stdint.h>
#endif

#include "LocalAppConfig.h"

typedef enum StatusVar
{
    StatusVar_NotLoaded = 10u,
    StatusVar_Configured = 20u,
    StatusVar_GPU_Unconfigured = 30u,
    StatusVar_Unconfigured = 40u,
    StatusVar_Cleared = 50u,

    StatusVar_BridgeFound = 60u,
    StatusVar_GpuFound = 70u,
    StatusVar_GpuStrapsConfigured = 80u,
    StatusVar_GpuStrapsPreConfigured = 90u,
    StatusVar_GpuStrapsConfirm = 100u,
    StatusVar_GpuDelayElapsed = 110u,
    StatusVar_GpuReBarConfigured = 120u,
    StatusVar_GpuStrapsNoConfirm = 130u,
    StatusVar_GpuNoReBarCapability = 140u,
    StatusVar_GpuExcluded = 150u,

    StatusVar_EFIAllocationError = 160u,
    StatusVar_Internal_EFIError = 170u,
    StatusVar_NVAR_API_Error = 180u,
    StatusVar_ParseError = 190u
}
    StatusVar;

typedef enum EFIErrorLocation
{
    EFIError_None,
    EFIError_ReadConfigVar,
    EFIError_WriteConfigVar,
    EFIError_PCI_StartFindCap,
    EFIError_PCI_FindCap,
    EFIError_PCI_BridgeConfig,
    EFIError_PCI_BridgeRestore,
    EFIError_PCI_DeviceBARConfig,
    EFIError_PCI_DeviceBARRestore,
    EFIError_PCI_DeviceSubsystem,
    EFIError_LocateBridgeProtocol,
    EFIError_LoadBridgeProtocol,
    EFIError_CMOSTime,
    EFIError_CreateTimer,
    EFIError_CloseTimer,
    EFIError_SetupTimer,
    EFIError_WaitTimer
}
    EFIErrorLocation;

extern char const StatusVar_Name[];

void SetStatusVar(StatusVar val);

#if defined(UEFI_SOURCE) || defined(EFIAPI)
void SetEFIError(EFIErrorLocation errLocation, EFI_STATUS status);
void SetDeviceStatusVar(UINTN pciAddress, StatusVar val);
void SetDeviceEFIError(UINTN pciAddress, EFIErrorLocation errLocation, EFI_STATUS status);
#else
#if defined(__cplusplus)
extern "C"
{
#endif

uint_least64_t ReadStatusVar(ERROR_CODE *errorCode);

#if defined(__cplusplus)
}
#endif
#endif

#endif          // !defined(NV_STRAPS_REBAR_STATUS_VAR_H)
