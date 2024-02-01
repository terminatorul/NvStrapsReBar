module;

#include "LocalAppConfig.h"

export module LocalAppConfig;

import std;

using std::uint_least32_t;
using std::uint_least64_t;

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
export auto DWORD_BITMASK = uint_least32_t { (uint_least64_t { 1u } << DWORD_BITSIZE) - 1u };

export using ::ERROR_CODE;
export using ::ERROR_CODE_SUCCESS;
