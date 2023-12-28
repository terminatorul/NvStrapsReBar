#if !defined(NV_STRAPS_REBAR_CONFIG_H)
#define NV_STRAPS_REBAR_CONFIG_H

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <windef.h>

# if defined(__cplusplus)
#  include <cstddef>
#  include <cstdint>
#  include <utility>
# endif

#define MAX_UINT16 UINT16_MAX
#define MAX_UINT8  UINT8_MAX
#endif

#if !defined(__cplusplus)
# include <stdbool.h>
# include <stddef.h>
#endif

#include "LocalAppConfig.h"
#include "DeviceRegistry.h"

typedef enum
{
    UNCONFIGURED = 0u,
    IMPLIED_GLOBAL = 1u,
    FOUND_GLOBAL = 2u,
    EXPLICIT_PCI_ID = 3u,
    EXPLICIT_SUBSYSTEM_ID = 4u,
    EXPLICIT_PCI_LOCATION = 5u
}
    ConfigPriority;

enum
{
    NvStraps_GPU_MAX_COUNT = 8u
};

enum
{
    TARGET_GPU_VENDOR_ID = 0x10DEu
};

// Special values for desired PCI BAR size in  NVAR variable
enum
{
    TARGET_PCI_BAR_SIZE_DISABLED = 0u,
    TARGET_PCI_BAR_SIZE_MIN = 1u,
    TARGET_PCI_BAR_SIZE_MAX = 32u,
    TARGET_PCI_BAR_SIZE_GPU_ONLY = 64u,
    TARGET_PCI_BAR_SIZE_GPU_STRAPS_ONLY = 65u
};

typedef struct NvStraps_GPUSelector
{
    uint_least16_t deviceID, subsysVendorID, subsysDeviceID;
    uint_least8_t  bus;
    uint_least8_t  device;
    uint_least8_t  function;
    uint_least8_t  barSizeSelector;

#if defined(__cplusplus)
    bool operator ==(NvStraps_GPUSelector const &other) const = default;

    bool deviceMatch(uint_least16_t deviceID) const;
    bool subsystemMatch(uint_least16_t subsysVenID, uint_least16_t subsysDevID) const;
    bool busLocationMatch(uint_least8_t busNr, uint_least8_t dev, uint_least8_t fn) const;
#endif
}
    NvStraps_GPUSelector;

typedef struct NvStraps_GPUConfig
{
    uint_least16_t deviceID, subsysVendorID, subsysDeviceID;
    uint_least8_t  bus, device, function;
    uint_least32_t baseAddressSelector;
}
    NvStraps_GPUConfig;

typedef struct NvStraps_BridgeConfig
{
    uint_least16_t vendorID;
    uint_least16_t deviceID;
    uint_least8_t  bridgeBus;
    uint_least8_t  bridgeDevice;
    uint_least8_t  bridgeFunction;
    uint_least8_t  bridgeSecondaryBus;
    uint_least8_t  bridgeSubsidiaryBus;
    uint_least16_t bridgeIOBaseLimit;
    uint_least32_t bridgeMemBaseLimit;
}
    NvStraps_BridgeConfig;

typedef struct NvStraps_BarSize
{
    ConfigPriority priority;
    BarSizeSelector barSizeSelector;
}
    NvStraps_BarSize;

