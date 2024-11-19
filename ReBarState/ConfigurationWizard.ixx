export module ConfigurationWizard;

import std;

import NvStraps.WinAPI;
import LocalAppConfig;
import StatusVar;
import DeviceRegistry;
import WinApiError;
import DeviceList;
import NvStrapsConfig;
import TextWizardPage;
import TextWizardMenu;

export void runConfigurationWizard();

module: private;

using std::uint_least8_t;
using std::uint_least32_t;
using std::uint_least64_t;
using std::span;
using std::begin;
using std::end;
using std::tuple;
using std::tie;
using std::vector;
using std::to_string;
using std::to_wstring;
using std::runtime_error;
using std::system_error;
using namespace std::literals::string_literals;

namespace ranges = std::ranges;

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

static vector<MenuCommand> buildConfigurationMenu(NvStrapsConfig const &nvStrapsConfig, bool isDirty)
{
    auto configMenu = vector<MenuCommand>
    {
    MenuCommand::GlobalEnable,
    MenuCommand::PerGPUConfig,
    MenuCommand::PerGPUConfigClear,
    MenuCommand::SkipS3Resume,
    MenuCommand::OverrideBarSizeMask,
    MenuCommand::EnableSetupVarCRC,
    MenuCommand::ClearSetupVarCRC,
    MenuCommand::UEFIConfiguration,
    MenuCommand::ShowConfiguration
    };

    if (isDirty)
    {
        configMenu.push_back(MenuCommand::SaveConfiguration);
        configMenu.push_back(MenuCommand::DiscardConfiguration);
        configMenu.push_back(MenuCommand::DiscardQuit);
    }
    else
        configMenu.push_back(MenuCommand::Quit);

    if (!nvStrapsConfig.hasSetupVarCRC())
    if (auto it = ranges::find(configMenu, MenuCommand::ClearSetupVarCRC); it != configMenu.end())
        configMenu.erase(it);

    return configMenu;
}

