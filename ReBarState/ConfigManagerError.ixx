export module ConfigManagerError;

import std;
import NvStraps.WinAPI;

using std::string;
using std::string_view;
using std::to_string;
using std::error_category;
using std::system_error;
using std::uncaught_exceptions;

using namespace std::literals::string_literals;

export class ConfigManagerErrorCategory: public error_category
{
public:
    char const *name() const noexcept override;
    string message(int error) const override;

protected:
    ConfigManagerErrorCategory() = default;

    friend error_category const &config_manager_error_category();
};

export error_category const &config_manager_error_category();
export system_error make_config_ret(int error);
export system_error make_config_ret(int error, char const *message);
export system_error make_config_ret(int error, string const &message);
export int validate_config_ret(int error);
export int validate_config_ret(int error, char const *message);

export template <int ...SUCCESS_VALUE>
    int validate_config_ret(int error);

export template <int ...SUCCESS_VALUES>
    int validate_config_ret(int error, char const *message);

export template <int ...SUCCESS_VALUES>
    int validate_config_ret(int error, std::string const &message);

inline char const *ConfigManagerErrorCategory::name() const noexcept
{
    return "PnP Configuration Manager";
}

inline error_category const &config_manager_error_category()
{
    static ConfigManagerErrorCategory errorCategory;

    return errorCategory;
}

inline system_error make_config_ret(int error)
{
    return { error, config_manager_error_category() };
}

inline system_error make_config_ret(int error, char const *message)
{
    return { error, config_manager_error_category(), message };
}

inline system_error make_config_ret(int error, string const &message)
{
    return { error, config_manager_error_category(), message };
}

inline int validate_config_ret(int error)
{
    if (error && !uncaught_exceptions())
        throw make_config_ret(error);

    return error;
}

inline int validate_config_ret(int error, char const *message)
{
    if (error && !uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

template <int ...SUCCESS_VALUE>
    inline int validate_config_ret(int error)
{
    if (error && !(false || ... || (error == SUCCESS_VALUE)) && !uncaught_exceptions())
        throw make_config_ret(error);

    return error;
}

template <int ...SUCCESS_VALUES>
    inline int validate_config_ret(int error, char const *message)
{
    if (error && !(false || ... || (error == SUCCESS_VALUES)) && !uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

template <int ...SUCCESS_VALUES>
    inline int validate_config_ret(int error, std::string const &message)
{
    if (error && !(false || ... || (error == SUCCESS_VALUES)) && !uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

module: private;

static char const *configurationManagerErrorMessage(int error)
{
    switch (error)
    {
    case CR_SUCCESS:
        return "Success";

    case CR_DEFAULT:
        return "Default";

    case CR_OUT_OF_MEMORY:
        return "Out of memory";

    case CR_INVALID_POINTER:
        return "Invalid pointer";

    case CR_INVALID_FLAG:
        return "Invalid flag";

    case CR_INVALID_DEVNODE:
    //case CR_INVALID_DEVINST:
        return "Invalid device";

    case CR_INVALID_RES_DES:
        return "Invalid resource descriptor";

    case CR_INVALID_LOG_CONF:
        return "Invalid logical configuration";

    case CR_INVALID_ARBITRATOR:
        return "Invalid arbitrator";

    case CR_INVALID_NODELIST:
        return "Invalid node list";

    case CR_DEVNODE_HAS_REQS:
        return "Device has reqs";

    case CR_INVALID_RESOURCEID:
        return "Invalid resource ID";

    case CR_DLVXD_NOT_FOUND:
        return "DLVXD not found";

    case CR_NO_SUCH_DEVNODE:
    //case CR_NO_SUCH_DEVINST:
        return "No such device";

    case CR_NO_MORE_LOG_CONF:
        return "No more logical configurations";

    case CR_NO_MORE_RES_DES:
        return "No more resource descriptors";

    case CR_ALREADY_SUCH_DEVNODE:
    //case CR_ALREADY_SUCH_DEVINST:
        return "Device already exists";

    case CR_INVALID_RANGE_LIST:
        return "Invalid range list";

    case CR_INVALID_RANGE:
        return "Invalid range";

    case CR_FAILURE:
        return "Failure";

    case CR_NO_SUCH_LOGICAL_DEV:
        return "No such logical device";

    case CR_CREATE_BLOCKED:
        return "Create blocked";

    case CR_NOT_SYSTEM_VM:
        return "Not system VM";

    case CR_REMOVE_VETOED:
        return "Remove vetoed";

    case CR_APM_VETOED:
        return "APM vetoed";

    case CR_INVALID_LOAD_TYPE:
        return "Invalid load type";

    case CR_BUFFER_SMALL:
        return "Buffer too small";

    case CR_NO_ARBITRATOR:
        return "No arbitrator";

    case CR_NO_REGISTRY_HANDLE:
        return "No registry handle";

    case CR_REGISTRY_ERROR:
        return "Registry error";

    case CR_INVALID_DEVICE_ID:
        return "Invalid device ID";

    case CR_INVALID_DATA:
        return "Invalid data";

    case CR_INVALID_API:
        return "Invalid API";

    case CR_DEVLOADER_NOT_READY:
        return "Device loader not ready";

    case CR_NEED_RESTART:
        return "Need restart";

    case CR_NO_MORE_HW_PROFILES:
        return "No more hardware profiles";

    case CR_DEVICE_NOT_THERE:
        return "Device not there";

    case CR_NO_SUCH_VALUE:
        return "No such value";

    case CR_WRONG_TYPE:
        return "Wrong type";

    case CR_INVALID_PRIORITY:
        return "Invalid priority";

    case CR_NOT_DISABLEABLE:
        return "Not disableble";

    case CR_FREE_RESOURCES:
        return "Free resources";

    case CR_QUERY_VETOED:
        return "Query vetoed";

    case CR_CANT_SHARE_IRQ:
        return "Cannot share IRQ";

    case CR_NO_DEPENDENT:
        return "No dependent";

    case CR_SAME_RESOURCES:
        return "Same resources";

    case CR_NO_SUCH_REGISTRY_KEY:
        return "No such registry key";

    case CR_INVALID_MACHINENAME:
        return "Invalid machine name";

    case CR_REMOTE_COMM_FAILURE:
        return "Remote communication failure";

    case CR_MACHINE_UNAVAILABLE:
        return "Machine unavailable";

    case CR_NO_CM_SERVICES:
        return "No ConfigManager services";

    case CR_ACCESS_DENIED:
        return "Access denied";

    case CR_CALL_NOT_IMPLEMENTED:
        return "Call not implemented";

    case CR_INVALID_PROPERTY:
        return "Invalid property";

    case CR_DEVICE_INTERFACE_ACTIVE:
        return "Device interface active";

    case CR_NO_SUCH_DEVICE_INTERFACE:
        return "No such device interface";

    case CR_INVALID_REFERENCE_STRING:
        return "Invalid reference string";

    case CR_INVALID_CONFLICT_LIST:
        return "Invalid conflict list";

    case CR_INVALID_INDEX:
        return "Invalid index";

    case CR_INVALID_STRUCTURE_SIZE:
        return "Invalid structure size";

    default:
        return "Configuration Manager error";
    }

    return "Configuration Manager error";
}

inline string ConfigManagerErrorCategory::message(int error) const
{
    return "PnP Configuration Manager error code " + to_string(error) + ": " + configurationManagerErrorMessage(error);
}

// vim: ft=cpp
