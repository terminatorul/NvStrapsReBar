export module TextWizardMenu;

import std;
import LocalAppConfig;
import NvStrapsConfig;
import DeviceRegistry;
import DeviceList;

using std::tuple;
using std::span;
using std::optional;
using std::vector;

export enum class MenuCommand
{
    Quit,
    DiscardQuit,
    SaveConfiguration,
    DiscardConfiguration,
    ShowConfiguration,
    DiscardPrompt,
    GlobalEnable,
    GlobalFallbackEnable,
    SkipS3Resume,
    OverrideBarSizeMask,
    EnableSetupVarCRC,
    ClearSetupVarCRC,
    UEFIConfiguration,
    UEFIBARSizePrompt,
    PerGPUConfigClear,
    PerGPUConfig,
    GPUSelectorByPCIID,
    GPUSelectorByPCISubsystem,
    GPUSelectorByPCILocation,
    GPUVRAMSize,
    GPUSelectorClear,
    GPUSelectorExclude,

    DefaultChoice
};

export enum class MenuType
{
    Main,
    GPUConfig,
    GPUBARSize,
    PCIBARSize
};

export tuple<MenuCommand, unsigned> showMenuPrompt
    (
        span<MenuCommand>	  menu,
        optional<MenuCommand>	  defaultCommand,
        unsigned short		  defaultValue,
        bool			  isGlobalEnable,
        unsigned short		  device,
        vector<DeviceInfo> const &devices,
    NvStrapsConfig const	 &config
    );

export bool runConfirmationPrompt(MenuCommand menuCommand);

module: private;

using std::optional;
using std::nullopt;
using std::tuple;
using std::span;
using std::map;
using std::vector;
using std::max_element;
using std::locale;
using std::use_facet;
using std::ctype;
using std::toupper;
using std::wstring;
using std::wstring_view;
using std::to_wstring;
using std::wcout;
using std::wcin;
using std::getline;
using std::hex;
using std::dec;
using std::left;
using std::right;
using std::setw;
using std::setfill;
using std::find;
using std::get;
using std::find;
using std::views::all;

namespace execution = std::execution;
namespace views = std::ranges::views;
using namespace std::literals::string_view_literals;

static auto const mainMenuShortcuts = map<wchar_t, MenuCommand>
{
    { L'E', MenuCommand::GlobalEnable },
    { L'D', MenuCommand::GlobalEnable },
 // { L'G', MenuCommand::PerGPUConfig },
    { L'C', MenuCommand::PerGPUConfigClear },
    { L'K', MenuCommand::SkipS3Resume },
    { L'O', MenuCommand::OverrideBarSizeMask },
    { L'R', MenuCommand::EnableSetupVarCRC },
    { L'L', MenuCommand::ClearSetupVarCRC },
    { L'P', MenuCommand::UEFIConfiguration },
    { L'S', MenuCommand::SaveConfiguration },
    { L'W', MenuCommand::ShowConfiguration },
    { L'I', MenuCommand::DiscardConfiguration },
    { L'Q', MenuCommand::Quit }
};

static auto const gpuMenuShortcuts = map<wchar_t, MenuCommand>
{
    { L'P', MenuCommand::GPUSelectorByPCIID },
    { L'S', MenuCommand::GPUSelectorByPCISubsystem },
    { L'L', MenuCommand::GPUSelectorByPCILocation }
};

static auto const barSizeMenuShortcuts = map<wchar_t, MenuCommand>
{
    { L'C', MenuCommand::GPUSelectorClear },
    { L'X', MenuCommand::GPUSelectorExclude },
    { L'O', MenuCommand::OverrideBarSizeMask }
};

static wchar_t FindMenuShortcut(map<wchar_t, MenuCommand> const &menuShortcuts, MenuCommand menuCommand)
{
    auto it = find_if(menuShortcuts.cbegin(), menuShortcuts.cend(), [menuCommand](auto const &entry)
        {
            return entry.second == menuCommand;
        });

   return it == menuShortcuts.cend() ? L'\0' : it->first;
}

static constexpr wchar_t const WCHAR_T_HIGH_BIT_MASK = L'\001' << (sizeof(L'\0') * BYTE_BITSIZE - 1u);

