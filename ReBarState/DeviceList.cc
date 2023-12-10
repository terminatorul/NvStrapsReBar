#include <cstdint>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <locale>
#include <iterator>
#include <sstream>
#include <memory>
#include <iomanip>
#include <regex>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <utility>
#include <algorithm>
#include <ranges>
#include <iostream>
#include <ranges>

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#include <initguid.h>
#include <Windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devpkey.h>
#include <dxgi.h>

#undef min
#undef max
#endif

#include "WinAPIError.hh"
#include "ConfigManagerError.hh"
#include "DeviceList.hh"

using std::max;
using std::min;
using std::uint_least64_t;
using std::move;
using std::exchange;
using std::wcstoul;
using std::to_string;
using std::to_wstring;
using std::string;
using std::wstring;
using std::vector;
using std::exception;
using std::runtime_error;
using std::system_error;
using std::unique_ptr;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::wcout;
using wregexp = std::wregex;
using std::match_results;
using std::regex_match;
using std::wcmatch;
using std::span;
using std::wstring_view;
using std::hex;
using std::setfill;
using std::setw;
using std::endl;
using std::isprint;
using std::ranges::views::all;
namespace ranges = std::ranges;
namespace views = std::ranges::views;
namespace regexp_constants = std::regex_constants;
using namespace std::literals::string_literals;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

