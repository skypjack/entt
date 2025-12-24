#include <array>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>

struct SignalLess: testing::Test {
    enum entity : std::uint32_t {};

    template<typename, typename = void>
    struct has_on_construct: std::false_type {};

    template<typename Type>
    struct has_on_construct<Type, std::void_t<decltype(&entt::storage_type_t<Type, entity>::on_construct)>>: std::true_type {};

    template<typename Type>
    static constexpr auto has_on_construct_v = has_on_construct<Type>::value;
};

template<typename Type>
struct entt::storage_type<Type, SignalLess::entity> {
    // no signal regardless of element type ...
    using type = basic_storage<Type, SignalLess::entity>;
};

template<>
struct entt::storage_type<char, SignalLess::entity> {
    // ... unless it's char, because yes.
    using type = sigh_mixin<basic_storage<char, SignalLess::entity>>;
};

TEST_F(SignalLess, Example) {
    // invoking registry::on_construct<int> is a compile-time error
    ASSERT_FALSE((has_on_construct_v<int>));
    ASSERT_TRUE((has_on_construct_v<char>));

    entt::basic_registry<entity> registry;
    const std::array entity{registry.create()};

    // literally a test for storage_adapter_mixin
    registry.emplace<int>(entity[0], 0);
    registry.erase<int>(entity[0]);
    registry.insert<int>(entity.begin(), entity.end(), 3);
    registry.patch<int>(entity[0], [](auto &value) { value = 2; });

    ASSERT_EQ(registry.get<int>(entity[0]), 2);
}