static wstring showMainMenuEntry(MenuCommand menuCommand, bool isGlobalEnable, vector<DeviceInfo> const &devices, NvStrapsConfig const &config)
{
    auto chShortcut = FindMenuShortcut(mainMenuShortcuts, menuCommand);

    switch (menuCommand)
    {
    case MenuCommand::GlobalEnable:
        if (isGlobalEnable)
            wcout << L"\t(D) Disable auto-settings BAR size for known Turing GPUs (GTX 1600 / RTX 2000 line)\n"sv;
        else
            wcout << L"\t(E) Enable auto-setting BAR size for known Turing GPUs (GTX 1600 / RTX 2000 line)\n"sv;

        return wstring(1u, isGlobalEnable ? L'D' : L'E');

    case MenuCommand::SkipS3Resume:
    if (config.skipS3Resume())
        wcout << L"\t(" << chShortcut << L") Configure BAR size during resume from S3 (sleep)\n"sv;
    else
        wcout << L"\t(" << chShortcut << L") Skip BAR size configuration during resume from S3 (sleep)\n"sv;

    return wstring(1u, chShortcut);

    case MenuCommand::OverrideBarSizeMask:
    if (config.overrideBarSizeMask())
        wcout << L"\t(" << chShortcut << L") Disable"sv;
    else
        wcout << L"\t(" << chShortcut << L") Enable"sv;

    wcout << L" override for BAR size mask for PCI ReBAR capability\n"sv;

    return wstring(1u, chShortcut);

    case MenuCommand::EnableSetupVarCRC:
    if (config.enableSetupVarCRC())
        wcout << L"\t(" << chShortcut << L") Disable"sv;
    else
        wcout << L"\t(" << chShortcut << L") Enable"sv;

    wcout << L" Setup variable change detection using CRC check\n"sv;

    return wstring(1u, chShortcut);

    case MenuCommand::ClearSetupVarCRC:
    wcout << L"\t\t(" << chShortcut << L") Clear Setup variable CRC and re-compute it on next boot.\n"sv;

    return wstring(1u, chShortcut);

    case MenuCommand::PerGPUConfig:
        if (devices | all)
        {
            wstring commands(1u, L'G');
            wcout << L"\t    Manually configure BAR size for specific GPUs:\n"sv;

#if defined(__clang__)
        auto it = max_element(/* execution::par_unseq, */ devices.cbegin(), devices.cend(), [](auto const &left, auto const &right)
#else
        auto it = max_element(execution::par_unseq, devices.cbegin(), devices.cend(), [](auto const &left, auto const &right)
#endif
            {
            return left.productName.length() < right.productName.length();
            });

        auto maxNameLen = it == devices.cend() ? 0u : it->productName.length();

        for (auto const &&[index, device]: devices | views::enumerate)
            {
                wcout << L"\t\t("sv << index + 1u << L"). "sv << setw(maxNameLen) << setfill(L' ') << left << device.productName;

        auto [configPriority, barSizeSelector] = config.lookupBarSize
            (
            device.deviceID,
            device.subsystemVendorID,
            device.subsystemDeviceID,
            device.bus,
            device.device,
            device.function
            );
        auto gpuConfig = config.lookupGPUConfig(device.bus, device.device, device.function);
        auto barAddressRangeMismatch = !!configPriority && barSizeSelector < BarSizeSelector_Excluded
            && (!gpuConfig || !gpuConfig->bar0.base || gpuConfig->bar0.base != device.bar0.Base || gpuConfig->bar0.top != device.bar0.Top);

        if (device.bar0.Base != device.bar0.Top)
        {
            if (device.bar0.Base < DWORD_BITMASK && device.bar0.Top <= DWORD_BITMASK)
            {
            if (barAddressRangeMismatch)
                wcout << "       ! BAR0 at: 0x";
            else
                wcout << "         BAR0 at: 0x";

            wcout << hex << right << setfill(L'0');
            wcout << setw(DWORD_SIZE) << (device.bar0.Base >> WORD_BITSIZE & WORD_BITMASK);
            wcout << L'\'';
            wcout << setw(DWORD_SIZE) << (device.bar0.Base & WORD_BITMASK);
            }
            else
            {
            if (barAddressRangeMismatch)
                wcout << "       ! BAR0 64-bit: 0x";
            else
                wcout << "         BAR0 64-bit: 0x";

            wcout << hex << setfill(L'0') << right << setw(QWORD_SIZE * 2u) << device.bar0.Base;
            }

            wcout << dec << left << setfill(L' ') << ", size: " << formatMemorySize(device.bar0.Top - device.bar0.Base + 1u);
        }
        else
            if (barAddressRangeMismatch)
            wcout << "       ! BAR0";

        wcout << L'\n';
                commands.push_back(static_cast<wchar_t>((L'0' + index + 1u) | WCHAR_T_HIGH_BIT_MASK));
            }

            return commands;
        }
        else
            return { };

    case MenuCommand::PerGPUConfigClear:
        if (devices | all)
        {
            wcout << L"\t("sv << chShortcut << L") Clear per-GPU configuration\n"sv;
            return wstring(1u, chShortcut);
        }
        else
            return { };

    case MenuCommand::UEFIConfiguration:
        wcout << L"\t("sv << chShortcut << L") Select target PCI BAR size, for all (supported) PCI devices (for older boards without ReBAR).\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::ShowConfiguration:
    wcout << L"\t("sv << chShortcut << L") Show DXE driver configuration (for debugging).\n"sv;
    return wstring(1u, chShortcut);

    case MenuCommand::SaveConfiguration:
        wcout << L"\t("sv << chShortcut << L") Save configuration changes.\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::DiscardConfiguration:
        wcout << L"\t("sv << chShortcut << L") Discard configuration changes\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::Quit:
        wcout << L"\t("sv << chShortcut << L") Quit\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::DiscardQuit:
        chShortcut = FindMenuShortcut(mainMenuShortcuts, MenuCommand::Quit);
        wcout << L"\t("sv << chShortcut << L") Discard configuration changes and Quit\n"sv;
        return wstring(1u, chShortcut);
    }

    return { };
}

static wstring showUEFIReBarMenuEntry(MenuCommand menuCommand)
{
    switch (menuCommand)
    {
    case MenuCommand::UEFIBARSizePrompt:
        wcout << L"      0: System default (needs ReBAR enabled in UEFI setup)\n"sv;
        wcout << L"   1-31: Set maximum BAR size to 2^x MiB for all PCI devices:\n"sv;
        wcout << L"      1:   2 MiB\n"sv;
        wcout << L"   ...8: 256 MiB\n"sv;
        wcout << L"   ..10:   1 GiB\n"sv;
        wcout << L"     11:   2 GiB\n"sv;
        wcout << L"     12:   4 GiB\n"sv;
        wcout << L"     13:   8 GiB\n"sv;
        wcout << L"     14:  16 GiB\n"sv;
        wcout << L"     15:  32 GiB\n"sv;
        wcout << L"     16:  64 GiB\n"sv;
        wcout << L"  ...20:   1 TiB\n"sv;
        wcout << L"  ...31:   2 PiB\n"sv;
        wcout << L"     32: No BAR size limit for PCI devices\n\n"sv;
        wcout << L"     64: Configure BAR size for selected GPUs only\n\n"sv;
        wcout << L"  Enter: Leave unchanged\n"sv;

        return { };
    }

    return { };
}

static wstring showBarSizeMenuEntry(MenuCommand menuCommand, unsigned short device, vector<DeviceInfo> const &devices, NvStrapsConfig const &config)
{
    auto chShortcut = FindMenuShortcut(barSizeMenuShortcuts, menuCommand);

    switch (menuCommand)
    {
    case MenuCommand::GPUSelectorClear:
        wcout << L"\t("sv << chShortcut << L"): Clear GPU-specific configuration\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::GPUSelectorExclude:
        wcout << L"\t("sv << chShortcut << L"): Add exclusion for the GPU from auto-selected configuration.\n"sv;
        return wstring(1u, chShortcut);

    case MenuCommand::OverrideBarSizeMask:
    if (isTuringGPU(devices[device].deviceID))
    {
        auto &&deviceInfo = devices[device];

        if (config.lookupBarSizeMaskOverride(deviceInfo.deviceID, deviceInfo.subsystemVendorID, deviceInfo.subsystemDeviceID, deviceInfo.bus, deviceInfo.device, deviceInfo.function).sizeMaskOverride)
        wcout << L"\t("sv << chShortcut << L"): Disable"sv;
        else
        wcout << L"\t("sv << chShortcut << L"): Enable"sv;

        wcout << L" override for BAR size mask for PCIe ReBAR capability\n"sv;

        return wstring(1u, chShortcut);
    }

    case MenuCommand::GPUVRAMSize:
        wcout << L"\t 0):  64 MiB\n"sv;
        wcout << L"\t 1): 128 MiB\n"sv;
        wcout << L"\t 2): 256 MiB\n"sv;
        wcout << L"\t 3): 512 MiB\n"sv;
        wcout << L"\t 4):   1 GiB\n"sv;
        wcout << L"\t 5):   2 GiB\n"sv;
        wcout << L"\t 6):   4 GiB\n"sv;
        wcout << L"\t 7):   8 GiB\n"sv;
        wcout << L"\t 8):  16 GiB\n"sv;
        wcout << L"\t 9):  32 GiB\n"sv;
        wcout << L"\t10):  64 GiB\n"sv;
        wcout << L"    [Enter]: Leave unchanged\n"sv;

        {
            wstring commands(11u, L'\0');

            for (auto &&[index, ch]: commands | views::enumerate)
                ch = static_cast<wchar_t>(L'0' + index) | WCHAR_T_HIGH_BIT_MASK;

            return commands;
        }
    }

    return { };
}

static wstring showGPUConfigurationMenuEntry(MenuCommand menuCommand, unsigned short device, vector<DeviceInfo> const &devices)
{
    auto chShortcut = FindMenuShortcut(gpuMenuShortcuts, menuCommand);

    switch (menuCommand)
    {
    case MenuCommand::GPUSelectorByPCIID:
        wcout << L"\t("sv << chShortcut << L"): Select the GPU by PCI ID: "sv;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L" (default)\n";
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, chShortcut);

    case MenuCommand::GPUSelectorByPCISubsystem:
        wcout << L"\t("sv << chShortcut << L"): Select the GPU by PCI ID and Subsystem ID: "sv;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L", "sv;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemDeviceID << L'\n';
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, chShortcut);

    case MenuCommand::GPUSelectorByPCILocation:
        wcout << L"\t("sv << chShortcut << L"): Select the GPU by PCI ID, subystem and bus Location: ";
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L", "sv;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemDeviceID << L", "sv;
        wcout << right << hex << setw(BYTE_SIZE * 2u) << setfill(L'0') << devices[device].bus << L':' << hex << setw(BYTE_SIZE * 2u) << setfill(L'0') << devices[device].device;
        wcout << L'.' << hex << devices[device].function << L'\n';
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, chShortcut);
    }

    return { };
}

