export module DeviceList;

import std;

import NvStraps.WinAPI;
import NvStraps.DXGI;
import WinApiError;
import ConfigManagerError;
import LocalAppConfig;
import NvStrapsConfig;

using std::uint_least8_t;
using std::uint_least16_t;
using std::uint_least64_t;
using std::wstring;
using std::vector;

export struct DeviceInfo
{
    uint_least16_t vendorID, deviceID, subsystemVendorID, subsystemDeviceID;
    uint_least8_t  bus, device, function;

    bool           busLocationSelector;

    struct
    {
    uint_least16_t vendorID, deviceID;
    uint_least8_t  bus, dev, func;
    }
           bridge;

    struct
    {
    uint_least64_t Base, Top;
    }
           bar0;
    uint_least64_t currentBARSize;
    uint_least64_t dedicatedVideoMemory;
    wstring        productName;
};

export wstring formatMemorySize(uint_least64_t size);
export vector<DeviceInfo> const &getDeviceList();

module: private;

using std::max;
using std::uint_least8_t;
using std::uint_least16_t;
using std::uint_least32_t;
using std::uint_least64_t;
using std::move;
using std::exchange;
using std::forward;
using std::wcstoul;
using std::to_string;
using std::to_wstring;
using std::string;
using std::wstring;
using std::vector;
using std::tuple;
using std::tie;
using std::exception;
using std::runtime_error;
using std::system_error;
using std::unique_ptr;
using std::stringstream;
using std::cerr;
using std::endl;
using std::wcerr;
using wregexp = std::wregex;
using std::wcmatch;
using std::wstring_view;
using std::endl;
using std::isprint;
using std::ranges::views::all;

namespace ranges = std::ranges;
namespace regexp_constants = std::regex_constants;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

template <typename ...ArgsT>
    static inline auto regexp_match(ArgsT && ...args) ->decltype(std::regex_match(forward<ArgsT>(args)...))
{
    return std::regex_match(forward<ArgsT>(args)...);
}

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

static bool fillDedicatedMemorySize(vector<DeviceInfo> &deviceSet)
{
    dxgi::IFactory *pFactory = nullptr;
    HRESULT hr = dxgi::CreateFactory(dxgi::IID_IFactory, static_cast<void **>(static_cast<void *>(&pFactory)));

    if (SUCCEEDED(hr))
    {
        UINT iAdapter = 0u;
    dxgi::IAdapter *pAdapter = nullptr;

        do
        {
            auto hr = pFactory->EnumAdapters(iAdapter, &pAdapter);

            if (SUCCEEDED(hr))
            {
        dxgi::ADAPTER_DESC adapterDescription { };

                if (SUCCEEDED(pAdapter->GetDesc(&adapterDescription)))
                {
                    ranges::for_each(deviceSet, [&adapterDescription](auto &devInfo)
                    {
                        if (devInfo.vendorID == adapterDescription.VendorId && devInfo.deviceID == adapterDescription.DeviceId
                                && (uint_least32_t { devInfo.subsystemDeviceID } << 16u | devInfo.subsystemVendorID) == adapterDescription.SubSysId)
                            devInfo.dedicatedVideoMemory = adapterDescription.DedicatedVideoMemory;
                    });
                }

                iAdapter++;
            }
            else
                if (hr != dxgi::ERROR_NOT_FOUND)
                {
                    pAdapter = nullptr;
                    throw runtime_error("DirectX Graphics Infrastructure function error listing GPUs\n"s);
                }
                else
                    pAdapter = nullptr;
        }
        while (pAdapter);

        return true;
    }
    else
        throw runtime_error("DirectX Graphics Infrastructure function error listing GPUs\n"s);

    return false;
}

