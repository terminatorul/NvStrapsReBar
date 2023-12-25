#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <windef.h>
#include <winerror.h>

#include <iterator>
#include <system_error>
#include <string>
#include <algorithm>
#include <execution>
#include <ranges>

#include "WinApiError.hh"
#include "NvStrapsConfig.h"

using std::begin;
using std::end;
using std::size;
using std::find_if;
using std::copy;
using std::system_error;

namespace execution = std::execution;
using namespace std::literals::string_literals;

bool NvStrapsConfig::setGPUSelector(UINT8 barSizeSelector, UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn)
{
    NvStraps_GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn,
        .barSizeSelector = barSizeSelector
    };

    auto end_it = begin(GPUs) + nGPUSelector;
    auto it = find_if(execution::par_unseq, begin(GPUs), end_it, [&gpuSelector](auto const &selector)
        {
            return selector.deviceMatch(gpuSelector.deviceID)
                 && selector.subsystemMatch(gpuSelector.subsysVendorID, gpuSelector.subsysDeviceID)
                 && selector.busLocationMatch(gpuSelector.bus, gpuSelector.device, gpuSelector.function);
        });

    if (it == end_it)
        if (nGPUSelector >= size(GPUs))
            return false;
        else
        {
            dirty = true;
            GPUs[nGPUSelector++] = gpuSelector;
        }
    else
        if (*it != gpuSelector)
        {
            dirty = true;
            *it = gpuSelector;
        }

    return true;
}

bool NvStrapsConfig::clearGPUSelector(UINT16 deviceID, UINT16 subsysVenID, UINT16 subsysDevID, UINT8 bus, UINT8 dev, UINT8 fn)
{
    NvStraps_GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn
    };

    auto end_it = begin(GPUs) + nGPUSelector;
    auto it = find_if(execution::par_unseq, begin(GPUs), end_it, [&gpuSelector](auto const &selector)
        {
            return selector.deviceMatch(gpuSelector.deviceID)
                 && selector.subsystemMatch(gpuSelector.subsysVendorID, gpuSelector.subsysDeviceID)
                 && selector.busLocationMatch(gpuSelector.bus, gpuSelector.device, gpuSelector.function);
        });

    if (it == end_it)
        return false;

    dirty = true;
    copy(it + 1u, end_it, it);
    nGPUSelector--;

    return true;
}

NvStrapsConfig &GetNvStrapsConfig(bool reload)
{
    auto dwLastError = DWORD { ERROR_SUCCESS };
    auto strapsConfig = GetNvStrapsConfig(reload, &dwLastError);

    return dwLastError == ERROR_SUCCESS ? *strapsConfig :
        throw system_error { static_cast<int>(dwLastError), winapi_error_category(), "Error loading configuration from EFI variable"s };
}

void SaveNvStrapsConfig()
{
    auto dwLastError = DWORD { ERROR_SUCCESS };

    SaveNvStrapsConfig(&dwLastError);

    if (dwLastError != ERROR_SUCCESS)
        throw system_error { static_cast<int>(dwLastError), winapi_error_category(), "Error saving configuration to EFI variable"s };
}
