#if defined(UEFI_SOURCE) || defined(EFIAPI)
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

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <algorithm>
#include <utility>

#include "NvStrapsPciConfig.hh"

using std::as_const;
using std::begin;
using std::size;

#if defined(UEFI_SOURCE) || defined(EFIAPI)

extern "C"
{
# include <Uefi.h>
# include <Library/UefiRuntimeServicesTableLib.h>
}

typedef UINT8 BYTE;
static char16_t NvStrapsPciConfigName[] = u"NvStrapsPciConfig";
static EFI_GUID NvStrapsPciConfigGUID = { 0x25dd6022u, 0x6b3eu, 0x430bu, { 0xb7u, 0xf4u, 0x11u, 0xe2u, 0x30u, 0x93u, 0xbbu, 0xe4u } };
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN32) || defined(_WIN64)
#  include <system_error>
#  include "WinApiError.hh"
static constexpr TCHAR const NvStrapsPciConfigName[] = TEXT("NvStrapsPciConfig");
static constexpr TCHAR const NvStrapsPciConfigGUID[] = TEXT("{25dd6022-6b3e-430b-b7f4-11e23093bbe4}");
using std::system_error;
# endif

constexpr UINT32 const EFI_VARIABLE_NON_VOLATILE        = UINT32_C(0x00000001);
constexpr UINT32 const EFI_VARIABLE_BOOTSERVICE_ACCESS  = UINT32_C(0x00000002);
constexpr UINT32 const EFI_VARIABLE_RUNTIME_ACCESS      = UINT32_C(0x00000004);

template <typename ContainerType>
    constexpr inline auto ARRAY_SIZE(ContainerType &cont) -> decltype(size(cont))
{
    return size(cont);
}

#endif          // #elseif defined(UEFI_SOURCE) || defined(EFIAPI)

static NvStrapsConfig strapsConfig { .nGPUSelector = 0xFFu };

constexpr size_t const
        GPU_SELECTOR_SIZE = WORD_SIZE + 3u * BYTE_SIZE,
        GPU_CONFIG_SIZE = WORD_SIZE + BYTE_SIZE + DWORD_SIZE,
        BRIDGE_CONFIG_SIZE = 2u * WORD_SIZE + 4u * BYTE_SIZE + WORD_SIZE + DWORD_SIZE,
        NV_STRAPS_CONFIG_SIZE = BYTE_SIZE + GPU_SELECTOR_SIZE * ARRAY_SIZE(strapsConfig.GPUs)
                              + BYTE_SIZE + GPU_CONFIG_SIZE * ARRAY_SIZE(strapsConfig.gpuConfig)
                              + BYTE_SIZE + BRIDGE_CONFIG_SIZE * ARRAY_SIZE(strapsConfig.bridge);

template <typename UnsignedType>
    static UnsignedType unpack(BYTE const *&buffer)
{
    UnsignedType value = UnsignedType { };

    for (auto i = 0u; i < sizeof value; i++)
        value |= *buffer++ << 0x08u * i;

    return value;
}

template <typename UnsignedType>
    static BYTE *pack(BYTE *&buffer, UnsignedType &&value)
{
    for (auto i = 0u; i < sizeof value; i++)
        *buffer++ = BYTE { value >> i & 0xFFu };

    return buffer;
}

