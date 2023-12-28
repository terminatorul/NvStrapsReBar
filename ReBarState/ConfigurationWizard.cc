#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <tuple>
#include <vector>
#include <string>
#include <system_error>

#include "LocalAppConfig.h"
#include "StatusVar.h"
#include "WinApiError.hh"
#include "NvStrapsConfig.h"
#include "DeviceList.hh"
#include "ReBarState.hh"
#include "TextWizardPage.hh"
#include "TextWizardMenu.hh"
#include "ConfigurationWizard.hh"

using std::uint_least8_t;
using std::uint_least32_t;
using std::uint_least64_t;
using std::optional;
using std::span;
using std::tuple;
using std::vector;
using std::to_wstring;
using std::system_error;
using namespace std::literals::string_literals;

MenuCommand
    GPUConfigMenu[] =
{
    MenuCommand::GPUSelectorByPCIID,
    MenuCommand::GPUSelectorByPCISubsystem,
    MenuCommand::GPUSelectorByPCILocation,
    MenuCommand::GPUSelectorClear,
    MenuCommand::DefaultChoice
},
    ReBarUEFIMenu[] =
{
    MenuCommand::UEFIBARSizePrompt,
    MenuCommand::DefaultChoice
},

    GPUBarSizePrompt[] =
{
    MenuCommand::GPUSelectorClear,
    MenuCommand::GPUSelectorExclude,
    MenuCommand::GPUVRAMSize,
    MenuCommand::DefaultChoice
};

static vector<MenuCommand> buildConfigurationMenu(NvStrapsConfig &nvStrapsConfig, bool isDirty)
{
    auto configMenu = vector<MenuCommand> { MenuCommand::GlobalEnable, MenuCommand::PerGPUConfig, MenuCommand::PerGPUConfigClear, MenuCommand::UEFIConfiguration };

    if (isDirty)
    {
        configMenu.push_back(MenuCommand::SaveConfiguration);
        configMenu.push_back(MenuCommand::DiscardConfiguration);
    }

    if (isDirty)
        configMenu.push_back(MenuCommand::DiscardQuit);
    else
        configMenu.push_back(MenuCommand::Quit);

    return configMenu;
}

static span<MenuCommand> selectCurrentMenu(MenuType menuType, NvStrapsConfig &nvStrapsConfig)
{
    static auto mainMenu = vector<MenuCommand> { };

    switch (menuType)
    {
    case MenuType::Main:
        mainMenu = buildConfigurationMenu(nvStrapsConfig, nvStrapsConfig.isDirty());
        return mainMenu;

    case MenuType::GPUConfig:
        return GPUConfigMenu;

    case MenuType::GPUBARSize:
        return GPUBarSizePrompt;

    case MenuType::PCIBARSize:
        return ReBarUEFIMenu;
    }

    return mainMenu;
}

static tuple<MenuCommand, unsigned> getDefaultCommand(MenuType menuType, NvStrapsConfig const &nvStrapsConfig, vector<DeviceInfo> const &deviceList)
{
    switch (menuType)
    {
    case MenuType::Main:
        return { nvStrapsConfig.isDirty() ? MenuCommand::DiscardQuit : MenuCommand::Quit, 0u };

    case MenuType::GPUConfig:
    case MenuType::GPUBARSize:
    case MenuType::PCIBARSize:
        return { MenuCommand::DefaultChoice, 0u };
    }

    return { MenuCommand::DefaultChoice, 0u };
}

static bool setGPUBarSize(NvStrapsConfig &nvStrapsConfig, UINT8 barSizeSelector, unsigned selectedDevice, MenuCommand deviceSelector, vector<DeviceInfo> const &deviceList)
{
    auto configured = false;
    auto const &device = deviceList[selectedDevice];

    switch (deviceSelector)
    {
    case MenuCommand::GPUSelectorByPCIID:
        configured = nvStrapsConfig.setGPUSelector(barSizeSelector, device.deviceID);
        break;

    case MenuCommand::GPUSelectorByPCISubsystem:
        configured = nvStrapsConfig.setGPUSelector(barSizeSelector, device.deviceID, device.subsystemVendorID, device.subsystemDeviceID);
        break;

    case MenuCommand::GPUSelectorByPCILocation:
        configured = nvStrapsConfig.setGPUSelector(barSizeSelector, device.deviceID, device.subsystemVendorID, device.subsystemDeviceID, device.bus, device.device, device.function);
        break;
    }

    if (!configured)
        showError(L"Cannot configure GPU. Too many GPU configurations ? Clear existing configurations and re-configure.\n"s);

    return configured;
}

