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
# include <execution>
#endif

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <algorithm>
#include <tuple>
#include <utility>
#include <ranges>

#include "DeviceRegistry.hh"
#include "NvStrapsPciConfig.hh"

using std::size_t;
using std::byte;
using std::to_integer;
using std::as_const;
using std::exchange;
using std::begin;
using std::end;
using std::size;
using std::find_if;
using std::copy;
using std::tuple;
using std::tie;

#if defined(UEFI_SOURCE) || defined(EFIAPI)
#else
namespace execution = std::execution;
#endif

namespace views = std::views;

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

static NvStrapsConfig strapsConfig;

template <typename UnsignedType, size_t SIZE>
    static inline UnsignedType unpack(byte const *&buffer)
{
    static_assert(sizeof(UnsignedType) >= SIZE, "Result type for unpack<>() too small for the number of bytes requested");
    UnsignedType value = UnsignedType { };

    for (auto i = 0u; i < SIZE; i++)
        value |= to_integer<UnsignedType>(*buffer++) << 0x08u * i;

    return value;
}

static inline UINT8 unpack_BYTE(byte const *&buffer)
{
    return unpack<UINT8, BYTE_SIZE>(buffer);
}

static inline UINT16 unpack_WORD(byte const *&buffer)
{
    return unpack<UINT16, WORD_SIZE>(buffer);
}

static inline UINT32 unpack_DWORD(byte const *&buffer)
{
    return unpack<UINT32, DWORD_SIZE>(buffer);
}

static inline UINT64 unpack_QWORD(byte const *&buffer)
{
    return unpack<UINT64, QWORD_SIZE>(buffer);
}

template <size_t SIZE, typename UnsignedType>
    static inline byte *pack(byte *&buffer, UnsignedType &&value)
{
    static_assert(sizeof value >= SIZE, "Source type for pack<>() too small for the number of bytes requested");

    for (auto i = 0u; i < SIZE; i++)
        *buffer++ = byte { value >> i & 0xFFu };

    return buffer;
}

static inline byte *pack_BYTE(byte *&buffer, UINT8 value)
{
    return pack<BYTE_SIZE>(buffer, value);
}

static inline byte *pack_WORD(byte *&buffer, UINT16 value)
{
    return pack<WORD_SIZE>(buffer, value);
}

static inline byte *pack_DWORD(byte *&buffer, UINT32 value)
{
    return pack<DWORD_SIZE>(buffer, value);
}

static inline byte *pack_QWORD(byte *&buffer, UINT64 value)
{
    return pack<QWORD_SIZE>(buffer, value);
}

