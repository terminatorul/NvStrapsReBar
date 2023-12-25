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
#  include <errhandlingapi.h>
# endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <uchar.h>
#include <stdint.h>

#include "LocalAppConfig.h"
#include "DeviceRegistry.h"
#include "StatusVar.h"

#include "NvStrapsConfig.h"

#if defined(UEFI_SOURCE) || defined(EFIAPI)

static char16_t const NvStrapsPciConfigName[] = u"NvStrapsPciConfig";
static EFI_GUID const NvStrapsPciConfigGUID = { 0x25dd6022u, 0x6b3eu, 0x430bu, { 0xb7u, 0xf4u, 0x11u, 0xe2u, 0x30u, 0x93u, 0xbbu, 0xe4u } };
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)

static TCHAR const NvStrapsPciConfigName[] = TEXT("NvStrapsPciConfig");
static TCHAR const NvStrapsPciConfigGUID[] = TEXT("{25dd6022-6b3e-430b-b7f4-11e23093bbe4}");

# endif

static UINT32 const EFI_VARIABLE_NON_VOLATILE        = UINT32_C(0x00000001);
static UINT32 const EFI_VARIABLE_BOOTSERVICE_ACCESS  = UINT32_C(0x00000002);
static UINT32 const EFI_VARIABLE_RUNTIME_ACCESS      = UINT32_C(0x00000004);

#endif          // #elseif defined(UEFI_SOURCE) || defined(EFIAPI)

static NvStrapsConfig strapsConfig;

static inline UINT8 unpack_BYTE(BYTE const *buffer)
{
    return *buffer;
}

static inline UINT16 unpack_WORD(BYTE const *buffer)
{
    return *buffer | (UINT16)buffer[1u] << BYTE_BITSIZE;
}

static inline UINT32 unpack_DWORD(BYTE const *buffer)
{
    return *buffer | (UINT16)buffer[1u] << BYTE_SIZE | (UINT32)buffer[2u] << 2u * BYTE_BITSIZE | (UINT32)buffer[3u] << 3u * BYTE_BITSIZE;
}

static inline UINT64 unpack_QWORD(BYTE const *buffer)
{
    return
        *buffer | (UINT16)buffer[1u] << BYTE_SIZE | (UINT32)buffer[2u] << 2u * BYTE_BITSIZE | (UINT32)buffer[3u] << 3u * BYTE_BITSIZE
        | (UINT64)buffer[4u] << 4u * BYTE_BITSIZE | (UINT64)buffer[5u] << 5u * BYTE_BITSIZE | (UINT64)buffer[6u] << 6u * BYTE_BITSIZE | (UINT64)buffer[7u] << 7u * BYTE_BITSIZE;
}

static inline UINT8 *pack_BYTE(BYTE *buffer, UINT8 value)
{
    return *buffer++ = value, buffer;
}

static inline UINT8 *pack_WORD(BYTE *buffer, UINT16 value)
{
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK;

    return buffer;
}

static inline UINT8 *pack_DWORD(BYTE *buffer, UINT32 value)
{
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK;

    return buffer;
}

static inline UINT8 *pack_QWORD(BYTE *buffer, UINT64 value)
{
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK;

    return buffer;
}

static void GPUSelector_unpack(BYTE const *buffer, NvStraps_GPUSelector *selector)
{

    selector->deviceID         = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->subsysVendorID   = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->subsysDeviceID   = unpack_WORD(buffer), buffer += WORD_SIZE;
    selector->bus              = unpack_BYTE(buffer), buffer += BYTE_SIZE;

    UINT8 busPos = unpack_BYTE(buffer); buffer += BYTE_SIZE;

    selector->device           = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos >> 3u & 0b0001'1111u;
    selector->function         = selector->bus == 0xFFu && busPos == 0xFFu ? 0xFFu : busPos & 0b0111u;
    selector->barSizeSelector  = unpack_BYTE(buffer), buffer += BYTE_SIZE;
}