static wstring showMenuEntry
    (
    MenuType		  menuType,
    MenuCommand		  menuCommand,
    bool			  isGlobalEnable,
    unsigned short		  device,
    vector<DeviceInfo> const &devices,
    NvStrapsConfig const	 &config
    )
{
    switch (menuType)
    {
    case MenuType::Main:
        return showMainMenuEntry(menuCommand, isGlobalEnable, devices, config);

    case MenuType::GPUConfig:
        return showGPUConfigurationMenuEntry(menuCommand, device, devices);

    case MenuType::GPUBARSize:
        return showBarSizeMenuEntry(menuCommand, device, devices, config);

    case MenuType::PCIBARSize:
        return showUEFIReBarMenuEntry(menuCommand);
    }

    return { };
}

static bool shortcutMatchesDefaultCommand(wchar_t commandChar, MenuCommand defaultCommand, unsigned short defaultValue)
{
    auto it = mainMenuShortcuts.find(commandChar);

    return it != mainMenuShortcuts.cend() && it->second == defaultCommand;
}

static bool numericInputMatchesDefaultCommand(unsigned inputValue, MenuCommand defaultCommand, unsigned short defaultValue)
{
    switch (defaultCommand)
    {
    case MenuCommand::PerGPUConfig:
    case MenuCommand::GPUVRAMSize:
    case MenuCommand::UEFIBARSizePrompt:
        return inputValue == defaultValue;

    default:
        return false;
    }

    return false;
}

