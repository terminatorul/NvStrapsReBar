#if !defined(NV_STRAPS_REBAR_CONFIG_H)
#define NV_STRAPS_REBAR_CONFIG_H

#if defined(UEFI_SOURCE)
# include <Uefi.h>
#else
# if defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)
import std;
using std::uint_least8_t;
using std::uint_least16_t;
using std::uint_least32_t;
using std::uint_least64_t;

import NvStraps.WinAPI;
import LocalAppConfig;
import DeviceRegistry;

constexpr auto const MAX_UINT8  = UINT8_MAX;
constexpr auto const MAX_UINT16 = UINT16_MAX;
constexpr auto const MAX_UINT32 = UINT32_MAX;
constexpr auto const MAX_UINT64 = UINT64_MAX;
# else
#  include <stdbool.h>
#  include <stdint.h>
#  define MAX_UINT16 UINT16_MAX
#  define MAX_UINT8  UINT8_MAX
# endif
#endif

#if !defined(__cplusplus) || defined(NVSTRAPS_DXE_DRIVER)
# include "LocalAppConfig.h"
# include "DeviceRegistry.h"
#endif

typedef enum ConfigPriority
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
    TARGET_BRIDGE_IO_BASE_LIMIT = 0xF1F1u,
    TARGET_GPU_VENDOR_ID = 0x10DEu
};

// Special values for desired PCI BAR size in NVAR variable
enum TARGET_PCI_BAR_SIZE
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
    uint_least8_t  overrideBarSizeMask;

#if defined(__cplusplus)
    bool operator ==(NvStraps_GPUSelector const &other) const = default;

    bool deviceMatch(uint_least16_t deviceID) const;
    bool subsystemMatch(uint_least16_t subsysVenID, uint_least16_t subsysDevID) const;
    bool busLocationMatch(uint_least8_t busNr, uint_least8_t dev, uint_least8_t fn) const;
#endif
}
    NvStraps_GPUSelector;

enum
{
    GPU_SELECTOR_SIZE = WORD_SIZE * 3u + BYTE_SIZE * 4u,
};

typedef struct NvStraps_GPUConfig
{
    uint_least16_t deviceID, subsysVendorID, subsysDeviceID;
    uint_least8_t  bus, device, function;

    struct MMIO_Range
    {
    uint_least64_t base, top;

#if defined(__cplusplus)
    bool operator ==(MMIO_Range const &other) const = default;
#endif
    }
    bar0;

#if defined(__cplusplus)
    bool operator ==(NvStraps_GPUConfig const &other) const = default;
    bool deviceMatch(uint_least16_t devID) const;
    bool subsystemMatch(uint_least16_t subsysVenID, uint_least16_t subsysDevID) const;
#endif
}
    NvStraps_GPUConfig;

enum
{
    GPU_CONFIG_SIZE = 3u * WORD_SIZE + 2u * BYTE_SIZE + 2u * QWORD_SIZE,
};

typedef struct NvStraps_BridgeConfig
{
    uint_least16_t vendorID;
    uint_least16_t deviceID;
    uint_least8_t  bridgeBus;
    uint_least8_t  bridgeDevice;
    uint_least8_t  bridgeFunction;
    uint_least8_t  bridgeSecondaryBus;

#if defined(__cplusplus)
    bool operator ==(NvStraps_BridgeConfig const &other) const = default;
    bool deviceMatch(uint_least16_t venID, uint_least16_t devID) const;
    bool busLocationMatch(uint_least8_t bus, uint_least8_t dev, uint_least8_t func) const;
#endif
}
    NvStraps_BridgeConfig;

enum
{
    BRIDGE_CONFIG_SIZE = 2u * WORD_SIZE + 3u * BYTE_SIZE,
};

typedef struct NvStraps_BarSize
{
    ConfigPriority priority;
    BarSizeSelector barSizeSelector;
}
    NvStraps_BarSize;

typedef struct NvStraps_BarSizeMaskOverride
{
    ConfigPriority priority;
    bool sizeMaskOverride;
}
    NvStraps_BarSizeMaskOverride;

