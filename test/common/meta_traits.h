#ifndef ENTT_COMMON_META_TRAITS_H
#define ENTT_COMMON_META_TRAITS_H

#include <entt/core/enum.hpp>

namespace test {

enum class meta_traits {
    none = 0x0000,
    one = 0x0001,
    two = 0x0002,
    three = 0x0100,
    _entt_enum_as_bitmask
};

} // namespace test

#endif