static wstring_view getPromptLine(MenuType menuType)
{
    switch (menuType)
    {
    case MenuType::Main:
        return L"Choose configuration command"sv;

    case MenuType::PCIBARSize:
        return L"Enter NvStrapsReBar Value"sv;

    case MenuType::GPUConfig:
        return L"Choose GPU selector"sv;

    case MenuType::GPUBARSize:
        return L"Chose option"sv;
    }

    return L"Input an option"sv;
}

static locale const &c_locale = locale::classic();

static bool isNumeric(wstring const &inputValue)
{
    auto &ctype_facet = use_facet<ctype<wchar_t>>(c_locale);
    auto dataStart = inputValue.data(), dataEnd = inputValue.data() + inputValue.size();

    if (ctype_facet.scan_not(ctype_facet.digit, dataStart, dataEnd) == dataEnd)
        return !!(inputValue | all);

    return false;
}

static bool hasShortcut(wchar_t input, wstring commands)
{
    return find(commands.cbegin(), commands.cend(), toupper(input, wcin.getloc())) != commands.cend();
}

static tuple<optional<MenuCommand>, unsigned> translateInput(MenuType menuType, wstring commands, wstring inputValue, std::vector<DeviceInfo> const& devices)
{
    switch (menuType)
    {
    case MenuType::PCIBARSize:
        if (isNumeric(inputValue) && (stoul(inputValue) <= TARGET_PCI_BAR_SIZE_MAX || stoul(inputValue) == TARGET_PCI_BAR_SIZE_GPU_ONLY))
            return { MenuCommand::UEFIBARSizePrompt, stoul(inputValue) };

        return { nullopt, 0u };

    case MenuType::GPUBARSize:
        if (isNumeric(inputValue))
            return { MenuCommand::GPUVRAMSize, stoul(inputValue) };

    if (inputValue.length() == 1u && hasShortcut(*inputValue.cbegin(), commands))
        if (auto it = barSizeMenuShortcuts.find(toupper(*inputValue.cbegin(), wcin.getloc())); it != barSizeMenuShortcuts.end())
            return { it->second, 0u };

        return { nullopt, 0u };

    case MenuType::GPUConfig:
        if (inputValue.length() == 1u && hasShortcut(*inputValue.cbegin(), commands))
            if (auto it = gpuMenuShortcuts.find(toupper(*inputValue.cbegin(), wcin.getloc())); it != gpuMenuShortcuts.end())
                return { it->second, 0u };

        return { nullopt, 0u };

    case MenuType::Main:
        if (isNumeric(inputValue) && hasShortcut(L'G', commands) && [&devices](auto val) { return 1u <= val && val <= devices.size(); }(stoul(inputValue)))
            return { MenuCommand::PerGPUConfig, stoul(inputValue) - 1u };

        if (inputValue.length() == 1u && hasShortcut(*inputValue.cbegin(), commands) && toupper(*inputValue.cbegin(), wcin.getloc()) != L'G')         // L'G' - per-GPU configuration, GPU index used instead
            if (auto it = mainMenuShortcuts.find(toupper(*inputValue.cbegin(), wcin.getloc())); it != mainMenuShortcuts.end())
                return { it->second, 0u };

        return { nullopt, 0u };
    }

    return { nullopt, 0u };
}

