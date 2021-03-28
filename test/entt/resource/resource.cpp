#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>

struct resource { int value; };

struct loader: entt::resource_loader<loader, resource> {
    std::shared_ptr<resource> load(int value) const {
        return std::shared_ptr<resource>(new resource{ value });
    }
};

struct broken_loader: entt::resource_loader<broken_loader, resource> {
    std::shared_ptr<resource> load(int) const {
        return nullptr;
    }
};

TEST(Resource, Functionalities) {
    entt::resource_cache<resource> cache;

    constexpr auto hs1 = entt::hashed_string{"res1"};
    constexpr auto hs2 = entt::hashed_string{"res2"};

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_FALSE(cache.load<broken_loader>(hs1, 42));
    ASSERT_FALSE(cache.reload<broken_loader>(hs1, 42));

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<loader>(hs1, 42));
    ASSERT_TRUE(cache.reload<loader>(hs1, 42));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));
    ASSERT_EQ((*cache.handle(hs1)).value, 42);

    ASSERT_TRUE(cache.load<loader>(hs1, 42));
    ASSERT_TRUE(cache.load<loader>(hs2, 42));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ((*cache.handle(hs1)).value, 42);
    ASSERT_EQ(cache.handle(hs2)->value, 42);

    ASSERT_NO_FATAL_FAILURE(cache.discard(hs1));

    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ(cache.handle(hs2)->value, 42);

    ASSERT_TRUE(cache.load<loader>(hs1, 42));
    ASSERT_NO_FATAL_FAILURE(cache.clear());

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<loader>(hs1, 42));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.handle(hs1));
    ASSERT_FALSE(cache.handle(hs2));

    ASSERT_TRUE(cache.handle(hs1));
    ASSERT_EQ(&cache.handle(hs1).get(), &static_cast<const resource &>(cache.handle(hs1)));
    ASSERT_NO_FATAL_FAILURE(cache.clear());

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());

    ASSERT_TRUE(cache.temp<loader>(42));
    ASSERT_TRUE(cache.empty());

    ASSERT_FALSE(entt::resource_handle<resource>{});
    ASSERT_TRUE(std::is_copy_constructible_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_move_constructible_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_copy_assignable_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_move_assignable_v<entt::resource_handle<resource>>);
}

TEST(Resource, MutableHandle) {
    entt::resource_cache<resource> cache;

    constexpr auto hs = entt::hashed_string{"res"};
    auto handle = cache.load<loader>(hs, 0);

    ASSERT_TRUE(handle);

    ++handle.get().value;
    ++static_cast<resource &>(handle).value;
    ++(*handle).value;
    ++handle->value;

    ASSERT_EQ(cache.handle(hs)->value, 4);
}

TEST(Resource, Each) {
    using namespace entt::literals;

    entt::resource_cache<resource> cache;
    cache.load<loader>("resource"_hs, 0);

    cache.each([](entt::resource_handle<resource> res) {
        ++res->value;
    });

    ASSERT_FALSE(cache.empty());
    ASSERT_EQ(cache.handle("resource"_hs)->value, 1);

    cache.each([](auto id, auto res) {
        ASSERT_EQ(id, "resource"_hs);
        ++res->value;
    });

    ASSERT_FALSE(cache.empty());
    ASSERT_EQ(cache.handle("resource"_hs)->value, 2);

    cache.each([&cache](entt::id_type id) {
        cache.discard(id);
    });

    ASSERT_TRUE(cache.empty());
}
