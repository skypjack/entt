#ifndef ENTT_COMMON_TYPE_HASH_MISMATCH_H
#define ENTT_COMMON_TYPE_HASH_MISMATCH_H

#include <entt/core/type_info.hpp>

namespace test {

struct type_hash_mismatch {};

} // namespace test

template<>
struct entt::type_hash<test::type_hash_mismatch> {
    [[nodiscard]] static id_type value() noexcept {
        // a hash that changes with every request, to mimic inconsistencies
        static id_type identifier{};
        return ++identifier;
    }
};

#endif
