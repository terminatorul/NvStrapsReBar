#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "NvStrapsPciConfig.hh"
#include "TextWizardPage.hh"

#undef min
#undef max

using std::uint_least8_t;
using std::uint_least64_t;
using std::wstring;
using std::vector;
using std::cerr;
using std::wcout;
using std::wostringstream;
using std::hex;
using std::dec;
using std::left;
using std::right;
using std::uppercase;
using std::setw;
using std::setfill;
using std::min;
using std::max;

using namespace std::literals::string_literals;

static wstring formatDirectMemorySize(uint_least64_t memorySize)
{
    if (!memorySize)
        return L"    "s;

    return formatMemorySize(memorySize);
}

static wstring formatLocation(DeviceInfo const &devInfo)
{
    wostringstream str;

    str << hex << uppercase << setfill(L'0') << right;
    str << setw(2u) << devInfo.bus << L':' << setw(2u) << devInfo.device << L'.' << devInfo.function;

    return str.str();
}

static wstring formatDirectBARSize(uint_least64_t size)
{
    if (size < 64u * 1024u * 1024u)
        return { };

    return formatMemorySize(size);
}

static wstring formatBarSizeSelector(uint_least8_t barSizeSelector)
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

static wchar_t formatSelectorChar(DeviceInfo const &devInfo, bool showBusLocationColumn)
{
    if (devInfo.barSizeSelector && devInfo.busLocationSelector == showBusLocationColumn)
        return L'*';

    return L' ';
}

static void showLocalGPUs(vector<DeviceInfo> const &deviceSet)
{
    if (deviceSet.empty())
    {
        cerr << "No display adapters found.\n";
        return;
    }

    auto
        nMaxLocationSize = max("PCI location"s.size(), "bus:dev.fn"s.size()),
        nMaxTargetBarSize = max("Target"s.size(), "BAR size"s.size()),
        nMaxCurrentBarSize = max("current"s.size(), "BAR size"s.size()),
        nMaxVRAMSize = "VRAM"s.size(),
        nMaxNameSize = "Product Name"s.size();

    for (auto const &deviceInfo: deviceSet)
    {
        nMaxLocationSize = max(nMaxLocationSize, formatLocation(deviceInfo).size());
        nMaxCurrentBarSize = max<uint_least64_t>(nMaxCurrentBarSize, formatMemorySize(deviceInfo.currentBARSize).size());
        nMaxTargetBarSize = max(nMaxTargetBarSize, formatBarSizeSelector(deviceInfo.barSizeSelector).size());
        nMaxVRAMSize = max(nMaxVRAMSize, formatDirectMemorySize(deviceInfo.dedicatedVideoMemory).size());
        nMaxNameSize = max(nMaxNameSize, deviceInfo.productName.size());
    }

    wcout << L"+----+------------+------------+--"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxTargetBarSize, L'-') << L"-+-"s << wstring(nMaxCurrentBarSize, L'-') << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n"s;
    wcout << L"|    |   PCI ID   |  subsystem |  "s << left << setw(nMaxLocationSize) << L"PCI location"s << L" | "s << left << setw(nMaxTargetBarSize) << " Target " << L" | "s << setw(nMaxTargetBarSize) << left << "Current" << L" | "s << wstring(nMaxVRAMSize, L' ') << L" | "s << wstring(nMaxNameSize, L' ') << L" |\n"s;
    wcout << L"| Nr |  VID:DID   |   VID:DID  |  "s << setw(nMaxLocationSize) << left << "bus:dev.fn" << L" | "s << setw(nMaxTargetBarSize) << L"BAR size"s << L" | "s << setw(nMaxCurrentBarSize) << L"BAR size"s << L" | "s << setw(nMaxVRAMSize) << L"VRAM"s << L" | "s << setw(nMaxNameSize) << L"Product Name"s << L" |\n"s;
    wcout << L"+----+------------+------------+--"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxTargetBarSize, L'-') << L"-+-"s << wstring(nMaxCurrentBarSize, L'-') << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n"s;

    unsigned i = 1u;

    for (auto const &deviceInfo: deviceSet)
    {
        wcout << L"| "s << dec << right << setw(2u) << setfill(L' ') << i++;
        wcout << L" | "s << formatSelectorChar(deviceInfo, false) << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.vendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << deviceInfo.deviceID;
        wcout << L" | "s << formatSelectorChar(deviceInfo, false) << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemVendorID << L':' << hex << setw(WORD_SIZE * 2u) << setfill(L'0') << uppercase << deviceInfo.subsystemDeviceID;
        wcout << L" | "s << formatSelectorChar(deviceInfo, true) << right << setw(nMaxLocationSize) << setfill(L' ') << left << formatLocation(deviceInfo);
        wcout << L" | "s << dec << setw(nMaxTargetBarSize) << right << setfill(L' ') << formatBarSizeSelector(deviceInfo.barSizeSelector);
        wcout << L" | "s << dec << setw(nMaxCurrentBarSize) << right << setfill(L' ') << formatDirectBARSize(deviceInfo.currentBARSize);
        wcout << L" | "s << dec << setw(nMaxVRAMSize) << right << setfill(L' ') << formatDirectMemorySize(deviceInfo.dedicatedVideoMemory);
        wcout << L" | "s << left << setw(nMaxNameSize) << deviceInfo.productName;
        wcout << L" |\n"s;
    }

    wcout << L"+----+------------+------------+--"s << wstring(nMaxLocationSize, L'-') << L"-+-"s << wstring(nMaxTargetBarSize, L'-') << L"-+-"s << wstring(nMaxCurrentBarSize, L'-')  << L"-+-"s << wstring(nMaxVRAMSize, L'-') << L"-+-"s << wstring(nMaxNameSize, L'-') << L"-+\n\n";
}

void showWizardPage(std::vector<DeviceInfo> const &devices)
{
    showLocalGPUs(devices);
}