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
using std::to_wstring;
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
    wcout << L"NvStrapsReBar, based on:\nReBarState (c) 2023 xCuri0\n\n"sv;
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
        return L"64 MiB"sv;
    case 1:
        return L"128 MiB"sv;
    case 2:
        return L"256 MiB"sv;
    case 3:
        return L"512 MiB"sv;
    case 4:
        return L"1 GiB"sv;
    case 5:
        return L"2 GiB"sv;
    case 6:
        return L"4 GiB"sv;
    case 7:
        return L"8 GiB"sv;
    case 8:
        return L"16 GiB"sv;
    case 9:
        return L"32 GiB"sv;
    case 10:
        return L"64 GiB"sv;
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

static wstring_view driverStatusString(uint_least64_t driverStatus)
{
    switch (driverStatus)
    {
    case StatusVar_NotLoaded:
        return L"Not loaded"sv;

    case StatusVar_Configured:
        return L"Configured"sv;

    case StatusVar_GPU_Unconfigured:
        return L"GPU Unconfigured"sv;

    case StatusVar_Unconfigured:
        return L"Unconfigured"sv;

    case StatusVar_Cleared:
        return L"Cleared"sv;

    case StatusVar_BridgeFound:
        return L"Bridge Found"sv;

    case StatusVar_GpuFound:
        return L"GPU Found"sv;

    case StatusVar_GpuStrapsConfigured:
        return L"GPU-side ReBAR Configured"sv;

    case StatusVar_GpuStrapsPreConfigured:
        return L"GPU side Already Configured"sv;

    case StatusVar_GpuStrapsConfirm:
        return L"GPU-side ReBAR Configured with PCI confirm"sv;

    case StatusVar_GpuDelayElapsed:
        return L"GPU PCI delay posted"sv;

    case StatusVar_GpuReBarConfigured:
        return L"GPU PCI ReBAR Configured"sv;

    case StatusVar_GpuStrapsNoConfirm:
        return L"GPU-side ReBAR Configured without PCI confirm"sv;

    case StatusVar_GpuNoReBarCapability:
        return L"ReBAR capability not advertised"sv;

    case StatusVar_GpuExcluded:
        return L"GPU excluded"sv;

    case StatusVar_EFIAllocationError:
        return L"EFI Allocation error"sv;

    case StatusVar_Internal_EFIError:
        return L"EFI Error"sv;

    case StatusVar_NVAR_API_Error:
        return L"NVAR access API error"sv;

    case StatusVar_ParseError:
    default:
        return L"Parse error"sv;
    }

    return L"Parse error"sv;
}

wstring_view driverErrorString(EFIErrorLocation errLocation)
{
    switch (errLocation)
    {
    case EFIError_None:
        return L""sv;

    case EFIError_ReadConfigVar:
        return L" (at Read config var)"sv;

    case EFIError_WriteConfigVar:
        return L" (at Write config var)"sv;

    case EFIError_PCI_StartFindCap:
        return L" (at start PCI find capability)"sv;

    case EFIError_PCI_FindCap:
        return L" (at PCI find capability)"sv;

    case EFIError_PCI_BridgeConfig:
        return L" (at PCI bridge configuration)"sv;

    case EFIError_PCI_BridgeRestore:
        return L" (at PCI bridge restore)"sv;

    case EFIError_PCI_DeviceBARConfig:
        return L" (at PCI device BAR config)"sv;

    case EFIError_PCI_DeviceBARRestore:
        return L" (at PCI device BAR restor)"sv;

    case EFIError_PCI_DeviceSubsystem:
        return L" (at PCI read device subsystem"sv;

    case EFIError_LocateBridgeProtocol:
        return L" (at Locate bridge protocol)"sv;

    case EFIError_LoadBridgeProtocol:
        return L" (at Load bridge protocol)"sv;

    case EFIError_CMOSTime:
        return L" (at CMOS Time)"sv;

    case EFIError_CreateTimer:
        return L" (at Create Timer)"sv;

    case EFIError_CloseTimer:
        return L" (at Close Timer)"sv;

    case EFIError_SetupTimer:
        return L" (at Setup Timer)"sv;

    case EFIError_WaitTimer:
        return L" (at Wait Timer)"sv;

    default:
        return L""sv;
    }

    return L""sv;
}

static void showDriverStatus(uint_least64_t driverStatus)
{
    uint_least32_t status = driverStatus & DWORD_BITMASK;

    wcout << L"UEFI DXE driver status: "sv << driverStatusString(status)
        << (status == StatusVar_Internal_EFIError ? driverErrorString(static_cast<EFIErrorLocation>(driverStatus >> (DWORD_BITSIZE + BYTE_BITSIZE) & BYTE_BITMASK)) : L""sv)
        <<  L" (0x"sv << hex << right << setfill(L'0') << setw(QWORD_SIZE * 2u) << driverStatus << dec << setfill(L' ') << L")\n"sv;
}

static wstring formatPciBarSize(unsigned sizeSelector)
{
    auto suffix = sizeSelector < 10u ? L" MiB"s : sizeSelector < 20u ? L" GiB"s : sizeSelector < 30u ? L" TiB"s : L" PiB"s;

    return to_wstring(1u << sizeSelector % 10) + suffix;
}

static void showPciReBarState(uint_least8_t reBarState)
{
    switch (reBarState)
    {
    case TARGET_PCI_BAR_SIZE_DISABLED:
        wcout << L"Target PCI BAR size: "sv << +reBarState << L" / System default\n"sv;
        break;
    case TARGET_PCI_BAR_SIZE_MAX:
        wcout << L"Target PCI BAR size: "sv << +reBarState << L" / Any BAR size supported by PCI devices.\n"sv;
        break;
    case TARGET_PCI_BAR_SIZE_GPU_ONLY:
        wcout << L"Target PCI BAR size: "sv << +reBarState << L" / Selected GPUs only\n"sv;
        break;
    case TARGET_PCI_BAR_SIZE_GPU_STRAPS_ONLY:
        wcout << L"Target PCI BAR size: "sv << +reBarState << L" / GPU-side only for selected GPUs, without PCI BAR configuration\n"sv;
        break;
    default:
        if (TARGET_PCI_BAR_SIZE_MIN <= reBarState && reBarState < TARGET_PCI_BAR_SIZE_MAX)
            wcout << L"Target PCI BAR size: "sv << +reBarState << L" / Maximum "sv << formatPciBarSize(reBarState) << L" BAR size for PCI devices\n"sv;
        else
            wcout << L"Target PCI BAR size value not unsupported\n"sv;
        break;
    }
}

void showConfiguration(vector<DeviceInfo> const &devices, NvStrapsConfig const &nvStrapsConfig, uint_least64_t driverStatus)
{
    showLocalGPUs(devices, nvStrapsConfig);
    showDriverStatus(driverStatus);
    showPciReBarState(nvStrapsConfig.targetPciBarSizeSelector());
}
