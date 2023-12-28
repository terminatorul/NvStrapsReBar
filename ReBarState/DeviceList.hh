#if !defined(NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH)
#define NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH

#include <cstdint>
#include <string>
#include <vector>

struct DeviceInfo
{
    std::uint_least16_t vendorID, deviceID, subsystemVendorID, subsystemDeviceID;
    std::uint_least8_t  bus, device, function;
    bool                busLocationSelector;
    std::uint_least64_t currentBARSize;
    std::uint_least64_t dedicatedVideoMemory;
    std::wstring        productName;
};

std::wstring formatMemorySize(uint_least64_t size);
std::vector<DeviceInfo> const &getDeviceList();

#endif          // !defined(NVSTRAPS_REBAR_STATE_DEVICE_LIST_HH)
