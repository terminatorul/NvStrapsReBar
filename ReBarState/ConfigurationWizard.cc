#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cstdint>
#include <optional>
#include <vector>

#include "ReBarState.hh"
#include "DeviceList.hh"
#include "TextWizardPage.hh"
#include "ConfigurationWizard.hh"

using std::uint_least8_t;
using std::optional;
using std::vector;

void runConfigurationWizard()
{
   optional<uint_least8_t> reBarState = getReBarState();
   vector<DeviceInfo> deviceList = getDeviceList();

   showWizardPage(deviceList, reBarState);
}
