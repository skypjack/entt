#ifndef ENTT_COMMON_META_TRAITS_H
#define ENTT_COMMON_META_TRAITS_H

#include <entt/core/enum.hpp>

namespace test {

enum class meta_traits : std::uint8_t {
    none = 0x00,
    one = 0x01,
    two = 0x02,
    three = 0x04,
    all = 0xFF,
    _entt_enum_as_bitmask = all
};

} // namespace test

#endif
