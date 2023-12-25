#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ranges>

#include "StatusVar.h"
#include "NvStrapsConfig.h"
#include "TextWizardPage.hh"

#undef min
#undef max

using std::uint_least8_t;
using std::uint_least64_t;
using std::optional;
using std::string;
using std::wstring;
using std::wstring_view;
using std::tuple;
using std::span;
using std::tuple;
using std::vector;
using std::cerr;
using std::wcout;
using std::wcerr;
using std::wcin;
using std::wostringstream;
using std::getline;
using std::hex;
using std::dec;
using std::left;
using std::right;
using std::uppercase;
using std::setw;
using std::setfill;
using std::min;
using std::max;
using std::get;
using std::find;
using std::views::all;

namespace views = std::ranges::views;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

void showInfo(wstring const &message)
{
    wcout << message;
}

void showError(wstring const &message)
{
    wcerr << L'\n' << message;
}

void showError(string const &message)
{
    cerr << '\n' << message;
}

void showStartupLogo()
{
    wcout << L"NvStrapsReBar, based on ReBarState (c) 2023 xCuri0\n\n"sv;
}

static wstring formatDirectMemorySize(uint_least64_t memorySize)
{
    if (!memorySize)
        return L"    "s;

    return formatMemorySize(memorySize);
}

static wstring formatLocation(DeviceInfo const &devInfo)
{
    wostringstream str;

    str << hex << uppercase << setfill(L'0') << right;
    str << setw(2u) << devInfo.bus << L':' << setw(2u) << devInfo.device << L'.' << devInfo.function;

    return str.str();
}

static wstring formatDirectBARSize(uint_least64_t size)
{
    if (size < UINT64_C(64) * 1024u * 1024u)
        return { };

    return formatMemorySize(size);
}

static wstring_view formatBarSizeSelector(uint_least8_t barSizeSelector)
{
    switch (barSizeSelector)
    {
    case 0:
        return L"64 MB"sv;
    case 1:
        return L"128 MB"sv;
    case 2:
        return L"256 MB"sv;
    case 3:
        return L"512 MB"sv;
    case 4:
        return L"1 GB"sv;
    case 5:
        return L"2 GB"sv;
    case 6:
        return L"4 GB"sv;
    case 7:
        return L"8 GB"sv;
    case 8:
        return L"16 GB"sv;
    case 9:
        return L"32 GB"sv;
    case 10:
        return L"64 GB"sv;
    default:
        return L" "sv;
    }

    return L" "sv;
}

static void showLocalGPUs(vector<DeviceInfo> const &deviceSet, NvStrapsConfig const &nvStrapsConfig)
{
    if (deviceSet.empty())
    {
        cerr << "No NVIDIA GPUS present!\n";
        return;
    }

    auto
        nMaxLocationSize = max("PCI location"sv.size(), "bus:dev.fn"sv.size()),
        nMaxTargetBarSize = max("Target"sv.size(), "BAR size"sv.size()),
        nMaxCurrentBarSize = max("current"sv.size(), "BAR size"sv.size()),
        nMaxVRAMSize = "VRAM"sv.size(),
        nMaxNameSize = "Product Name"sv.size();

    for (auto const &deviceInfo: deviceSet)
    {
        nMaxLocationSize = max(nMaxLocationSize, formatLocation(deviceInfo).size());
        nMaxCurrentBarSize = max<uint_least64_t>(nMaxCurrentBarSize, formatMemorySize(deviceInfo.currentBARSize).size());
        nMaxTargetBarSize = max(nMaxTargetBarSize, formatBarSizeSelector(MAX_BAR_SIZE_SELECTOR).size());
        nMaxVRAMSize = max(nMaxVRAMSize, formatDirectMemorySize(deviceInfo.dedicatedVideoMemory).size());
        nMaxNameSize = max(nMaxNameSize, deviceInfo.productName.size());
    }

    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+-"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-') << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n"sv;
    wcout << L"| Nr |   PCI ID   |  subsystem |  "sv << left << setw(nMaxLocationSize) << L"PCI location"sv << L" | "sv << left << setw(nMaxTargetBarSize) << " Target " << L" | "sv << setw(nMaxTargetBarSize) << left << "Current" << L" | "sv << setw(nMaxVRAMSize) << L"VRAM"sv << L" | "sv << setw(nMaxNameSize) << L"Product Name"sv << L" |\n"sv;
    wcout << L"|    |  VID:DID   |   VID:DID  |  "sv << setw(nMaxLocationSize) << left << "bus:dev.fn" << L" | "sv << setw(nMaxTargetBarSize) << L"BAR size"sv << L" | "sv << setw(nMaxCurrentBarSize) << L"BAR size"sv << L" | "sv << setw(nMaxVRAMSize) << L"size"sv << L" | "sv << wstring(nMaxNameSize, L' ') << L" |\n"sv;
    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+-"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-') << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n"sv;

    unsigned i = 1u;

    for (auto const &deviceInfo: deviceSet)
    {
        auto [configPriority, barSizeSelector] = nvStrapsConfig.lookupBarSize
            (
                deviceInfo.deviceID,
                deviceInfo.subsystemVendorID,
                deviceInfo.subsystemDeviceID,
                deviceInfo.bus,
                deviceInfo.device,
                deviceInfo.function
            );

        // GPU number
        wcout << L"| "sv << dec << right << setw(2u) << setfill(L' ') << i++;

        // PCI ID
        wcout << L" | "sv << (configPriority < ConfigPriority::EXPLICIT_PCI_ID ? L' ' : L'*') << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << deviceInfo.deviceID;

        // PCI subsystem ID
        wcout << L" | "sv << (configPriority < ConfigPriority::EXPLICIT_SUBSYSTEM_ID ? L' ' : L'*') << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemDeviceID;

        // PCI bus location
        wcout << L" | "sv << (configPriority < ConfigPriority::EXPLICIT_PCI_LOCATION ? L' ' : L'*') << right << setw(nMaxLocationSize) << setfill(L' ') << left << formatLocation(deviceInfo);

        // Target BAR1 size
        wcout << L" | "sv << dec << setw(nMaxTargetBarSize) << right << setfill(L' ') << formatBarSizeSelector(barSizeSelector);

        // Current BAR size
        wcout << L" | "sv << dec << setw(nMaxCurrentBarSize) << right << setfill(L' ') << formatDirectBARSize(deviceInfo.currentBARSize);

        // VRAM capacity
        wcout << L" | "sv << dec << setw(nMaxVRAMSize) << right << setfill(L' ') << formatDirectMemorySize(deviceInfo.dedicatedVideoMemory);

        // GPU model name
        wcout << L" | "sv << left << setw(nMaxNameSize) << deviceInfo.productName;

        wcout << L" |\n"sv;
    }

    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+-"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-')  << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n\n"sv;
}

