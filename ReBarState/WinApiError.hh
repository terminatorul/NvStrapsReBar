#if !defined(NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH)
#define NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

#include <string>
#include <system_error>

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

#endif          // defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#endif          // !defined(NV_STRAPS_REBAR_STATE_WINAPI_ERROR_HH)
