/*
Copyright (c) 2022-2023 xCuri0 <zkqri0@gmail.com>
SPDX-License-Identifier: MIT
*/

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <optional>
#include <string>
#include <iostream>
#include <iterator>
#include <ranges>
#include <memory>
#include <array>

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#include "WinApiError.hh"
#else
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

#include "StatusVar.h"
#include "NvStrapsConfig.h"
#include "TextWizardPage.hh"
#include "ConfigurationWizard.hh"

using std::uint8_t;
using std::uint_least8_t;
using std::pow;
using std::exception;
using std::out_of_range;
using std::system_error;
using std::unique_ptr;
using std::make_error_code;
using std::errc;
using std::optional;
using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::string;
using std::array;
using std::stoi;
using std::size;
using std::views::all;

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

bool CheckPriviledge()
{
#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

    auto hToken = HANDLE { };
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    auto tokp = TOKEN_PRIVILEGES { };
    LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tokp.Privileges[0].Luid);

    tokp.PrivilegeCount = 1;
    tokp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    auto len = DWORD { };
    AdjustTokenPrivileges(hToken, FALSE, &tokp, 0, NULL, &len);

    if (GetLastError() != ERROR_SUCCESS)
        return cout << "Failed to obtain SE_SYSTEM_ENVIRONMENT_NAME\n"sv, false;

    return true;
#else   // Linux
    return getuid() == 0;
#endif
}

void pause()
{
// Linux will probably be run from terminal not requiring this
#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
    cout << "You can close the app now\n"sv;
    cin.get();
#endif
}

int main(int argc, char const *argv[])
try
{
    showStartupLogo();

    if (!CheckPriviledge())
        throw system_error(make_error_code(errc::permission_denied), "No access permissions to EFI variable (try running as admin/root)"s);

    runConfigurationWizard();

    return pause(), EXIT_SUCCESS;
}
catch (system_error const &ex)
{
    cerr << ex.what() << endl;
    return pause(), 252u;
}
catch (exception const &ex)
{
    cerr << "Application error: " << ex.what() << endl;
    return pause(), 253u;
}
catch (...)
{
    cerr << "Application error!" << endl;
    return pause(), 254u;
}