template <>
    NvStrapsConfig::GPUSelector unpack<NvStrapsConfig::GPUSelector>(BYTE const *&buffer)
{
    UINT8 busPos;

    return NvStrapsConfig::GPUSelector
    {
        .deviceID         = unpack<UINT16>(buffer),
        .bus              = unpack<UINT8>(buffer),
        .device           = UINT8 { (busPos = unpack<UINT8>(buffer), busPos >> 3u & 0b0001'1111u) },
        .function         = UINT8 { busPos & 0b0111u },
        .barSizeSelector  = unpack<UINT8>(buffer)
    };
}

template <>
    BYTE *pack<NvStrapsConfig::GPUSelector const &>(BYTE *&buffer, NvStrapsConfig::GPUSelector const &gpuSelector)
{
    pack(buffer, gpuSelector.deviceID);
    pack(buffer, gpuSelector.bus);
    pack(buffer, UINT8 { unsigned { gpuSelector.device } << 3u & 0x1111'1000u | unsigned { gpuSelector.function } & 0b0111u });
    pack(buffer, gpuSelector.barSizeSelector);

    return buffer;
}

template <>
    BYTE *pack<NvStrapsConfig::GPUSelector &>(BYTE *&buffer, NvStrapsConfig::GPUSelector &gpuSelector)
{
    return pack(buffer, as_const(gpuSelector));
}


template <>
    NvStrapsConfig::GPUConfig unpack<NvStrapsConfig::GPUConfig>(BYTE const *&buffer)
{
    return NvStrapsConfig::GPUConfig
    {
        .deviceID            = unpack<UINT16>(buffer),
        .bus                 = unpack<UINT8>(buffer),
        .baseAddressSelector = unpack<UINT32>(buffer)
    };
}

template <>
    BYTE *pack<NvStrapsConfig::GPUConfig const &>(BYTE *&buffer, NvStrapsConfig::GPUConfig const &gpuConfig)
{
    pack(buffer, gpuConfig.deviceID);
    pack(buffer, gpuConfig.bus);
    pack(buffer, gpuConfig.baseAddressSelector);

    return buffer;
}

template <>
    BYTE *pack<NvStrapsConfig::GPUConfig &>(BYTE *&buffer, NvStrapsConfig::GPUConfig &gpuConfig)
{
    return pack(buffer, as_const(gpuConfig));
}

template <>
    NvStrapsConfig::BridgeConfig unpack<NvStrapsConfig::BridgeConfig>(BYTE const *&buffer)
{
    UINT8 busPos;

    return NvStrapsConfig::BridgeConfig
    {
        .vendorID            = unpack<UINT16>(buffer),
        .deviceID            = unpack<UINT16>(buffer),
        .bridgeBus           = unpack<UINT8>(buffer),
        .bridgeDevice        = UINT8 { (busPos = unpack<UINT8>(buffer), busPos >> 3u & 0b0001'1111u) },
        .bridgeFunction      = UINT8 { busPos & 0b0111u },
        .bridgeSecondaryBus  = unpack<UINT8>(buffer),
        .bridgeSubsidiaryBus = unpack<UINT8>(buffer),
        .bridgeIOBaseLimit   = unpack<UINT16>(buffer),
        .bridgeMemBaseLimit  = unpack<UINT32>(buffer)
    };
}

template <>
    BYTE *pack<NvStrapsConfig::BridgeConfig const &>(BYTE *&buffer, NvStrapsConfig::BridgeConfig const &pciConfig)
{
    pack(buffer, pciConfig.vendorID);
    pack(buffer, pciConfig.deviceID);
    pack(buffer, pciConfig.bridgeBus);
    pack(buffer, UINT8 { unsigned { pciConfig.bridgeDevice } << 3u & 0b1111'1000u | unsigned { pciConfig.bridgeFunction } & 0b0111u });
    pack(buffer, pciConfig.bridgeSecondaryBus);
    pack(buffer, pciConfig.bridgeSubsidiaryBus);
    pack(buffer, pciConfig.bridgeIOBaseLimit);
    pack(buffer, pciConfig.bridgeMemBaseLimit);

    return buffer;
}

template <>
    BYTE *pack<NvStrapsConfig::BridgeConfig &>(BYTE *&buffer, NvStrapsConfig::BridgeConfig &pciConfig)
{
    return pack(buffer, as_const(pciConfig));
}

static void unpackStrapsConfig(BYTE const *buffer, unsigned size)
{
    do
    {
        if (!size)
            break;

        strapsConfig.nGPUSelector = unpack<UINT8>(buffer);

        if (strapsConfig.nGPUSelector > ARRAY_SIZE(strapsConfig.GPUs) || size < BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE)
            break;

        for (auto index = 0u; index < strapsConfig.nGPUSelector; index++)
            strapsConfig.GPUs[index] = unpack<NvStrapsConfig::GPUSelector>(buffer);

        strapsConfig.nGPUConfig = unpack<UINT8>(buffer);

        if (strapsConfig.nGPUConfig > ARRAY_SIZE(strapsConfig.gpuConfig)
                || size < BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE + strapsConfig.nGPUConfig * GPU_CONFIG_SIZE)
        {
            break;
        }

        for (auto index = 0u; index < strapsConfig.nGPUConfig; index++)
            strapsConfig.gpuConfig[index] = unpack<NvStrapsConfig::GPUConfig>(buffer);

        strapsConfig.nBridgeConfig = unpack<UINT8>(buffer);

        if (strapsConfig.nBridgeConfig > ARRAY_SIZE(strapsConfig.bridge)
                 || size < BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE
                         + BYTE_SIZE + strapsConfig.nGPUConfig * GPU_CONFIG_SIZE
                         + BYTE_SIZE + strapsConfig.nBridgeConfig * BRIDGE_CONFIG_SIZE)
        {
            break;
        }

        for (auto index = 0u; index < strapsConfig.nBridgeConfig; index++)
            strapsConfig.bridge[index] = unpack<NvStrapsConfig::BridgeConfig>(buffer);

        return;
    }
    while (false);

    strapsConfig = NvStrapsConfig { };
}

static unsigned packStrapsConfig(BYTE *buffer)
{
    if (strapsConfig.nGPUSelector
         && strapsConfig.nGPUSelector < ARRAY_SIZE(strapsConfig.GPUs)
         && strapsConfig.nGPUConfig < ARRAY_SIZE(strapsConfig.gpuConfig)
         && strapsConfig.nBridgeConfig < ARRAY_SIZE(strapsConfig.bridge))
    {
        pack(buffer, strapsConfig.nGPUSelector);

        for (auto index = 0u; index < strapsConfig.nGPUSelector; index++)
            pack(buffer, strapsConfig.GPUs[index]);

        pack(buffer, strapsConfig.nGPUConfig);

        for (unsigned index = 0u; index < strapsConfig.nGPUConfig; index++)
            pack(buffer, strapsConfig.gpuConfig[index]);

        pack(buffer, strapsConfig.nBridgeConfig);

        for (auto index = 0u; index < strapsConfig.nBridgeConfig; index++)
            pack(buffer, strapsConfig.bridge[index]);

        return BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE
             + BYTE_SIZE + strapsConfig.nGPUConfig * GPU_CONFIG_SIZE
             + BYTE_SIZE + strapsConfig.nBridgeConfig * BRIDGE_CONFIG_SIZE;
    }

    return 0u;
}

NvStrapsConfig &GetNvStrapsPciConfig()
{
    if (strapsConfig.nGPUSelector == 0xFFu)
    {
#if defined(UEFI_SOURCE) || defined(EFIAPI)
        BYTE buffer[NV_STRAPS_CONFIG_SIZE];
        UINTN bufferSize = sizeof buffer;
        UINT32 attributes = 0u;
        EFI_STATUS status = gRT->GetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, &attributes, &bufferSize, buffer);

        if (status != EFI_SUCCESS)
            bufferSize = 0u;
#else
        BYTE buffer[NV_STRAPS_CONFIG_SIZE];
        DWORD bufferSize = GetFirmwareEnvironmentVariable(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, sizeof buffer);
#endif

        unpackStrapsConfig(buffer, static_cast<unsigned>(bufferSize));
    }

    return strapsConfig;
}

bool SaveNvStrapsPciConfig()
{
    constexpr auto const attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    BYTE buffer[NV_STRAPS_CONFIG_SIZE];
    bool done = false;

#if defined(UEFI_SOURCE) || defined(EFIAPI)
    if (strapsConfig.nGPUSelector && strapsConfig.nGPUSelector != 0xFFu)
        done = gRT->SetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, attributes, sizeof buffer, buffer) == EFI_SUCCESS;
    else
        done = gRT->SetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, attributes, 0u, buffer) == EFI_SUCCESS;
#else
    if (strapsConfig.nGPUSelector && strapsConfig.nGPUSelector != 0xFFu)
        done = !!SetFirmwareEnvironmentVariableEx(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, packStrapsConfig(buffer), attributes);
    else
        done = !!SetFirmwareEnvironmentVariableEx(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, 0u, attributes);

    if (!done)
        throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error setting EFI variable in NVRAM");
#endif

    return done;
}
