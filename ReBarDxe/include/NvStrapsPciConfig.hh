#if !defined(NV_STRAPS_REBAR_PCI_CONFIG_HH)
#define NV_STRAPS_REBAR_PCI_CONFIG_HH

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <cstdint>

typedef std::uint_least8_t  UINT8;
typedef std::uint_least16_t UINT16;
typedef std::uint_least32_t UINT32;
typedef std::uint_least64_t UINT64;

#endif

struct NvStrapsConfig
{
    static constexpr unsigned const GPU_MAX_COUNT = 8u;

    UINT8 nGPUSelector;

    struct GPUSelector
    {
        UINT16 deviceID;
        UINT8  bus;
        UINT8  device;
        UINT8  function;
        UINT8  barSizeSelector;
    }
        GPUs[GPU_MAX_COUNT];

    UINT8 nGPUConfig;

    struct GPUConfig
    {
        UINT16 deviceID;
        UINT8  bus;
        UINT32 baseAddressSelector;
    }
        gpuConfig[GPU_MAX_COUNT];

    UINT8 nBridgeConfig;

    struct BridgeConfig
    {
        UINT16 vendorID;
        UINT16 deviceID;
        UINT8  bridgeBus;
        UINT8  bridgeDevice;
        UINT8  bridgeFunction;
        UINT8  bridgeSecondaryBus;
        UINT8  bridgeSubsidiaryBus;
        UINT16 bridgeIOBaseLimit;
        UINT32 bridgeMemBaseLimit;
    }
        bridge[GPU_MAX_COUNT];
}
    &GetNvStrapsPciConfig();

bool SaveNvStrapsPciConfig();

#endif          // !defined(NV_STRAPS_REBAR_PCI_CONFIG_HH)
