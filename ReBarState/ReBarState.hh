#if !defined(NV_STRAPS_REBAR_STATE_HH)
#define NV_STRAPS_REBAR_STATE_HH

#include <cstdint>
#include <optional>

constexpr unsigned short const MAX_UEFI_BAR_SIZE_SELECTOR = 32u;
constexpr unsigned short const NV_GPU_BAR_ONLY_SELECTOR = 64u;
constexpr unsigned short const NV_GPU_BAR_STRAPS_ONLY_SELECTOR = 65u;

std::optional<std::uint_least8_t> getReBarState();
bool setReBarState(std::uint_least8_t rebarState);

#endif          // !defined(NV_STRAPS_REBAR_STATE_HH)