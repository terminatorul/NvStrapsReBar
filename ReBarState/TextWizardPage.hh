#if !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
#define NV_STRAPS_REBAR_WIZARD_PAGE_HH

#include <cstdint>
#include <optional>
#include <vector>

#include "DeviceList.hh"
#include "NvStrapsConfig.h"

void showInfo(std::wstring const &message);
void showError(std::wstring const &message);
void showError(std::string const &message);
void showStartupLogo();

#if defined(NDEBUG)
void showMotherboardReBarMenu();
#endif

void showConfiguration(std::vector<DeviceInfo> const &devices, NvStrapsConfig const &nvStrapsConfig, uint_least64_t driverStatus);

#endif  // !defined(NV_STRAPS_REBAR_WIZARD_PAGE_HH)
