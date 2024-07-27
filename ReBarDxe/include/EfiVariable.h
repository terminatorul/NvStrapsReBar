#if !defined(NV_STRAPS_REBAR_EFI_VARIABLE_H)
#define NV_STRAPS_REBAR_EFI_VARIABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "LocalAppConfig.h"

#if defined(__cplusplus)
extern "C"
{
#endif

enum
{
   MAX_VARIABLE_NAME_LENGTH = 64u
};

#if defined(WINDOWS_SOURCE)
# if !defined(EFI_VARIABLE_NON_VOLATILE) && !defined(EFI_VARIABLE_BOOTSERVICE_ACCESS) && !defined(EFI_VARIABLE_RUNTIME_ACCESS)
enum
{
    EFI_VARIABLE_NON_VOLATILE = UINT32_C(0x0000'0001),
    EFI_VARIABLE_BOOTSERVICE_ACCESS = UINT32_C(0x0000'0002),
    EFI_VARIABLE_RUNTIME_ACCESS = UINT32_C(0x0000'0004),
    EFI_VARIABLE_HARDWARE_ERROR_RECORD = UINT32_C(0x0000'0008)
};
# endif
#endif

inline uint_least8_t unpack_BYTE(BYTE const *buffer);
inline uint_least16_t unpack_WORD(BYTE const *buffer);
inline uint_least32_t unpack_DWORD(BYTE const *buffer);
inline uint_least64_t unpack_QWORD(BYTE const *buffer);
inline BYTE *pack_BYTE(BYTE *buffer, uint_least8_t value);
inline BYTE *pack_WORD(BYTE *buffer, uint_least16_t value);
inline BYTE *pack_DWORD(BYTE *buffer, uint_least32_t value);
inline BYTE *pack_QWORD(BYTE *buffer, uint_least64_t value);

ERROR_CODE ReadEfiVariable(char const name[MAX_VARIABLE_NAME_LENGTH], BYTE *buffer, uint_least32_t *size);
ERROR_CODE WriteEfiVariable(char const name[MAX_VARIABLE_NAME_LENGTH], BYTE /* const */ *buffer, uint_least32_t size, uint_least32_t attributes);

inline uint_least8_t unpack_BYTE(BYTE const *buffer)
{
    return *buffer;
}

inline uint_least16_t unpack_WORD(BYTE const *buffer)
{
    return *buffer | (uint_least16_t)buffer[1u] << BYTE_BITSIZE;
}

inline uint_least32_t unpack_DWORD(BYTE const *buffer)
{
    return *buffer
    | (uint_least16_t)buffer[1u] <<	     BYTE_BITSIZE
    | (uint_least32_t)buffer[2u] << 2u * BYTE_BITSIZE
    | (uint_least32_t)buffer[3u] << 3u * BYTE_BITSIZE;
}

inline uint_least64_t unpack_QWORD(BYTE const *buffer)
{
    return *buffer
    | (uint_least16_t)buffer[1u] <<      BYTE_BITSIZE
    | (uint_least32_t)buffer[2u] << 2u * BYTE_BITSIZE
    | (uint_least32_t)buffer[3u] << 3u * BYTE_BITSIZE
        | (uint_least64_t)buffer[4u] << 4u * BYTE_BITSIZE
    | (uint_least64_t)buffer[5u] << 5u * BYTE_BITSIZE
    | (uint_least64_t)buffer[6u] << 6u * BYTE_BITSIZE
    | (uint_least64_t)buffer[7u] << 7u * BYTE_BITSIZE;
}

inline BYTE *pack_BYTE(BYTE *buffer, uint_least8_t value)
{
    return *buffer++ = value, buffer;
}

inline BYTE *pack_WORD(BYTE *buffer, uint_least16_t value)
{
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;
    *buffer++ = value & BYTE_BITMASK;

    return buffer;
}

inline BYTE *pack_DWORD(BYTE *buffer, uint_least32_t value)
{
    for (unsigned i = 0u; i < DWORD_SIZE; i++)
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;

    return buffer;
}

inline BYTE *pack_QWORD(BYTE *buffer, uint_least64_t value)
{
    for (unsigned i = 0u; i < QWORD_SIZE; i++)
    *buffer++ = value & BYTE_BITMASK, value >>= BYTE_BITSIZE;

    return buffer;
}

#if defined(__cplusplus)
}       // extern "C"
#endif

#endif          // !defined(NV_STRAPS_REBAR_EFI_VARIABLE_H)
