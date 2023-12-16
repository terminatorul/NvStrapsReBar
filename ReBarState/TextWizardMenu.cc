#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cwctype>
#include <optional>
#include <tuple>
#include <initializer_list>
#include <span>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <execution>
#include <ranges>

#include "NvStrapsPciConfig.hh"
#include "ReBarState.hh"
#include "TextWizardMenu.hh"

using std::optional;
using std::nullopt;
using std::tuple;
using std::span;
using std::map;
using std::vector;
using std::wstring;
using std::to_wstring;
using std::towupper;
using std::cerr;
using std::wclog;
using std::wcout;
using std::wcin;
using std::endl;
using std::getline;
using std::hex;
using std::dec;
using std::left;
using std::right;
using std::setw;
using std::setfill;
using std::min;
using std::max;
using std::find;
using std::get;
using std::find;
using std::views::all;

namespace execution = std::execution;
namespace views = std::ranges::views;
using namespace std::literals::string_literals;

static map<wchar_t, MenuCommand> const mainMenuShortcuts
{
    { L'E', MenuCommand::GlobalEnable },
    { L'D', MenuCommand::GlobalEnable },
 // { L'G', MenuCommand::PerGPUConfig },
    { L'C', MenuCommand::PerGPUConfigClear },
    { L'U', MenuCommand::UEFIConfiguration },
    { L'S', MenuCommand::SaveConfiguration },
    { L'I', MenuCommand::DiscardConfiguration },
    { L'Q', MenuCommand::Quit }
};

static map<wchar_t, MenuCommand> const gpuMenuShortcuts
{
    { L'I', MenuCommand::GPUSelectorByPCIID },
    { L'S', MenuCommand::GPUSelectorByPCISubsystem },
    { L'L', MenuCommand::GPUSelectorByPCILocation },
    { L'C', MenuCommand::GPUSelectorClear }
};

static map<wchar_t, MenuCommand> const barSizeMenuShortcuts
{
    { L'C', MenuCommand::GPUSelectorClear },
    { L'X', MenuCommand::GPUSelectorExclude }
};

static constexpr wchar_t const WCHAR_T_HIGH_BIT_MASK = L'\001' << (sizeof(L'\0') * BYTE_BITSIZE - 1u);

static wstring showMainMenuEntry(MenuCommand menuCommand, bool isGlobalEnable, vector<DeviceInfo> const &devices)
{
    switch (menuCommand)
    {
    case MenuCommand::GlobalEnable:
        if (isGlobalEnable)
            wcout << L"\t(D) Disable auto-settings BAR size for known Turing GPUs (GTX 1600 / RTX 2000 line)\n";
        else
            wcout << L"\t(E) Enable auto-setting BAR size for known Turing GPUs (GTX 1600 / RTX 2000 line)\n";

        return wstring(1u, isGlobalEnable ? L'D' : L'E');

    case MenuCommand::PerGPUConfig:
        if (devices | all)
        {
            wstring commands(1u, L'G');
            wcout << L"\t    Manually configure BAR size for specific GPUs:\n";

            for (auto const &&[index, device]: devices | views::enumerate)
            {
                wcout << L"\t\t(" << index + 1u << L"). " << device.productName << L'\n';
                commands.push_back(static_cast<wchar_t>((L'0' + index + 1u) | WCHAR_T_HIGH_BIT_MASK));
            }

            return commands;
        }
        else
            return { };

    case MenuCommand::PerGPUConfigClear:
        if (devices | all)
        {
            wcout << L"\t(C) Clear per-GPU configuration\n"s;
            return wstring(1u, L'C');
        }
        else
            return { };

    case MenuCommand::UEFIConfiguration:
        wcout << L"\t(U) Configure motherboard UEFI BAR size support\n"s;
        return wstring(1u, L'U');

    case MenuCommand::SaveConfiguration:
        wcout << L"\t(S) Save configuration changes.\n"s;
        return wstring(1u, L'S');

    case MenuCommand::DiscardConfiguration:
        wcout << L"\t(I) Discard configuration changes\n"s;
        return wstring(1u, 'I');

    case MenuCommand::Quit:
        wcout << L"\t(Q) Quit\n"s;
        return wstring(1u, L'Q');

    case MenuCommand::DiscardQuit:
        wcout << L"\t(Q) Discard configuration changes and Quit\n"s;
        return wstring(1u, L'Q');
    }

    return { };
}

static wstring showUEFIReBarMenuEntry(MenuCommand menuCommand)
{
    switch (menuCommand)
    {
    case MenuCommand::UEFIBARSizePrompt:
        wcout << L"      0: Disabled \n"s;
        wcout << L"Above 0: Maximum BAR size set to 2^x MB \n"s;
        wcout << L"     32: Unlimited BAR size\n\n"s;
        wcout << L"Enter:   Leave unchanged\n";
        return { };
    }

    return { };
}

