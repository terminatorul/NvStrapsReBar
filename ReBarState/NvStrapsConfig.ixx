module;

#include "NvStrapsConfig.h"
#include "NvStrapsConfig.hh"

export module NvStrapsConfig;

import std;
import LocalAppConfig;

using std::wstring;
using std::function;

export using ::TARGET_GPU_VENDOR_ID;
export using ::TARGET_PCI_BAR_SIZE;
export using enum ::TARGET_PCI_BAR_SIZE;
export using ::ConfigPriority;
export using ::NvStraps_BarSize;
export using ::NvStraps_GPUSelector;
export using ::NvStraps_GPUConfig;
export using ::NvStraps_BridgeConfig;
export using ::NvStrapsConfig;

export NvStrapsConfig &GetNvStrapsConfig(bool reload = false);
export void SaveNvStrapsConfig();
export void ShowNvStrapsConfig(function<void (wstring const &)> show);

module: private;

using std::to_wstring;
namespace views = std::views;

NvStrapsConfig &GetNvStrapsConfig(bool reload)
{
    auto errorCode = ERROR_CODE { ERROR_CODE_SUCCESS };
    auto strapsConfig = GetNvStrapsConfig(reload, &errorCode);

    return errorCode == ERROR_CODE_SUCCESS
    ? *strapsConfig
    : throw system_error(static_cast<int>(errorCode), winapi_error_category(), "Error loading configuration from "s + NvStrapsConfig_VarName + " EFI variable"s);
}

void SaveNvStrapsConfig()
{
    auto errorCode = ERROR_CODE { ERROR_CODE_SUCCESS };

    SaveNvStrapsConfig(&errorCode);

    if (errorCode != ERROR_CODE_SUCCESS)
    throw system_error { static_cast<int>(errorCode), winapi_error_category(), "Error saving configuration to "s + NvStrapsConfig_VarName + " EFI variable"s };
}

static wchar_t const hexDigits[16 + 1] = L"0123456789ABCDEF";

static wstring formatPCI_ID(uint_least16_t pciID)
{
    wstring hexStr(WORD_SIZE * 2u, L' ');

    for (auto &ch: hexStr)
    ch = hexDigits[pciID & 0x0Fu], pciID >>= 4u;

    return wstring { hexStr.rbegin(), hexStr.rend() };
}

