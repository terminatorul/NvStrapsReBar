module;

#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) || !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <windef.h>
#include <intsafe.h>
#include <winbase.h>
#include <winerror.h>
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <securitybaseapi.h>
#include <winnt.h>
#include <guiddef.h>
#include <winreg.h>
#include <winuser.h>
#include <wingdi.h>
#include <wincon.h>
#include <setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <cfgmgr32.h>

#if defined(__clang__)
#include <stdint.h>
#endif

export module NvStraps.WinAPI;
import std;

static constexpr auto const local_FALSE = FALSE;
static constexpr auto const local_TRUE  = TRUE;
static constexpr auto const dw_ERROR_SUCCESS = ERROR_SUCCESS;
static constexpr auto const dw_ERROR_NO_MORE_ITEMS = ERROR_NO_MORE_ITEMS;
static constexpr auto const  b_UINT8_MAX = UINT8_MAX;
static constexpr auto const  w_UINT16_MAX = UINT16_MAX;
static constexpr auto const dw_UINT32_MAX = UINT32_MAX;
static constexpr auto const qw_UINT64_MAX = UINT64_MAX;
static constexpr auto const e_LANG_NEUTRAL = LANG_NEUTRAL;
static constexpr auto const e_SUBLANG_NEUTRAL = SUBLANG_NEUTRAL;
static constexpr auto const
    f_FORMAT_MESSAGE_FROM_SYSTEM = FORMAT_MESSAGE_FROM_SYSTEM,
    f_FORMAT_MESSAGE_IGNORE_INSERTS = FORMAT_MESSAGE_IGNORE_INSERTS,
    f_FORMAT_MESSAGE_ALLOCATE_BUFFER = FORMAT_MESSAGE_ALLOCATE_BUFFER;

static           auto const f_INVALID_HANDLE_VALUE = INVALID_HANDLE_VALUE;
static constexpr auto const f_ALLOC_LOG_CONF = ALLOC_LOG_CONF;
#if defined(__clang__)
static constexpr auto const z_MType_Range = sizeof(Mem_Range_s);
static constexpr auto const z_MLType_Range = sizeof(Mem_Large_Range_s);
#else
static constexpr auto const z_MType_Range = MType_Range;
static constexpr auto const z_MLType_Range = MLType_Range;
#endif
static constexpr auto const
    local_ResType_None     = ResType_None,
    local_ResType_All      = ResType_All,
    local_ResType_Mem      = ResType_Mem,
    local_ResType_MemLarge = ResType_MemLarge,
    local_ResType_IO       = ResType_IO;
static constexpr auto const e_mMD_MemoryType = mMD_MemoryType;
static constexpr auto const e_mMD_Readable = mMD_Readable;
static constexpr auto const e_fMD_RAM = fMD_RAM;
static constexpr auto const e_fMD_ReadAllowed = fMD_ReadAllowed;
static constexpr auto const
    local_DEVPROP_TYPE_STRING = DEVPROP_TYPE_STRING,
    local_DEVPROP_TYPE_UINT16 = DEVPROP_TYPE_UINT16,
    local_DEVPROP_TYPE_UINT32 = DEVPROP_TYPE_UINT32,
    local_DEVPROP_TYPE_UINT64 = DEVPROP_TYPE_UINT64;

static constexpr auto const f_DIGCF_PRESENT = DIGCF_PRESENT;

static constexpr auto const
    local_TOKEN_ADJUST_PRIVILEGES = TOKEN_ADJUST_PRIVILEGES,
    local_TOKEN_QUERY = TOKEN_QUERY;

static constexpr auto const local_SE_SYSTEM_ENVIRONMENT_NAME = SE_SYSTEM_ENVIRONMENT_NAME;
static constexpr auto const local_SE_PRIVILEGE_ENABLED = SE_PRIVILEGE_ENABLED;

