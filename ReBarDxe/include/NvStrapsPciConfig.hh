#if !defined(NV_STRAPS_REBAR_PCI_CONFIG_HH)
#define NV_STRAPS_REBAR_PCI_CONFIG_HH

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <windef.h>
# include <cstddef>
# include <cstdint>
#endif

#include <limits>
#include <span>
#include <tuple>
#include <utility>

#include "DeviceRegistry.hh"

// static_assert(std::numeric_limits<unsigned char>::radix == 2u, "Binary digits expected for std::numeric_limits<>.");

constexpr std::size_t const
        BYTE_BITSIZE = std::numeric_limits<unsigned char>::digits,
        BYTE_SIZE = 1u,
        WORD_SIZE = 2u,
        DWORD_SIZE = 4u,
        QWORD_SIZE = 8u;

enum class ConfigPriority
{
    UNCONFIGURED = 0u,
    IMPLIED_GLOBAL = 1u,
    FOUND_GLOBAL = 2u,
    EXPLICIT_PCI_ID = 3u,
    EXPLICIT_SUBSYSTEM_ID = 4u,
    EXPLICIT_PCI_LOCATION = 5u
};
class NvStrapsConfig
{
protected:
    static constexpr unsigned const GPU_MAX_COUNT = 8u;

    mutable bool dirty = false;

    UINT8 nGlobalEnable = 0u;
    UINT8 nGPUSelector = 0u;

    struct GPUSelector
    {
        UINT16 deviceID, subsysVendorID, subsysDeviceID;
        UINT8  bus;
        UINT8  device;
        UINT8  function;
        UINT8  barSizeSelector;

        bool deviceMatch(UINT16 deviceID) const;
        bool subsystemMatch(UINT16 subsysVendorID, UINT16 subsysDeviceID) const;
        bool hasSubsystem() const;
        bool busLocationMatch(UINT8 bus, UINT8 device, UINT8 function) const;
        bool hasBusLocation() const;

        std::byte *pack(std::byte *&buffer) const;
        void unpack(std::byte const *&buffer);

        bool operator ==(GPUSelector const &other) const = default;
    }
        GPUs[GPU_MAX_COUNT];

    UINT8 nGPUConfig = 0u;

    struct GPUConfig
    {
        UINT16 deviceID, subsysVendorID, subsysDeviceID;
        UINT8  bus;
        UINT32 baseAddressSelector;

        std::byte *pack(std::byte *&buffer) const;
        void unpack(std::byte const *&buffer);
    }
        gpuConfig[GPU_MAX_COUNT];

    UINT8 nBridgeConfig = 0u;

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

        std::byte *pack(std::byte *&buffer) const;
        void unpack(std::byte const *&buffer);
    }
        bridge[GPU_MAX_COUNT + 2u];

    static constexpr std::size_t const
        GPU_SELECTOR_SIZE = WORD_SIZE * 3u + BYTE_SIZE * 3u,
        GPU_CONFIG_SIZE = WORD_SIZE * 3u + BYTE_SIZE + DWORD_SIZE,
        BRIDGE_CONFIG_SIZE = 2u * WORD_SIZE + 4u * BYTE_SIZE + WORD_SIZE + DWORD_SIZE,
        NV_STRAPS_CONFIG_SIZE = 2u * BYTE_SIZE + GPU_SELECTOR_SIZE * sizeof(GPUs) / sizeof(GPUs[0])
                                  + BYTE_SIZE + GPU_CONFIG_SIZE * sizeof(gpuConfig) / sizeof(gpuConfig[0])
                                  + BYTE_SIZE + BRIDGE_CONFIG_SIZE * sizeof(bridge) / sizeof(bridge[0]);

    void clear();

    unsigned save(std::span<std::byte> buffer) const;
    void load(std::span<std::byte const> buffer);
    bool isConfigured() const;

    friend NvStrapsConfig &GetNvStrapsPciConfig(bool reload);
    friend bool SaveNvStrapsPciConfig();

public:
    static constexpr std::size_t bufferSize();

    UINT8 isGlobalEnable() const;
    UINT8 setGlobalEnable(UINT8 gloablEnable);
    bool isDirty() const;
    bool isDirty(bool dirty);

    bool setGPUSelector( UINT8 barSizeSelector, UINT16 deviceID, UINT16 subsysVenID = 0xFFFFu, UINT16 subsysDevID = 0xFFFFu, UINT8 bus = 0xFFu, UINT8 dev = 0xFFu, UINT8 fn = 0xFFu);
    bool clearGPUSelector(UINT16 deviceID, UINT16 subsysVenID = 0xFFFFu, UINT16 subsysDevID = 0xFFFFu, UINT8 bus = 0xFFu, UINT8 dev = 0xFFu, UINT8 fn = 0xFFu);
    bool clearGPUSelectors();

    std::tuple<ConfigPriority, BarSizeSelector> lookupBarSize(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn) const;
};

NvStrapsConfig &GetNvStrapsPciConfig(bool reload = false);
bool SaveNvStrapsPciConfig();

inline UINT8 NvStrapsConfig::isGlobalEnable() const
{
    return nGlobalEnable;
}

inline UINT8 NvStrapsConfig::setGlobalEnable(UINT8 globalEnable)
{
    dirty = globalEnable != nGlobalEnable;

    return std::exchange(nGlobalEnable, globalEnable);
}

inline bool NvStrapsConfig::isDirty() const
{
    return dirty;
}

inline bool NvStrapsConfig::isDirty(bool dirtyFlag)
{
    return std::exchange(dirty, dirtyFlag);
}

constexpr std::size_t NvStrapsConfig::bufferSize()
{
    return NV_STRAPS_CONFIG_SIZE;
}

#endif          // !defined(NV_STRAPS_REBAR_PCI_CONFIG_HH)