static bool clearGPUBarSize(NvStrapsConfig &nvStrapsConfig, unsigned selectedDevice, MenuCommand deviceSelector, vector<DeviceInfo> const &deviceList)
{
    auto configured = false;
    auto const &device = deviceList[selectedDevice];

    switch (deviceSelector)
    {
    case MenuCommand::GPUSelectorByPCIID:
        configured = nvStrapsConfig.clearGPUSelector(device.deviceID);
        break;

    case MenuCommand::GPUSelectorByPCISubsystem:
        configured = nvStrapsConfig.clearGPUSelector(device.deviceID, device.subsystemVendorID, device.subsystemDeviceID);
        break;

    case MenuCommand::GPUSelectorByPCILocation:
        configured = nvStrapsConfig.clearGPUSelector(device.deviceID, device.subsystemVendorID, device.subsystemDeviceID, device.bus, device.device, device.function);
        break;
    }

    if (!configured)
        showError(L"Cannot configure GPU. Too many GPU configurations ? Clear existing configurations and re-configure.\n"s);

    return configured;
}

void runConfigurationWizard()
{
    auto menuType = MenuType::Main;
    auto dwStatusVarLastError = ERROR_CODE { ERROR_CODE_SUCCESS };
    auto driverStatus = ReadStatusVar(&dwStatusVarLastError);

    if (dwStatusVarLastError)
    {
        showError(L"Status var last error: " + to_wstring(dwStatusVarLastError) + L' ');
        showError(system_error(static_cast<int>(dwStatusVarLastError), winapi_error_category()).code().message());
    }

    auto &&nvStrapsConfig = GetNvStrapsConfig();
    auto &deviceList = getDeviceList();
    auto selectedDevice = 0u;
    auto deviceSelector = MenuCommand::GPUSelectorByPCIID;

    showConfiguration(deviceList, nvStrapsConfig, driverStatus);

    bool runMenuLoop = true;

    auto showConfig = [&]()
    {
        showConfiguration(deviceList, nvStrapsConfig, driverStatus);
    };

    while (runMenuLoop)
    {
        auto [defaultCommand, defaultValue] = getDefaultCommand(menuType, nvStrapsConfig, deviceList);

        auto [menuCommand, value] = showMenuPrompt
            (
                selectCurrentMenu(menuType, nvStrapsConfig),
                defaultCommand,
                defaultValue,
                nvStrapsConfig.isGlobalEnable(),
                selectedDevice,
                deviceList
            );

        switch (menuCommand)
        {
        case MenuCommand::DiscardQuit:
        case MenuCommand::Quit:
            runMenuLoop = false;
            break;

        case MenuCommand::GlobalEnable:
            nvStrapsConfig.setGlobalEnable(nvStrapsConfig.isGlobalEnable() ? 0x00u : 0x02u);
            showConfig();
            break;

        case MenuCommand::PerGPUConfigClear:
            nvStrapsConfig.clearGPUSelectors();
            showConfig();
            break;

        case MenuCommand::PerGPUConfig:
            if (value < deviceList.size())
            {
                menuType = MenuType::GPUConfig;
                selectedDevice = value;
            }
            break;

        case MenuCommand::GPUSelectorByPCIID:
        case MenuCommand::GPUSelectorByPCISubsystem:
        case MenuCommand::GPUSelectorByPCILocation:
            deviceSelector = menuCommand;
            menuType = MenuType::GPUBARSize;
            break;

        case MenuCommand::GPUSelectorClear:
            clearGPUBarSize(nvStrapsConfig, selectedDevice, deviceSelector, deviceList);
            showConfig();
            menuType = MenuType::Main;
            break;

        case MenuCommand::GPUSelectorExclude:
            value = BarSizeSelector_Excluded;
            [[fallthrough]];

        case MenuCommand::GPUVRAMSize:
            if (value <= MAX_BAR_SIZE_SELECTOR || value == BarSizeSelector_Excluded)
            {
                setGPUBarSize(nvStrapsConfig, value, selectedDevice, deviceSelector, deviceList);
                menuType = MenuType::Main;
            }

            showConfig();

            break;

        case MenuCommand::DefaultChoice:
            if (menuType == MenuType::PCIBARSize)
                showInfo(L"No BAR size changes to apply.\n");

            menuType = MenuType::Main;
            showConfig();
            break;

        case MenuCommand::UEFIConfiguration:
            menuType = MenuType::PCIBARSize;
            break;

        case MenuCommand::UEFIBARSizePrompt:
            nvStrapsConfig.targetPciBarSizeSelector(value);
            menuType = MenuType::Main;
            showConfig();
            break;

        case MenuCommand::DiscardConfiguration:
            if (nvStrapsConfig.isDirty())
                GetNvStrapsConfig(true);

            showConfig();
            break;

        case MenuCommand::SaveConfiguration:
            SaveNvStrapsConfig();

            showInfo(L"Successfully saved configuration to NvStrapsReBar UEFI variable\n"s);
            showInfo(L"\nReboot for changes to take effect\n\n"s);

            showConfig();
            break;
        }
    }
}
