export module WinApiError;

import std;
import NvStraps.WinAPI;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

using std::string;
using std::exchange;
using std::error_category;
using std::system_error;
using std::stringstream;
using std::hex;
using std::setfill;
using std::setw;
using std::endl;
using namespace std::literals::string_literals;

export class WinAPIErrorCategory: public error_category
{
public:
    char const* name() const noexcept override;
    string message(int error) const override;

protected:
    WinAPIErrorCategory() = default;

    friend error_category const &winapi_error_category();
};

export inline error_category const &winapi_error_category()
{
    static WinAPIErrorCategory errorCategory;

    return errorCategory;
}

export inline void check_last_error(DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw system_error(static_cast<int>(dwLastError), winapi_error_category());
}

export inline void check_last_error(DWORD dwLastError, char const* msg)
{
    if (dwLastError && !std::uncaught_exceptions())
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

export inline void check_last_error(DWORD dwLastError, string const& msg)
{
    if (dwLastError && !std::uncaught_exceptions())
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

export inline void check_last_error(char const* msg, DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

export inline void check_last_error(string const& msg, DWORD dwLastError = ::GetLastError())
{
    if (dwLastError && !std::uncaught_exceptions())
        throw system_error(static_cast<int>(dwLastError), winapi_error_category(), msg);
}

inline char const *WinAPIErrorCategory::name() const noexcept
{
    return "winapi";
}

string WinAPIErrorCategory::message(int error) const
{
    struct LocalMem
    {
        HLOCAL hLocal = nullptr;

        ~LocalMem()
        {
            ::LocalFree(exchange(hLocal, nullptr));
        }
    }
        localMessageBuffer;

    if (auto nLength = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, static_cast<DWORD>(error),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), static_cast<LPSTR>(static_cast<void *>(&localMessageBuffer.hLocal)), 0, nullptr))
    {
        return { static_cast<char const *>(static_cast<void *>(localMessageBuffer.hLocal)), nLength };
    }
    else
    {
        auto dwFormatMessageError = ::GetLastError();

        return static_cast<stringstream &>(stringstream { "Windows API error 0x"s } << hex << setfill('0') << setw(sizeof(DWORD) * 2u) << static_cast<DWORD>(error) << endl
                 << "(FormatError() returned 0x" << hex << setfill('0') << setw(sizeof(dwFormatMessageError) * 2) << dwFormatMessageError << ')').str();
    }
}

#endif
