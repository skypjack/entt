#include <array>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>

template<typename Type, typename Entity>
struct entt::storage_type<Type, Entity> {
    // no signal regardless of element type ...
    using type = basic_storage<Type, Entity>;
};

template<typename Entity>
struct entt::storage_type<char, Entity> {
    // ... unless it's char, because yes.
    using type = sigh_mixin<basic_storage<char, Entity>>;
};

template<typename, typename, typename = void>
struct has_on_construct: std::false_type {};

template<typename Entity, typename Type>
struct has_on_construct<Entity, Type, std::void_t<decltype(&entt::storage_type_t<Type>::on_construct)>>: std::true_type {};

template<typename Entity, typename Type>
inline constexpr auto has_on_construct_v = has_on_construct<Entity, Type>::value;

TEST(Example, SignalLess) {
    // invoking registry::on_construct<int> is a compile-time error
    ASSERT_FALSE((has_on_construct_v<entt::entity, int>));
    ASSERT_TRUE((has_on_construct_v<entt::entity, char>));

    entt::registry registry;
    const std::array entity{registry.create()};

    // literally a test for storage_adapter_mixin
    registry.emplace<int>(entity[0], 0);
    registry.erase<int>(entity[0]);
    registry.insert<int>(entity.begin(), entity.end(), 3);
    registry.patch<int>(entity[0], [](auto &value) { value = 2; });

    ASSERT_EQ(registry.get<int>(entity[0]), 2);
}
