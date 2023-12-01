/*
Copyright (c) 2022-2023 xCuri0 <zkqri0@gmail.com>
SPDX-License-Identifier: MIT
*/

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdint>
#include <exception>
#include <system_error>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#else
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif

#include "DeviceList.hh"

#undef max

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

#define VNAME NvStrapsReBar
#define VGUID 481893F5-2436-4FD5-9D5A-69B121C3F0BA

#define VARIABLE_ATTRIBUTE_NON_VOLATILE 0x00000001
#define VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS 0x00000002
#define VARIABLE_ATTRIBUTE_RUNTIME_ACCESS 0x00000004

using std::exception;
using std::system_error;
using std::max;
using std::uint_least8_t;
using std::uint_least64_t;
using std::size_t;
using std::string;
using std::cout;
using std::cerr;
using std::wcout;
using std::dec;
using std::hex;
using std::uppercase;
using std::left;
using std::right;
using std::setw;
using std::setfill;
using std::endl;
using std::string;
using std::vector;
using std::wstring;
using std::to_string;
using std::to_wstring;
using namespace std::literals::string_literals;

bool notExist = false;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

wstring formatMemorySize(uint_least64_t memorySize)
{
    if (!memorySize)
        return L"    "s;

    if (memorySize < 1024u * 1024u * 1024u)
        return to_wstring((memorySize + 512u * 1024u) / (1024u * 1024u)) + L"M"s;

    return to_wstring((memorySize + 512u * 1024u * 1024u) / (1024u * 1024u * 1024u)) + L"G"s;
}

wstring formatLocation(DeviceInfo const &devInfo)
{
    return L"bus: "s + to_wstring(devInfo.bus) + L", dev: "s + to_wstring(devInfo.device) + L", fn: "s + to_wstring(devInfo.function);
}

wstring formatBarSize(uint_least8_t barSizeSelector)
{
    switch (barSizeSelector)
    {
    case 0:
        return L" "s;
    case 1:
        return L"512 MB"s;
    case 2:
        return L"1 GB"s;
    case 3:
        return L"2 GB"s;
    case 4:
        return L"4 GB"s;
    case 5:
        return L"8 GB"s;
    case 6:
        return L"16 GB"s;
    case 7:
        return L"32 GB"s;
    case 8:
        return L"64 GB"s;
    case 9:
        return L"128 GB"s;
    case 10:
        return L"256 GB"s;
    case 11:
        return L"512 GB"s;
    default:
        return L" "s;
    }

    return L" "s;
}

void showLocalGPUs(vector<DeviceInfo> const &deviceSet)
{
    if (deviceSet.empty())
    {
        cerr << "No display adapters found.\n";
        return;
    }

    auto
        nMaxLocationSize = "Pci bus location"s.size(),
        nMaxBarSize = "BAR"s.size(),
        nMaxVRAMSize = "VRAM"s.size(),
        nMaxNameSize = "Product Name"s.size();

    for (auto const &deviceInfo: deviceSet)
    {
        nMaxLocationSize = max(nMaxLocationSize, formatLocation(deviceInfo).size());
        nMaxBarSize = max(nMaxBarSize, formatBarSize(deviceInfo.barSizeSelector).size());
        nMaxVRAMSize = max(nMaxVRAMSize, formatMemorySize(deviceInfo.dedicatedVideoMemory).size());
        nMaxNameSize = max(nMaxNameSize, deviceInfo.productName.size());
    }


    wcout << L"+----+-----------+-"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxBarSize, L'-') << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n"s;
    wcout << L"| Nr | PCI ID    | "s << setw(nMaxLocationSize) << left << L"PCI bus location"s << L" | "s << setw(nMaxBarSize) << L"BAR"s << L" | "s << setw(nMaxVRAMSize) << L"VRAM"s << L" | "s << setw(nMaxNameSize) << L"Product Name"s << L" |\n"s;
    wcout << L"+----+-----------+-"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxBarSize, L'-') << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n"s;

    unsigned i = 1u;

    for (auto const &deviceInfo: deviceSet)
    {
        wcout << L"| "s << dec << right << setw(2u) << setfill(L' ') << i++;
        wcout << L" | "s << hex << setw(sizeof(WORD) * 2u) << setfill(L'0') << uppercase << deviceInfo.vendorID << L':' << hex << setw(sizeof(WORD) * 2u) << setfill(L'0') << deviceInfo.deviceID;
        wcout << L" | "s << right << setw(nMaxLocationSize) << formatLocation(deviceInfo);
        wcout << L" | "s << dec << setw(nMaxBarSize) << setfill(L' ') << formatBarSize(deviceInfo.barSizeSelector);
        wcout << L" | "s << dec << setw(nMaxVRAMSize) << setfill(L' ') << formatMemorySize(deviceInfo.dedicatedVideoMemory);
        wcout << L" | "s << left << setw(nMaxNameSize) << deviceInfo.productName;
        wcout << L" |\n"s;
    }

    wcout << L"+----+-----------+-"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxBarSize, L'-') << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n\n";
}

