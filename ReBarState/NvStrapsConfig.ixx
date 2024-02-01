module;

#include "NvStrapsConfig.h"
#include "NvStrapsConfig.hh"

export module NvStrapsConfig;

import LocalAppConfig;

export using ::TARGET_GPU_VENDOR_ID;
export using ::TARGET_PCI_BAR_SIZE;
export using enum ::TARGET_PCI_BAR_SIZE;
export using ::ConfigPriority;
export using ::NvStraps_BarSize;
export using ::NvStraps_GPUSelector;
export using ::NvStraps_GPUConfig;
export using ::NvStraps_BridgeConfig;
export using ::NvStrapsConfig;

export NvStrapsConfig &GetNvStrapsConfig(bool reload = false)
{
    auto errorCode = ERROR_CODE { ERROR_CODE_SUCCESS };
    auto strapsConfig = GetNvStrapsConfig(reload, &errorCode);

    return errorCode == ERROR_CODE_SUCCESS ? *strapsConfig :
        throw system_error(static_cast<int>(errorCode), winapi_error_category(), "Error loading configuration from "s + NvStrapsConfig_VarName + " EFI variable"s);
}

export void SaveNvStrapsConfig()
{
    auto errorCode = ERROR_CODE { ERROR_CODE_SUCCESS };

    SaveNvStrapsConfig(&errorCode);

    if (errorCode != ERROR_CODE_SUCCESS)
	throw system_error { static_cast<int>(errorCode), winapi_error_category(), "Error saving configuration to "s + NvStrapsConfig_VarName + " EFI variable"s };
}

