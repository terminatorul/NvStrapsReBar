#include <stdexcept>
#include <system_error>
#include <locale>
#include <sstream>
#include <memory>
#include <iomanip>
#include <regex>
#include <vector>
#include <utility>
#include <ranges>
#include <iostream>

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#include <initguid.h>
#include <Windows.h>
#include <setupapi.h>
#include <devpkey.h>
#include <dxgi.h>
#endif

#include "DeviceList.hh"

using std::move;
using std::exchange;
using std::string;
using std::wstring;
using std::vector;
using std::runtime_error;
using std::error_category;
using std::system_error;
using std::unique_ptr;
using std::stringstream;
using wregexp = std::wregex;
using std::match_results;
using std::regex_match;
using std::wcmatch;
using std::hex;
using std::setfill;
using std::setw;
using std::endl;
using std::isprint;
namespace regexp_constants = std::regex_constants;
using namespace std::literals::string_literals;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

class WinAPIErrorCategory: public error_category
{
public:
    char const *name() const noexcept override;
    string message(int error) const override;

protected:
    WinAPIErrorCategory() = default;

    friend error_category const &winapi_error_category();
};

char const *WinAPIErrorCategory::name() const noexcept
{
    return "winapi";
}

string WinAPIErrorCategory::message(int error) const
{
    struct LocalMem
    {
        HLOCAL hLocal = nullptr;
        ~LocalMem()
        {
            ::LocalFree(exchange(hLocal, nullptr));
        }
    }
        localMessageBuffer;

    if (auto nLength = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, static_cast<DWORD>(error),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), static_cast<LPSTR>(static_cast<void *>(&localMessageBuffer.hLocal)), 0, nullptr))
    {
        return { static_cast<char const *>(static_cast<void *>(localMessageBuffer.hLocal)), nLength };
    }
    else
    {
        auto dwFormatMessageError = ::GetLastError();

        return static_cast<stringstream &>(stringstream { "Windows API error 0x"s } << hex << setfill('0') << setw(sizeof(DWORD) * 2u) << static_cast<DWORD>(error) << endl
                 << "(FormatError() returned 0x" << hex << setfill('0') << setw(sizeof(dwFormatMessageError) * 2) << dwFormatMessageError << ')').str();
    }
}

inline error_category const &winapi_error_category()
{
    static WinAPIErrorCategory errorCategory;

    return errorCategory;
}

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
                    auto it = std::ranges::find_if(deviceSet, [&adapterDescription](auto &devInfo) -> bool
                        {
                            return devInfo.vendorID == adapterDescription.VendorId && devInfo.deviceID == adapterDescription.DeviceId;
                        });

                    if (it != deviceSet.end())
                        it->dedicatedVideoMemory = adapterDescription.DedicatedVideoMemory;
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

// System-defined setup class for Display Adapters: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/system-defined-device-setup-classes-available-to-vendors
static constexpr GUID const DisplayAdapterClass { 0x4d36e968, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };
static wregexp const pciInstanceRegexp { L"^PCI\\\\VEN_([0-9a-fA-F]*)&DEV_([0-9a-fA-F]*).*$"s, regexp_constants::extended },
        pciLocationRegexp { L"^PCI bus ([0-9]*), device ([0-9]*), function ([0-9]*).*$"s, regexp_constants::extended };

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
        throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters");

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
            throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters");

        if (wcmatch matches; regex_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciInstanceRegexp))
        {
            deviceInfo.vendorID = static_cast<uint_least16_t>(std::wcstoul(matches[1u].first, nullptr, 16));
            deviceInfo.deviceID = static_cast<uint_least16_t>(std::wcstoul(matches[2u].first, nullptr, 16));

            if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_NAME, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
                throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters");

            deviceInfo.productName.assign(static_cast<wchar_t *>(static_cast<void *>(devPropBuffer)), devPropLength / sizeof(wchar_t));

            while (!isprint(*deviceInfo.productName.rbegin(), globalLocale))
                deviceInfo.productName.pop_back();

            if (!::SetupDiGetDevicePropertyW(dev.hDeviceInfoSet, &devInfoData, &DEVPKEY_Device_LocationInfo, &devPropType, devPropBuffer, sizeof devPropBuffer, &devPropLength, 0u))
                throw system_error(static_cast<int>(::GetLastError()), winapi_error_category(), "Error listing display adapters");

            if (wcmatch matches; regex_match(devProp, devProp + devPropLength / sizeof *devProp, matches, pciLocationRegexp))
            {
                deviceInfo.bus = static_cast<uint_least8_t>(wcstoul(matches[1u].first, nullptr, 10));
                deviceInfo.device = static_cast<uint_least8_t>(wcstoul(matches[2u].first, nullptr, 10));
                deviceInfo.device = static_cast<uint_least8_t>(wcstoul(matches[3u].first, nullptr, 10));
            }
            else
            {
                std::wcout << "PCI location: [" << wstring(devProp, devPropLength / sizeof *devProp) << ']' << std::endl;
                throw runtime_error("Error listing display adapters: wrong PCI location property value.");
            }
        }
        else
            throw runtime_error("Error listing display adapters: wrong PCI instance ID property value");

        deviceSet.push_back(move(deviceInfo));
        devInfoData.Reserved = 0ul;
    }

    if (DWORD dwLastError = ::GetLastError(); dwLastError != ERROR_NO_MORE_ITEMS)
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), "Error listing display adapters");
}

vector<DeviceInfo> &getDeviceList()
{
    static vector<DeviceInfo> deviceSet;

    if (deviceSet.empty())
    {
        enumPciDisplayAdapters(deviceSet);
        fillDedicatedMemorySize(deviceSet);
    }

    return deviceSet;
}

#endif