static span<MenuCommand> selectCurrentMenu(MenuType menuType, NvStrapsConfig &nvStrapsConfig, unsigned selectedDevice, vector<DeviceInfo> const &deviceList)
{
    static auto mainMenu = vector<MenuCommand> { };
    static auto barSizeMenu = vector<MenuCommand> { begin(GPUBarSizePrompt), end(GPUBarSizePrompt) };

    switch (menuType)
    {
    case MenuType::Main:
        mainMenu = buildConfigurationMenu(nvStrapsConfig, nvStrapsConfig.isDirty());
        return mainMenu;

    case MenuType::GPUConfig:
        return GPUConfigMenu;

    case MenuType::GPUBARSize:
    if (isTuringGPU(deviceList[selectedDevice].deviceID))
        if (barSizeMenu[2u] != MenuCommand::OverrideBarSizeMask)
        barSizeMenu.insert(barSizeMenu.begin() + 2u, MenuCommand::OverrideBarSizeMask);
        else
        ;
    else
        if (barSizeMenu[2u] == MenuCommand::OverrideBarSizeMask)
        barSizeMenu.erase(barSizeMenu.begin() + 2u);

        return barSizeMenu;

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

static bool setGPUBarSize(NvStrapsConfig &nvStrapsConfig, uint_least8_t barSizeSelector, unsigned selectedDevice, MenuCommand deviceSelector, vector<DeviceInfo> const &deviceList)
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

static bool setGPUBarSizeMaskOverride(NvStrapsConfig &nvStrapsConfig, bool maskOverride, unsigned selectedDevice, MenuCommand deviceSelector, vector<DeviceInfo> const &deviceList)
{
    auto configured = false;
    auto const &device = deviceList[selectedDevice];

    switch (deviceSelector)
    {
    case MenuCommand::GPUSelectorByPCIID:
        configured = nvStrapsConfig.setBarSizeMaskOverride(maskOverride, device.deviceID);
        break;

    case MenuCommand::GPUSelectorByPCISubsystem:
        configured = nvStrapsConfig.setBarSizeMaskOverride(maskOverride, device.deviceID, device.subsystemVendorID, device.subsystemDeviceID);
        break;

    case MenuCommand::GPUSelectorByPCILocation:
        configured = nvStrapsConfig.setBarSizeMaskOverride(maskOverride, device.deviceID, device.subsystemVendorID, device.subsystemDeviceID, device.bus, device.device, device.function);
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

static void setConfigDirtyOnMismatch(vector<DeviceInfo> const &deviceList, NvStrapsConfig &config)
{
    auto errorCode = ERROR_CODE { };
    auto statusVar = ReadStatusVar(&errorCode);

    if (errorCode == ERROR_CODE_SUCCESS && statusVar == StatusVar_Cleared && config.hasSetupVarCRC())
    return (void)config.isDirty(true);

    for (auto const &device: deviceList)
    {
    auto const [priority, barSize] = config.lookupBarSize
        (
        device.deviceID,
        device.subsystemVendorID,
        device.subsystemDeviceID,
        device.bus,
        device.device,
        device.function
        );

    if (!!priority && barSize < BarSizeSelector_Excluded)
    {
        auto &&bridgeConfig = config.lookupBridgeConfig(device.bus);

        if (
               !bridgeConfig
            || !bridgeConfig->deviceMatch(device.bridge.vendorID, device.bridge.deviceID)
            || !bridgeConfig->busLocationMatch(device.bridge.bus, device.bridge.dev, device.bridge.func)
            || config.hasBridgeDevice(device.bridge.bus, device.bridge.dev, device.bridge.func) != tie(device.bridge.vendorID, device.bridge.deviceID)
        )
        {
        return (void)config.isDirty(true);
        }

        auto &&gpuConfig = config.lookupGPUConfig(device.bus, device.device, device.function);

        if (!gpuConfig || !gpuConfig->bar0.base || gpuConfig->bar0.base != device.bar0.Base || gpuConfig->bar0.top != device.bar0.Top)
        return (void)config.isDirty(true);
    }
    }
}

static void populateBridgeAndGpuConfig(NvStrapsConfig &config, vector<DeviceInfo> deviceList)
{
    config.resetConfig();

    for (auto const &device: deviceList)
    {
    auto const [priority, barSize] = config.lookupBarSize
        (
        device.deviceID,
        device.subsystemVendorID,
        device.subsystemDeviceID,
        device.bus,
        device.device,
        device.function
        );

    if (!!priority && barSize < BarSizeSelector_Excluded)
    {
        auto gpuConfig = NvStraps_GPUConfig
        {
        .deviceID	= device.deviceID,
        .subsysVendorID = device.subsystemVendorID,
        .subsysDeviceID = device.subsystemDeviceID,
        .bus		= device.bus,
        .device		= device.device,
        .function	= device.function,
        .bar0		= { .base = device.bar0.Base, .top = device.bar0.Top }
        };

        if (gpuConfig.bar0.base >= UINT32_MAX || gpuConfig.bar0.top >= UINT32_MAX)
        throw runtime_error("64-bit address for GPU BAR0 not implmented"s);

        if (gpuConfig.bar0.base & uint_least32_t { 0x0000'000Ful })
        throw runtime_error("PCI BARs must be aligned at least to 16 bytes"s);

        if (gpuConfig.bar0.base % (gpuConfig.bar0.top - gpuConfig.bar0.base + 1u))
        throw runtime_error("PCI BARs must the naturally aligned (aligned to their size)"s);

        if (!config.setGPUConfig(gpuConfig))
        throw runtime_error("Unsupported configuration: too many GPUs to configure: " + to_string(config.nGPUConfig) + '+');

        auto bridgeConfig = NvStraps_BridgeConfig
        {
        .vendorID	    = device.bridge.vendorID,
        .deviceID 	    = device.bridge.deviceID,
        .bridgeBus	    = device.bridge.bus,
        .bridgeDevice	    = device.bridge.dev,
        .bridgeFunction     = device.bridge.func,
        .bridgeSecondaryBus = device.bus
        };

        auto &&previousBridge = config.lookupBridgeConfig(device.bus);

        if (previousBridge)
        if (*previousBridge != bridgeConfig)
            throw runtime_error("Unsupported system: multiple PCI bridges for bus " + to_string(device.bus));
        else
            ;
        else
        if (!config.setBridgeConfig(bridgeConfig))
            throw runtime_error("Unsupported configuration: too many PCI bridges to record: " + to_string(config.nBridgeConfig) + '+');
    }
    }
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

    setConfigDirtyOnMismatch(deviceList, nvStrapsConfig);
    showConfiguration(deviceList, nvStrapsConfig, driverStatus);

    auto runMenuLoop = true;

    auto showConfig = [&]()
    {
        showConfiguration(deviceList, nvStrapsConfig, driverStatus);
    };

    while (runMenuLoop)
    {
        auto [defaultCommand, defaultValue] = getDefaultCommand(menuType, nvStrapsConfig, deviceList);

        auto [menuCommand, value] = showMenuPrompt
            (
                selectCurrentMenu(menuType, nvStrapsConfig, selectedDevice, deviceList),
                defaultCommand,
                defaultValue,
                nvStrapsConfig.isGlobalEnable(),
                selectedDevice,
                deviceList,
        nvStrapsConfig
            );

        switch (menuCommand)
        {
        case MenuCommand::DiscardQuit:
        if (runConfirmationPrompt(MenuCommand::DiscardQuit))
        runMenuLoop = false;
        else
        showConfig();

        break;

        case MenuCommand::Quit:
            runMenuLoop = false;
            break;

        case MenuCommand::GlobalEnable:
            nvStrapsConfig.setGlobalEnable(nvStrapsConfig.isGlobalEnable() ? 0x00u : 0x02u);
            showConfig();
            break;

    case MenuCommand::SkipS3Resume:
        if (nvStrapsConfig.skipS3Resume() || runConfirmationPrompt(MenuCommand::SkipS3Resume))
        nvStrapsConfig.skipS3Resume(!nvStrapsConfig.skipS3Resume());

        showConfig();
        break;

    case MenuCommand::OverrideBarSizeMask:
        switch (menuType)
        {
        case MenuType::Main:
        nvStrapsConfig.overrideBarSizeMask(!nvStrapsConfig.overrideBarSizeMask());
        break;

        case MenuType::GPUBARSize:
        {
            auto &&deviceInfo = deviceList[selectedDevice];
            auto barSizeMaskOverride =
            nvStrapsConfig.lookupBarSizeMaskOverride
                (
                deviceInfo.deviceID,
                deviceInfo.subsystemVendorID,
                deviceInfo.subsystemDeviceID,
                deviceInfo.bus,
                deviceInfo.device,
                deviceInfo.function
                )
                .sizeMaskOverride;
            setGPUBarSizeMaskOverride(nvStrapsConfig, !barSizeMaskOverride, selectedDevice, deviceSelector, deviceList);
        }

        break;
        }

        showConfig();
        menuType = MenuType::Main;
        break;

    case MenuCommand::EnableSetupVarCRC:
        if (!nvStrapsConfig.enableSetupVarCRC() || runConfirmationPrompt(MenuCommand::EnableSetupVarCRC))
        nvStrapsConfig.enableSetupVarCRC(!nvStrapsConfig.enableSetupVarCRC());

        showConfig();
        break;

    case MenuCommand::ClearSetupVarCRC:
        nvStrapsConfig.hasSetupVarCRC(false);
        nvStrapsConfig.setupVarCRC(0u);

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

    case MenuCommand::ShowConfiguration:
        ShowNvStrapsConfig(showInfo);
        break;

        case MenuCommand::DiscardConfiguration:
            if (nvStrapsConfig.isDirty())
        {
                GetNvStrapsConfig(true);
        setConfigDirtyOnMismatch(deviceList, nvStrapsConfig);
        }

            showConfig();
            break;

        case MenuCommand::SaveConfiguration:
        populateBridgeAndGpuConfig(nvStrapsConfig, deviceList);
        nvStrapsConfig.hasSetupVarCRC(false);
        nvStrapsConfig.setupVarCRC(0u);
            SaveNvStrapsConfig();
        setConfigDirtyOnMismatch(deviceList, nvStrapsConfig);

            showInfo(L"Configuration saved to NvStrapsReBar UEFI variable\n"s);
            showInfo(L"\nReboot for changes to take effect\n\n"s);

            showConfig();
            break;
        }
    }
}

// vim: ft=cpp