static wstring showBarSizeMenuEntry(MenuCommand menuCommand)
{
    switch (menuCommand)
    {
    case MenuCommand::GPUSelectorClear:
        wcout << L"\t(C): Clear GPU-specific configuration\n";
        return wstring(1u, L'C');

    case MenuCommand::GPUSelectorExclude:
        wcout << L"\t(X): Add exclusion for the GPU from auto-selected configuration.\n";
        return wstring(1u, L'X');

    case MenuCommand::GPUVRAMSize:
        wcout << L"\t 0):  64 MB\n";
        wcout << L"\t 1): 128 MB\n";
        wcout << L"\t 2): 256 MB\n";
        wcout << L"\t 3): 512 MB\n";
        wcout << L"\t 4):   1 GB\n";
        wcout << L"\t 5):   2 GB\n";
        wcout << L"\t 6):   4 GB\n";
        wcout << L"\t 7):   8 GB\n";
        wcout << L"\t 8):  16 GB\n";
        wcout << L"\t 9):  32 GB\n";
        wcout << L"\t10):  64 GB\n";
        wcout << L"    [Enter]: Leave unchanged\n";

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
    switch (menuCommand)
    {
    case MenuCommand::GPUSelectorByPCIID:
        wcout << L"\t(I): Select the GPU by PCI ID: ";
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L'\n';
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, L'I');

    case MenuCommand::GPUSelectorByPCISubsystem:
        wcout << L"\t(S): Select the GPU by PCI ID and Subsystem ID: ";
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L", "s;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemDeviceID << L'\n';
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, L'S');

    case MenuCommand::GPUSelectorByPCILocation:
        wcout << L"\t(L): Select the GPU by PCI ID, subystem and bus Location: ";
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].deviceID << L", "s;
        wcout << right << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << devices[device].subsystemDeviceID << L", "s;
        wcout << right << hex << setw(BYTE_SIZE * 2u) << setfill(L'0') << devices[device].bus << L':' << hex << setw(BYTE_SIZE * 2u) << setfill(L'0') << devices[device].device;
        wcout << L'.' << hex << devices[device].function << L'\n';
        wcout << dec << setfill(L' ') << left;
        return wstring(1u, L'L');
    }

    return { };
}

