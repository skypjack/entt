#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/entity/entity.hpp>
#include "../../common/entity.h"

template<typename T, typename U = T, typename = void>
inline constexpr bool is_equality_comparable_v = false;

template<typename T, typename U>
inline constexpr bool is_equality_comparable_v<T, U, std::void_t<decltype(std::declval<T>() == std::declval<U>())>> = true;

template<typename T, typename U = T, typename = void>
inline constexpr bool is_not_equality_comparable_v = false;

template<typename T, typename U>
inline constexpr bool is_not_equality_comparable_v<T, U, std::void_t<decltype(std::declval<T>() != std::declval<U>())>> = true;

template<typename T, typename U = T>
inline constexpr bool is_comparable_v = is_equality_comparable_v<T, U>
                                        && is_equality_comparable_v<U, T>
                                        && is_not_equality_comparable_v<T, U>
                                        && is_not_equality_comparable_v<U, T>;
struct unrelated {};
struct use_my_operator {};
template<typename T>
bool operator==(use_my_operator, T &&);

template<typename T>
bool operator!=(use_my_operator, T &&);

template<typename T>
bool operator==(T &&, use_my_operator);

template<typename T>
bool operator!=(T &&, use_my_operator);

struct entity_traits {
    using value_type = test::entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0x3FFFF; // 18b
    static constexpr entity_type version_mask = 0x0FFF; // 12b
};

struct other_entity_traits {
    using value_type = test::other_entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0xFFFFFFFF; // 32b
    static constexpr entity_type version_mask = 0x00;      // 0b
};

template<>
struct entt::entt_traits<test::entity>: entt::basic_entt_traits<entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

template<>
struct entt::entt_traits<test::other_entity>: entt::basic_entt_traits<other_entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

TEST(Sfinae, NullComparison) {
    static_assert(is_comparable_v<entt::null_t>);
    static_assert(is_comparable_v<entt::null_t, entt::entity>);
    static_assert(is_comparable_v<entt::null_t, test::entity>);
    static_assert(is_comparable_v<entt::null_t, test::other_entity>);

    static_assert(is_comparable_v<use_my_operator, entt::null_t>);
    
    static_assert(!is_comparable_v<entt::null_t, unrelated>);
}

TEST(Sfinae, TombstoneComparison) {
    static_assert(is_comparable_v<entt::tombstone_t>);
    static_assert(is_comparable_v<entt::tombstone_t, test::entity>);
    static_assert(is_comparable_v<entt::tombstone_t, test::other_entity>);
    
    static_assert(is_comparable_v<use_my_operator, entt::tombstone_t>);

    static_assert(!is_comparable_v<entt::tombstone_t, unrelated>);

    static_assert(!is_comparable_v<entt::tombstone_t, entt::null_t>);
}
