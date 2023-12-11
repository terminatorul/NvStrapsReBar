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

enum BarSizeSelector: UINT8
{
    _256M = 0u,
    _512M = 1u,
      _1G = 2u,
      _2G = 3u,
      _4G = 4u,
      _8G = 5u,
     _16G = 6u,
     _32G = 7u,
     _64G = 8u,

    Excluded = 0xFEu,
    None     = 0xFFu
};

bool isTuringGPU(UINT16 deviceID);
BarSizeSelector lookupBarSizeInRegistry(UINT16 deviceID);

#endif          // !defined(NV_STRAPS_REBAR_DEVICE_REGISTRY_HH)
