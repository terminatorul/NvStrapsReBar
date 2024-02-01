module;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) && !defined(_AMD64_)
#  define _AMD64_
# endif
# include <windef.h>
# include <initguid.h>
# include <dxgi.h>
#endif

export module NvStraps.DXGI;

#undef ERROR_NOT_FOUND

namespace dxgi
{
    export using IID = IID;
    export using GUID = GUID;

    export using IFactory = IDXGIFactory;
    export using IAdapter = IDXGIAdapter;
    export using ADAPTER_DESC = DXGI_ADAPTER_DESC;

    export auto const &IID_IFactory = ::IID_IDXGIFactory;
    export auto const &CreateFactory = ::CreateDXGIFactory;

    export constexpr auto const ERROR_NOT_FOUND = DXGI_ERROR_NOT_FOUND;
}
