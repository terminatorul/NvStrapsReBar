// Some test to check if compiling UEFI code
#if defined(UEFI_SOURCE) || defined(EFIAPI)
# include <Uefi.h>
#else
# include <cstdint>

typedef std::uint_least8_t  UINT8;
typedef std::uint_least16_t UINT16;
typedef std::uint_least32_t UINT32;
typedef std::uint_least64_t UINT64;

#endif