static tuple<optional<MenuCommand>, unsigned> runInputPrompt(MenuType menuType, wstring const &commands, optional<MenuCommand> defaultCommand, unsigned short defaultValue, std::vector<DeviceInfo> const& devices)
{
    auto promptLine = getPromptLine(menuType);

    wcout << promptLine << L" ("sv;

    for (auto [index, commandChar]: commands | views::enumerate)
        if (menuType != MenuType::Main || commandChar != L'G')      // L'G' - per-GPU configuration, GPU index used instead)
        {
            if (index)
                wcout << L", "sv;

            if (commandChar & WCHAR_T_HIGH_BIT_MASK)
            {
                wchar_t ch = commandChar & ~WCHAR_T_HIGH_BIT_MASK;
                ch -= L'0';

                if (defaultCommand && numericInputMatchesDefaultCommand(unsigned { ch }, *defaultCommand, defaultValue))
                    wcout << L'[' << to_wstring(unsigned { ch }) << L']';
                else
                    wcout << to_wstring(unsigned { ch });
            }
            else
                if (defaultCommand && shortcutMatchesDefaultCommand(commandChar, *defaultCommand, defaultValue))
                    wcout << L'[' << commandChar << L']';
                else
                    wcout << commandChar;
        }

    wcout << L"): "sv;

    auto inputValue = wstring { };
    getline(wcin, inputValue);

    while (inputValue | all && std::isspace(*inputValue.cbegin()))
        inputValue.erase(inputValue.cbegin());

    while (inputValue | all && std::isspace(*inputValue.crbegin()))
            inputValue.pop_back();

    if (inputValue.empty())
        return tuple { defaultCommand, defaultValue };

    return translateInput(menuType, commands, inputValue, devices);
}

