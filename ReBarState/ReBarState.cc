/*
 * Copyright (C) 2024-2025 Timothy Madden <terminatorul@gmail.com>
 * Copyright (c) 2022-2023 xCuri0 <zkqri0@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
import NvStraps.WinAPI;
#else
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

import std;

import NvStrapsConfig;
import TextWizardPage;
import ConfigurationWizard;

using std::exception;
using std::system_error;
using std::make_error_code;
using std::errc;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

bool CheckPriviledge()
{
#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
    if (auto hToken = HANDLE { }; OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
    auto tokp = TOKEN_PRIVILEGES { };
    LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tokp.Privileges[0].Luid);

    tokp.PrivilegeCount = 1;
    tokp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    auto len = DWORD { };
    AdjustTokenPrivileges(hToken, FALSE, &tokp, 0, NULL, &len);

    if (GetLastError() != ERROR_SUCCESS)
        return cout << "Failed to obtain SE_SYSTEM_ENVIRONMENT_NAME\n"sv, false;

    return true;
    }

    return false;
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

    return pause(), 0;
}
catch (system_error const &ex)
{
    cerr << ex.what() << endl;
    return pause(), 252u;
}
catch (exception const &ex)
{
    cerr << "Application error: "sv << ex.what() << endl;
    return pause(), 253u;
}
catch (...)
{
    cerr << "Application error!"sv << endl;
    return pause(), 254u;
}
