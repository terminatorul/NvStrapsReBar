#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <iterator>
#include <algorithm>
#include <execution>
#include <ranges>

#include "NvStrapsConfig.h"

using std::begin;
using std::end;
using std::size;
using std::find_if;
using std::copy;

namespace execution = std::execution;

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