static tuple<MenuCommand, unsigned> showMenu
    (
        MenuType		       menuType,
        std::span<MenuCommand>	       menu,
        optional<MenuCommand>	       defaultCommand,
        unsigned short		       defaultValue,
        bool			       isGlobalEnable,
        unsigned short		       device,
        std::vector<DeviceInfo> const &devices,
    NvStrapsConfig const	      &config
    )
{
    auto commands = wstring { };

    for (auto menuCommand: menu)
        commands += showMenuEntry(menuType, menuCommand, isGlobalEnable, device, devices, config);

    wcout << L'\n';

    tuple<optional<MenuCommand>, unsigned> responseCmd;

    do
    {
        responseCmd = runInputPrompt(menuType, commands, defaultCommand, defaultValue, devices);
    }
    while (!get<0u>(responseCmd));

    return { *get<0u>(responseCmd), get<1u>(responseCmd) };
}

static MenuType getMenuType(span<MenuCommand> menu)
{
    if (find(execution::par_unseq, menu.begin(), menu.end(), MenuCommand::GPUVRAMSize) != menu.end())
        return MenuType::GPUBARSize;

    if (find(execution::par_unseq, menu.begin(), menu.end(), MenuCommand::UEFIBARSizePrompt) != menu.end())
        return MenuType::PCIBARSize;

    if (!menu.empty() && *menu.rbegin() != MenuCommand::Quit && *menu.rbegin() != MenuCommand::DiscardQuit)
        return MenuType::GPUConfig;

    return MenuType::Main;
}

static void showMenuHeader(MenuType menuType, unsigned device, vector<DeviceInfo> const &devices)
{
    switch (menuType)
    {
    case MenuType::Main:
    wcout << '\n';
        wcout << L"BAR size configuration menu:\n"sv;
        break;

    case MenuType::PCIBARSize:
        wcout << L"\nFirst verify Above 4G Decoding is enabled and CSM is disabled in UEFI setup, otherwise system will not POST with GPU.\n"sv;
        wcout << L"If your NvStrapsReBar value keeps getting reset, then check your system time.\n"sv;

        wcout << L"\nIt is recommended to first try smaller sizes above 256 MiB in case an old BIOS doesn't support large BARs.\n"sv;
        wcout << L"\nSelect target BAR size for supported PCI devices:\n"sv;
        break;

    case MenuType::GPUConfig:
        wcout << L"Configure "sv << devices[device].productName << L":\n"sv;
        break;

    case MenuType::GPUBARSize:
        wcout << L"Input GPU BAR size:\n"sv;
        break;
    }
}

tuple<MenuCommand, unsigned> showMenuPrompt
    (
        span<MenuCommand>	  menu,
        optional<MenuCommand>	  defaultCommand,
        unsigned short		  defaultValue,
        bool			  isGlobalEnable,
        unsigned short		  device,
        vector<DeviceInfo> const &devices,
    NvStrapsConfig const	 &config
    )
{
    auto menuType = getMenuType(menu);

    showMenuHeader(menuType, device, devices);
    return showMenu(menuType, menu, defaultCommand, defaultValue, isGlobalEnable, device, devices, config);
}

bool runConfirmationPrompt(MenuCommand menuCommand)
{
    wstring input;

    switch (menuCommand)
    {
    case MenuCommand::SkipS3Resume:
    wcout << L"WARNING: Only skip S3 Resume if the system does not support S3 sleep or the DXE driver status shows an S3 EFI error !\n"sv;
    wcout << L"Skip BAR size configuration during S3 Resume ? (y/N) "sv;
    break;

    case MenuCommand::DiscardQuit:
    wcout << L"Quit without saving ? (y/N) "sv;
    break;

    case MenuCommand::EnableSetupVarCRC:
    wcout << L"WARNING: Manually disable ReBAR before changing settings in UEFI Setup and before making hardware changes !\n"sv;
    wcout << L"Disable automatic Setup variable change detection ? (y/N) "sv;
    break;

    default:
    wcout << L"Confirmation to continue (y/N) "sv;
    break;
    }

    getline(wcin, input);

    while (input | all && isspace(*input.begin()))
    input.erase(input.begin());

    while (input | all && isspace(*input.rbegin()))
    input.resize(input.size() - 1u);

    for (auto &ch: input)
    ch = toupper(ch, wcin.getloc());

    return input | all && L"YES"sv.starts_with(input);
}

// vim:ft=cpp
