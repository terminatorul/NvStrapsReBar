#if !defined(NV_STRAPS_REBAR_STATE_CONFIG_MANAGER_ERROR_HH)
#define NV_STRAPS_REBAR_STATE_CONFIG_MANAGER_ERROR_HH

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

#include <utility>
#include <string>
#include <system_error>

class ConfigManagerErrorCategory: public std::error_category
{
public:
    char const *name() const noexcept override;
    std::string message(int error) const override;

protected:
    ConfigManagerErrorCategory() = default;

    friend error_category const &config_manager_error_category();
};

inline char const *ConfigManagerErrorCategory::name() const noexcept
{
    return "PnP Configuration Manager";
}

inline std::error_category const &config_manager_error_category()
{
    static ConfigManagerErrorCategory errorCategory;

    return errorCategory;
}

inline std::system_error make_config_ret(int error)
{
    return { error, config_manager_error_category() };
}

inline std::system_error make_config_ret(int error, char const *message)
{
    return { error, config_manager_error_category(), message };
}

inline std::system_error make_config_ret(int error, std::string const &message)
{
    return { error, config_manager_error_category(), message };
}

inline int validate_config_ret(int error)
{
    if (error && !std::uncaught_exceptions())
        throw make_config_ret(error);

    return error;
}

inline int validate_config_ret(int error, char const *message)
{
    if (error && !std::uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

template <int ...SUCCESS_VALUE>
    inline int validate_config_ret(int error)
{
    if (error && !(false || ... || (error == SUCCESS_VALUE)) && !std::uncaught_exceptions())
        throw make_config_ret(error);

    return error;
}

template <int ...SUCCESS_VALUES>
    inline int validate_config_ret(int error, char const *message)
{
    if (error && !(false || ... || (error == SUCCESS_VALUES)) && !std::uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

template <int ...SUCCESS_VALUES>
    inline int validate_config_ret(int error, std::string const &message)
{
    if (error && !(false || ... || (error == SUCCESS_VALUES)) && !std::uncaught_exceptions())
        throw make_config_ret(error, message);

    return error;
}

#endif          // defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#endif          // !defined(NV_STRAPS_REBAR_STATE_CONFIG_MANAGER_ERROR_HH)
