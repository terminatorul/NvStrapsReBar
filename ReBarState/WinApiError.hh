#if !defined(NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH)
#define NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

#include <windef.h>
#include <errhandlingapi.h>

#include <exception>
#include <system_error>
#include <string>

class WinAPIErrorCategory: public std::error_category
{
public:
    char const *name() const noexcept override;
    std::string message(int error) const override;

protected:
    WinAPIErrorCategory() = default;

    friend error_category const &winapi_error_category();
};

inline char const *WinAPIErrorCategory::name() const noexcept
{
    return "winapi";
}

inline std::error_category const &winapi_error_category()
{
    static WinAPIErrorCategory errorCategory;

    return errorCategory;
}

inline void check_last_error(DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw std::system_error(static_cast<int>(dwLastError), winapi_error_category());
}

inline void check_last_error(DWORD dwLastError, char const *msg)
{
    if (dwLastError && !std::uncaught_exceptions())
        throw std::system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

inline void check_last_error(DWORD dwLastError, std::string const &msg)
{
    if (dwLastError && !std::uncaught_exceptions())
        throw std::system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

inline void check_last_error(char const *msg, DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw std::system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

inline void check_last_error(std::string const &msg, DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw std::system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

#endif          // defined(WINDOWS_SOURCE)
#endif          // !defined(NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH)
