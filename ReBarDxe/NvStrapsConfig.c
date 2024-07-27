// vi: ft=cpp
//
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
# include <Library/UefiRuntimeServicesTableLib.h>
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#  if defined(_M_AMD64) && !defined(_AMD64_)
#   define _AMD64_
#  endif
#  include <windef.h>
#  include <winbase.h>
#  include <winerror.h>
#  include <errhandlingapi.h>
# endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>
#include <stdint.h>

#include "LocalAppConfig.h"
#include "EfiVariable.h"
#include "DeviceRegistry.h"
#include "StatusVar.h"

#include "NvStrapsConfig.h"

char const NvStrapsConfig_VarName[] = "NvStrapsReBar";
static NvStrapsConfig strapsConfig;

static void GPUSelector_unpack(BYTE const *buffer, NvStraps_GPUSelector *selector)
{

    selector->deviceID         = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->subsysVendorID   = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->subsysDeviceID   = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->bus              = unpack_BYTE(buffer), buffer += BYTE_SIZE;

    uint_least8_t busPos = unpack_BYTE(buffer); buffer += BYTE_SIZE;

    selector->device		  = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos >> 3u & 0b0001'1111u;
    selector->function         	  = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos & 0b0111u;
    selector->barSizeSelector  	  = unpack_BYTE(buffer), buffer += BYTE_SIZE;
    selector->overrideBarSizeMask = unpack_BYTE(buffer), buffer += BYTE_SIZE;
}

