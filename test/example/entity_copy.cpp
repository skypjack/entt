#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/registry.hpp>

enum class my_entity : entt::id_type {};

TEST(Example, EntityCopy) {
    using namespace entt::literals;

    entt::registry registry{};
    auto &&custom = registry.storage<double>("custom"_hs);

    const auto src = registry.create();
    const auto dst = registry.create();
    const auto other = registry.create();

    custom.emplace(src, 1.);
    registry.emplace<int>(src, 42);
    registry.emplace<char>(src, 'c');
    registry.emplace<double>(other, 3.);

    ASSERT_TRUE(custom.contains(src));
    ASSERT_FALSE(registry.all_of<double>(src));
    ASSERT_TRUE((registry.all_of<int, char>(src)));
    ASSERT_FALSE((registry.any_of<int, char, double>(dst)));
    ASSERT_FALSE(custom.contains(dst));

    for(auto [id, storage]: registry.storage()) {
        // discard the custom storage because why not, this is just an example after all
        if(id != "custom"_hs && storage.contains(src)) {
            storage.emplace(dst, storage.get(src));
        }
    }

    ASSERT_TRUE((registry.all_of<int, char>(dst)));
    ASSERT_FALSE((registry.all_of<double>(dst)));
    ASSERT_FALSE(custom.contains(dst));

    ASSERT_EQ(registry.get<int>(dst), 42);
    ASSERT_EQ(registry.get<char>(dst), 'c');
}

TEST(Example, DifferentRegistryTypes) {
    using namespace entt::literals;

    entt::basic_registry<entt::entity> registry{};
    entt::basic_registry<my_entity> other{};

    /*
        TODO These are currently needed to ensure that the source and
             target registries have the proper storage initialized
             prior to copying, as this isn't done automatically
             when emplacing storages (as is done below).

             There is an open issue about this, and these two
             lines should be removed when a fix is properly landed.
             https://github.com/skypjack/entt/issues/827
    */
    static_cast<void>(registry.storage<double>());
    static_cast<void>(other.storage<int>());

    const auto src = registry.create();
    const auto dst = other.create();

    registry.emplace<int>(src, 42);
    registry.emplace<char>(src, 'c');

    for(auto [id, storage]: registry.storage()) {
        if(auto it = other.storage(id); it != other.storage().end() && storage.contains(src)) {
            it->second.emplace(dst, storage.get(src));
        }
    }

    ASSERT_TRUE((registry.all_of<int, char>(src)));
    ASSERT_FALSE(other.all_of<char>(dst));
    ASSERT_TRUE(other.all_of<int>(dst));
    ASSERT_EQ(other.get<int>(dst), 42);
}