static wstring_view statusString(uint_least64_t driverStatus)
{
    switch (driverStatus)
    {
    case StatusVar_NotLoaded:
        return L"Not loaded"sv;

    case StatusVar_Unconfigured:
        return L"Unconfigured"sv;

    case StatusVar_GPU_Unconfigured:
        return L"GPU Unconfigured"sv;

    case StatusVar_Cleared:
        return L"Cleared"sv;

    case StatusVar_Configured:
        return L"Configured"sv;

    case StatusVar_BridgeFound:
        return L"Bridge Found"sv;

    case StatusVar_GpuFound:
        return L"GPU Found"sv;

    case StatusVar_GpuStrapsConfigured:
        return L"GPU STRAPS Configured"sv;

    case StatusVar_GpuDelayElapsed:
        return L"GPU PCI delay posted"sv;

    case StatusVar_GpuReBarConfigured:
        return L"GPU ReBAR Configured"sv;

    case StatusVar_GpuNoReBarCapability:
        return L"ReBAR not advertised"sv;

    case StatusVar_EFIAllocationError:
        return L"Allocation error"sv;

    case StatusVar_EFIError:
        return L"EFI Error"sv;

    case StatusVar_NVAR_API_Error:
        return L"NVAR access API error"sv;

    case StatusVar_ParseError:
    default:
        return L"Parse error"sv;
    }

    return L"Parse error"sv;
}

static void showDriverStatus(uint_least64_t driverStatus)
{
    wcout << L"EFI DXE driver status: "sv << statusString(driverStatus & 0xFFFFFFFFuL) << L" (0x" << hex << right << setfill(L'0') << setw(QWORD_SIZE * 2u) << driverStatus << dec << setfill(L' ') << L")\n";
}

static void showMotherboardReBarState(optional<uint_least8_t> reBarState)
{
    if (reBarState)
    {
        if (reBarState == 0)
                wcout << L"Current NvStrapsReBar "sv << +*reBarState << L" / Disabled\n"sv;
        else
                if (reBarState == 32)
                        wcout << L"Current NvStrapsReBar "sv << +*reBarState << L" / Unlimited\n"sv;
                else
                        wcout << L"Current NvStrapsReBar "sv << +*reBarState << L" / "sv << pow(2, *reBarState) << L" MB\n"sv;
        }
    else
        wcout << L"NvStrapsReBar variable doesn't exist" L" / Disabled. Enter a value to create it\n"sv;
}

#if !defined(NDEBUG)
static
#endif
void showMotherboardReBarMenu()
{
    wcout << L"\nFirst verify Above 4G Decoding is enabled and CSM is disabled in UEFI setup, otherwise system will not POST with GPU.\n"sv;
    wcout << L"If your NvStrapsReBar value keeps getting reset then check your system time.\n"sv;

    wcout << L"\nIt is recommended to first try smaller sizes above 256MB in case an old BIOS doesn't support large BARs.\n"sv;
    wcout << L"\nEnter NvStrapsReBar Value\n"sv;
    wcout << L"      0: Disabled \n"sv;
    wcout << L"Above 0: Maximum BAR size set to 2^x MB \n"sv;
    wcout << L"     32: Unlimited BAR size\n\n"sv;
    wcout << L"     64: Enable ReBAR for selected GPUs only\n"sv;
    wcout << L"     65: Configure ReBAR STRAPS bits only on the GPUs\n\n"sv;
    wcout << L"  Enter: Leave unchanged\n"sv;
}

void showConfiguration(vector<DeviceInfo> const &devices, NvStrapsConfig const &nvStrapsConfig, optional<uint_least8_t> reBarState, uint_least64_t driverStatus)
{
    showLocalGPUs(devices, nvStrapsConfig);
    showDriverStatus(driverStatus);
    showMotherboardReBarState(reBarState);
}
