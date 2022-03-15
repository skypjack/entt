#include <gtest/gtest.h>
#include <entt/entity/component.hpp>

struct self_contained {
    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 4u;
};

struct traits_based {};

template<>
struct entt::component_traits<traits_based> {
    static constexpr auto in_place_delete = false;
    static constexpr auto page_size = 8u;
};

struct default_params_empty {};
struct default_params_non_empty {
    int value;
};

TEST(Component, DefaultParamsEmpty) {
    using traits = entt::component_traits<default_params_empty>;

    static_assert(!traits::in_place_delete);
    static_assert(traits::page_size == 0u);
    static_assert(entt::ignore_as_empty_v<default_params_empty>);
}

TEST(Component, DefaultParamsNonEmpty) {
    using traits = entt::component_traits<default_params_non_empty>;

    static_assert(!traits::in_place_delete);
    static_assert(traits::page_size == ENTT_PACKED_PAGE);
    static_assert(!entt::ignore_as_empty_v<default_params_non_empty>);
}

TEST(Component, SelfContained) {
    using traits = entt::component_traits<self_contained>;

    static_assert(traits::in_place_delete);
    static_assert(traits::page_size == 4u);
    static_assert(!entt::ignore_as_empty_v<self_contained>);
}

TEST(Component, TraitsBased) {
    using traits = entt::component_traits<traits_based>;

    static_assert(!traits::in_place_delete);
    static_assert(traits::page_size == 8u);
    static_assert(!entt::ignore_as_empty_v<traits_based>);
}