static constexpr auto const
    nvs_winapi_CR_SUCCESS                  = CR_SUCCESS,
    nvs_winapi_CR_DEFAULT                  = CR_DEFAULT,
    nvs_winapi_CR_OUT_OF_MEMORY            = CR_OUT_OF_MEMORY,
    nvs_winapi_CR_INVALID_POINTER          = CR_INVALID_POINTER,
    nvs_winapi_CR_INVALID_FLAG             = CR_INVALID_FLAG,
    nvs_winapi_CR_INVALID_DEVNODE          = CR_INVALID_DEVNODE,
    nvs_winapi_CR_INVALID_DEVINST          = CR_INVALID_DEVINST,
    nvs_winapi_CR_INVALID_RES_DES          = CR_INVALID_RES_DES,
    nvs_winapi_CR_INVALID_LOG_CONF         = CR_INVALID_LOG_CONF,
    nvs_winapi_CR_INVALID_ARBITRATOR       = CR_INVALID_ARBITRATOR,
    nvs_winapi_CR_INVALID_NODELIST         = CR_INVALID_NODELIST,
    nvs_winapi_CR_DEVNODE_HAS_REQS         = CR_DEVNODE_HAS_REQS,
    nvs_winapi_CR_INVALID_RESOURCEID       = CR_INVALID_RESOURCEID,
    nvs_winapi_CR_DLVXD_NOT_FOUND          = CR_DLVXD_NOT_FOUND,
    nvs_winapi_CR_NO_SUCH_DEVNODE          = CR_NO_SUCH_DEVNODE,
    nvs_winapi_CR_NO_SUCH_DEVINST          = CR_NO_SUCH_DEVINST,
    nvs_winapi_CR_NO_MORE_LOG_CONF         = CR_NO_MORE_LOG_CONF,
    nvs_winapi_CR_NO_MORE_RES_DES          = CR_NO_MORE_RES_DES,
    nvs_winapi_CR_ALREADY_SUCH_DEVNODE     = CR_ALREADY_SUCH_DEVNODE,
    nvs_winapi_CR_ALREADY_SUCH_DEVINST     = CR_ALREADY_SUCH_DEVINST,
    nvs_winapi_CR_INVALID_RANGE_LIST       = CR_INVALID_RANGE_LIST,
    nvs_winapi_CR_INVALID_RANGE            = CR_INVALID_RANGE,
    nvs_winapi_CR_FAILURE                  = CR_FAILURE,
    nvs_winapi_CR_NO_SUCH_LOGICAL_DEV      = CR_NO_SUCH_LOGICAL_DEV,
    nvs_winapi_CR_CREATE_BLOCKED           = CR_CREATE_BLOCKED,
    nvs_winapi_CR_NOT_SYSTEM_VM            = CR_NOT_SYSTEM_VM,
    nvs_winapi_CR_REMOVE_VETOED            = CR_REMOVE_VETOED,
    nvs_winapi_CR_APM_VETOED               = CR_APM_VETOED,
    nvs_winapi_CR_INVALID_LOAD_TYPE        = CR_INVALID_LOAD_TYPE,
    nvs_winapi_CR_BUFFER_SMALL             = CR_BUFFER_SMALL,
    nvs_winapi_CR_NO_ARBITRATOR            = CR_NO_ARBITRATOR,
    nvs_winapi_CR_NO_REGISTRY_HANDLE       = CR_NO_REGISTRY_HANDLE,
    nvs_winapi_CR_REGISTRY_ERROR           = CR_REGISTRY_ERROR,
    nvs_winapi_CR_INVALID_DEVICE_ID        = CR_INVALID_DEVICE_ID,
    nvs_winapi_CR_INVALID_DATA             = CR_INVALID_DATA,
    nvs_winapi_CR_INVALID_API              = CR_INVALID_API,
    nvs_winapi_CR_DEVLOADER_NOT_READY      = CR_DEVLOADER_NOT_READY,
    nvs_winapi_CR_NEED_RESTART             = CR_NEED_RESTART,
    nvs_winapi_CR_NO_MORE_HW_PROFILES      = CR_NO_MORE_HW_PROFILES,
    nvs_winapi_CR_DEVICE_NOT_THERE         = CR_DEVICE_NOT_THERE,
    nvs_winapi_CR_NO_SUCH_VALUE            = CR_NO_SUCH_VALUE,
    nvs_winapi_CR_WRONG_TYPE               = CR_WRONG_TYPE,
    nvs_winapi_CR_INVALID_PRIORITY         = CR_INVALID_PRIORITY,
    nvs_winapi_CR_NOT_DISABLEABLE          = CR_NOT_DISABLEABLE,
    nvs_winapi_CR_FREE_RESOURCES           = CR_FREE_RESOURCES,
    nvs_winapi_CR_QUERY_VETOED             = CR_QUERY_VETOED,
    nvs_winapi_CR_CANT_SHARE_IRQ           = CR_CANT_SHARE_IRQ,
    nvs_winapi_CR_NO_DEPENDENT             = CR_NO_DEPENDENT,
    nvs_winapi_CR_SAME_RESOURCES           = CR_SAME_RESOURCES,
    nvs_winapi_CR_NO_SUCH_REGISTRY_KEY     = CR_NO_SUCH_REGISTRY_KEY,
    nvs_winapi_CR_INVALID_MACHINENAME      = CR_INVALID_MACHINENAME,
    nvs_winapi_CR_REMOTE_COMM_FAILURE      = CR_REMOTE_COMM_FAILURE,
    nvs_winapi_CR_MACHINE_UNAVAILABLE      = CR_MACHINE_UNAVAILABLE,
    nvs_winapi_CR_NO_CM_SERVICES           = CR_NO_CM_SERVICES,
    nvs_winapi_CR_ACCESS_DENIED            = CR_ACCESS_DENIED,
    nvs_winapi_CR_CALL_NOT_IMPLEMENTED     = CR_CALL_NOT_IMPLEMENTED,
    nvs_winapi_CR_INVALID_PROPERTY         = CR_INVALID_PROPERTY,
    nvs_winapi_CR_DEVICE_INTERFACE_ACTIVE  = CR_DEVICE_INTERFACE_ACTIVE,
    nvs_winapi_CR_NO_SUCH_DEVICE_INTERFACE = CR_NO_SUCH_DEVICE_INTERFACE,
    nvs_winapi_CR_INVALID_REFERENCE_STRING = CR_INVALID_REFERENCE_STRING,
    nvs_winapi_CR_INVALID_CONFLICT_LIST    = CR_INVALID_CONFLICT_LIST,
    nvs_winapi_CR_INVALID_INDEX            = CR_INVALID_INDEX,
    nvs_winapi_CR_INVALID_STRUCTURE_SIZE   = CR_INVALID_STRUCTURE_SIZE;

