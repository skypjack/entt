#ifndef ENTT_COMMON_BITMASK_H
#define ENTT_COMMON_BITMASK_H

#include <entt/core/enum.hpp>

namespace test {

enum class enum_is_bitmask {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04,
    _entt_enum_as_bitmask
};

// small type on purpose
enum class enum_as_bitmask : std::uint8_t {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04
};

} // namespace test

template<>
struct entt::enum_as_bitmask<test::enum_as_bitmask>
    : std::true_type {};

#endif
