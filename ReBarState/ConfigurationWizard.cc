#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <vector>

#include "DeviceList.hh"
#include "TextWizardPage.hh"
#include "ConfigurationWizard.hh"

using std::vector;

void runConfigurationWizard()
{
   vector<DeviceInfo> deviceList = getDeviceList();

   showWizardPage(deviceList);
}
