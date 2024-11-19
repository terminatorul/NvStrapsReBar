
#include "NvStrapsConfig.h"

import std;
import NvStraps.WinAPI;
import WinApiError;

using std::begin;
using std::end;
using std::size;
using std::find_if;
using std::copy;
using std::system_error;

namespace views = std::ranges::views;
namespace execution = std::execution;
using namespace std::literals::string_literals;

bool NvStrapsConfig::setGPUSelector(uint_least8_t barSizeSelector, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    NvStraps_GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn,
        .barSizeSelector = barSizeSelector,
    .overrideBarSizeMask = 0u
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
        if (it->barSizeSelector != barSizeSelector)
        {
            dirty = true;
            it->barSizeSelector = barSizeSelector;
        }

    return true;
}

bool NvStrapsConfig::setBarSizeMaskOverride(bool sizeMaskOverride, uint_least16_t deviceID, uint_least16_t subsysVenID, uint_least16_t subsysDevID, uint_least8_t bus, uint_least8_t dev, uint_least8_t fn)
{
    NvStraps_GPUSelector gpuSelector
    {
        .deviceID = deviceID,
        .subsysVendorID = subsysVenID,
        .subsysDeviceID = subsysDevID,
        .bus = bus,
        .device = dev,
        .function = fn,
        .barSizeSelector = BarSizeSelector_None,
    .overrideBarSizeMask = sizeMaskOverride ? (uint_least8_t)0x01u : (uint_least8_t)0xFFu
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
        if (it->overrideBarSizeMask != gpuSelector.overrideBarSizeMask)
        {
            dirty = true;
            it->overrideBarSizeMask = gpuSelector.overrideBarSizeMask;
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