static wstring showMenuEntry(MenuType menuType, MenuCommand menuCommand, bool isGlobalEnable, unsigned short device, vector<DeviceInfo> const &devices)
{
    switch (menuType)
    {
    case MenuType::Main:
        return showMainMenuEntry(menuCommand, isGlobalEnable, devices);

    case MenuType::GPUConfig:
        return showGPUConfigurationMenuEntry(menuCommand, device, devices);

    case MenuType::GPUBARSize:
        return showBarSizeMenuEntry(menuCommand);

    case MenuType::MotherboradRBARSize:
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

static wstring getPromptLine(MenuType menuType)
{
    switch (menuType)
    {
    case MenuType::Main:
        return L"Choose configuration command"s;

    case MenuType::MotherboradRBARSize:
        return L"Enter NvStrapsReBar Value"s;

    case MenuType::GPUConfig:
        return L"Choose GPU selector"s;

    case MenuType::GPUBARSize:
        return L"Chose option"s;
    }

    return L"Input an option"s;
}

static bool isNumeric(wstring const &inputValue)
{
    for (auto const ch: inputValue)
        if (!isdigit(ch))
            return false;

    return !!(inputValue | all);
}

static bool hasShortcut(wchar_t input, wstring commands)
{
    return find(commands.cbegin(), commands.cend(), towupper(input)) != commands.cend();
}

static tuple<optional<MenuCommand>, unsigned> translateInput(MenuType menuType, wstring commands, wstring inputValue, std::vector<DeviceInfo> const& devices)
{
    switch (menuType)
    {
    case MenuType::MotherboradRBARSize:
        if (isNumeric(inputValue) && stoul(inputValue) <= MAX_UEFI_BAR_SIZE_SELECTOR)
            return { MenuCommand::UEFIBARSizePrompt, stoul(inputValue) };

        return { nullopt, 0u };

    case MenuType::GPUBARSize:
        if (isNumeric(inputValue))
            return { MenuCommand::GPUVRAMSize, stoul(inputValue) };

        if (inputValue | all && towupper(*inputValue.cbegin()) == L'C' && commands.find(L'C') != commands.npos)
            return { MenuCommand::GPUSelectorClear, 0u };

        if (inputValue | all && towupper(*inputValue.cbegin()) == L'X' && commands.find(L'X') != commands.npos)
            return { MenuCommand::GPUSelectorExclude, 0u };

        return { nullopt, 0u };

    case MenuType::GPUConfig:
        if (inputValue.length() == 1u && hasShortcut(*inputValue.cbegin(), commands))
            if (auto it = gpuMenuShortcuts.find(towupper(*inputValue.cbegin())); it != gpuMenuShortcuts.end())
                return { it->second, 0u };

        return { nullopt, 0u };

    case MenuType::Main:
        if (isNumeric(inputValue) && hasShortcut(L'G', commands) && [&devices](auto val) { return 1u <= val && val <= devices.size(); }(stoul(inputValue)))
            return { MenuCommand::PerGPUConfig, stoul(inputValue) - 1u };

        if (inputValue.length() == 1u && hasShortcut(*inputValue.cbegin(), commands) && towupper(*inputValue.cbegin()) != L'G')         // L'G' - per-GPU configuration, GPU index used instead
            if (auto it = mainMenuShortcuts.find(towupper(*inputValue.cbegin())); it != mainMenuShortcuts.end())
                return { it->second, 0u };

        return { nullopt, 0u };
    }

    return { nullopt, 0u };
}

static tuple<optional<MenuCommand>, unsigned> runInputPrompt(MenuType menuType, wstring const &commands, optional<MenuCommand> defaultCommand, unsigned short defaultValue, std::vector<DeviceInfo> const& devices)
{
    wstring promptLine = getPromptLine(menuType);

    wcout << promptLine << L" ("s;

    for (auto [index, commandChar]: commands | views::enumerate)
        if (menuType != MenuType::Main || commandChar != L'G')      // L'G' - per-GPU configuration, GPU index used instead)
        {
            if (index)
                wcout << L", "s;

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

    wcout << L"): "s;

    wstring inputValue;
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
        MenuType menuType,
        std::span<MenuCommand> menu,
        optional<MenuCommand> defaultCommand,
        unsigned short defaultValue,
        bool isGlobalEnable,
        unsigned short device,
        std::vector<DeviceInfo> const &devices
    )
{
    wstring commands;

    for (auto menuCommand: menu)
        commands += showMenuEntry(menuType, menuCommand, isGlobalEnable, device, devices);

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
    if (find(execution::par_unseq, menu.cbegin(), menu.cend(), MenuCommand::GPUVRAMSize) != menu.cend())
        return MenuType::GPUBARSize;

    if (find(execution::par_unseq, menu.cbegin(), menu.cend(), MenuCommand::UEFIBARSizePrompt) != menu.cend())
        return MenuType::MotherboradRBARSize;

    if (!menu.empty() && *menu.crbegin() != MenuCommand::Quit && *menu.crbegin() != MenuCommand::DiscardQuit)
        return MenuType::GPUConfig;

    return MenuType::Main;
}

static void showMenuHeader(MenuType menuType, unsigned device, vector<DeviceInfo> const &devices)
{
    switch (menuType)
    {
    case MenuType::Main:
        std::wcerr << "Warning: Setting GPU BAR size NOT IMPLEMENTED. GPU BAR size configuration is HARD-CODED.\n";
        std::wcerr << "Please see README page on github, project terminatorul/NvStrapsReBar\n\n";
        std::wcerr << "You should still set UEFI BAR size to non-zero, in order to enable resizable BAR support\n";
        std::wcerr << "(for both GPU + motherboard), using the below menu (choose option 'U')\n\n";

        wcout << L"BAR size configuration menu:\n"s;
        break;

    case MenuType::MotherboradRBARSize:
        wcout << L"\nFirst verify Above 4G Decoding is enabled and CSM is disabled in UEFI setup, otherwise system will not POST with GPU.\n"s;
        wcout << L"If your NvStrapsReBar value keeps getting reset then check your system time.\n"s;

        wcout << L"\nIt is recommended to first try smaller sizes above 256MB in case an old BIOS doesn't support large BARs.\n"s;
        wcout << L"\nEnter NvStrapsReBar Value\n"s;
        break;

    case MenuType::GPUConfig:
        wcout << L"Configure " << devices[device].productName << L":\n";
        break;

    case MenuType::GPUBARSize:
        wcout << L"Input GPU BAR size:\n";
        break;
    }
}

tuple<MenuCommand, unsigned> showMenuPrompt
    (
        span<MenuCommand> menu,
        optional<MenuCommand> defaultCommand,
        unsigned short defaultValue,
        bool isGlobalEnable,
        unsigned short device,
        vector<DeviceInfo> const &devices
    )
{
    MenuType menuType = getMenuType(menu);

    showMenuHeader(menuType, device, devices);
    return showMenu(menuType, menu, defaultCommand, defaultValue, isGlobalEnable, device, devices);
}
