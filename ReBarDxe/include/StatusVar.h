#if !defined(NV_STRAPS_REBAR_STATUS_VAR_H)
#define NV_STRAPS_REBAR_STATUS_VAR_H

#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <stdint.h>
#endif

typedef enum StatusVar
{
    StatusVar_NotLoaded = 10u,
    StatusVar_Unconfigured = 20u,
    StatusVar_GPU_Unconfigured = 30u,
    StatusVar_Cleared = 40u,
    StatusVar_Configured = 50u,
    StatusVar_BridgeFound = 60u,
    StatusVar_GpuFound = 70u,
    StatusVar_GpuStrapsConfigured = 80u,
    StatusVar_GpuDelayElapsed = 90u,
    StatusVar_GpuReBarConfigured = 100u,
    StatusVar_GpuNoReBarCapability = 110u,
    StatusVar_EFIAllocationError = 120u,
    StatusVar_EFIError = 130u,
    StatusVar_NVAR_API_Error = 140u,
    StatusVar_ParseError = 150u
}
    StatusVar;

void SetStatusVar(StatusVar val);

#if defined(UEFI_SOURCE) || defined(EFIAPI)
EFI_STATUS WriteStatusVar();
#else
#if defined(__cplusplus)
extern "C"
{
#endif
uint_least64_t ReadStatusVar(uint_least32_t *dwLastError);
#if defined(__cplusplus)
}
#endif
#endif

#endif          // !defined(NV_STRAPS_REBAR_STATUS_VAR_H)
