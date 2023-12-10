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
#include <string>
#include <iostream>
#include <ranges>

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#include "WinApiError.hh"
#else
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

#include "ConfigurationWizard.hh"

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

#define VNAME NvStrapsReBar
#define VGUID 481893F5-2436-4FD5-9D5A-69B121C3F0BA

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

#define TEXT_QUOTE(x) TEXT(QUOTE(x))

constexpr TCHAR const NVAR_Name[] = TEXT_QUOTE(VNAME);
constexpr TCHAR const NVAR_GUID[] = TEXT("{") TEXT_QUOTE(VGUID) TEXT("}");
#endif

constexpr UINT32 const
        VARIABLE_ATTRIBUTE_NON_VOLATILE = 0x00000001u,
        VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS = 0x00000002u,
        VARIABLE_ATTRIBUTE_RUNTIME_ACCESS = 0x00000004u;

using std::uint8_t;
using std::uint_least8_t;
using std::pow;
using std::exception;
using std::out_of_range;
using std::system_error;
using std::make_error_code;
using std::errc;
using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::string;
using std::stoi;
using std::views::all;

using namespace std::literals::string_literals;

bool notExist = false;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

bool CheckPriviledge()
{
	DWORD len;
	HANDLE hTok;
	TOKEN_PRIVILEGES tokp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTok))
            return false;

	LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tokp.Privileges[0].Luid);
	tokp.PrivilegeCount = 1;
	tokp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hTok, FALSE, &tokp, 0, NULL, &len);

	if (GetLastError() != ERROR_SUCCESS)
		return cout << "Failed to obtain SE_SYSTEM_ENVIRONMENT_NAME\n", false;

	return cout << "Obtained SE_SYSTEM_ENVIRONMENT_NAME\n", true;
}

uint_least8_t GetState()
{
	UINT8 rBarState;
	DWORD rSize;

	rSize = GetFirmwareEnvironmentVariable(NVAR_Name, NVAR_GUID, &rBarState, 1);

	if (rSize == 1)
		return rBarState;
	else
        {
		notExist = true;
		return 0u;
	}
}

bool WriteState(uint8_t rBarState)
{
	DWORD size = sizeof(UINT8);
	DWORD dwAttributes = VARIABLE_ATTRIBUTE_NON_VOLATILE | VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS | VARIABLE_ATTRIBUTE_RUNTIME_ACCESS;

	return SetFirmwareEnvironmentVariableEx(NVAR_Name, NVAR_GUID, &rBarState, size, dwAttributes)
                || (throw system_error(static_cast<int>(::GetLastError()), winapi_error_category()), FALSE);
}

#else   // Linux

#define REBARPATH /sys/firmware/efi/efivars/VNAME-VGUID
#define REBARPS STR(REBARPATH)

struct __attribute__((__packed__)) rebarVar {
	uint32_t attr;
	uint8_t value;
};

bool CheckPriviledge() {
	return getuid() == 0;
}

uint8_t GetState() {
	uint8_t rebarState[5];

	FILE* f = fopen(REBARPS, "rb");

	if (!(f && (fread(&rebarState, 5, 1, f) == 1))) {
		rebarState[4] = 0;
		notExist = true;
	} else
		fclose(f);

	return rebarState[4];
}

bool WriteState(uint8_t rBarState) {
	int attr;
	rebarVar rVar;
	FILE* f = fopen(REBARPS, "rb");

	if (f) {
		// remove immuteable flag that linux sets on all unknown efi variables
		ioctl(fileno(f), FS_IOC_GETFLAGS, &attr);
		attr &= ~FS_IMMUTABLE_FL;
		ioctl(fileno(f), FS_IOC_SETFLAGS, &attr);

		if (remove(REBARPS) != 0) {
			std::cout << "Failed to remove old variable\n";
			return false;
		}
	}

	f = fopen(REBARPS, "wb");

	rVar.attr = VARIABLE_ATTRIBUTE_NON_VOLATILE | VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS | VARIABLE_ATTRIBUTE_RUNTIME_ACCESS;
	rVar.value = rBarState;

	return fwrite(&rVar, sizeof(rVar), 1, f) == 1;;
}
#endif

void pause()
{
// Linux will probably be run from terminal not requiring this
#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
	cout << "You can close the app now\n";
        cin.get();
#endif
}

int main(int argc, char const *argv[])
try
{
	int ret = 0;
	string i;
	uint8_t reBarState;

	cout << "NvStrapsReBar, based on ReBarState (c) 2023 xCuri0\n\n";

	if (!CheckPriviledge())
                throw system_error(make_error_code(errc::permission_denied), "Failed to obtain EFI variable access try running as admin/root"s);

        runConfigurationWizard();

	reBarState = GetState();

	if (!notExist) {
		if (reBarState == 0)
			cout << "Current NvStrapsReBar " << +reBarState << " / Disabled\n";
		else
			if (reBarState == 32)
				cout << "Current NvStrapsReBar " << +reBarState << " / Unlimited\n";
			else
				cout << "Current NvStrapsReBar " << +reBarState << " / " << pow(2, reBarState) << " MB\n";
	}
	else
		cout << "NvStrapsReBar variable doesn't exist / Disabled. Enter a value to create it\n";

	cout << "\nVerify that 4G Decoding is enabled and CSM is disabled otherwise system will not POST with GPU.\n";
        cout << "If your NvStrapsReBar value keeps getting reset then check your system time.\n";
	cout << "\nIt is recommended to first try smaller sizes above 256MB in case an old BIOS doesn't support large BARs.\n";
	cout << "\nEnter NvStrapsReBar Value\n      0: Disabled \nAbove 0: Maximum BAR size set to 2^x MB \n     32: Unlimited BAR size\n\n";

	getline(cin, i);

        while (i | all && std::isspace(*i.crbegin()))
                i.pop_back();

        if (i.empty())
        {
                cerr << "No BAR size changes to apply\n";
                return EXIT_FAILURE;
        }

	if (stoi(i) < 0 || stoi(i) > 32)
                throw out_of_range("Invalid BAR size selector value");

	reBarState = stoi(i);

	if (reBarState < 20)
		if (reBarState == 0)
			cout << "Writing value of 0 / Disabled to NvStrapsReBar\n\n";
		else
			cout << "Writing value of " << +reBarState << " / " << pow(2, reBarState) << " MB to NvStrapsReBar\n\n";
	else
		cout << "Writing value to NvStrapsReBar\n\n";

	if (WriteState(reBarState))
        {
		cout << "Successfully wrote NvStrapsReBar UEFI variable\n";
		cout << "\nReboot for changes to take effect\n";
	}
	else
		cout << "Failed to write NvStrapsReBar UEFI variable\n";

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