namespace
{
    inline auto nvs_winapi_MAKELANGID(WORD langID, WORD subLangID) -> decltype(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
    {
        return MAKELANGID(langID, subLangID);
    }

    inline bool nvs_winapi_SUCCEEDED(HRESULT hr)
    {
        return SUCCEEDED(hr);
    }

    inline bool nvs_winapi_FAILED(HRESULT hr)
    {
        return SUCCEEDED(hr);
    }
}

#undef NULL
#undef FALSE
#undef TRUE
#undef UINT8_MAX
#undef UINT16_MAX
#undef UINT32_MAX
#undef UINT64_MAX
#undef ERROR_SUCCESS
#undef ERROR_NO_MORE_ITEMS
#undef INVALID_HANDLE_VALUE
#undef LANG_NEUTRAL
#undef SUBLANG_NEUTRAL
#undef MAKELANGID
#undef FORMAT_MESSAGE_FROM_SYSTEM
#undef FORMAT_MESSAGE_IGNORE_INSERTS
#undef FORMAT_MESSAGE_ALLOCATE_BUFFER
#undef ALLOC_LOG_CONF
#undef MType_Range
#undef MLType_Range
#undef ResType_All
#undef ResType_None
#undef ResType_IO
#undef ResType_Mem
#undef ResType_MemLarge
#undef mMD_MemoryType
#undef mMD_Readable
#undef fMD_RAM
#undef fMD_ReadAllowed
#undef DIGCF_PRESENT

#undef TOKEN_ADJUST_PRIVILEGES
#undef TOKEN_QUERY
#undef SE_SYSTEM_ENVIRONMENT_NAME
#undef SE_PRIVILEGE_ENABLED

#undef CR_SUCCESS
#undef CR_DEFAULT
#undef CR_OUT_OF_MEMORY
#undef CR_INVALID_POINTER
#undef CR_INVALID_FLAG
#undef CR_INVALID_DEVNODE
#undef CR_INVALID_DEVINST
#undef CR_INVALID_RES_DES
#undef CR_INVALID_LOG_CONF
#undef CR_INVALID_ARBITRATOR
#undef CR_INVALID_NODELIST
#undef CR_DEVNODE_HAS_REQS
#undef CR_INVALID_RESOURCEID
#undef CR_DLVXD_NOT_FOUND
#undef CR_NO_SUCH_DEVNODE
#undef CR_NO_SUCH_DEVINST
#undef CR_NO_MORE_LOG_CONF
#undef CR_NO_MORE_RES_DES
#undef CR_ALREADY_SUCH_DEVNODE
#undef CR_ALREADY_SUCH_DEVINST
#undef CR_INVALID_RANGE_LIST
#undef CR_INVALID_RANGE
#undef CR_FAILURE
#undef CR_NO_SUCH_LOGICAL_DEV
#undef CR_CREATE_BLOCKED
#undef CR_NOT_SYSTEM_VM
#undef CR_REMOVE_VETOED
#undef CR_APM_VETOED
#undef CR_INVALID_LOAD_TYPE
#undef CR_BUFFER_SMALL
#undef CR_NO_ARBITRATOR
#undef CR_NO_REGISTRY_HANDLE
#undef CR_REGISTRY_ERROR
#undef CR_INVALID_DEVICE_ID
#undef CR_INVALID_DATA
#undef CR_INVALID_API
#undef CR_DEVLOADER_NOT_READY
#undef CR_NEED_RESTART
#undef CR_NO_MORE_HW_PROFILES
#undef CR_DEVICE_NOT_THERE
#undef CR_NO_SUCH_VALUE
#undef CR_WRONG_TYPE
#undef CR_INVALID_PRIORITY
#undef CR_NOT_DISABLEABLE
#undef CR_FREE_RESOURCES
#undef CR_QUERY_VETOED
#undef CR_CANT_SHARE_IRQ
#undef CR_NO_DEPENDENT
#undef CR_SAME_RESOURCES
#undef CR_NO_SUCH_REGISTRY_KEY
#undef CR_INVALID_MACHINENAME
#undef CR_REMOTE_COMM_FAILURE
#undef CR_MACHINE_UNAVAILABLE
#undef CR_NO_CM_SERVICES
#undef CR_ACCESS_DENIED
#undef CR_CALL_NOT_IMPLEMENTED
#undef CR_INVALID_PROPERTY
#undef CR_DEVICE_INTERFACE_ACTIVE
#undef CR_NO_SUCH_DEVICE_INTERFACE
#undef CR_INVALID_REFERENCE_STRING
#undef CR_INVALID_CONFLICT_LIST
#undef CR_INVALID_INDEX
#undef CR_INVALID_STRUCTURE_SIZE
#undef DEVPROP_TYPE_STRING
#undef DEVPROP_TYPE_UINT16
#undef DEVPROP_TYPE_UINT32
#undef DEVPROP_TYPE_UINT64
#undef SUCCEEDED
#undef FAILED

export using ::PVOID;
export using ::BOOL;
export using ::BYTE;
export using ::WORD;
export using ::DWORD;
export using ::UINT;
export using ::SIZE_T;
export using ::PBYTE;
export using ::PWORD;
export using ::PDWORD;
export using ::CHAR;
export using ::WCHAR;
export using ::LPSTR;
export using ::PSTR;
export using ::LPCSTR;
export using ::PCSTR;
export using ::LPWSTR;
export using ::PWSTR;
export using ::LPCWSTR;
export using ::PCWSTR;
export using ::HANDLE;
export using ::HRESULT;
export using ::HGLOBAL;
export using ::HLOCAL;
export using ::LONG;
export using ::ULONG;
export using ::UINT8;
export using ::UINT16;
export using ::UINT32;
export using ::UINT64;

export constexpr auto const FALSE = local_FALSE;
export constexpr auto const TRUE  = local_TRUE;
export constexpr auto const ERROR_SUCCESS = dw_ERROR_SUCCESS;
export constexpr auto const ERROR_NO_MORE_ITEMS = dw_ERROR_NO_MORE_ITEMS;
export constexpr auto const UINT8_MAX = b_UINT8_MAX;
export constexpr auto const UINT16_MAX = w_UINT16_MAX;
export constexpr auto const UINT32_MAX = dw_UINT32_MAX;
export constexpr auto const UINT64_MAX = qw_UINT64_MAX;

export inline bool SUCCEEDED(HRESULT hr)
{
    return nvs_winapi_SUCCEEDED(hr);
}

export inline bool FAILED(HRESULT hr)
{
    return nvs_winapi_FAILED(hr);
}

export using ::TOKEN_PRIVILEGES;
export using ::GUID;
export using ::RES_DES;
export using ::RESOURCEID;
export using ::LOG_CONF;
export using ::CONFIGRET;
export using ::DEVINST;
export using ::MEM_RESOURCE;
export using ::MEM_LARGE_RESOURCE;
export using ::HDEVINFO;
export using ::SP_DEVINFO_DATA;
export using ::DEVPROPKEY;
export using ::DEVPROPTYPE;
export using ::DEVPKEY_NAME;
export using ::DEVPKEY_Device_InstanceId;
export using ::DEVPKEY_Device_Parent;
export using ::DEVPKEY_Device_LocationInfo;
export using ::DEVPKEY_Device_BusNumber;
export using ::DEVPKEY_Device_Address;

export constexpr auto const NULL = nullptr;
export constexpr auto const LANG_NEUTRAL = e_LANG_NEUTRAL;
export constexpr auto const SUBLANG_NEUTRAL = e_SUBLANG_NEUTRAL;

export constexpr auto const
    FORMAT_MESSAGE_FROM_SYSTEM = f_FORMAT_MESSAGE_FROM_SYSTEM,
    FORMAT_MESSAGE_IGNORE_INSERTS = f_FORMAT_MESSAGE_IGNORE_INSERTS,
    FORMAT_MESSAGE_ALLOCATE_BUFFER = f_FORMAT_MESSAGE_ALLOCATE_BUFFER;

export           auto const INVALID_HANDLE_VALUE = f_INVALID_HANDLE_VALUE;
export constexpr auto const ALLOC_LOG_CONF = f_ALLOC_LOG_CONF;
export constexpr auto const MType_Range = z_MType_Range;
export constexpr auto const MLType_Range = z_MLType_Range;
export constexpr auto const
    ResType_None     = local_ResType_None,
    ResType_All      = local_ResType_All,
    ResType_Mem      = local_ResType_Mem,
    ResType_MemLarge = local_ResType_MemLarge,
    ResType_IO       = local_ResType_IO;
export constexpr auto const mMD_Readable = e_mMD_Readable;
export constexpr auto const mMD_MemoryType = e_mMD_MemoryType;
export constexpr auto const fMD_RAM = e_fMD_RAM;
export constexpr auto const fMD_ReadAllowed = e_fMD_ReadAllowed;

export constexpr auto const DIGCF_PRESENT = f_DIGCF_PRESENT;

export constexpr auto const
    TOKEN_ADJUST_PRIVILEGES = local_TOKEN_ADJUST_PRIVILEGES,
    TOKEN_QUERY = local_TOKEN_QUERY;

export constexpr auto const SE_SYSTEM_ENVIRONMENT_NAME = local_SE_SYSTEM_ENVIRONMENT_NAME;
export constexpr auto const SE_PRIVILEGE_ENABLED = local_SE_PRIVILEGE_ENABLED;
export constexpr auto const
    DEVPROP_TYPE_STRING = local_DEVPROP_TYPE_STRING,
    DEVPROP_TYPE_UINT16 = local_DEVPROP_TYPE_UINT16,
    DEVPROP_TYPE_UINT32 = local_DEVPROP_TYPE_UINT32,
    DEVPROP_TYPE_UINT64 = local_DEVPROP_TYPE_UINT64;

export constexpr auto const
    CR_SUCCESS                  = nvs_winapi_CR_SUCCESS,
    CR_DEFAULT                  = nvs_winapi_CR_DEFAULT,
    CR_OUT_OF_MEMORY            = nvs_winapi_CR_OUT_OF_MEMORY,
    CR_INVALID_POINTER          = nvs_winapi_CR_INVALID_POINTER,
    CR_INVALID_FLAG             = nvs_winapi_CR_INVALID_FLAG,
    CR_INVALID_DEVNODE          = nvs_winapi_CR_INVALID_DEVNODE,
    CR_INVALID_DEVINST          = nvs_winapi_CR_INVALID_DEVINST,
    CR_INVALID_RES_DES          = nvs_winapi_CR_INVALID_RES_DES,
    CR_INVALID_LOG_CONF         = nvs_winapi_CR_INVALID_LOG_CONF,
    CR_INVALID_ARBITRATOR       = nvs_winapi_CR_INVALID_ARBITRATOR,
    CR_INVALID_NODELIST         = nvs_winapi_CR_INVALID_NODELIST,
    CR_DEVNODE_HAS_REQS         = nvs_winapi_CR_DEVNODE_HAS_REQS,
    CR_INVALID_RESOURCEID       = nvs_winapi_CR_INVALID_RESOURCEID,
    CR_DLVXD_NOT_FOUND          = nvs_winapi_CR_DLVXD_NOT_FOUND,
    CR_NO_SUCH_DEVNODE          = nvs_winapi_CR_NO_SUCH_DEVNODE,
    CR_NO_SUCH_DEVINST          = nvs_winapi_CR_NO_SUCH_DEVINST,
    CR_NO_MORE_LOG_CONF         = nvs_winapi_CR_NO_MORE_LOG_CONF,
    CR_NO_MORE_RES_DES          = nvs_winapi_CR_NO_MORE_RES_DES,
    CR_ALREADY_SUCH_DEVNODE     = nvs_winapi_CR_ALREADY_SUCH_DEVNODE,
    CR_ALREADY_SUCH_DEVINST     = nvs_winapi_CR_ALREADY_SUCH_DEVINST,
    CR_INVALID_RANGE_LIST       = nvs_winapi_CR_INVALID_RANGE_LIST,
    CR_INVALID_RANGE            = nvs_winapi_CR_INVALID_RANGE,
    CR_FAILURE                  = nvs_winapi_CR_FAILURE,
    CR_NO_SUCH_LOGICAL_DEV      = nvs_winapi_CR_NO_SUCH_LOGICAL_DEV,
    CR_CREATE_BLOCKED           = nvs_winapi_CR_CREATE_BLOCKED,
    CR_NOT_SYSTEM_VM            = nvs_winapi_CR_NOT_SYSTEM_VM,
    CR_REMOVE_VETOED            = nvs_winapi_CR_REMOVE_VETOED,
    CR_APM_VETOED               = nvs_winapi_CR_APM_VETOED,
    CR_INVALID_LOAD_TYPE        = nvs_winapi_CR_INVALID_LOAD_TYPE,
    CR_BUFFER_SMALL             = nvs_winapi_CR_BUFFER_SMALL,
    CR_NO_ARBITRATOR            = nvs_winapi_CR_NO_ARBITRATOR,
    CR_NO_REGISTRY_HANDLE       = nvs_winapi_CR_NO_REGISTRY_HANDLE,
    CR_REGISTRY_ERROR           = nvs_winapi_CR_REGISTRY_ERROR,
    CR_INVALID_DEVICE_ID        = nvs_winapi_CR_INVALID_DEVICE_ID,
    CR_INVALID_DATA             = nvs_winapi_CR_INVALID_DATA,
    CR_INVALID_API              = nvs_winapi_CR_INVALID_API,
    CR_DEVLOADER_NOT_READY      = nvs_winapi_CR_DEVLOADER_NOT_READY,
    CR_NEED_RESTART             = nvs_winapi_CR_NEED_RESTART,
    CR_NO_MORE_HW_PROFILES      = nvs_winapi_CR_NO_MORE_HW_PROFILES,
    CR_DEVICE_NOT_THERE         = nvs_winapi_CR_DEVICE_NOT_THERE,
    CR_NO_SUCH_VALUE            = nvs_winapi_CR_NO_SUCH_VALUE,
    CR_WRONG_TYPE               = nvs_winapi_CR_WRONG_TYPE,
    CR_INVALID_PRIORITY         = nvs_winapi_CR_INVALID_PRIORITY,
    CR_NOT_DISABLEABLE          = nvs_winapi_CR_NOT_DISABLEABLE,
    CR_FREE_RESOURCES           = nvs_winapi_CR_FREE_RESOURCES,
    CR_QUERY_VETOED             = nvs_winapi_CR_QUERY_VETOED,
    CR_CANT_SHARE_IRQ           = nvs_winapi_CR_CANT_SHARE_IRQ,
    CR_NO_DEPENDENT             = nvs_winapi_CR_NO_DEPENDENT,
    CR_SAME_RESOURCES           = nvs_winapi_CR_SAME_RESOURCES,
    CR_NO_SUCH_REGISTRY_KEY     = nvs_winapi_CR_NO_SUCH_REGISTRY_KEY,
    CR_INVALID_MACHINENAME      = nvs_winapi_CR_INVALID_MACHINENAME,
    CR_REMOTE_COMM_FAILURE      = nvs_winapi_CR_REMOTE_COMM_FAILURE,
    CR_MACHINE_UNAVAILABLE      = nvs_winapi_CR_MACHINE_UNAVAILABLE,
    CR_NO_CM_SERVICES           = nvs_winapi_CR_NO_CM_SERVICES,
    CR_ACCESS_DENIED            = nvs_winapi_CR_ACCESS_DENIED,
    CR_CALL_NOT_IMPLEMENTED     = nvs_winapi_CR_CALL_NOT_IMPLEMENTED,
    CR_INVALID_PROPERTY         = nvs_winapi_CR_INVALID_PROPERTY,
    CR_DEVICE_INTERFACE_ACTIVE  = nvs_winapi_CR_DEVICE_INTERFACE_ACTIVE,
    CR_NO_SUCH_DEVICE_INTERFACE = nvs_winapi_CR_NO_SUCH_DEVICE_INTERFACE,
    CR_INVALID_REFERENCE_STRING = nvs_winapi_CR_INVALID_REFERENCE_STRING,
    CR_INVALID_CONFLICT_LIST    = nvs_winapi_CR_INVALID_CONFLICT_LIST,
    CR_INVALID_INDEX            = nvs_winapi_CR_INVALID_INDEX,
    CR_INVALID_STRUCTURE_SIZE   = nvs_winapi_CR_INVALID_STRUCTURE_SIZE;

export inline auto MAKELANGID(DWORD langID, DWORD subLangID) -> decltype(nvs_winapi_MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
{
    return nvs_winapi_MAKELANGID(langID, subLangID);
}

export using ::GetLastError;
export using ::GetCurrentProcess;
export using ::FormatMessageA;
export using ::FormatMessageW;
export using ::GlobalAlloc;
export using ::GlobalFree;
export using ::LocalAlloc;
export using ::LocalFree;
export using ::GetConsoleWindow;

export using ::CM_Get_Next_Res_Des;
export using ::CM_Free_Res_Des_Handle;
export using ::CM_Get_Res_Des_Data;
export using ::CM_Get_First_Log_Conf;
export using ::CM_Free_Log_Conf_Handle;

export using ::SetupDiEnumDeviceInfo;
export using ::SetupDiGetDevicePropertyW;
export using ::SetupDiGetClassDevsW;
export using ::SetupDiOpenDeviceInfoA;
export using ::SetupDiOpenDeviceInfoW;
export using ::SetupDiCreateDeviceInfoList;
export using ::SetupDiDestroyDeviceInfoList;

export using ::OpenProcessToken;
export using ::LookupPrivilegeValueA;
export using ::LookupPrivilegeValueW;
export using ::AdjustTokenPrivileges;

#undef FormatMessage
#undef LookupPrivilegeValue
#undef SetupDiOpenDeviceInfo

#if defined(UNICODE)
    export auto &FormatMessage = ::FormatMessageW;
    export auto &LookupPrivilegeValue = ::LookupPrivilegeValueW;
    export auto &SetupDiOpenDeviceInfo = ::SetupDiOpenDeviceInfoW;
#else
    export auto &FormatMessage = ::FormatMessageA;
    export auto &LookupPrivilegeValue = ::LookupPrivilegeValueA;
    export auto &SetupDiOpenDeviceInfo = ::SetupDiOpenDeviceInfoA;
#endif
