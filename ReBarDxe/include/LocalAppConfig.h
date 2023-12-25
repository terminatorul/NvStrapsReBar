#if !defined(NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H)
#define NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H

#include <limits.h>

enum
{
    BYTE_BITSIZE = (unsigned)CHAR_BIT,
    BYTE_BITMASK = (1u << BYTE_BITSIZE) - 1u,
    BYTE_SIZE = 1u,
    WORD_SIZE = 2u,
    DWORD_SIZE = 4u,
    QWORD_SIZE = 8u
};

#endif          // !defined(NV_STRAPS_REBAR_LOCAL_APP_CONFIG_H)