static UINT8 *GPUSelector_pack(BYTE *buffer, NvStraps_GPUSelector const *selector)
{
    buffer = pack_WORD(buffer, selector->deviceID);
    buffer = pack_WORD(buffer, selector->subsysVendorID);
    buffer = pack_WORD(buffer, selector->subsysDeviceID);
    buffer = pack_BYTE(buffer, selector->bus);
    buffer = pack_BYTE(buffer, (UINT8)((unsigned)selector->device << 3u & 0b1111'1000u | (unsigned)selector->function & 0b0111u));
    buffer = pack_BYTE(buffer, selector->barSizeSelector);

    return buffer;
}

static void GPUConfig_unpack(BYTE const *buffer, NvStraps_GPUConfig *config)
{
    config->deviceID            = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->subsysVendorID      = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->subsysDeviceID      = unpack_WORD(buffer),  buffer += WORD_SIZE;
    config->bus                 = unpack_BYTE(buffer),  buffer += BYTE_SIZE;

    UINT8 busPosition = unpack_BYTE(buffer); buffer += BYTE_SIZE;
    config->device = busPosition >> 3u & 0b0001'1111u;
    config->function = busPosition & 0b0111u;

    config->baseAddressSelector = unpack_DWORD(buffer), buffer += DWORD_SIZE;
}

static UINT8 *GPUConfig_pack(BYTE *buffer, NvStraps_GPUConfig const *config)
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
    UINT8 busPos;

    config->vendorID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->deviceID            = unpack_WORD(buffer), buffer += WORD_SIZE;
    config->bridgeBus           = unpack_BYTE(buffer), buffer += BYTE_SIZE;

    busPos = unpack_BYTE(buffer), buffer += BYTE_SIZE;
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

static void NvStrapsConfig_Clear(NvStrapsConfig *config)
{
    config->dirty = false;
    config->nGlobalEnable = 0u;
    config->nGPUSelector = 0u;
    config->nGPUConfig = 0u;
    config->nBridgeConfig = 0u;
}

static unsigned NvStrapsConfig_BufferSize(NvStrapsConfig const *config)
{
    return 2u * BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE
        + BYTE_SIZE + config->nGPUConfig * GPU_CONFIG_SIZE
        + BYTE_SIZE + config->nBridgeConfig * BRIDGE_CONFIG_SIZE;
}

static void NvStrapsConfig_Load(BYTE const *buffer, unsigned size, NvStrapsConfig *config)
{
    do
    {
        if (size < 4u)
            break;

        config->nGlobalEnable = unpack_BYTE(buffer), buffer += BYTE_SIZE;
        config->nGPUSelector = unpack_BYTE(buffer), buffer += BYTE_SIZE;

        if (config->nGPUSelector > ARRAY_SIZE(config->GPUs) || size < 2u * BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE)
            break;

        for (unsigned i = 0u; i < config->nGPUSelector; i++)
            GPUSelector_unpack(buffer, config->GPUs + i), buffer += GPU_SELECTOR_SIZE;

        config->nGPUConfig = unpack_BYTE(buffer), buffer += BYTE_SIZE;

        if (config->nGPUConfig > ARRAY_SIZE(config->gpuConfig)
                || size < 2u * BYTE_SIZE + config->nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE + config->nGPUConfig * GPU_CONFIG_SIZE + BYTE_SIZE)
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

    if ((config->nGlobalEnable || config->nGPUSelector)
         && config->nGPUSelector <= ARRAY_SIZE(config->GPUs)
         && config->nGPUConfig <= ARRAY_SIZE(config->gpuConfig)
         && config->nBridgeConfig <= ARRAY_SIZE(config->bridge)
         && size >= BUFFER_SIZE)
    {
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

static inline  bool NvStrapsConfig_IsConfigured(NvStrapsConfig const *config)
{
    return config->nGlobalEnable || config->nGPUSelector;
};

static inline bool NvStrapsConfig_GPUSelector_HasSubsystem(NvStraps_GPUSelector const *selector)
{
    return selector->subsysVendorID != MAX_UINT16 && selector->subsysDeviceID != MAX_UINT16;
}

static inline bool NvStrapsConfig_GPUSelector_HasBusLocation(NvStraps_GPUSelector const *selector)
{
    return selector->bus != MAX_UINT8 || selector->device != MAX_UINT8 || selector->function != MAX_UINT8;
}

NvStraps_BarSize NvStrapsConfig_LookupBarSize(NvStrapsConfig const *config, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn)
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

NvStrapsConfig *GetNvStrapsConfig(bool reload)
{
    static bool isLoaded = false;

    if (!isLoaded || reload)
    {
        BYTE buffer[NVSTRAPSCONFIG_BUFFERSIZE(strapsConfig)];

#if defined(UEFI_SOURCE) || defined(EFIAPI)
        UINTN bufferSize = sizeof buffer;
        UINT32 attributes = 0u;
        CHAR16 configVarName[ARRAY_SIZE(NvStrapsPciConfigName)];
        EFI_GUID configVarGUID = NvStrapsPciConfigGUID;

        for (unsigned i = 0u; i < ARRAY_SIZE(NvStrapsPciConfigName); i++)
            configVarName[i] = NvStrapsPciConfigName[i];

        EFI_STATUS status = gRT->GetVariable(configVarName, &configVarGUID, &attributes, &bufferSize, buffer);

        if (status != EFI_SUCCESS)
        {
            SetStatusVar(status == EFI_NOT_FOUND ? StatusVar_GPU_Unconfigured : StatusVar_EFIError);
            bufferSize = 0u;
        }
#else
        DWORD bufferSize = GetFirmwareEnvironmentVariable(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, sizeof buffer);
#endif

        NvStrapsConfig_Load(buffer, (unsigned)bufferSize, &strapsConfig);

        isLoaded = true;
    };

    return &strapsConfig;
}

ERROR_CODE SaveNvStrapsConfig()
{
    if (NvStrapsConfig_IsDirty(&strapsConfig))
    {
        UINT32 const attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
        BYTE buffer[NVSTRAPSCONFIG_BUFFERSIZE(strapsConfig)];
        ERROR_CODE errorCode;

#if defined(UEFI_SOURCE) || defined(EFIAPI)
        CHAR16 configVarName[ARRAY_SIZE(NvStrapsPciConfigName)];
        EFI_GUID configVarGUID = NvStrapsPciConfigGUID;

        for (unsigned i = 0u; i < ARRAY_SIZE(NvStrapsPciConfigName); i++)
            configVarName[i] = NvStrapsPciConfigName[i];

        errorCode = gRT->SetVariable(configVarName, &configVarGUID, attributes, NvStrapsConfig_Save(buffer, ARRAY_SIZE(buffer), &strapsConfig), buffer);

        if (errorCode)
            SetStatusVar(StatusVar_EFIError);
#else
        errorCode = SetFirmwareEnvironmentVariableEx(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, NvStrapsConfig_Save(buffer, ARRAY_SIZE(buffer), &strapsConfig), attributes)
                 ? ERROR_SUCCESS
                 : GetLastError();
#endif

        if (!errorCode)
            NvStrapsConfig_SetIsDirty(&strapsConfig, false);

        return errorCode;
    }

    return 0u;
}
