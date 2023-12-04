#if !defined(NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH)
#define NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH

#include <cstdint>
#include <string>
#include <vector>

struct DeviceInfo
{
    std::uint_least16_t vendorID, deviceID;
    std::uint_least8_t  bus, device, function;
    bool                busLocationSelector;
    std::uint_least8_t  barSizeSelector;
    std::uint_least64_t dedicatedVideoMemory;
    std::wstring        productName;
};

std::vector<DeviceInfo> &getDeviceList();

#endif          // !defined(NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH)
