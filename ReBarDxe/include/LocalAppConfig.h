#if !defined(NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H)
#define NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H

#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
typedef UINT8 BYTE;
typedef EFI_STATUS ERROR_CODE;
enum
{
    ERROR_CODE_SUCCESS = (ERROR_CODE)EFI_SUCCESS
};
#else
# if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
#  include <windef.h>
#  include <winerror.h>
#  define WINDOWS_SOURCE
typedef DWORD ERROR_CODE;
enum
{
    ERROR_CODE_SUCCESS = (ERROR_CODE)ERROR_SUCCESS
};
# else
#  include <errno.h>
typedef errno_t ERROR_CODE;
enum
{
    ERROR_CODE_SUCCESS = (ERROR_CODE)0
};
# endif
# define ARRAY_SIZE(Container) (sizeof(Container) / sizeof((Container)[0u]))
#endif

#include <stdint.h>
#include <limits.h>
#include <errno.h>

enum
{
    BYTE_BITSIZE = (unsigned)CHAR_BIT,
    BYTE_BITMASK = (1u << BYTE_BITSIZE) - 1u,
    BYTE_SIZE = 1u,
    WORD_SIZE = 2u,
    WORD_BITSIZE = WORD_SIZE * BYTE_BITSIZE,
    WORD_BITMASK = (1u << WORD_BITSIZE) - 1u,
    DWORD_SIZE = 4u,
    DWORD_BITSIZE = DWORD_SIZE * BYTE_BITSIZE,
    DWORD_BITMASK = (UINT64_C(1) << DWORD_BITSIZE) - 1u,
    QWORD_SIZE = 8u
};

#endif          // !defined(NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H)
