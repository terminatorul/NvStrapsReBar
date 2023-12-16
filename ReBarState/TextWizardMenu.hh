#if !defined(NV_STRAPS_REBAR_TEXT_WIZARD_MENU_HH)
#define NV_STRAPS_REBAR_TEXT_WIZARD_MENU_HH

#include <optional>
#include <tuple>
#include <span>
#include <vector>

#include "DeviceList.hh"

enum class MenuCommand
{
    Quit,
    DiscardQuit,
    SaveConfiguration,
    DiscardConfiguration,
    DiscardPrompt,
    GlobalEnable,
    GlobalFallbackEnable,
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

enum class MenuType
{
    Main,
    GPUConfig,
    GPUBARSize,
    MotherboradRBARSize
};

std::tuple<MenuCommand, unsigned> showMenuPrompt
    (
        std::span<MenuCommand> menu,
        std::optional<MenuCommand> defaultCommand,
        unsigned short defaultValue,
        bool isGlobalEnable,
        unsigned short device,
        std::vector<DeviceInfo> const &devices
    );

#endif          // !definded(NV_STRAPS_REBAR_TEXT_WIZARD_MENU_HH)
