#if !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
#define NV_STRAPS_REBAR_WIZARD_PAGE_HH

#include <cstdint>
#include <optional>
#include <vector>

#include "DeviceList.hh"
#include "NvStrapsPciConfig.hh"

void showInfo(std::wstring const &message);
void showError(std::wstring const &message);
void showStartupLogo();

#if defined(NDEBUG)
void showMotherboardReBarMenu();
#endif

void showConfiguration(std::vector<DeviceInfo> const &devices, NvStrapsConfig const &nvStrapsConfig, std::optional<std::uint_least8_t> reBarState);

#endif  // !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