wstring formatMemorySize(uint_least64_t size)
{
    wstring_view const suffixes[] = { L"Bytes"sv, L"KiB"sv, L"MiB"sv, L"GiB"sv, L"TiB"sv, L"PiB"sv };
    wstring_view unit = L"EiB"sv; // UINT64 can hold values up to 2 EBytes - 1

    for (auto suffix: suffixes)
        if (size >= 1024u)
            size = (size + 512u) / 1024u;
        else
        {
            unit = suffix;
            break;
        }

    return to_wstring(size) + L' ' + wstring { unit };
}

static bool nextResourceDescriptor(RES_DES &descriptor, RESOURCEID &resourceType)
{
    RES_DES nextDescriptor;
    resourceType = ResType_None;

    return validate_config_ret<CR_NO_MORE_RES_DES>(::CM_Get_Next_Res_Des(&nextDescriptor, descriptor, ResType_All, &resourceType, 0ul)) != CR_NO_MORE_RES_DES
         && (validate_config_ret(::CM_Free_Res_Des_Handle(exchange(descriptor, nextDescriptor))), true);
}

static tuple<uint_least64_t, tuple<uint_least64_t, uint_least64_t>> getMaxBarSize(DWORD deviceInstanceHandle, wstring const &productName)
try
{
    uint_least64_t maxBarSize { };
    tuple<uint_least64_t, uint_least64_t> BAR0 { };
    LOG_CONF logicalConfiguration;
    CONFIGRET configRet = CM_Get_First_Log_Conf(&logicalConfiguration, DEVINST { deviceInstanceHandle }, ALLOC_LOG_CONF);

    validate_config_ret(configRet, "Cannot get allocated configuration for device"s);

    unique_ptr<LOG_CONF, void (*)(LOG_CONF *)> configGuard(&logicalConfiguration, [](LOG_CONF *p) { validate_config_ret(::CM_Free_Log_Conf_Handle(*p)); });

    RESOURCEID resourceType = ResType_None;
    RES_DES resourceDescriptor;

    configRet = ::CM_Get_Next_Res_Des(&resourceDescriptor, RES_DES { logicalConfiguration }, ResType_All, &resourceType, ULONG { 0u });

    if (validate_config_ret<CR_NO_MORE_RES_DES>(configRet, "Cannot list resources allocated for device") != CR_NO_MORE_RES_DES)
    {
        unique_ptr<RES_DES, void (*)(RES_DES *)> resourceGuard(&resourceDescriptor, [](RES_DES *p) { validate_config_ret(::CM_Free_Res_Des_Handle(*p)); });

    bool firstResource = true;

        do
        {
        switch (resourceType)
        {
        case ResType_Mem:
        {
        MEM_RESOURCE memoryDescriptor { { .MD_Count = 0u, .MD_Type = MType_Range, .MD_Reserved = 0u } };
        validate_config_ret(::CM_Get_Res_Des_Data(resourceDescriptor, &memoryDescriptor, sizeof memoryDescriptor, ULONG { 0u }), "Error reading memory range resource for device");

        if ((memoryDescriptor.MEM_Header.MD_Flags & mMD_MemoryType) == fMD_RAM && (memoryDescriptor.MEM_Header.MD_Flags & mMD_Readable) == fMD_ReadAllowed)
        {
            maxBarSize = max(maxBarSize, uint_least64_t { memoryDescriptor.MEM_Header.MD_Alloc_End - memoryDescriptor.MEM_Header.MD_Alloc_Base });
            // std::cout << " - "s << formatMemorySize(memoryDescriptor.MEM_Header.MD_Alloc_End - memoryDescriptor.MEM_Header.MD_Alloc_Base) << " memory resource\n";

            if (firstResource)
            {
            BAR0 = tie(memoryDescriptor.MEM_Header.MD_Alloc_Base, memoryDescriptor.MEM_Header.MD_Alloc_End);
            firstResource = false;
            }
        }
        else
        {
            // BAR0 is not read-write
            firstResource = false;
            wcerr << L"BAR0 must be read-write for adapter: " << productName << endl;
        }

        break;
        }
        case ResType_MemLarge:
        {
        MEM_LARGE_RESOURCE memoryDescriptor { { .MLD_Count = 0u, .MLD_Type = MLType_Range, .MLD_Reserved = 0u } };
        validate_config_ret(::CM_Get_Res_Des_Data(resourceDescriptor, &memoryDescriptor, sizeof memoryDescriptor, ULONG { 0u }), "Error reading memory range resource for device");

        if ((memoryDescriptor.MEM_LARGE_Header.MLD_Flags & mMD_MemoryType) == fMD_RAM && (memoryDescriptor.MEM_LARGE_Header.MLD_Flags & mMD_Readable) == fMD_ReadAllowed)
        {
            maxBarSize = max(maxBarSize, uint_least64_t { memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_End - memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_Base });
            // std::cout << " - "s << formatMemorySize(memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_End - memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_Base) << " memory resource\n";

            if (firstResource)
            {
            BAR0 = tie(memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_Base, memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_End);
            firstResource = false;
            }
        }
        else
        {
            // Large BAR0 is not read-write
            firstResource = false;
            wcerr << L"BAR0 must be read-write for adapter: " << productName << endl;
        }

        break;
        }
        case ResType_IO:
        if (firstResource)
        {
            // BAR0 in the I/O port address space
            firstResource = false;
            wcerr << "Unexpected BAR0 in the I/O port address space for adapter: " << productName << endl;
        }

        [[fallthrough]];
        default:
        break;
        }

        }
        while (nextResourceDescriptor(resourceDescriptor, resourceType));

    if (BAR0 == tuple(0u, 0u))
        throw runtime_error("No proper BAR0 could be found for display adapter"s);
    }

    return tuple(maxBarSize, BAR0);
}
catch (system_error const &ex)
{
    cerr << ex.what() << endl;

    return { };
}
catch (exception const &ex)
{
    cerr << "Application error: " << ex.what() << endl;

    return { };
}
catch (...)
{
    cerr << "Application error while reading memory resource configuration\n";

    return { };
}