static BYTE *GPUSelector_pack(BYTE *buffer, NvStraps_GPUSelector const *selector)
{
    buffer = pack_WORD(buffer, selector->deviceID);
    buffer = pack_WORD(buffer, selector->subsysVendorID);
    buffer = pack_WORD(buffer, selector->subsysDeviceID);
    buffer = pack_BYTE(buffer, selector->bus);
    buffer = pack_BYTE(buffer, (uint_least8_t)((unsigned)selector->device << 3u & 0b1111'1000u | (unsigned)selector->function & 0b0111u));
    buffer = pack_BYTE(buffer, selector->barSizeSelector);
    buffer = pack_BYTE(buffer, selector->overrideBarSizeMask);

    return buffer;
}

static void GPUConfig_unpack(BYTE const *buffer, NvStraps_GPUConfig *config)
{
    config->deviceID            = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->subsysVendorID      = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->subsysDeviceID      = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->bus                 = unpack_BYTE(buffer),  buffer += BYTE_SIZE;

    uint_least8_t busPosition = unpack_BYTE(buffer); buffer += BYTE_SIZE;
    config->device = busPosition >> 3u & 0b0001'1111u;
    config->function = busPosition & 0b0111u;

    config->bar0.base = unpack_QWORD(buffer), buffer += QWORD_SIZE;
    config->bar0.top  = unpack_QWORD(buffer), buffer += QWORD_SIZE;
}

static BYTE *GPUConfig_pack(BYTE *buffer, NvStraps_GPUConfig const *config)
{
    buffer = pack_WORD(buffer, config->deviceID);
    buffer = pack_WORD(buffer, config->subsysVendorID);
    buffer = pack_WORD(buffer, config->subsysDeviceID);
    buffer = pack_BYTE(buffer, config->bus);
    buffer = pack_BYTE(buffer, (unsigned)config->device << 3u & 0b1111'1000u | (unsigned)config->function & 0b0111u);
    buffer = pack_QWORD(buffer, config->bar0.base);
    buffer = pack_QWORD(buffer, config->bar0.top);

    return buffer;
}

static void BridgeConfig_unpack(BYTE const *buffer, NvStraps_BridgeConfig *config)
{
    config->vendorID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->deviceID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->bridgeBus           = unpack_BYTE(buffer), buffer += BYTE_SIZE;

    uint_least8_t busPos = unpack_BYTE(buffer); buffer += BYTE_SIZE;

    config->bridgeDevice        = config->bridgeBus == 0xFF && busPos == 0xFFu ? 0xFFu : busPos >> 3u & 0b0001'1111u;
    config->bridgeFunction      = config->bridgeBus == 0xFF && busPos == 0xFFu ? 0xFFu : busPos & 0b0111u;

    config->bridgeSecondaryBus  = unpack_BYTE(buffer), buffer += BYTE_SIZE;
}

static UINT8 *BridgeConfig_pack(BYTE *buffer, NvStraps_BridgeConfig  const *config)
{
    buffer = pack_WORD(buffer, config->vendorID);
    buffer = pack_WORD(buffer, config->deviceID);
    buffer = pack_BYTE(buffer, config->bridgeBus);
    buffer = pack_BYTE(buffer, (unsigned)config->bridgeDevice << 3u & 0b1111'1000u | (unsigned)config->bridgeFunction & 0b0111u);
    buffer = pack_BYTE(buffer, config->bridgeSecondaryBus);

    return buffer;
}

bool NvStrapsConfig_ResetConfig(NvStrapsConfig *config)
{
    bool hasConfig = !!config->nGPUConfig && !!config->nBridgeConfig;

    if (hasConfig)
    config->dirty = true;

    config->nGPUConfig = 0u;
    config->nBridgeConfig = 0u;

    return hasConfig;
}

void NvStrapsConfig_Clear(NvStrapsConfig *config)
{
    config->dirty = false;
    config->nPciBarSize = 0u;
    config->nOptionFlags = 0u;
    config->nSetupVarCRC = 0u;
    config->nGPUSelector = 0u;
    config->nGPUConfig = 0u;
    config->nBridgeConfig = 0u;
}

static unsigned NvStrapsConfig_BufferSize(NvStrapsConfig const *config)
{
    return NV_STRAPS_HEADER_SIZE
        + BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE
        + BYTE_SIZE + config->nGPUConfig * GPU_CONFIG_SIZE
        + BYTE_SIZE + config->nBridgeConfig * BRIDGE_CONFIG_SIZE;
}

static void NvStrapsConfig_Load(BYTE const *buffer, unsigned size, NvStrapsConfig *config)
{
    do
    {
        if (size < NV_STRAPS_HEADER_SIZE + 3u * BYTE_SIZE)
            break;

        config->nPciBarSize = unpack_BYTE(buffer), buffer += BYTE_SIZE;
        config->nOptionFlags = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->nSetupVarCRC = unpack_QWORD(buffer), buffer += QWORD_SIZE;
        config->nGPUSelector = unpack_BYTE(buffer), buffer += BYTE_SIZE;

        if (config->nGPUSelector > ARRAY_SIZE(config->GPUs) || size < (unsigned)NV_STRAPS_HEADER_SIZE + BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE)
            break;

        for (unsigned i = 0u; i < config->nGPUSelector; i++)
            GPUSelector_unpack(buffer, config->GPUs + i), buffer += GPU_SELECTOR_SIZE;

        config->nGPUConfig = unpack_BYTE(buffer), buffer += BYTE_SIZE;

        if (config->nGPUConfig > ARRAY_SIZE(config->gpuConfig)
                || size < (unsigned)NV_STRAPS_HEADER_SIZE + BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE + config->nGPUConfig * GPU_CONFIG_SIZE + BYTE_SIZE)
        {
            break;
        }

        for (unsigned i = 0u; i < config->nGPUConfig; i++)
            GPUConfig_unpack(buffer, config->gpuConfig + i), buffer += GPU_CONFIG_SIZE;

        config->nBridgeConfig = unpack_BYTE(buffer), buffer += BYTE_SIZE;

        if (config->nBridgeConfig > ARRAY_SIZE(config->bridge)
                 || size < NvStrapsConfig_BufferSize(config))
        {
            break;
        }

        for (unsigned i = 0u; i < config->nBridgeConfig; i++)
            BridgeConfig_unpack(buffer, config->bridge + i), buffer += BRIDGE_CONFIG_SIZE;

        config->dirty = false;

        return;
    }
    while (false);

    NvStrapsConfig_Clear(config);
}

static unsigned NvStrapsConfig_Save(BYTE *buffer, unsigned size, NvStrapsConfig const *config)
{
    unsigned const BUFFER_SIZE = NvStrapsConfig_BufferSize(config);

    if (NvStrapsConfig_IsDriverConfigured(config)
         && config->nGPUSelector <= ARRAY_SIZE(config->GPUs)
         && config->nGPUConfig <= ARRAY_SIZE(config->gpuConfig)
         && config->nBridgeConfig <= ARRAY_SIZE(config->bridge)
         && size >= BUFFER_SIZE)
    {
        buffer = pack_BYTE(buffer, config->nPciBarSize);
        buffer = pack_WORD(buffer, config->nOptionFlags);
    buffer = pack_QWORD(buffer, config->nSetupVarCRC);
        buffer = pack_BYTE(buffer, config->nGPUSelector);

        for (unsigned i = 0u; i < config->nGPUSelector; i++)
            buffer = GPUSelector_pack(buffer, config->GPUs + i);

        buffer = pack_BYTE(buffer, config->nGPUConfig);

        for (unsigned i = 0u; i < config->nGPUConfig; i++)
            buffer = GPUConfig_pack(buffer, config->gpuConfig + i);

        buffer = pack_BYTE(buffer, config->nBridgeConfig);

        for (unsigned i = 0u; i < config->nBridgeConfig; i++)
            buffer = BridgeConfig_pack(buffer, config->bridge + i);

        return BUFFER_SIZE;
    }

    return 0u;
}

static inline bool NvStrapsConfig_GPUSelector_HasSubsystem(NvStraps_GPUSelector const *selector)
{
    return selector->subsysVendorID != WORD_BITMASK && selector->subsysDeviceID != WORD_BITMASK;
}

static inline bool NvStrapsConfig_GPUSelector_HasBusLocation(NvStraps_GPUSelector const *selector)
{
    return selector->bus != BYTE_BITMASK || selector->device != BYTE_BITMASK || selector->function != BYTE_BITMASK;
}

NvStraps_BarSize NvStrapsConfig_LookupBarSize(NvStrapsConfig const *config, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    ConfigPriority configPriority = UNCONFIGURED;
    BarSizeSelector barSizeSelector = BarSizeSelector_None;

    for (unsigned iGPU = 0u; iGPU < config->nGPUSelector; iGPU++)
        if (NvStrapsConfig_GPUSelector_DeviceMatch(config->GPUs + iGPU, deviceID))
            if (NvStrapsConfig_GPUSelector_HasSubsystem(config->GPUs + iGPU))
                if (NvStrapsConfig_GPUSelector_SubsystemMatch(config->GPUs + iGPU, subsysVenID, subsysDevID))
                    if (NvStrapsConfig_GPUSelector_HasBusLocation(config->GPUs + iGPU))
                        if (NvStrapsConfig_GPUSelector_BusLocationMatch(config->GPUs + iGPU, bus, dev, fn))
                if (config->GPUs[iGPU].barSizeSelector != BarSizeSelector_None)
                {
                NvStraps_BarSize sizeSelector = { .priority = EXPLICIT_PCI_LOCATION, .barSizeSelector = (BarSizeSelector)config->GPUs[iGPU].barSizeSelector };

                return sizeSelector;
                }
                else
                ;
                        else
                            ;
                    else
            if (config->GPUs[iGPU].barSizeSelector != BarSizeSelector_None)
                configPriority = EXPLICIT_SUBSYSTEM_ID, barSizeSelector = (BarSizeSelector)config->GPUs[iGPU].barSizeSelector;
            else
                ;
                else
                    ;
            else
        if (config->GPUs[iGPU].barSizeSelector != BarSizeSelector_None)
        {
            if (configPriority < EXPLICIT_SUBSYSTEM_ID)
            configPriority = EXPLICIT_PCI_ID, barSizeSelector = (BarSizeSelector)config->GPUs[iGPU].barSizeSelector;
        }
        else
            ;

    if (configPriority == UNCONFIGURED && NvStrapsConfig_IsGlobalEnable(config))
    {
        barSizeSelector = lookupBarSizeInRegistry(deviceID);

        if (barSizeSelector == BarSizeSelector_None)
        {
            if (NvStrapsConfig_IsGlobalEnable(config) > 1u && isTuringGPU(deviceID))
            {
                NvStraps_BarSize sizeSelector = { .priority = IMPLIED_GLOBAL, .barSizeSelector = BarSizeSelector_2G };

                return sizeSelector;
            }
        }
        else
            configPriority = FOUND_GLOBAL;
    }

    NvStraps_BarSize sizeSelector = { .priority = configPriority, .barSizeSelector = barSizeSelector };

    return sizeSelector;
}

NvStraps_BarSizeMaskOverride NvStrapsConfig_LookupBarSizeMaskOverride(NvStrapsConfig const *config, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    ConfigPriority configPriority = UNCONFIGURED;
    bool barSizeMaskOverride = false;

    for (unsigned iGPU = 0u; iGPU < config->nGPUSelector; iGPU++)
        if (NvStrapsConfig_GPUSelector_DeviceMatch(config->GPUs + iGPU, deviceID))
            if (NvStrapsConfig_GPUSelector_HasSubsystem(config->GPUs + iGPU))
                if (NvStrapsConfig_GPUSelector_SubsystemMatch(config->GPUs + iGPU, subsysVenID, subsysDevID))
                    if (NvStrapsConfig_GPUSelector_HasBusLocation(config->GPUs + iGPU))
                        if (NvStrapsConfig_GPUSelector_BusLocationMatch(config->GPUs + iGPU, bus, dev, fn))
                if (config->GPUs[iGPU].overrideBarSizeMask)
                {
                NvStraps_BarSizeMaskOverride maskOverride = { .priority = EXPLICIT_PCI_LOCATION, .sizeMaskOverride = config->GPUs[iGPU].overrideBarSizeMask != 0xFFu };

                return maskOverride;
                }
                else
                ;
                        else
                            ;
                    else
            if (config->GPUs[iGPU].overrideBarSizeMask)
                configPriority = EXPLICIT_SUBSYSTEM_ID, barSizeMaskOverride = config->GPUs[iGPU].overrideBarSizeMask != 0xFFu;
            else
                ;
                else
                    ;
            else
        if (config->GPUs[iGPU].overrideBarSizeMask)
        {
            if (configPriority < EXPLICIT_SUBSYSTEM_ID)
            configPriority = EXPLICIT_PCI_ID, barSizeMaskOverride = config->GPUs[iGPU].overrideBarSizeMask != 0xFFu;
        }
        else
            ;

    if (configPriority == UNCONFIGURED)
    {
    configPriority = FOUND_GLOBAL;
    barSizeMaskOverride = NvStrapsConfig_OverrideBarSizeMask(config);
    }

    NvStraps_BarSizeMaskOverride maskOverride = { .priority = configPriority, .sizeMaskOverride = barSizeMaskOverride };

    return maskOverride;
}

static unsigned NvStrapsConfig_FindGPUConfig(NvStrapsConfig const *config, uint_least8_t busNr, uint_least8_t dev, uint_least8_t fun)
{
    for (unsigned i = 0u; i < config->nGPUConfig; i++)
    if (config->gpuConfig[i].bus == busNr && config->gpuConfig[i].device == dev && config->gpuConfig[i].function == fun)
        return i;

    return WORD_BITMASK;
}

static unsigned NvStrapsConfig_FindBridgeConfig(NvStrapsConfig const *config, uint_least8_t busNr, uint_least8_t dev, uint_least8_t fun)
{
    for (unsigned i = 0u; i < config->nBridgeConfig; i++)
    if (config->bridge[i].bridgeBus == busNr && config->bridge[i].bridgeDevice == dev && config->bridge[i].bridgeFunction == fun)
        return i;

    return WORD_BITMASK;
}

NvStraps_GPUConfig const *NvStrapsConfig_LookupGPUConfig(NvStrapsConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    unsigned index = NvStrapsConfig_FindGPUConfig(config, bus, dev, fn);

    return index == WORD_BITMASK ? NULL : config->gpuConfig + index;
}

NvStraps_BridgeConfig const *NvStrapsConfig_LookupBridgeConfig(NvStrapsConfig const *config, uint_least8_t secondaryBus)
{
    for (unsigned index = 0u; index < config->nBridgeConfig; index++)
    if (config->bridge[index].bridgeSecondaryBus == secondaryBus)
        return config->bridge + index;

    return NULL;
}

uint_least32_t NvStrapsConfig_HasBridgeDevice(NvStrapsConfig const *config, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    unsigned index = NvStrapsConfig_FindBridgeConfig(config, bus, dev, fn);

    if (index == WORD_BITMASK)
    return (uint_least32_t)WORD_BITMASK << WORD_BITSIZE | WORD_BITMASK;

    return (uint_least32_t)config->bridge[index].deviceID << WORD_BITSIZE | config->bridge[index].vendorID & WORD_BITMASK;
}

static void NvStraps_UpdateGPUConfig(NvStrapsConfig *config, unsigned gpuIndex, NvStraps_GPUConfig const *gpuConfig)
{
    if (config->gpuConfig[gpuIndex].deviceID != gpuConfig->deviceID)
    config->gpuConfig[gpuIndex].deviceID = gpuConfig->deviceID, config->dirty = true;

    if (config->gpuConfig[gpuIndex].subsysVendorID != gpuConfig->subsysVendorID)
    config->gpuConfig[gpuIndex].subsysVendorID = gpuConfig->subsysVendorID, config->dirty = true;

    if (config->gpuConfig[gpuIndex].subsysDeviceID != gpuConfig->subsysDeviceID)
    config->gpuConfig[gpuIndex].subsysDeviceID = gpuConfig->subsysDeviceID, config->dirty = true;

    if (config->gpuConfig[gpuIndex].bar0.base != gpuConfig->bar0.base)
    config->gpuConfig[gpuIndex].bar0.base = gpuConfig->bar0.base, config->dirty = true;

    if (config->gpuConfig[gpuIndex].bar0.top != gpuConfig->bar0.top)
    config->gpuConfig[gpuIndex].bar0.top = gpuConfig->bar0.top, config->dirty = true;
}

bool NvStrapsConfig_SetGPUConfig(NvStrapsConfig *config, NvStraps_GPUConfig const *gpuConfig)
{
    unsigned gpuIndex = NvStrapsConfig_FindGPUConfig(config, gpuConfig->bus, gpuConfig->device, gpuConfig->function);

    if (gpuIndex == WORD_BITMASK)
    if (config->nGPUConfig < ARRAY_SIZE(config->gpuConfig))
    {
        config->gpuConfig[config->nGPUConfig++] = *gpuConfig;
        config->dirty = true;

        return true;
    }
    else
        ;
    else
    {
    NvStraps_UpdateGPUConfig(config, gpuIndex, gpuConfig);

    return true;
    }

    return false;
}

static void NvStrapsConfig_UpdateBridgeConfig(NvStrapsConfig *config, unsigned bridgeIndex, NvStraps_BridgeConfig const *bridgeConfig)
{
    if (config->bridge[bridgeIndex].vendorID != bridgeConfig->vendorID)
    config->bridge[bridgeIndex].vendorID = bridgeConfig->vendorID, config->dirty = true;

    if (config->bridge[bridgeIndex].deviceID != bridgeConfig->deviceID)
    config->bridge[bridgeIndex].deviceID = bridgeConfig->deviceID, config->dirty = true;

    if (config->bridge[bridgeIndex].bridgeSecondaryBus != bridgeConfig->bridgeSecondaryBus)
    config->bridge[bridgeIndex].bridgeSecondaryBus = bridgeConfig->bridgeSecondaryBus, config->dirty = true;
}

bool NvStrapsConfig_SetBridgeConfig(NvStrapsConfig *config, NvStraps_BridgeConfig const *bridgeConfig)
{
    unsigned bridgeIndex = NvStrapsConfig_FindBridgeConfig(config, bridgeConfig->bridgeBus, bridgeConfig->bridgeDevice, bridgeConfig->bridgeFunction);

    if (bridgeIndex == WORD_BITMASK)
    if (config->nBridgeConfig < ARRAY_SIZE(config->bridge))
    {
        config->bridge[config->nBridgeConfig++] = *bridgeConfig;
        config->dirty = true;

        return true;
    }
    else
        ;
    else
    {
    NvStrapsConfig_UpdateBridgeConfig(config, bridgeIndex, bridgeConfig);

    return true;
    }

    return false;
}

NvStrapsConfig *GetNvStrapsConfig(bool reload, ERROR_CODE *errorCode)
{
    static bool isLoaded = false;

    if (!isLoaded || reload)
    {
        BYTE buffer[NVSTRAPSCONFIG_BUFFERSIZE(strapsConfig)];
        uint_least32_t size = sizeof buffer;
        ERROR_CODE status = ReadEfiVariable(NvStrapsConfig_VarName, buffer, &size);

#if defined(UEFI_SOURCE)
        if (EFI_ERROR(status))
            SetEFIError(EFIError_ReadConfigVar, status);
        else
            if (size == 0u)
                SetStatusVar(StatusVar_Unconfigured);
#endif

        if (errorCode)
            *errorCode = status;

        NvStrapsConfig_Load(buffer, size, &strapsConfig);

        isLoaded = true;
    }
    else
        if (errorCode)
            *errorCode = 0u;

    return &strapsConfig;
}

void SaveNvStrapsConfig(ERROR_CODE *errorCode)
{
    if (NvStrapsConfig_IsDirty(&strapsConfig))
    {
        BYTE buffer[NVSTRAPSCONFIG_BUFFERSIZE(strapsConfig)];
        ERROR_CODE errorStatus = WriteEfiVariable
            (
                NvStrapsConfig_VarName,
                buffer,
                NvStrapsConfig_Save(buffer, ARRAY_SIZE(buffer), &strapsConfig),
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
            );

#if defined(UEFI_SOURCE) || defined(EFIAPI)
        if (errorStatus)
            SetEFIError(EFIError_WriteConfigVar, errorStatus);
#endif

        if (!errorStatus)
            NvStrapsConfig_SetIsDirty(&strapsConfig, false);

        if (errorCode)
            *errorCode = errorStatus;
    }
    else
        if (errorCode)
            *errorCode = 0u;
}
