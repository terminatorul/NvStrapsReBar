#if !defined(NV_STRAPS_REBAR_STATE_HH)
#define NV_STRAPS_REBAR_STATE_HH

#include <cstdint>
#include <optional>

std::optional<std::uint_least8_t> getReBarState();
bool setReBarState(std::uint_least8_t rebarState);

#endif          // !defined(NV_STRAPS_REBAR_STATE_HH)