tuple<uint_least8_t, uint_least8_t, uint_least8_t> getDeviceBusLocation(HDEVINFO hDeviceInfoSet, SP_DEVINFO_DATA &devInfoData, auto &devPropBuffer, char const *deviceTypeDisplayName)
{
    DEVPROPTYPE devPropType;
    DWORD devPropLength;

    if (!::SetupDiGetDevicePropertyW(hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_BusNumber, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
    check_last_error("Error listing bus information for "s + deviceTypeDisplayName);
    else
    if (devPropType != DEVPROP_TYPE_UINT32 || devPropLength != sizeof(ULONG) || *static_cast<ULONG const *>(static_cast<void const *>(devPropBuffer)) > BYTE_BITMASK)
        throw runtime_error("Unexpected PCI bus number format " + to_string(devPropType) + ", length " + to_string(devPropLength) + ", value "s + to_string(*static_cast<ULONG const *>(static_cast<void const *>(devPropBuffer))) + "for display adapter"s);

    uint_least8_t bus = static_cast<uint_least8_t>(*static_cast<ULONG const *>(static_cast <void const *>(devPropBuffer)));

    if (!::SetupDiGetDevicePropertyW(hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_Address, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
    check_last_error("Error listing bus information for "s + deviceTypeDisplayName);
    else
    if (devPropType != DEVPROP_TYPE_UINT32 || devPropLength != sizeof(ULONG) || *static_cast<ULONG const *>(static_cast<void const *>(devPropBuffer)) & uint_least32_t { 0xFF'E0'FF'F8u })
        throw runtime_error("Unexpected PCI bus address format " + to_string(devPropType) + ", length " + to_string(devPropLength) + ", value "s + to_string(*static_cast<ULONG const *>(static_cast<void const *>(devPropBuffer))) + "for display adapter"s);

    uint_least8_t device = static_cast<uint_least8_t>(*static_cast<ULONG const *>(static_cast <void const *>(devPropBuffer)) >> 16u & BYTE_BITMASK);
    uint_least8_t function = static_cast<uint_least8_t>(*static_cast<ULONG const *>(static_cast <void const *>(devPropBuffer))) & BYTE_BITMASK;

    return tuple(bus, device, function);
}

tuple<uint_least8_t, uint_least8_t, uint_least8_t> getParentBridgeLocation(HDEVINFO hBridgeList, PCWSTR instanceID, auto &devPropBuffer)
{
    SP_DEVINFO_DATA devInfoData { .cbSize = sizeof devInfoData };

    if (::SetupDiOpenDeviceInfoW(hBridgeList, instanceID, ::GetConsoleWindow(), 0u, &devInfoData))
    return getDeviceBusLocation(hBridgeList, devInfoData, devPropBuffer, "PCI bridge");

    check_last_error("Reading PCI bridge for display adapter failed.");

    return tuple(BYTE_BITMASK, BYTE_BITMASK, BYTE_BITMASK);
}

// System-defined setup class for Display Adapters, from:
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/system-defined-device-setup-classes-available-to-vendors
static constexpr GUID const DisplayAdapterClass { 0x4D36E968u, 0xE325u, 0x11CEu, 0xBFu, 0xC1u, 0x08u, 0x00u, 0x2Bu, 0xE1u, 0x03u, 0x18u };

// Identifiers from the PCI bus driver, see:
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-pci-devices
static wregexp const pciInstanceRegexp { L"^PCI\\\\VEN_([0-9a-fA-F]{4})&DEV_([0-9a-fA-F]{4})&SUBSYS_([0-9a-fA-F]{4})([0-9a-fA-F]{4}).*$"s, regexp_constants::extended };

static void enumPciDisplayAdapters(vector<DeviceInfo> &deviceSet)
{
    struct DeviceInfoSet
    {
        HDEVINFO hDeviceInfoSet = INVALID_HANDLE_VALUE;

    DeviceInfoSet(HDEVINFO hDevInfo)
        : hDeviceInfoSet(hDevInfo)
    {
        if (hDevInfo == INVALID_HANDLE_VALUE)
        check_last_error("Error listing display adapters"s);
    }

        ~DeviceInfoSet()
        {
            if (hDeviceInfoSet != INVALID_HANDLE_VALUE)
                if (!::SetupDiDestroyDeviceInfoList(exchange(hDeviceInfoSet, INVALID_HANDLE_VALUE)))
            check_last_error("Error listing display adapters"s);
        }
    }
        dev(SetupDiGetClassDevsW(&DisplayAdapterClass, L"PCI", GetConsoleWindow(), DIGCF_PRESENT)),
    bridge(SetupDiCreateDeviceInfoList(NULL, GetConsoleWindow()));

    DWORD iDeviceIndex = 0u;
    SP_DEVINFO_DATA devInfoData { .cbSize = sizeof devInfoData, .Reserved = 0ul };
    DEVPROPTYPE     devPropType;
    BYTE            devPropBuffer[512u * sizeof(wchar_t)];
    WCHAR const    *devProp = static_cast<WCHAR *>(static_cast<void *>((devPropBuffer)));
    DWORD           devPropLength;

    std::locale globalLocale;

    while (::SetupDiEnumDeviceInfo(dev.hDeviceInfoSet, iDeviceIndex++, &devInfoData))
    {
        DeviceInfo deviceInfo { .dedicatedVideoMemory = 0ull };

        if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_InstanceId, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
            check_last_error("Error listing display adapters"s);

        if (wcmatch matches; regexp_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciInstanceRegexp))
        {
            deviceInfo.vendorID = static_cast<uint_least16_t>(wcstoul(matches[1u].str().c_str(), nullptr, 16));

#if defined(NDEBUG)
            if (deviceInfo.vendorID != TARGET_GPU_VENDOR_ID)
                continue;
#endif
            deviceInfo.deviceID = static_cast<uint_least16_t>(wcstoul(matches[2u].str().c_str(), nullptr, 16));
            deviceInfo.subsystemDeviceID = static_cast<uint_least16_t>(wcstoul(matches[3u].str().c_str(), nullptr, 16));
            deviceInfo.subsystemVendorID = static_cast<uint_least16_t>(wcstoul(matches[4u].str().c_str(), nullptr, 16));

            if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_NAME, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
                check_last_error("Error listing display adapters"s);

            deviceInfo.productName.assign(static_cast<wchar_t *>(static_cast<void *>(devPropBuffer)), devPropLength / sizeof(wchar_t));

            while (deviceInfo.productName | all && !isprint(*deviceInfo.productName.rbegin(), globalLocale))
                deviceInfo.productName.pop_back();

        tie(deviceInfo.bus, deviceInfo.device, deviceInfo.function) = getDeviceBusLocation(dev.hDeviceInfoSet, devInfoData, devPropBuffer, "display adapter");

        if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_Parent, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
        check_last_error("Error listing bus information for display adapter"s);
        else
        if (devPropType != DEVPROP_TYPE_STRING || devPropLength % sizeof(WCHAR) || devPropLength + sizeof(WCHAR) > sizeof devPropBuffer)
            throw runtime_error("Unexpected parent bus ID format " + to_string(devPropType) + ", of length " + to_string(devPropLength));

        auto len = devPropLength / sizeof(WCHAR);

        static_cast<WCHAR *>(static_cast<void *>(devPropBuffer))[len] = WCHAR { };

        if (wcmatch matches; regexp_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciInstanceRegexp))
        {
        deviceInfo.bridge.vendorID = static_cast<uint_least16_t>(wcstoul(matches[1u].str().c_str(), nullptr, 16));
        deviceInfo.bridge.deviceID = static_cast<uint_least16_t>(wcstoul(matches[2u].str().c_str(), nullptr, 16));
        }
        else
        {
        std::wcout << L"Device instace ID: "s << wstring_view(devProp, devPropLength / sizeof *devProp) << std::endl;
        throw runtime_error("Error listing PCI bridge for display adapter: wrong PCI instance ID property value"s);
        }

        tie(deviceInfo.bridge.bus, deviceInfo.bridge.dev, deviceInfo.bridge.func) = getParentBridgeLocation(bridge.hDeviceInfoSet, devProp, devPropBuffer);

        auto DeviceBAR0 = tie(deviceInfo.bar0.Base, deviceInfo.bar0.Top);
            tie(deviceInfo.currentBARSize, DeviceBAR0) = getMaxBarSize(devInfoData.DevInst, deviceInfo.productName);
        }
        else
        {
            std::wcout << L"Device instace ID: "s << wstring_view(devProp, devPropLength / sizeof *devProp) << std::endl;
            throw runtime_error("Error listing display adapters: wrong PCI instance ID property value"s);
        }

        deviceSet.push_back(move(deviceInfo));
        devInfoData.Reserved = 0ul;
    }

    if (DWORD dwLastError = ::GetLastError(); dwLastError != ERROR_NO_MORE_ITEMS)
        check_last_error(dwLastError, "Error listing display adapters"s);
}

static vector<DeviceInfo> emptyDeviceSet;

vector<DeviceInfo> const &getDeviceList()
try
{
    static vector<DeviceInfo> deviceSet;

    if (deviceSet.empty())
    {
        enumPciDisplayAdapters(deviceSet);
        fillDedicatedMemorySize(deviceSet);
    }

    return deviceSet;
}
catch (system_error const &ex)
{
    cerr << ex.what() << endl;

    return emptyDeviceSet;
}
catch (exception const &ex)
{
    cerr << "Application error while listing display adapters: " << ex.what() << endl;

    return emptyDeviceSet;
}
catch (...)
{
    cerr << "Application error while listing display adapters\n";

    return emptyDeviceSet;
}

#endif
