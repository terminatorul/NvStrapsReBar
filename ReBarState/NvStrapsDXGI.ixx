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

export namespace dxgi
{
    using IID = IID;
    using GUID = GUID;

    using IFactory = IDXGIFactory;
    using IAdapter = IDXGIAdapter;
    using ADAPTER_DESC = DXGI_ADAPTER_DESC;

    auto const &IID_IFactory = ::IID_IDXGIFactory;
    auto const &CreateFactory = ::CreateDXGIFactory;

    constexpr auto const ERROR_NOT_FOUND = DXGI_ERROR_NOT_FOUND;
}
