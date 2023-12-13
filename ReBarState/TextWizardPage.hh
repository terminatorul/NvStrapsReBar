#if !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
#define NV_STRAPS_REBAR_WIZARD_PAGE_HH

#include <cstdint>
#include <optional>
#include <vector>

#include "DeviceList.hh"

enum class MenuCommand
{
    Quit,
    SaveConfiguration,
    DiscardConfiguration,
    GlobalEnable,
    GlobalFallbackEnable,
    UEFIConfiguration,
    UEFIBARSizeConfiguration,
    PerGPUConfigClear,
    PerGPUConfig,
    GPUSelectorClear,
    GPUSelectorByPCIID,
    GPUSelectorByPCISubsystem,
    GPUSelectorByPCILocation,
    GPUVRAMSize,
};

void showStartupLogo();
void showWizardPage(std::vector<DeviceInfo> const &devices, std::optional<std::uint_least8_t> reBarState);

#endif  // !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
