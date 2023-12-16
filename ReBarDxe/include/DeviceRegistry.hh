#if !defined(NV_STRAPS_REBAR_DEVICE_REGISTRY_HH)
#define NV_STRAPS_REBAR_DEVICE_REGISTRY_HH

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <windef.h>
# include <iostream>

void listRegistry(std::ostream &out);

#endif

// From PSTRAPS documentation in envytools:
// https://envytools.readthedocs.io/en/latest/hw/io/pstraps.html
constexpr unsigned short const MAX_BAR_SIZE_SELECTOR = 10u;

enum BarSizeSelector: UINT8
{
     _64M =  0u,
    _128M =  1u,
    _256M =  2u,
    _512M =  3u,
      _1G =  4u,
      _2G =  5u,
      _4G =  6u,
      _8G =  7u,
     _16G =  8u,
     _32G =  9u,
     _64G = 10u,

    Excluded = 0xFEu,
    None     = 0xFFu
};


bool isTuringGPU(UINT16 deviceID);
BarSizeSelector lookupBarSizeInRegistry(UINT16 deviceID);

#endif          // !defined(NV_STRAPS_REBAR_DEVICE_REGISTRY_HH)