static bool fillDedicatedMemorySize(vector<DeviceInfo> &deviceSet)
{
    IDXGIFactory *pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(IID_IDXGIFactory, static_cast<void **>(static_cast<void *>(&pFactory)));

    if (SUCCEEDED(hr))
    {
        UINT iAdapter = 0u;
        IDXGIAdapter *pAdapter = nullptr;

        do
        {
            auto hr = pFactory->EnumAdapters(iAdapter, &pAdapter);

            if (SUCCEEDED(hr))
            {
                DXGI_ADAPTER_DESC adapterDescription { };

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
                if (hr != DXGI_ERROR_NOT_FOUND)
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
    wchar_t const * const suffixes[] = { L"Bytes", L"KB", L"MB", L"GB", L"TB", L"PB" };
    wchar_t const *unit = L"EB"; // UINT64 can hold values up to 2 EBytes - 1

    for (auto suffix: suffixes)
        if (size >= 1024u)
            size = (size + 512u) / 1024u;
        else
        {
            unit = suffix;
            break;
        }

    return to_wstring(size) + L' ' + unit;
}

static bool nextResourceDescriptor(RES_DES &descriptor, RESOURCEID forResource)
{
    RES_DES nextDescriptor;

    return validate_config_ret<CR_NO_MORE_RES_DES>(::CM_Get_Next_Res_Des(&nextDescriptor, descriptor, forResource, NULL, 0ul)) != CR_NO_MORE_RES_DES
         && (validate_config_ret(::CM_Free_Res_Des_Handle(exchange(descriptor, nextDescriptor))), true);
}

static uint_least64_t getMaxBarSize(DWORD deviceInstanceHandle)
try
{
    uint_least64_t maxBarSize { };
    LOG_CONF logicalConfiguration;
    CONFIGRET configRet = CM_Get_First_Log_Conf(&logicalConfiguration, DEVINST { deviceInstanceHandle }, ALLOC_LOG_CONF);

    validate_config_ret(configRet, "Cannot get allocated configuration for device"s);

    unique_ptr<LOG_CONF, void (*)(LOG_CONF *)> configGuard(&logicalConfiguration, [](LOG_CONF *p) { validate_config_ret(::CM_Free_Log_Conf_Handle(*p)); });

    RES_DES resourceDescriptor;

    configRet = ::CM_Get_Next_Res_Des(&resourceDescriptor, RES_DES { logicalConfiguration }, ResType_Mem, NULL, 0ul);

    if (validate_config_ret<CR_NO_MORE_RES_DES>(configRet, "Cannot list memory resources allocated for device") != CR_NO_MORE_RES_DES)
    {
        unique_ptr<RES_DES, void (*)(RES_DES *)> resourceGuard(&resourceDescriptor, [](RES_DES *p) { validate_config_ret(::CM_Free_Res_Des_Handle(*p)); });

        do
        {
            MEM_RESOURCE memoryDescriptor { { .MD_Count = 0u, .MD_Type = MType_Range, .MD_Reserved = 0u } };
            validate_config_ret(::CM_Get_Res_Des_Data(resourceDescriptor, &memoryDescriptor, sizeof memoryDescriptor, 0ul), "Error reading memory range resource for device");

            if ((memoryDescriptor.MEM_Header.MD_Flags & mMD_MemoryType) == fMD_RAM && (memoryDescriptor.MEM_Header.MD_Flags & mMD_Readable) == fMD_ReadAllowed)
                maxBarSize = max(maxBarSize, uint_least64_t { memoryDescriptor.MEM_Header.MD_Alloc_End - memoryDescriptor.MEM_Header.MD_Alloc_Base });
                // std::cout << " - "s << formatMemorySize(memoryDescriptor.MEM_Header.MD_Alloc_End - memoryDescriptor.MEM_Header.MD_Alloc_Base) << " memory resource\n";
        }
        while (nextResourceDescriptor(resourceDescriptor, ResType_Mem));
    }

    configRet = ::CM_Get_Next_Res_Des(&resourceDescriptor, RES_DES { logicalConfiguration }, ResType_MemLarge, NULL, 0ul);

    if (validate_config_ret<CR_NO_MORE_RES_DES>(configRet, "Cannot list memory resources allocated for device") != CR_NO_MORE_RES_DES)
    {
        unique_ptr<RES_DES, void (*)(RES_DES *)> resourceGuard(&resourceDescriptor, [](RES_DES *p) { validate_config_ret(::CM_Free_Res_Des_Handle(*p)); });

        do
        {
            MEM_LARGE_RESOURCE memoryDescriptor { { .MLD_Count = 0u, .MLD_Type = MLType_Range, .MLD_Reserved = 0u } };
            validate_config_ret(::CM_Get_Res_Des_Data(resourceDescriptor, &memoryDescriptor, sizeof memoryDescriptor, 0ul), "Error reading memory range resource for device");

            if ((memoryDescriptor.MEM_LARGE_Header.MLD_Flags & mMD_MemoryType) == fMD_RAM && (memoryDescriptor.MEM_LARGE_Header.MLD_Flags & mMD_Readable) == fMD_ReadAllowed)
                maxBarSize = max(maxBarSize, uint_least64_t { memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_End - memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_Base });
                // std::cout << " - "s << formatMemorySize(memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_End - memoryDescriptor.MEM_LARGE_Header.MLD_Alloc_Base) << " memory resource\n";
        }
        while (nextResourceDescriptor(resourceDescriptor, ResType_MemLarge));
    }

    return maxBarSize;
}
catch (system_error const &ex)
{
    cerr << ex.what() << endl;

    return 0u;
}
catch (exception const &ex)
{
    cerr << "Application error: " << ex.what() << endl;

    return 0u;
}
catch (...)
{
    cerr << "Application error while reading memory resource configuration\n";

    return 0u;
}

// System-defined setup class for Display Adapters, from:
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/system-defined-device-setup-classes-available-to-vendors
static constexpr GUID const DisplayAdapterClass { 0x4d36e968, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

// Identifiers from the PCI bus driver, see:
// https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-pci-devices
static wregexp const pciInstanceRegexp { L"^PCI\\\\VEN_([0-9a-fA-F]{4})&DEV_([0-9a-fA-F]{4})&SUBSYS_([0-9a-fA-F]{4})([0-9a-fA-F]{4}).*$"s, regexp_constants::extended },
        pciLocationRegexp { L"^PCI bus ([0-9]+), device ([0-9]+), function ([0-9]+).*$"s, regexp_constants::extended };

static void enumPciDisplayAdapters(vector<DeviceInfo> &deviceSet)
{
    struct DeviceInfoSet
    {
        HDEVINFO hDeviceInfoSet = SetupDiGetClassDevsW(&DisplayAdapterClass, L"PCI", GetConsoleWindow(), DIGCF_PRESENT);

        ~DeviceInfoSet()
        {
            if (hDeviceInfoSet != INVALID_HANDLE_VALUE)
                ::SetupDiDestroyDeviceInfoList(exchange(hDeviceInfoSet, INVALID_HANDLE_VALUE));
        }
    }
        dev;

    if (dev.hDeviceInfoSet == INVALID_HANDLE_VALUE)
        throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters"s);

    DWORD iDeviceIndex = 0u;
    SP_DEVINFO_DATA devInfoData { .cbSize = sizeof devInfoData, .Reserved = 0ul };
    DEVPROPTYPE     devPropType;
    BYTE            devPropBuffer[512u * sizeof(wchar_t)];
    wchar_t const  *devProp = static_cast<wchar_t *>(static_cast<void *>((devPropBuffer)));
    DWORD           devPropLength;

    std::locale globalLocale;

    while (::SetupDiEnumDeviceInfo(dev.hDeviceInfoSet, iDeviceIndex++, &devInfoData))
    {
        DeviceInfo deviceInfo { .barSizeSelector = 0u, .dedicatedVideoMemory = 0ull };

        if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_InstanceId, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
            throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters"s);

        if (wcmatch matches; regex_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciInstanceRegexp))
        {
            deviceInfo.vendorID = static_cast<uint_least16_t>(wcstoul(matches[1u].str().c_str(), nullptr, 16));
            deviceInfo.deviceID = static_cast<uint_least16_t>(wcstoul(matches[2u].str().c_str(), nullptr, 16));
            deviceInfo.subsystemDeviceID = static_cast<uint_least16_t>(wcstoul(matches[3u].str().c_str(), nullptr, 16));
            deviceInfo.subsystemVendorID = static_cast<uint_least16_t>(wcstoul(matches[4u].str().c_str(), nullptr, 16));

            if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_NAME, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
                throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters"s);

            deviceInfo.productName.assign(static_cast<wchar_t *>(static_cast<void *>(devPropBuffer)), devPropLength / sizeof(wchar_t));

            while (deviceInfo.productName | all && !isprint(*deviceInfo.productName.rbegin(), globalLocale))
                deviceInfo.productName.pop_back();

            if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_LocationInfo, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
                throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters"s);

            if (wcmatch matches; regex_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciLocationRegexp))
            {
                deviceInfo.bus = static_cast<uint_least8_t>(wcstoul(matches[1u].first, nullptr, 10));
                deviceInfo.device = static_cast<uint_least8_t>(wcstoul(matches[2u].first, nullptr, 10));
                deviceInfo.function = static_cast<uint_least8_t>(wcstoul(matches[3u].first, nullptr, 10));
            }
            else
            {
                std::wcout << L"PCI location: ["s << wstring_view(devProp, devPropLength / sizeof *devProp) << L']' << std::endl;
                throw runtime_error("Error listing display adapters: wrong PCI location property value."s);
            }

            deviceInfo.currentBARSize = getMaxBarSize(devInfoData.DevInst);
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
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), "Error listing display adapters"s);
}

static vector<DeviceInfo> emptyDeviceSet;

vector<DeviceInfo> &getDeviceList()
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
    cerr << "Applicaion error while listing display adapters: " << ex.what() << endl;

    return emptyDeviceSet;
}
catch (...)
{
    cerr << "Application error while listing display adapters\n";

    return emptyDeviceSet;
}

#endif