typedef struct NvStrapsConfig
{
    bool dirty;
    uint_least8_t nPciBarSize;
    uint_least16_t nOptionFlags;
    uint_least64_t nSetupVarCRC;

    uint_least8_t nGPUSelector;
    NvStraps_GPUSelector GPUs[NvStraps_GPU_MAX_COUNT];

    uint_least8_t nGPUConfig;
    NvStraps_GPUConfig gpuConfig[NvStraps_GPU_MAX_COUNT];

    uint_least8_t nBridgeConfig;
    NvStraps_BridgeConfig bridge[NvStraps_GPU_MAX_COUNT + 2u];

#if defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)
    bool isDirty() const;
    bool isDirty(bool fDirty);
    uint_least8_t isGlobalEnable() const;
    uint_least8_t setGlobalEnable(uint_least8_t val);
    bool skipS3Resume() const;
    bool skipS3Resume(bool fSkip);
    bool overrideBarSizeMask() const;
    bool overrideBarSizeMask(bool overrideSizeMask);
    bool hasSetupVarCRC() const;
    bool hasSetupVarCRC(bool hasCRC);
    bool enableSetupVarCRC() const;
    bool enableSetupVarCRC(bool enableCRC);

    uint_least8_t targetPciBarSizeSelector() const;
    uint_least8_t targetPciBarSizeSelector(uint_least8_t barSizeSelector);
    uint_least64_t setupVarCRC() const;
    uint_least64_t setupVarCRC(uint_least64_t varCRC);

    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID);
    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
    bool setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
    bool setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID);
    bool setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
    bool setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
    bool setGPUConfig(NvStraps_GPUConfig const &config);
    bool setBridgeConfig(NvStraps_BridgeConfig const &config);

    bool clearGPUSelector(uint_least16_t deviceID);
    bool clearGPUSelector(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
    bool clearGPUSelector(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);

    bool resetConfig();
    bool clearGPUSelectors();

    NvStraps_BarSize lookupBarSize(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const;
    NvStraps_BarSizeMaskOverride lookupBarSizeMaskOverride(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const;
    std::tuple<uint_least16_t, uint_least16_t> hasBridgeDevice(uint_least8_t bridgeBus, uint_least8_t bridgeDevice, uint_least8_t bridgeFunction) const;
    NvStraps_BridgeConfig const *lookupBridgeConfig(uint_least8_t bridgeSecondaryBus) const;
    NvStraps_GPUConfig const *lookupGPUConfig(uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const;
#endif
}
    NvStrapsConfig;

enum
{
    NV_STRAPS_HEADER_SIZE = BYTE_SIZE /* PCI BAR size */ + WORD_SIZE /* Option flags */ + QWORD_SIZE /* SetupVar CRC64 */,
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
bool NvStrapsConfig_GPUConfig_DeviceMatch(NvStraps_GPUConfig const *config, uint_least16_t devID);
bool NvStrapsConfig_GPUConfig_SubsystemMatch(NvStraps_GPUConfig const *config, uint_least16_t subsysVenID, uint_least16_t subsysDevID);
bool NvStrapsConfig_BridgeConfig_DeviceMatch(NvStraps_BridgeConfig const *config, uint_least16_t venID, uint_least16_t devID);
bool NvStrapsConfig_BridgeConfig_BusLocationMatch(NvStraps_BridgeConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t func);
uint_least8_t NvStrapsConfig_TargetPciBarSizeSelector(NvStrapsConfig const *config);
uint_least8_t NvStrapsConfig_SetTargetPciBarSizeSelector(NvStrapsConfig *config, uint_least8_t barSizeSelector);
uint_least8_t NvStrapsConfig_IsGlobalEnable(NvStrapsConfig const *config);
uint_least8_t NvStrapsConfig_SetGlobalEnable(NvStrapsConfig *config, uint_least8_t globalEnable);
uint_least64_t NvStrapsConfig_SetupVarCRC(NvStrapsConfig const *config);
uint_least64_t NvStrapsConfig_SetSetupVarCRC(NvStrapsConfig *config, uint_least64_t varCRC);
bool NvStrapsConfig_SetGPUConfig(NvStrapsConfig *config, NvStraps_GPUConfig const *gpuConfig);
bool NvStrapsConfig_SetBridgeConfig(NvStrapsConfig *config, NvStraps_BridgeConfig const *bridgeConfig);
bool NvStrapsConfig_IsDirty(NvStrapsConfig const *config);
bool NvStrapsConfig_SetIsDirty(NvStrapsConfig *config, bool dirtyFlag);
bool NvStrapsConfig_SkipS3Resume(NvStrapsConfig const *config);
bool NvStrapsConfig_SetSkipS3Resume(NvStrapsConfig *config, bool fSkipS3Resume);
bool NvStrapsConfig_OverrideBarSizeMask(NvStrapsConfig const *config);
bool NvStrapsConfig_SetOverrideBarSizeMask(NvStrapsConfig *config, bool fOverrideSizeMask);
bool NvStrapsConfig_HasSetupVarCRC(NvStrapsConfig const *config);
bool NvStrapsConfig_SetHasSetupVarCRC(NvStrapsConfig *config, bool hasCrc);
bool NvStrapsConfig_IsGpuConfigured(NvStrapsConfig const *config);
bool NvStrapsConfig_IsDriverConfigured(NvStrapsConfig const *config);
bool NvStrapsConfig_ResetConfig(NvStrapsConfig *config);
void NvStrapsConfig_Clear(NvStrapsConfig *config);

NvStraps_BarSize NvStrapsConfig_LookupBarSize(NvStrapsConfig const *config, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
NvStraps_BarSizeMaskOverride NvStrapsConfig_LookupBarSizeMaskOverride(NvStrapsConfig const *config, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
NvStraps_GPUConfig const *NvStrapsConfig_LookupGPUConfig(NvStrapsConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);
NvStraps_BridgeConfig const *NvStrapsConfig_LookupBridgeConfig(NvStrapsConfig const *config, uint_least8_t secondaryBus);
uint_least32_t NvStrapsConfig_HasBridgeDevice(NvStrapsConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn);

NvStrapsConfig *GetNvStrapsConfig(bool reload, ERROR_CODE *errorCode);
void SaveNvStrapsConfig(ERROR_CODE *errorCode);

inline uint_least8_t NvStrapsConfig_TargetPciBarSizeSelector(NvStrapsConfig const *config)
{
    return config->nPciBarSize;
}

inline uint_least8_t NvStrapsConfig_SetTargetPciBarSizeSelector(NvStrapsConfig *config, uint_least8_t barSizeSelector)
{
    uint_least8_t pciBarSize = config->nPciBarSize;
    config->dirty = config->dirty || barSizeSelector != config->nPciBarSize;

    return config->nPciBarSize = barSizeSelector, pciBarSize;
}

inline uint_least64_t NvStrapsConfig_SetupVarCRC(NvStrapsConfig const *config)
{
    return config->nSetupVarCRC;
}

inline uint_least64_t NvStrapsConfig_SetSetupVarCRC(NvStrapsConfig *config, uint_least64_t varCRC)
{
    uint_least64_t previousCRC = NvStrapsConfig_SetupVarCRC(config);

    if (previousCRC != varCRC)
    {
    config->dirty = true;
    config->nSetupVarCRC = varCRC;
    }

    return previousCRC;
}

inline uint_least8_t NvStrapsConfig_IsGlobalEnable(NvStrapsConfig const *config)
{
    return config->nOptionFlags & 0x00'03u;
}

inline uint_least8_t NvStrapsConfig_SetGlobalEnable(NvStrapsConfig *config, uint_least8_t globalEnable)
{
    uint_least8_t previousGlobalEnable = NvStrapsConfig_IsGlobalEnable(config);

    if (previousGlobalEnable != globalEnable)
    {
    config->dirty = true;

    config->nOptionFlags &= ~(uint_least16_t)0x00'03u;
    config->nOptionFlags |= globalEnable & 0x00'03u;
    }

    return previousGlobalEnable;
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

inline bool NvStrapsConfig_SkipS3Resume(NvStrapsConfig const *config)
{
    return !!(config->nOptionFlags & 0x00'04u);
}

inline bool NvStrapsConfig_SetSkipS3Resume(NvStrapsConfig *config, bool fSkipS3Resume)
{
    bool previousFlag = NvStrapsConfig_SkipS3Resume(config);

    config->dirty = config->dirty || fSkipS3Resume != previousFlag;

    if (fSkipS3Resume)
    config->nOptionFlags |= 0x00'04u;
    else
    config->nOptionFlags &= (uint_least16_t) ~(uint_least16_t)0x04u;

    return previousFlag;
}

inline bool NvStrapsConfig_OverrideBarSizeMask(NvStrapsConfig const *config)
{
    return !!(config->nOptionFlags & 0x00'08u);
}

inline bool NvStrapsConfig_SetOverrideBarSizeMask(NvStrapsConfig *config, bool fOverrideSizeMask)
{
    bool previousFlag = NvStrapsConfig_OverrideBarSizeMask(config);

    config->dirty = config->dirty || fOverrideSizeMask != previousFlag;

    if (fOverrideSizeMask)
    config->nOptionFlags |= 0x00'08u;
    else
    config->nOptionFlags &= (uint_least16_t) ~(uint_least16_t)0x00'08u;

    return previousFlag;
}

inline bool NvStrapsConfig_HasSetupVarCRC(NvStrapsConfig const *config)
{
    return !!(config->nOptionFlags & 0x00'10u);
}

inline bool NvStrapsConfig_SetHasSetupVarCRC(NvStrapsConfig *config, bool hasCRC)
{
    bool previousFlag = NvStrapsConfig_HasSetupVarCRC(config);

    config->dirty = config->dirty || previousFlag != hasCRC;

    if (hasCRC)
    config->nOptionFlags |= 0x00'10u;
    else
    config->nOptionFlags &= (uint_least16_t) ~(uint_least16_t)0x00'10u;

    return previousFlag;
}

inline bool NvStrapsConfig_EnableSetupVarCRC(NvStrapsConfig const *config)
{
    return !(config->nOptionFlags & 0x00'20u);
}

inline bool NvStrapsConfig_SetEnableSetupVarCRC(NvStrapsConfig *config, bool enableCRC)
{
    bool previousFlag = NvStrapsConfig_EnableSetupVarCRC(config);

    config->dirty = config->dirty || previousFlag != enableCRC;

    if (enableCRC)
    config->nOptionFlags &= (uint_least16_t) ~(uint_least16_t)0x00'20u;
    else
    config->nOptionFlags |= 0x00'20u;

    return previousFlag;
}

inline bool NvStrapsConfig_IsGpuConfigured(NvStrapsConfig const *config)
{
    return NvStrapsConfig_IsGlobalEnable(config) || config->nGPUSelector;
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

inline bool NvStrapsConfig_GPUConfig_DeviceMatch(NvStraps_GPUConfig const *config, uint_least16_t devID)
{
    return config->deviceID == devID;
}

inline bool NvStrapsConfig_GPUConfig_SubsystemMatch(NvStraps_GPUConfig const *config, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return config->subsysVendorID == subsysVenID && config->subsysDeviceID == subsysDevID;
}

inline bool NvStrapsConfig_BridgeConfig_DeviceMatch(NvStraps_BridgeConfig const *config, uint_least16_t venID, uint_least16_t devID)
{
    return config->vendorID == venID && config->deviceID == devID;
}

inline bool NvStrapsConfig_BridgeConfig_BusLocationMatch(NvStraps_BridgeConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t func)
{
    return config->bridgeBus == bus && config->bridgeDevice == dev && config->bridgeFunction == func;
}

#if defined(__cplusplus)
}       // extern "C"
#endif

#if defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)
inline bool NvStrapsConfig::setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID)
{
     return setGPUSelector(barSizeSelector, deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return setGPUSelector(barSizeSelector, deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStrapsConfig::setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID)
{
    return setBarSizeMaskOverride(sizeMaskOverride, deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID)
{
    return setBarSizeMaskOverride(sizeMaskOverride, deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStrapsConfig::setGPUConfig(NvStraps_GPUConfig const &config)
{
    return NvStrapsConfig_SetGPUConfig(this, &config);
}

inline bool NvStrapsConfig::setBridgeConfig(NvStraps_BridgeConfig const &config)
{
    return NvStrapsConfig_SetBridgeConfig(this, &config);
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

inline bool NvStraps_GPUConfig::deviceMatch(uint_least16_t matchDeviceID) const
{
    return NvStrapsConfig_GPUConfig_DeviceMatch(this, matchDeviceID);
}

inline bool NvStraps_GPUConfig::subsystemMatch(uint_least16_t subsysVenID, uint_least16_t subsysDevID) const
{
    return NvStrapsConfig_GPUConfig_SubsystemMatch(this, subsysVenID, subsysDevID);
}

inline bool NvStraps_BridgeConfig::deviceMatch(uint_least16_t venID, uint_least16_t devID) const
{
    return NvStrapsConfig_BridgeConfig_DeviceMatch(this, venID, devID);
}

inline bool NvStraps_BridgeConfig::busLocationMatch(uint_least8_t bus, uint_least8_t dev, uint_least8_t func) const
{
    return NvStrapsConfig_BridgeConfig_BusLocationMatch(this, bus, dev, func);
}

inline bool NvStrapsConfig::resetConfig()
{
    return NvStrapsConfig_ResetConfig(this);
}

inline bool NvStrapsConfig::clearGPUSelectors()
{
    return dirty = dirty || !!nGPUSelector, !!std::exchange(nGPUSelector, 0u);
}

inline bool NvStrapsConfig::isDirty() const
{
    return NvStrapsConfig_IsDirty(this);
}

inline bool NvStrapsConfig::isDirty(bool fDirty)
{
    return NvStrapsConfig_SetIsDirty(this, fDirty);
}

inline bool NvStrapsConfig::skipS3Resume() const
{
    return NvStrapsConfig_SkipS3Resume(this);
}

inline bool NvStrapsConfig::skipS3Resume(bool fSkip)
{
    return NvStrapsConfig_SetSkipS3Resume(this, fSkip);
}

inline bool NvStrapsConfig::overrideBarSizeMask() const
{
    return NvStrapsConfig_OverrideBarSizeMask(this);
}

inline bool NvStrapsConfig::overrideBarSizeMask(bool overrideSizeMask)
{
    return NvStrapsConfig_SetOverrideBarSizeMask(this, overrideSizeMask);
}

inline uint_least8_t NvStrapsConfig::isGlobalEnable() const
{
    return NvStrapsConfig_IsGlobalEnable(this);
}

inline uint_least8_t NvStrapsConfig::setGlobalEnable(uint_least8_t val)
{
    return NvStrapsConfig_SetGlobalEnable(this, val);
}

inline bool NvStrapsConfig::hasSetupVarCRC() const
{
    return NvStrapsConfig_HasSetupVarCRC(this);
}

inline bool NvStrapsConfig::hasSetupVarCRC(bool hasCRC)
{
    return NvStrapsConfig_SetHasSetupVarCRC(this, hasCRC);
}

inline bool NvStrapsConfig::enableSetupVarCRC() const
{
    return NvStrapsConfig_EnableSetupVarCRC(this);
}

inline bool NvStrapsConfig::enableSetupVarCRC(bool enableCRC)
{
    return NvStrapsConfig_SetEnableSetupVarCRC(this, enableCRC);
}

inline uint_least8_t NvStrapsConfig::targetPciBarSizeSelector() const
{
    return NvStrapsConfig_TargetPciBarSizeSelector(this);
}

inline uint_least64_t NvStrapsConfig::setupVarCRC() const
{
    return NvStrapsConfig_SetupVarCRC(this);
}

inline uint_least64_t NvStrapsConfig::setupVarCRC(uint_least64_t crc)
{
    return NvStrapsConfig_SetSetupVarCRC(this, crc);
}

inline uint_least8_t NvStrapsConfig::targetPciBarSizeSelector(uint_least8_t barSizeSelector)
{
    return NvStrapsConfig_SetTargetPciBarSizeSelector(this, barSizeSelector);
}

inline NvStraps_BarSize NvStrapsConfig::lookupBarSize(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const
{
    return NvStrapsConfig_LookupBarSize(this, deviceID, subsysVenID, subsysDevID, bus, dev, fn);
}


inline NvStraps_BarSizeMaskOverride NvStrapsConfig::lookupBarSizeMaskOverride(uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const
{
    return NvStrapsConfig_LookupBarSizeMaskOverride(this, deviceID, subsysVenID, subsysDevID, bus, dev, fn);
}

inline std::tuple<uint_least16_t, uint_least16_t> NvStrapsConfig::hasBridgeDevice(uint_least8_t bridgeBus, uint_least8_t bridgeDevice, uint_least8_t bridgeFunction) const
{
    auto deviceID = uint_least32_t { NvStrapsConfig_HasBridgeDevice(this, bridgeBus, bridgeDevice, bridgeFunction) };

    return std::tuple { deviceID & WORD_BITMASK, deviceID >> WORD_BITSIZE & WORD_BITMASK };
}

inline NvStraps_BridgeConfig const *NvStrapsConfig::lookupBridgeConfig(uint_least8_t bridgeSecondaryBus) const
{
    return NvStrapsConfig_LookupBridgeConfig(this, bridgeSecondaryBus);
}

inline NvStraps_GPUConfig const *NvStrapsConfig::lookupGPUConfig(uint_least8_t bus, uint_least8_t dev, uint_least8_t fn) const
{
    return NvStrapsConfig_LookupGPUConfig(this, bus, dev, fn);
}

#endif          // defined(__cplusplus) && !defined(NVSTRAPS_DXE_DRIVER)

#endif          // !defined(NV_STRAPS_REBAR_CONFIG_H)
