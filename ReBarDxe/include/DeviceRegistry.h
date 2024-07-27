#if !defined(NV_STRAPS_REBAR_DEVICE_REGISTRY_H)
#define NV_STRAPS_REBAR_DEVICE_REGISTRY_H

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# if defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)
import NvStraps.WinAPI;
# else
#  include <windef.h>
# endif
#endif

#if !defined(__cplusplus)
# include <stdbool.h>
#endif

// From PSTRAPS documentation in envytools:
// https://envytools.readthedocs.io/en/latest/hw/io/pstraps.html
#if defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)
    extern constexpr
#else
    static
#endif
    unsigned short const MAX_BAR_SIZE_SELECTOR = 10u;

typedef enum BarSizeSelector
{
     BarSizeSelector_64M =  0u,
    BarSizeSelector_128M =  1u,
    BarSizeSelector_256M =  2u,
    BarSizeSelector_512M =  3u,
      BarSizeSelector_1G =  4u,
      BarSizeSelector_2G =  5u,
      BarSizeSelector_4G =  6u,
      BarSizeSelector_8G =  7u,
     BarSizeSelector_16G =  8u,
     BarSizeSelector_32G =  9u,
     BarSizeSelector_64G = 10u,

    BarSizeSelector_Excluded = 0xFEu,
    BarSizeSelector_None     = 0xFFu
}
    BarSizeSelector;

#if defined(__cplusplus)
extern "C"
{
#endif

bool isTuringGPU(UINT16 deviceID);
BarSizeSelector lookupBarSizeInRegistry(UINT16 deviceID);

#if defined(__cplusplus)
}       // extern "C"
#endif

#endif          // !defined(NV_STRAPS_REBAR_DEVICE_REGISTRY_H)
