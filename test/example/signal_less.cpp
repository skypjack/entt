#include <iterator>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

template<typename Entity, typename Type>
struct entt::storage_traits<Entity, Type> {
    // no signal regardless of component type ...
    using storage_type = storage_adapter_mixin<basic_storage<Entity, Type>>;
};

template<typename Entity>
struct entt::storage_traits<Entity, char> {
    // ... unless it's char, because yes.
    using storage_type = sigh_storage_mixin<basic_storage<Entity, char>>;
};

template<typename, typename, typename = void>
struct has_on_construct: std::false_type {};

template<typename Entity, typename Type>
struct has_on_construct<Entity, Type, std::void_t<decltype(&entt::storage_traits<Entity, Type>::storage_type::on_construct)>>: std::true_type {};

template<typename Entity, typename Type>
inline constexpr auto has_on_construct_v = has_on_construct<Entity, Type>::value;

TEST(Example, SignalLess) {
    // invoking registry::on_construct<int> is a compile-time error
    static_assert(!has_on_construct_v<entt::entity, int>);
    static_assert(has_on_construct_v<entt::entity, char>);

    entt::registry registry;
    const entt::entity entity[1u]{registry.create()};

    // literally a test for storage_adapter_mixin
    registry.emplace<int>(entity[0], 0);
    registry.remove<int>(entity[0]);
    registry.insert<int>(std::begin(entity), std::end(entity), 3);
    registry.patch<int>(entity[0], [](auto &value) { value = 42; });

    ASSERT_EQ(registry.get<int>(entity[0]), 42);
}
