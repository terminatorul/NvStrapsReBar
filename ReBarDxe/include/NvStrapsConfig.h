#if !defined(NV_STRAPS_REBAR_CONFIG_H)
#define NV_STRAPS_REBAR_CONFIG_H

// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
typedef EFI_STATUS ERROR_CODE;
typedef UINT8 BYTE;
#else
# include <windef.h>

# if defined(__cplusplus)
#  include <cstddef>
#  include <cstdint>
#  include <utility>
# endif

typedef DWORD ERROR_CODE;

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

typedef struct NvStraps_GPUSelector
{
    UINT16 deviceID, subsysVendorID, subsysDeviceID;
    UINT8  bus;
    UINT8  device;
    UINT8  function;
    UINT8  barSizeSelector;

#if defined(__cplusplus)
    bool operator ==(NvStraps_GPUSelector const &other) const = default;

    bool deviceMatch(UINT16 deviceID) const;
    bool subsystemMatch(UINT16 subsysVenID, UINT16 subsysDevID) const;
    bool busLocationMatch(UINT8 busNr, UINT8 dev, UINT8 fn) const;
#endif
}
    NvStraps_GPUSelector;

typedef struct NvStraps_GPUConfig
{
    UINT16 deviceID, subsysVendorID, subsysDeviceID;
    UINT8  bus, device, function;
    UINT32 baseAddressSelector;
}
    NvStraps_GPUConfig;

typedef struct NvStraps_BridgeConfig
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
    UINT8 nGlobalEnable;

    UINT8 nGPUSelector;
    NvStraps_GPUSelector GPUs[NvStraps_GPU_MAX_COUNT];

    UINT8 nGPUConfig;
    NvStraps_GPUConfig gpuConfig[NvStraps_GPU_MAX_COUNT];

    UINT8 nBridgeConfig;
    NvStraps_BridgeConfig bridge[NvStraps_GPU_MAX_COUNT + 2u];

#if defined(__cplusplus)
    bool isDirty() const;
    UINT8 isGlobalEnable() const;
    bool setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID);
    bool setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID);
    bool setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn);

    bool clearGPUSelector(UINT16 deviceID);
    bool clearGPUSelector(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID);
    bool clearGPUSelector(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn);

    bool clearGPUSelectors();
    UINT8 setGlobalEnable(UINT8 val);

    NvStraps_BarSize lookupBarSize(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn) const;
#endif
}
    NvStrapsConfig;

enum
{
    GPU_SELECTOR_SIZE = WORD_SIZE * 3u + BYTE_SIZE * 3u,
    GPU_CONFIG_SIZE = 3u * WORD_SIZE + 3u * BYTE_SIZE + DWORD_SIZE,
    BRIDGE_CONFIG_SIZE = 2u * WORD_SIZE + 4u * BYTE_SIZE + WORD_SIZE + DWORD_SIZE,
    NV_STRAPS_CONFIG_SIZE = 2u * BYTE_SIZE + GPU_SELECTOR_SIZE * NvStraps_GPU_MAX_COUNT
                                + BYTE_SIZE + GPU_CONFIG_SIZE * NvStraps_GPU_MAX_COUNT
                                + BYTE_SIZE + BRIDGE_CONFIG_SIZE * (NvStraps_GPU_MAX_COUNT + 2u)
};

inline UINT8 NvStrapsConfig_IsGlobalEnable(NvStrapsConfig const *config)
{
    return config->nGlobalEnable;
}

inline UINT8 NvStrapsConfig_SetGlobalEnable(NvStrapsConfig *config, UINT8 globalEnable)
{
    UINT8 oldGlobalEnable = config->nGlobalEnable;
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

#define NVSTRAPSCONFIG_BUFFERSIZE(config)       NV_STRAPS_CONFIG_SIZE

#if defined(__cplusplus)
extern "C"
{
#endif

NvStraps_BarSize NvStrapsConfig_LookupBarSize(NvStrapsConfig const *config, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn);

NvStrapsConfig *GetNvStrapsConfig(bool reload);
ERROR_CODE SaveNvStrapsConfig();

inline bool NvStrapsConfig_GPUSelector_DeviceMatch(NvStraps_GPUSelector const *selector, UINT16 devID)
{
    return selector->deviceID == devID;
}

inline bool NvStrapsConfig_GPUSelector_SubsystemMatch(NvStraps_GPUSelector const *selector, UINT16 subsysVenID, UINT16 subsysDevID)
{
    return selector->subsysVendorID == subsysVenID && selector->subsysDeviceID == subsysDevID;
}

inline bool NvStrapsConfig_GPUSelector_BusLocationMatch(NvStraps_GPUSelector const *selector, UINT8 busNr, UINT8 dev, UINT8 func)
{
    return selector->bus == busNr && selector->device == dev && selector->function == func;
}

#if defined(__cplusplus)
}       // extern "C"

inline bool NvStrapsConfig::setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID)
{
     return setGPUSelector(barSizeSelector, deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID)
{
    return setGPUSelector(barSizeSelector, deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStrapsConfig::clearGPUSelector(UINT16 deviceID)
{
    return clearGPUSelector(deviceID, MAX_UINT16, MAX_UINT16);
}

inline bool NvStrapsConfig::clearGPUSelector(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID)
{
    return clearGPUSelector(deviceID, subsysVenID, subsysDevID, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}

inline bool NvStraps_GPUSelector::deviceMatch(UINT16 devID) const
{
    return NvStrapsConfig_GPUSelector_DeviceMatch(this, devID);
}

inline bool NvStraps_GPUSelector::subsystemMatch(UINT16 subsysVenID, UINT16 subsysDevID) const
{
    return NvStrapsConfig_GPUSelector_SubsystemMatch(this, subsysVenID, subsysDevID);
}

inline bool NvStraps_GPUSelector::busLocationMatch(UINT8 busNr, UINT8 dev, UINT8 fn) const
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

inline UINT8 NvStrapsConfig::isGlobalEnable() const
{
    return NvStrapsConfig_IsGlobalEnable(this);
}

inline UINT8 NvStrapsConfig::setGlobalEnable(UINT8 val)
{
    return NvStrapsConfig_SetGlobalEnable(this, val);
}

inline NvStraps_BarSize NvStrapsConfig::lookupBarSize(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn) const
{
    return NvStrapsConfig_LookupBarSize(this, deviceID, subsysVenID, subsysDevID, bus, dev, fn);
}

#endif          // defined(__cplusplus)

#endif          // !defined(NV_STRAPS_REBAR_CONFIG_H)
