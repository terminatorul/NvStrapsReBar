export module TextWizardPage;

import std;
import LocalAppConfig;
import NvStrapsConfig;
import StatusVar;
import DeviceRegistry;
import DeviceList;

using std::uint_least64_t;
using std::string;
using std::wstring;
using std::vector;
using std::wcout;
using std::wcerr;
using std::cerr;
using namespace std::literals::string_view_literals;

export void showInfo(wstring const &message);
export void showError(wstring const &message);
export void showError(string const &message);
export void showStartupLogo();

export void showConfiguration(vector<DeviceInfo> const &devices, NvStrapsConfig const &nvStrapsConfig, uint_least64_t driverStatus);

inline void showInfo(wstring const &message)
{
    wcout << message;
}

inline void showError(wstring const &message)
{
    wcerr << L'\n' << message;
}

inline void showError(string const &message)
{
    cerr << '\n' << message;
}

inline void showStartupLogo()
{
    wcout << L"NvStrapsReBar, based on:\nReBarState (c) 2023 xCuri0\n\n"sv;
}

module: private;

using std::uint_least8_t;
using std::uint_least64_t;
using std::string;
using std::wstring_view;
using std::to_wstring;
using std::vector;
using std::wcout;
using std::wostringstream;
using std::hex;
using std::dec;
using std::left;
using std::right;
using std::uppercase;
using std::setw;
using std::setfill;
using std::max;

namespace views = std::ranges::views;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

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
    str << setw(2u) << devInfo.bridge.bus << L':' << setw(2u) << devInfo.bridge.dev << L'.' << devInfo.bridge.func;
    str << L' ';
    str << setw(2u) << devInfo.bus << L':' << setw(2u) << devInfo.device << L'.' << devInfo.function;


    return str.str();
}