typedef struct NvStrapsConfig
{

    bool dirty;
    uint_least8_t nPciBarSize;
    uint_least8_t nGlobalEnable;

    uint_least8_t nGPUSelector;
    NvStraps_GPUSelector GPUs[NvStraps_GPU_MAX_COUNT];

    uint_least8_t nGPUConfig;
    NvStraps_GPUConfig gpuConfig[NvStraps_GPU_MAX_COUNT];

    uint_least8_t nBridgeConfig;
    NvStraps_BridgeConfig bridge[NvStraps_GPU_MAX_COUNT + 2u];

#if defined(__cplusplus)
    bool isDirty() const;
    uint_least8_t isGlobalEnable() const;
    uint_least8_t setGlobalEnable(uint_least8_t val);

    uint_least8_t targetPciBarSizeSelector() const;
    uint_least8_t targetPciBarSizeSelector(uint_least8_t barSizeSelector);

    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID);
    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);

    bool clearGPUSelector(uint_least16_t deviceID);
    bool clearGPUSelector(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
    bool clearGPUSelector(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);

    bool clearGPUSelectors();

    NvStraps_BarSize lookupBarSize(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const;
#endif
}
    NvStrapsConfig;

enum
{
    NV_STRAPS_HEADER_SIZE = 2u,
    GPU_SELECTOR_SIZE = WORD_SIZE * 3u + BYTE_SIZE * 3u,
    GPU_CONFIG_SIZE = 3u * WORD_SIZE + 3u * BYTE_SIZE + DWORD_SIZE,
    BRIDGE_CONFIG_SIZE = 2u * WORD_SIZE + 4u * BYTE_SIZE + WORD_SIZE + DWORD_SIZE,
    NV_STRAPS_CONFIG_SIZE = NV_STRAPS_HEADER_SIZE
        + BYTE_SIZE + GPU_SELECTOR_SIZE * NvStraps_GPU_MAX_COUNT
        + BYTE_SIZE + GPU_CONFIG_SIZE * NvStraps_GPU_MAX_COUNT
        + BYTE_SIZE + BRIDGE_CONFIG_SIZE * (NvStraps_GPU_MAX_COUNT + 2u)
};

#define NVSTRAPSCONFIG_BUFFERSIZE(config)       NV_STRAPS_CONFIG_SIZE

#if defined(__cplusplus)
extern "C"
{
#endif

extern char const NvStrapsConfig_VarName[];

bool NvStrapsConfig_GPUSelector_DeviceMatch(NvStraps_GPUSelector const *selector, uint_least16_t devID);
bool NvStrapsConfig_GPUSelector_SubsystemMatch(NvStraps_GPUSelector const *selector, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
bool NvStrapsConfig_GPUSelector_BusLocationMatch(NvStraps_GPUSelector const *selector, uint_least8_t busNr, uint_least8_t dev, uint_least8_t func);
uint_least8_t NvStrapsConfig_TargetPciBarSizeSelector(NvStrapsConfig const *config);
uint_least8_t NvStrapsConfig_SetTargetPciBarSizeSelector(NvStrapsConfig *config, uint_least8_t barSizeSelector);
uint_least8_t NvStrapsConfig_IsGlobalEnable(NvStrapsConfig const *config);
uint_least8_t NvStrapsConfig_SetGlobalEnable(NvStrapsConfig *config, uint_least8_t globalEnable);
bool NvStrapsConfig_IsDirty(NvStrapsConfig const *config);
bool NvStrapsConfig_SetIsDirty(NvStrapsConfig *config, bool dirtyFlag);
bool NvStrapsConfig_IsGpuConfigured(NvStrapsConfig const *config);
bool NvStrapsConfig_IsDriverConfigured(NvStrapsConfig const *config);
void NvStrapsConfig_Clear(NvStrapsConfig *config);

NvStraps_BarSize NvStrapsConfig_LookupBarSize(NvStrapsConfig const *config, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
NvStrapsConfig *GetNvStrapsConfig(bool reload, ERROR_CODE *errorCode);
void SaveNvStrapsConfig(ERROR_CODE *errorCode);

inline uint_least8_t NvStrapsConfig_TargetPciBarSizeSelector(NvStrapsConfig const *config)
{
    return config->nPciBarSize;
}

inline uint_least8_t NvStrapsConfig_SetTargetPciBarSizeSelector(NvStrapsConfig *config, uint_least8_t barSizeSelector)
{
    uint_least8_t pciBarSize = config->nPciBarSize;
    config->dirty = barSizeSelector != config->nPciBarSize;

    return config->nPciBarSize = barSizeSelector, pciBarSize;
}

inline uint_least8_t NvStrapsConfig_IsGlobalEnable(NvStrapsConfig const *config)
{
    return config->nGlobalEnable;
}

inline uint_least8_t NvStrapsConfig_SetGlobalEnable(NvStrapsConfig *config, uint_least8_t globalEnable)
{
    uint_least8_t oldGlobalEnable = config->nGlobalEnable;
    config->dirty = globalEnable != config->nGlobalEnable;

    return config->nGlobalEnable = globalEnable, oldGlobalEnable;
}

inline bool NvStrapsConfig_IsDirty(NvStrapsConfig const *config)
{
    return config->dirty;
}

inline bool NvStrapsConfig_SetIsDirty(NvStrapsConfig *config, bool dirtyFlag)
{
    bool oldFlag = config->dirty;
    return config->dirty = dirtyFlag, oldFlag;
}

inline bool NvStrapsConfig_IsGpuConfigured(NvStrapsConfig const *config)
{
    return config->nGlobalEnable || config->nGPUSelector;
}

inline bool NvStrapsConfig_IsDriverConfigured(NvStrapsConfig const *config)
{
    return NvStrapsConfig_TargetPciBarSizeSelector(config) || NvStrapsConfig_IsGpuConfigured(config);
}

inline bool NvStrapsConfig_GPUSelector_DeviceMatch(NvStraps_GPUSelector const *selector, uint_least16_t devID)
{
    return selector->deviceID == devID;
}

inline bool NvStrapsConfig_GPUSelector_SubsystemMatch(NvStraps_GPUSelector const *selector, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return selector->subsysVendorID == subsysVenID && selector->subsysDeviceID == subsysDevID;
}

inline bool NvStrapsConfig_GPUSelector_BusLocationMatch(NvStraps_GPUSelector const *selector, uint_least8_t busNr, uint_least8_t dev, uint_least8_t func)
{
    return selector->bus == busNr && selector->device == dev && selector->function == func;
}

#if defined(__cplusplus)
}       // extern "C"

NvStrapsConfig &GetNvStrapsConfig(bool reload = false);
void SaveNvStrapsConfig();

inline bool NvStrapsConfig::setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID)
{
     return setGPUSelector(barSizeSelector, deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return setGPUSelector(barSizeSelector, deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStrapsConfig::clearGPUSelector(uint_least16_t deviceID)
{
    return clearGPUSelector(deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::clearGPUSelector(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return clearGPUSelector(deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStraps_GPUSelector::deviceMatch(uint_least16_t devID) const
{
    return NvStrapsConfig_GPUSelector_DeviceMatch(this, devID);
}

inline bool NvStraps_GPUSelector::subsystemMatch(uint_least16_t subsysVenID, uint_least16_t subsysDevID) const
{
    return NvStrapsConfig_GPUSelector_SubsystemMatch(this, subsysVenID, subsysDevID);
}

inline bool NvStraps_GPUSelector::busLocationMatch(uint_least8_t busNr, uint_least8_t dev, uint_least8_t fn) const
{
    return NvStrapsConfig_GPUSelector_BusLocationMatch(this, busNr, dev, fn);
}

inline bool NvStrapsConfig::clearGPUSelectors()
{
    return dirty = !!nGPUSelector, !!std::exchange(nGPUSelector, 0u);
}

inline bool NvStrapsConfig::isDirty() const
{
    return NvStrapsConfig_IsDirty(this);
}

inline uint_least8_t NvStrapsConfig::isGlobalEnable() const
{
    return NvStrapsConfig_IsGlobalEnable(this);
}

inline uint_least8_t NvStrapsConfig::setGlobalEnable(uint_least8_t val)
{
    return NvStrapsConfig_SetGlobalEnable(this, val);
}

inline uint_least8_t NvStrapsConfig::targetPciBarSizeSelector() const
{
    return NvStrapsConfig_TargetPciBarSizeSelector(this);
}

inline uint_least8_t NvStrapsConfig::targetPciBarSizeSelector(uint_least8_t barSizeSelector)
{
    return NvStrapsConfig_SetTargetPciBarSizeSelector(this, barSizeSelector);
}

inline NvStraps_BarSize NvStrapsConfig::lookupBarSize(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const
{
    return NvStrapsConfig_LookupBarSize(this, deviceID, subsysVenID, subsysDevID, bus, dev, fn);
}

#endif          // defined(__cplusplus)

#endif          // !defined(NV_STRAPS_REBAR_CONFIG_H)