static wstring formatAddress64(uint_least64_t addr, bool fCheckWidth = true)
{
    wstring hexStr(!fCheckWidth || addr > DWORD_BITMASK ? QWORD_SIZE * 2u + 3u : DWORD_SIZE * 2u + 1u, L' ');

    for (auto [i, ch]: hexStr | views::enumerate)
    if ((i + 1) % (WORD_SIZE * 2u + 1u) == 0)
        ch = L'\'';
    else
        ch = hexDigits[addr & 0x0000'000Fu], addr >>= 4u;

    return wstring { hexStr.rbegin(), hexStr.rend() };
}

static wstring formatHexByte(uint_least8_t val)
{
    return wstring { hexDigits[ val >> 4u & 0x0Fu], hexDigits[val & 0x0Fu] };
}

static wstring formatHexWord(uint_least16_t val)
{
    return wstring { hexDigits[val >> 4u + BYTE_BITSIZE & 0x0Fu], hexDigits[val >> BYTE_BITSIZE & 0x0Fu], hexDigits[val >> 4u & 0x0Fu], hexDigits[val & 0x0Fu] };
}

static wstring formatHexNibble(uint_least8_t val)
{
    wstring hexStr { hexDigits[val & 0x0Fu] };

    if (val > 0x0Fu)
    hexStr = hexDigits[val >> 4u & 0x0Fu] + hexStr;

    return hexStr;
}

void ShowNvStrapsConfig(function<void (wstring const &)> show)
{
    auto &&config = GetNvStrapsConfig();

    show(L"DXE Driver configuration:\n"s);
    show(L"\tisDirty:           "s + to_wstring(config.dirty) + L'\n');
    show(L"\tOptionFlags:       "s + L"0x"s + formatHexWord(config.nOptionFlags) + L'\n');
    show(L"\t                       - nGlobalEnable:      "s + to_wstring(config.isGlobalEnable()) + L'\n');
    show(L"\t                       - skipS3Resume:       "s + to_wstring(config.skipS3Resume()) + L'\n');
    show(L"\t                       - overrideBarSize:    "s + to_wstring(config.overrideBarSizeMask()) + L'\n');
    show(L"\t                       - hasSetupVarCRC:     "s + to_wstring(config.hasSetupVarCRC()) + L'\n');
    show(L"\t                       - disableSetupVarCRC: "s + to_wstring(!config.enableSetupVarCRC()) + L'\n');
    show(L"\tSetupVarCRC:       "s + L"0x"s + formatAddress64(config.nSetupVarCRC, false) + L'\n');
    show(L"\tnPciBarSize:       "s + to_wstring(config.nPciBarSize) + L'\n');
    show(L"\tnGPUSelectorCount: "s + to_wstring(config.nGPUSelector) + L'\n');

    for (auto const &&[i, gpuSelector]: config.GPUs | views::enumerate | views::take(config.nGPUSelector))
    {
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  deviceID:            "s + formatPCI_ID(gpuSelector.deviceID) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  subsysVendorID:      "s + formatPCI_ID(gpuSelector.subsysVendorID) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  subsysDeviceID:      "s + formatPCI_ID(gpuSelector.subsysDeviceID) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  bus:                 "s + formatHexByte(gpuSelector.bus) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  device:              "s + formatHexByte(gpuSelector.device) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  function:            "s + formatHexNibble(gpuSelector.function) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  barSizeSelector:     "s + to_wstring(gpuSelector.barSizeSelector) + L'\n');
    show(L"\t\tGPUSelector"s + to_wstring(i + 1) + L":  overridebarSizeMask: "s + to_wstring(gpuSelector.overrideBarSizeMask) + L'\n');
    show(L"\n"s);
    }

    show(L"\tnGPUConfigCount:   "s + to_wstring(config.nGPUConfig) + L'\n');

    for (auto const &&[i, gpuConfig]: config.gpuConfig | views::enumerate | views::take(config.nGPUConfig))
    {
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    deviceID:        "s + formatPCI_ID(gpuConfig.deviceID) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    subsysVendorID:  "s + formatPCI_ID(gpuConfig.subsysVendorID) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    subsysDeviceID:  "s + formatPCI_ID(gpuConfig.subsysDeviceID) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    bus:             "s + formatHexByte(gpuConfig.bus) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    device:          "s + formatHexByte(gpuConfig.device) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    function:        "s + formatHexNibble(gpuConfig.function) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    BAR0 base:       0x"s + formatAddress64(gpuConfig.bar0.base) + L'\n');
    show(L"\t\tGPUConfig"s + to_wstring(i + 1) + L":    BAR0 top:        0x"s + formatAddress64(gpuConfig.bar0.top) + L'\n');
    show(L"\n"s);
    }

    show(L"\tnBridgeCount:      "s + to_wstring(config.nBridgeConfig) + L'\n');

    for (auto const &&[i, bridgeConfig]: config.bridge | views::enumerate | views::take(config.nBridgeConfig))
    {
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": vendorID:        "s + formatPCI_ID(bridgeConfig.vendorID) + L'\n');
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": deviceID:        "s + formatPCI_ID(bridgeConfig.deviceID) + L'\n');
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": bus:             "s + formatHexByte(bridgeConfig.bridgeBus) + L'\n');
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": device:          "s + formatHexByte(bridgeConfig.bridgeDevice) + L'\n');
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": function:        "s + formatHexNibble(bridgeConfig.bridgeFunction) + L'\n');
    show(L"\t\tBridgeConfig"s + to_wstring(i + 1) + L": secondary bus:   "s + formatHexByte(bridgeConfig.bridgeSecondaryBus) + L'\n');
    show(L"\n"s);
    }
}

// vim:ft=cpp