static wstring formatDirectBARSize(uint_least64_t size)
{
    if (size < uint_least64_t { 64u } * 1024u * 1024u)
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

static wchar_t locationMarker(ConfigPriority location, ConfigPriority barSizePriority, ConfigPriority sizeMaskOverridePriority, bool bridgeMismatch)
{
    if (bridgeMismatch && location == ConfigPriority::EXPLICIT_PCI_LOCATION)
    return L'!';

    if (barSizePriority >= location)
    if (sizeMaskOverridePriority >= location)
        return L'+';
    else
        return L'*';
    else
    if (sizeMaskOverridePriority >= location)
        return L'\'';
    else
        return L' ';
}

static void showLocalGPUs(vector<DeviceInfo> const &deviceSet, NvStrapsConfig const &nvStrapsConfig)
{
#if defined(NDEBUG)
    if (deviceSet.empty())
    {
        cerr << "No NVIDIA GPUs present!\n";
        return;
    }
#endif

    auto
        nMaxLocationSize = max("Bridge + GPU"sv.size(), "bus:dev.fn"sv.size()),
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

    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+--"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-') << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n"sv;
    wcout << L"| Nr |   PCI ID   |  subsystem |  "sv << left << setw(nMaxLocationSize) << L"Bridge + GPU"sv << L" |  "sv << left << setw(nMaxTargetBarSize) << " Target " << L" | "sv << setw(nMaxTargetBarSize) << left << "Current" << L" | "sv << setw(nMaxVRAMSize) << L"VRAM"sv << L" | "sv << setw(nMaxNameSize) << L"Product Name"sv << L" |\n"sv;
    wcout << L"|    |  VID:DID   |   VID:DID  |  "sv << setw(nMaxLocationSize) << left << "bus:dev.fn" << L" |  "sv << setw(nMaxTargetBarSize) << L"BAR size"sv << L" | "sv << setw(nMaxCurrentBarSize) << L"BAR size"sv << L" | "sv << setw(nMaxVRAMSize) << L"size"sv << L" | "sv << wstring(nMaxNameSize, L' ') << L" |\n"sv;
    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+--"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-') << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n"sv;

    for (auto const &&[deviceIndex, deviceInfo]: deviceSet | views::enumerate)
    {
    auto bridgeInfo = nvStrapsConfig.lookupBridgeConfig(deviceInfo.bus);

        auto [configPriority, barSizeSelector] = nvStrapsConfig.lookupBarSize
            (
                deviceInfo.deviceID,
                deviceInfo.subsystemVendorID,
                deviceInfo.subsystemDeviceID,
                deviceInfo.bus,
                deviceInfo.device,
                deviceInfo.function
            );

    auto [sizeMaskOverridePriority, sizeMaskOverride] = nvStrapsConfig.lookupBarSizeMaskOverride
        (
                deviceInfo.deviceID,
                deviceInfo.subsystemVendorID,
                deviceInfo.subsystemDeviceID,
                deviceInfo.bus,
                deviceInfo.device,
                deviceInfo.function
            );

    auto bridgeMismatch = bool
    {
        !!configPriority && barSizeSelector < BarSizeSelector_Excluded
        &&
        (
           !bridgeInfo
        || !bridgeInfo->deviceMatch(deviceInfo.bridge.vendorID, deviceInfo.bridge.deviceID)
        || !bridgeInfo->busLocationMatch(deviceInfo.bridge.bus, deviceInfo.bridge.dev, deviceInfo.bridge.func)
        )
    };

    wchar_t marker;

        // GPU number
        wcout << L"| "sv << dec << right << setw(2u) << setfill(L' ') << deviceIndex + 1u;

        // PCI ID
        wcout << L" | "sv << locationMarker(ConfigPriority::EXPLICIT_PCI_ID, configPriority, sizeMaskOverridePriority, bridgeMismatch) << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << deviceInfo.deviceID;

        // PCI subsystem ID
        wcout << L" | "sv << locationMarker(ConfigPriority::EXPLICIT_SUBSYSTEM_ID, configPriority, sizeMaskOverridePriority, bridgeMismatch) << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemDeviceID;

        // PCI bus location
        wcout << L" | "sv << locationMarker(ConfigPriority::EXPLICIT_PCI_LOCATION, configPriority, sizeMaskOverridePriority, bridgeMismatch) << right << setw(nMaxLocationSize) << setfill(L' ') << left << formatLocation(deviceInfo);

        // Target BAR1 size
        wcout << L" | "sv << (sizeMaskOverride ? isTuringGPU(deviceInfo.deviceID) ? L'\'' : L' ' : L' ') << dec << setw(nMaxTargetBarSize) << right << setfill(L' ') << formatBarSizeSelector(barSizeSelector);

        // Current BAR size
        wcout << L" | "sv << dec << setw(nMaxCurrentBarSize) << right << setfill(L' ') << formatDirectBARSize(deviceInfo.currentBARSize);

        // VRAM capacity
        wcout << L" | "sv << dec << setw(nMaxVRAMSize) << right << setfill(L' ') << formatDirectMemorySize(deviceInfo.dedicatedVideoMemory);

        // GPU model name
        wcout << L" | "sv << left << setw(nMaxNameSize) << deviceInfo.productName;

        wcout << L" |\n"sv;
    }

    wcout << L"+----+------------+------------+--"sv << wstring(nMaxLocationSize, L'-') << L"-+--"sv << wstring(nMaxTargetBarSize, L'-') << L"-+-"sv << wstring(nMaxCurrentBarSize, L'-')  << L"-+-"sv << wstring(nMaxVRAMSize, L'-') << L"-+-"sv << wstring(nMaxNameSize, L'-') << L"-+\n\n"sv;
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

    case StatusVar_GpuReBarSizeOverride:
    return L"GPU-side ReABR Configured with PCI ReBAR size override"sv;

    case StatusVar_GpuNoReBarCapability:
        return L"ReBAR capability not advertised"sv;

    case StatusVar_GpuExcluded:
        return L"GPU excluded"sv;

    case StatusVar_NoBridgeConfig:
    return L"Missing bridge configuration"sv;

    case StatusVar_BadBridgeConfig:
    return L"Bad PCI Bridge Configuration"sv;

    case StatusVar_BridgeNotEnumerated:
    return L"GPU enumerated before bridge"sv;

    case StatusVar_BadGpuConfig:
    return L"Improper GPU BAR configuration"sv;

    case StatusVar_BadSetupVarAttributes:
    return L"Bad attributes for Setup variable"sv;

    case StatusVar_AmbiguousSetupVariable:
    return L"Ambiguous Setup variable"sv;

    case StatusVar_MissingSetupVariable:
    return L"Setup variable missing"sv;

    case StatusVar_NoGpuConfig:
    return L"Missing GPU BAR0 Configuration"sv;

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

    case EFIError_EnumVar:
    return L" (at Enumerate EFI variables)"sv;

    case EFIError_ReadSetupVar:
    return L" (at Read Setup variable)"sv;

    case EFIError_ReadSetupVarSize:
    return L" (at Read Setup variable size)"sv;

    case EFIError_AllocateSetupVarName:
    return L" (at Allocate Setup variable name)"sv;

    case EFIError_AllocateSetupVarData:
    return L" (at Allocate Setup variable data)"sv;

    case EFIError_WriteConfigVar:
        return L" (at Write config var)"sv;

    case EFIError_PCI_StartFindCap:
        return L" (at start PCI find capability)"sv;

    case EFIError_PCI_FindCap:
        return L" (at PCI find capability)"sv;

    case EFIError_PCI_BridgeSecondaryBus:
    return L" (at Secondary Bus read)"sv;

    case EFIError_PCI_BridgeConfig:
        return L" (at PCI bridge configuration)"sv;

    case EFIError_PCI_BridgeRestore:
        return L" (at PCI bridge restore)"sv;

    case EFIError_PCI_DeviceBARConfig:
        return L" (at PCI device BAR config)"sv;

    case EFIError_PCI_DeviceBARRestore:
        return L" (at PCI device BAR restore)"sv;

    case EFIError_PCI_DeviceSubsystem:
        return L" (at PCI read device subsystem"sv;

    case EFIError_LocateBridgeProtocol:
        return L" (at Locate bridge protocol)"sv;

    case EFIError_LoadBridgeProtocol:
        return L" (at Load bridge protocol)"sv;

    case EFIError_LocateS3SaveStateProtocol:
    return L" (at Locate S3 Save State Protocol)"sv;

    case EFIError_LoadS3SaveStateProtocol:
    return L" (at Load S3 Save State Protocol)"sv;

    case EFIError_ReadBaseAddress0:
    return L" (at read base address 0)"sv;

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

    case EFIError_CreateEvent:
    return L" (at Create Event BeforeExitBootServices)"sv;

    case EFIError_CloseEvent:
    return L" (at Close Event BeforeExitBootServices)"sv;

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

    if (status == StatusVar_GpuStrapsNoConfirm)
    wcout << L"(use Overide BAR Size Mask option)\n"sv;
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

// vi: ft=cpp
