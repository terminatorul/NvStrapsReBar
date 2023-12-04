
#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)

#if defined(_M_AMD64) && !defined(_AMD64_)
# define _AMD64_
#endif

#include <windef.h>
#include <winbase.h>

#include <utility>
#include <sstream>
#include <iomanip>

#include "WinApiError.hh"

using std::stringstream;
using std::hex;
using std::setfill;
using std::setw;
using std::endl;
using namespace std::literals::string_literals;

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