void NvStrapsConfig::GPUSelector::unpack(byte const *&buffer)
{
    UINT8 busPos;

    deviceID         = unpack_WORD(buffer);
    subsysVendorID   = unpack_WORD(buffer);
    subsysDeviceID   = unpack_WORD(buffer);
    bus              = unpack_BYTE(buffer);
    device           = UINT8 { (busPos = unpack_BYTE(buffer), busPos >> 3u & 0b0001'1111u) };
    function         = UINT8 { busPos & 0b0111u };
    barSizeSelector  = unpack_BYTE(buffer);
}

byte *NvStrapsConfig::GPUSelector::pack(byte *&buffer) const
{
    pack_WORD(buffer, deviceID);
    pack_WORD(buffer, subsysVendorID);
    pack_WORD(buffer, subsysDeviceID);
    pack_BYTE(buffer, bus);
    pack_BYTE(buffer, UINT8 { unsigned { device } << 3u & 0x1111'1000u | unsigned { function } & 0b0111u });
    pack_BYTE(buffer, barSizeSelector);

    return buffer;
}

void NvStrapsConfig::GPUConfig::unpack(byte const *&buffer)
{
    deviceID            = unpack_WORD(buffer);
    subsysVendorID      = unpack_WORD(buffer);
    subsysDeviceID      = unpack_WORD(buffer);
    bus                 = unpack_BYTE(buffer);
    baseAddressSelector = unpack_DWORD(buffer);
}

byte *NvStrapsConfig::GPUConfig::pack(byte *&buffer) const
{
    pack_WORD(buffer, deviceID);
    pack_WORD(buffer, subsysVendorID);
    pack_WORD(buffer, subsysDeviceID);
    pack_BYTE(buffer, bus);
    pack_DWORD(buffer, baseAddressSelector);

    return buffer;
}

void NvStrapsConfig::BridgeConfig::unpack(byte const *&buffer)
{
    UINT8 busPos;

    vendorID            = unpack_WORD(buffer);
    deviceID            = unpack_WORD(buffer);
    bridgeBus           = unpack_BYTE(buffer);
    bridgeDevice        = UINT8 { (busPos = unpack_BYTE(buffer), busPos >> 3u & 0b0001'1111u) };
    bridgeFunction      = UINT8 { busPos & 0b0111u };
    bridgeSecondaryBus  = unpack_BYTE(buffer);
    bridgeSubsidiaryBus = unpack_BYTE(buffer);
    bridgeIOBaseLimit   = unpack_WORD(buffer);
    bridgeMemBaseLimit  = unpack_DWORD(buffer);
}

byte *NvStrapsConfig::BridgeConfig::pack(byte *&buffer) const
{
    pack_WORD(buffer, vendorID);
    pack_WORD(buffer, deviceID);
    pack_BYTE(buffer, bridgeBus);
    pack_BYTE(buffer, UINT8 { unsigned { bridgeDevice } << 3u & 0b1111'1000u | unsigned { bridgeFunction } & 0b0111u });
    pack_BYTE(buffer, bridgeSecondaryBus);
    pack_BYTE(buffer, bridgeSubsidiaryBus);
    pack_WORD(buffer, bridgeIOBaseLimit);
    pack_DWORD(buffer, bridgeMemBaseLimit);

    return buffer;
}

void NvStrapsConfig::clear()
{
    nGlobalEnable = 0u;
    nGPUSelector = 0xFFu;
    nGPUConfig = 0u;
    nBridgeConfig = 0u;
}

void NvStrapsConfig::load(byte const *buffer, unsigned size)
{
    do
    {
        if (size < 4u)
            break;

        nGlobalEnable = unpack_BYTE(buffer);
        nGPUSelector = unpack_BYTE(buffer);

        if (strapsConfig.nGPUSelector > ARRAY_SIZE(strapsConfig.GPUs) || size < 2u * BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE)
            break;

        for (auto index = 0u; index < strapsConfig.nGPUSelector; index++)
            strapsConfig.GPUs[index].unpack(buffer);

        strapsConfig.nGPUConfig = unpack_BYTE(buffer);

        if (strapsConfig.nGPUConfig > ARRAY_SIZE(strapsConfig.gpuConfig)
                || size < 2u * BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE + BYTE_SIZE + strapsConfig.nGPUConfig * GPU_CONFIG_SIZE + BYTE_SIZE)
        {
            break;
        }

        for (auto index = 0u; index < strapsConfig.nGPUConfig; index++)
            strapsConfig.gpuConfig[index].unpack(buffer);

        strapsConfig.nBridgeConfig = unpack_BYTE(buffer);

        if (strapsConfig.nBridgeConfig > ARRAY_SIZE(strapsConfig.bridge)
                 || size < 2u * BYTE_SIZE + strapsConfig.nGPUSelector * GPU_SELECTOR_SIZE
                         + BYTE_SIZE + strapsConfig.nGPUConfig * GPU_CONFIG_SIZE
                         + BYTE_SIZE + strapsConfig.nBridgeConfig * BRIDGE_CONFIG_SIZE)
        {
            break;
        }

        for (auto index = 0u; index < strapsConfig.nBridgeConfig; index++)
            strapsConfig.bridge[index].unpack(buffer);

        return;
    }
    while (false);

    clear();
}

unsigned NvStrapsConfig::save(byte *buffer) const
{
    if ((nGlobalEnable || nGPUSelector)
         && nGPUSelector < ARRAY_SIZE(GPUs)
         && nGPUConfig < ARRAY_SIZE(gpuConfig)
         && nBridgeConfig < ARRAY_SIZE(bridge))
    {
        pack_BYTE(buffer, nGlobalEnable);
        pack_BYTE(buffer, nGPUSelector);

        for (auto index = 0u; index < nGPUSelector; index++)
            GPUs[index].pack(buffer);

        pack_BYTE(buffer, nGPUConfig);

        for (unsigned index = 0u; index < nGPUConfig; index++)
            gpuConfig[index].pack(buffer);

        pack_BYTE(buffer, nBridgeConfig);

        for (auto index = 0u; index < nBridgeConfig; index++)
            bridge[index].pack(buffer);

        return BYTE_SIZE + nGPUSelector * GPU_SELECTOR_SIZE
             + BYTE_SIZE + nGPUConfig * GPU_CONFIG_SIZE
             + BYTE_SIZE + nBridgeConfig * BRIDGE_CONFIG_SIZE;
    }

    return 0u;
}

bool NvStrapsConfig::isConfigured() const
{
    return nGlobalEnable || nGPUSelector;
};

bool NvStrapsConfig::GPUSelector::deviceMatch(UINT16 devID) const
{
    return deviceID == devID;
}

bool NvStrapsConfig::GPUSelector::subsystemMatch(UINT16 subsysVenID, UINT16 subsysDevID) const
{
    return subsysVendorID == subsysVenID && subsysDeviceID == subsysDevID;
}

bool NvStrapsConfig::GPUSelector::busLocationMatch(UINT8 busNr, UINT8 dev, UINT8 func) const
{
    return bus == busNr && device == dev && function == func;
}

#if defined(UEFI_SOURCE) || defined(EFIAPI)
#else

bool NvStrapsConfig::setGPUSelector(UINT16 deviceID, UINT16 subsysDevID, UINT16 subsysVenID, UINT8 bus, UINT8 dev, UINT8 fn, UINT8 barSizeSelector)
{
    GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn,
        .barSizeSelector = barSizeSelector
    };

    auto end_it = begin(GPUs) + nGPUSelector;
    auto it = find_if(execution::par_unseq, begin(GPUs), end_it, [&gpuSelector](auto const &selector)
        {
            return selector.deviceMatch(gpuSelector.deviceID)
                 && selector.subsystemMatch(gpuSelector.subsysVendorID, gpuSelector.subsysDeviceID)
                 && selector.busLocationMatch(gpuSelector.bus, gpuSelector.device, gpuSelector.function);
        });

    if (it == end_it)
        if (nGPUSelector >= size(GPUs))
            return false;
        else
            GPUs[nGPUSelector++] = gpuSelector;
    else
        *it = gpuSelector;

    return true;
}

bool NvStrapsConfig::clearGPUSelector(UINT16 deviceID, UINT16 subsysDevID, UINT16 subsysVenID, UINT8 bus, UINT8 dev, UINT8 fn)
{
    GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn
    };

    auto end_it = begin(GPUs) + nGPUSelector;
    auto it = find_if(execution::par_unseq, begin(GPUs), end_it, [&gpuSelector](auto const &selector)
        {
            return selector.deviceMatch(gpuSelector.deviceID)
                 && selector.subsystemMatch(gpuSelector.subsysVendorID, gpuSelector.subsysDeviceID)
                 && selector.busLocationMatch(gpuSelector.bus, gpuSelector.device, gpuSelector.function);
        });

    if (it == end_it)
        return false;

    copy(it + 1u, end_it, it);
    nGPUSelector--;

    return true;
}

bool NvStrapsConfig::clearGPUSelectors()
{
    return !!exchange(nGPUSelector, 0u);
}

#endif          // #lse defined(UEFI_SOURCE)...

tuple<ConfigPriority, BarSizeSelector> NvStrapsConfig::lookupBarSize(UINT16 deviceID, UINT16 subsysDevID, UINT16 subsysVenID, UINT8 bus, UINT8 dev, UINT8 fn) const
{
    ConfigPriority configPriority = ConfigPriority::UNCONFIGURED;
    BarSizeSelector barSizeSelector;

    for (auto const &selector: GPUs | views::take(nGPUSelector))
        if (selector.deviceMatch(deviceID))
            if (selector.subsystemMatch(subsysVenID, subsysDevID))
                if (selector.busLocationMatch(bus, dev, fn))
                    return tuple { ConfigPriority::EXPLICIT_PCI_LOCATION, BarSizeSelector { selector.barSizeSelector } };
                else
                    tie(configPriority, barSizeSelector) = tuple { ConfigPriority::EXPLICIT_SUBSYSTEM_ID, BarSizeSelector {selector.barSizeSelector } };
            else
            {
                if (configPriority < ConfigPriority::EXPLICIT_SUBSYSTEM_ID)
                    tie(configPriority, barSizeSelector) = tuple { ConfigPriority::EXPLICIT_PCI_ID, BarSizeSelector { selector.barSizeSelector } };
            }

    if (configPriority == ConfigPriority::UNCONFIGURED && nGlobalEnable)
    {
        barSizeSelector = lookupBarSizeInRegistry(deviceID);

        if (barSizeSelector == BarSizeSelector::None)
        {
            if (nGlobalEnable > 1u && isTuringGPU(deviceID))
                return tuple { ConfigPriority::IMPLIED_GLOBAL, BarSizeSelector::_2G };
        }
        else
            configPriority = ConfigPriority::FOUND_GLOBAL;
    }

    return tuple { configPriority, barSizeSelector };
}

NvStrapsConfig &GetNvStrapsPciConfig()
{
    static bool isLoaded = false;

    if (!isLoaded)
    {
        byte buffer[strapsConfig.bufferSize()];

#if defined(UEFI_SOURCE) || defined(EFIAPI)
        UINTN bufferSize = sizeof buffer;
        UINT32 attributes = 0u;
        EFI_STATUS status = gRT->GetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, &attributes, &bufferSize, buffer);

        if (status != EFI_SUCCESS)
            bufferSize = 0u;
#else
        DWORD bufferSize = GetFirmwareEnvironmentVariable(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, sizeof buffer);
#endif

        strapsConfig.load(buffer, static_cast<unsigned>(bufferSize));

        isLoaded = true;
    };

    return strapsConfig;
}

bool SaveNvStrapsPciConfig()
{
    constexpr auto const attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    byte buffer[strapsConfig.bufferSize()];
    bool done = false;

#if defined(UEFI_SOURCE) || defined(EFIAPI)
    if (strapsConfig.isConfigured())
        done = gRT->SetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, attributes, sizeof buffer, buffer) == EFI_SUCCESS;
    else
        done = gRT->SetVariable(static_cast<CHAR16 *>(static_cast<void *>(NvStrapsPciConfigName)), &NvStrapsPciConfigGUID, attributes, 0u, buffer) == EFI_SUCCESS;
#else
    if (strapsConfig.isConfigured())
        done = !!SetFirmwareEnvironmentVariableEx(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, strapsConfig.save(buffer), attributes);
    else
        done = !!SetFirmwareEnvironmentVariableEx(NvStrapsPciConfigName, NvStrapsPciConfigGUID, buffer, 0u, attributes);

    if (!done)
        throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error setting EFI variable in NVRAM");
#endif

    return done;
}
