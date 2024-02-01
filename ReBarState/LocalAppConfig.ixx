module;

#include "LocalAppConfig.h"

export module LocalAppConfig;

static constexpr bool const local_UEFI_SOURCE =
#if defined(UEFI_SOURCE)
    true;
#else
    false;
#endif

static constexpr bool const local_WINDOWS_SOURCE =
#if defined(WINDOWS_SOURCE)
    true;
#else
    false;
#endif

#undef UEFI_SOURCE
#undef WINDOWS_SOURCE

// can be used with if constexpr (WINDOWS_SOURCE)
export constexpr bool const WINDOWS_SOURCE = local_WINDOWS_SOURCE;
export constexpr bool const UEFI_SOURCE = local_UEFI_SOURCE;

export using ::BYTE_SIZE;
export using ::BYTE_BITSIZE;
export using ::BYTE_BITMASK;

export using ::WORD_SIZE;
export using ::WORD_BITSIZE;
export using ::WORD_BITMASK;

export using ::DWORD_SIZE;
export using ::DWORD_BITSIZE;
export using ::DWORD_BITMASK;

export using ::ERROR_CODE;
export using ::ERROR_CODE_SUCCESS;