bool CheckPriviledge()
{
	DWORD len;
	HANDLE hTok;
	TOKEN_PRIVILEGES tokp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hTok)) {
		return FALSE;
	}

	LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tokp.Privileges[0].Luid);
	tokp.PrivilegeCount = 1;
	tokp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hTok, FALSE, &tokp, 0, NULL, &len);

	if (GetLastError() != ERROR_SUCCESS) {
		std::cout << "Failed to obtain SE_SYSTEM_ENVIRONMENT_NAME\n";
		return FALSE;
	}
	std::cout << "Obtained SE_SYSTEM_ENVIRONMENT_NAME\n";
	return TRUE;
}

uint8_t GetState() {
	UINT8 rBarState;
	DWORD rSize;

	const TCHAR name[] = TEXT("NvStrapsReBar");
	const TCHAR guid[] = TEXT("{481893F5-2436-4FD5-9D5A-69B121C3F0BA}");

	rSize = GetFirmwareEnvironmentVariable(name, guid, &rBarState, 1);

	if (rSize == 1)
		return rBarState;
	else {
		notExist = true;
		return 0;
	}
}

bool WriteState(uint8_t rBarState) {
	DWORD size = sizeof(UINT8);
	DWORD dwAttributes = VARIABLE_ATTRIBUTE_NON_VOLATILE | VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS | VARIABLE_ATTRIBUTE_RUNTIME_ACCESS;

	const TCHAR name[] = TEXT("NvStrapsReBar");
	const TCHAR guid[] = TEXT("{481893F5-2436-4FD5-9D5A-69B121C3F0BA}");

	return SetFirmwareEnvironmentVariableEx(name, guid, &rBarState, size, dwAttributes) != 0;
}
// Linux
#else

#define REBARPATH /sys/firmware/efi/efivars/VNAME-VGUID
#define REBARPS STR(REBARPATH)

struct __attribute__((__packed__)) rebarVar {
	uint32_t attr;
	uint8_t value;
};

bool showLocalGPUs()
{
}

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


int main(int argc, char const *argv[])
try
{
	int ret = 0;
	std::string i;
	uint8_t reBarState;

	std::cout << "NvStrapsReBar, based on ReBarState (c) 2023 xCuri0\n\n";

        showLocalGPUs(getDeviceList());

	if (!CheckPriviledge()) {
		std::cout << "Failed to obtain EFI variable access try running as admin/root\n";
		ret = 1;
		goto exit;
	}

	reBarState = GetState();

	if (!notExist) {
		if (reBarState == 0)
			std::cout << "Current NvStrapsReBar " << +reBarState << " / Disabled\n";
		else
			if (reBarState == 32)
				std::cout << "Current NvStrapsReBar " << +reBarState << " / Unlimited\n";
			else
				std::cout << "Current NvStrapsReBar " << +reBarState << " / " << std::pow(2, reBarState) << " MB\n";
	}
	else {
		std::cout << "NvStrapsReBar variable doesn't exist / Disabled. Enter a value to create it\n";
	}

	std::cout << "\nVerify that 4G Decoding is enabled and CSM is disabled otherwise system will not POST with GPU. If your NvStrapsReBar value keeps getting reset then check your system time.\n";
	std::cout << "\nIt is recommended to first try smaller sizes above 256MB in case BIOS doesn't support large BARs.\n";
	std::cout << "\nEnter NvStrapsReBar Value\n      0: Disabled \nAbove 0: Maximum BAR size set to 2^x MB \n     32: Unlimited BAR size\n\n";

	std::getline(std::cin, i);

	if (std::stoi(i) > 32) {
		std::cout << "Invalid value\n";
		goto exit;
	}
	reBarState = std::stoi(i);

	if (reBarState < 20)
		if (reBarState == 0)
			std::cout << "Writing value of 0 / Disabled to NvStrapsReBar\n\n";
		else
			std::cout << "Writing value of " << +reBarState << " / " << std::pow(2, reBarState) << " MB to NvStrapsReBar\n\n";
	else
		std::cout << "Writing value to NvStrapsReBar\n\n";

	if (WriteState(reBarState)) {
		std::cout << "Successfully wrote NvStrapsReBar UEFI variable\n";
		std::cout << "\nReboot for changes to take effect\n";
	}
	else {
		std::cout << "Failed to write NvStrapsReBar UEFI variable\n";
		#ifdef _MSC_VER
		std::cout << "GetLastError: " << GetLastError() << "\n";
		#endif
	}

	// Linux will probably be run from terminal not requiring this
	#ifdef _MSC_VER
	std::cout << "You can close the app now\n";
exit:
	std::cin.get();
	#else
exit:
	#endif
	return ret;
}
catch (system_error const &ex)
{
    cerr << "Application error: " << ex.code().message() << endl;
    return 252u;
}
catch (exception const &ex)
{
    cerr << "Application error: " << ex.what() << endl;
    return 253u;
}
catch (...)
{
    cerr << "Application error!" << endl;
    return 254u;
}