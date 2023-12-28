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

    selector->device           = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos >> 3u & 0b0001'1111u;
    selector->function         = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos & 0b0111u;
    selector->barSizeSelector  = unpack_BYTE(buffer), buffer += BYTE_SIZE;
}

static BYTE *GPUSelector_pack(BYTE *buffer, NvStraps_GPUSelector const *selector)
{
    buffer = pack_WORD(buffer, selector->deviceID);
    buffer = pack_WORD(buffer, selector->subsysVendorID);
    buffer = pack_WORD(buffer, selector->subsysDeviceID);
    buffer = pack_BYTE(buffer, selector->bus);
    buffer = pack_BYTE(buffer, (uint_least8_t)((unsigned)selector->device << 3u & 0b1111'1000u | (unsigned)selector->function & 0b0111u));
    buffer = pack_BYTE(buffer, selector->barSizeSelector);

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

    config->baseAddressSelector = unpack_DWORD(buffer), buffer += DWORD_SIZE;
}

static BYTE *GPUConfig_pack(BYTE *buffer, NvStraps_GPUConfig const *config)
{
    buffer = pack_WORD(buffer, config->deviceID);
    buffer = pack_WORD(buffer, config->subsysVendorID);
    buffer = pack_WORD(buffer, config->subsysDeviceID);
    buffer = pack_BYTE(buffer, config->bus);
    buffer = pack_BYTE(buffer, (unsigned)config->device << 3u & 0b1111'1000u | (unsigned)config->function & 0b0111u);
    buffer = pack_DWORD(buffer, config->baseAddressSelector);

    return buffer;
}

static void BridgeConfig_unpack(BYTE const *buffer, NvStraps_BridgeConfig *config)
{
    config->vendorID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->deviceID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->bridgeBus           = unpack_BYTE(buffer), buffer += BYTE_SIZE;

    uint_least8_t busPos = unpack_BYTE(buffer); buffer += BYTE_SIZE;

    config->bridgeDevice        = busPos == 0xFFu ? 0xFFu : busPos >> 3u & 0b0001'1111u;
    config->bridgeFunction      = busPos == 0xFFu ? 0xFFu : busPos & 0b0111u;

    config->bridgeSecondaryBus  = unpack_BYTE(buffer), buffer += BYTE_SIZE;
    config->bridgeSubsidiaryBus = unpack_BYTE(buffer), buffer += BYTE_SIZE;
    config->bridgeIOBaseLimit   = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->bridgeMemBaseLimit  = unpack_DWORD(buffer), buffer += DWORD_SIZE;
}

static UINT8 *BridgeConfig_pack(BYTE *buffer, NvStraps_BridgeConfig  const *config)
{
    buffer = pack_WORD(buffer, config->vendorID);
    buffer = pack_WORD(buffer, config->deviceID);
    buffer = pack_BYTE(buffer, config->bridgeBus);
    buffer = pack_BYTE(buffer, (unsigned)config->bridgeDevice << 3u & 0b1111'1000u | (unsigned)config->bridgeFunction & 0b0111u);
    buffer = pack_BYTE(buffer, config->bridgeSecondaryBus);
    buffer = pack_BYTE(buffer, config->bridgeSubsidiaryBus);
    buffer = pack_WORD(buffer, config->bridgeIOBaseLimit);
    buffer = pack_DWORD(buffer,config-> bridgeMemBaseLimit);

    return buffer;
}

void NvStrapsConfig_Clear(NvStrapsConfig *config)
{
    config->dirty = false;
    config->nPciBarSize = 0u;
    config->nGlobalEnable = 0u;
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
        config->nGlobalEnable = unpack_BYTE(buffer), buffer += BYTE_SIZE;
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
        buffer = pack_BYTE(buffer, config->nGlobalEnable);
        buffer = pack_BYTE(buffer, config->nGPUSelector);

        for (unsigned i = 0; i < config->nGPUSelector; i++)
            buffer = GPUSelector_pack(buffer, config->GPUs + i);

        buffer = pack_BYTE(buffer, config->nGPUConfig);

        for (unsigned i = 0u; i < config->nGPUConfig; i++)
            buffer = GPUConfig_pack(buffer, config->gpuConfig + i);

        buffer = pack_BYTE(buffer, config->nBridgeConfig);

        for (unsigned i = 0; i < config->nBridgeConfig; i++)
            BridgeConfig_pack(buffer, config->bridge + i);

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
                        {
                            NvStraps_BarSize sizeSelector = { .priority = EXPLICIT_PCI_LOCATION, .barSizeSelector = config->GPUs[iGPU].barSizeSelector };

                            return sizeSelector;
                        }
                        else
                            ;
                    else
                        configPriority = EXPLICIT_SUBSYSTEM_ID, barSizeSelector = config->GPUs[iGPU].barSizeSelector;
                else
                    ;
            else
            {
                if (configPriority < EXPLICIT_SUBSYSTEM_ID)
                    configPriority = EXPLICIT_PCI_ID, barSizeSelector = config->GPUs[iGPU].barSizeSelector;
            }

    if (configPriority == UNCONFIGURED && config->nGlobalEnable)
    {
        barSizeSelector = lookupBarSizeInRegistry(deviceID);

        if (barSizeSelector == BarSizeSelector_None)
        {
            if (config->nGlobalEnable > 1u && isTuringGPU(deviceID))
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

NvStrapsConfig *GetNvStrapsConfig(bool reload, ERROR_CODE *errorCode)
{
    static bool isLoaded = false;

    if (!isLoaded || reload)
    {
        BYTE buffer[NVSTRAPSCONFIG_BUFFERSIZE(strapsConfig)];
        uint_least32_t size = sizeof buffer;
        ERROR_CODE status = ReadEfiVariable(NvStrapsConfig_VarName, buffer, &size);

#if defined(UEFI_SOURCE) || defined(EFIAPI)
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
