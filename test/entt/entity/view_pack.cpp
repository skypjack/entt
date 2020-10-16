#include <cstdint>
#include <iterator>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <entt/entity/view_pack.hpp>

struct empty_type {};

TEST(ViewPack, Construction) {
    entt::registry registry;

    const auto view1 = registry.view<int, const char>();
    const auto view2 = registry.view<empty_type>();
    const auto view3 = registry.view<double>();

    static_assert(std::is_same_v<decltype(entt::view_pack{view1}), entt::view_pack<entt::view<entt::exclude_t<>, int, const char>>>);
    static_assert(std::is_same_v<decltype(entt::view_pack{view1, view2, view3}), decltype(view1 | view2 | view3)>);

    static_assert(std::is_same_v<
        decltype(entt::view_pack{view1, view2, view3}),
        entt::view_pack<
            entt::view<entt::exclude_t<>, int, const char>,
            entt::view<entt::exclude_t<>, empty_type>,
            entt::view<entt::exclude_t<>, double>
        >
    >);

    static_assert(std::is_same_v<decltype(entt::view_pack{view1, view2, view3}), decltype(entt::view_pack{view1} | view2 | view3)>);
    static_assert(std::is_same_v<decltype(entt::view_pack{view1, view2, view3}), decltype(view1 | entt::view_pack{view2} | view3)>);
    static_assert(std::is_same_v<decltype(entt::view_pack{view1, view2, view3}), decltype(view1 | view2 | entt::view_pack{view3})>);
}

TEST(ViewPack, ShortestPool) {
    entt::registry registry;
    entt::entity entities[3];

    registry.create(std::begin(entities), std::end(entities));

    registry.insert<int>(std::begin(entities), std::end(entities));
    registry.insert<empty_type>(std::begin(entities), std::end(entities));
    registry.insert<char>(std::rbegin(entities), std::rend(entities) - 1u);

    const auto tmp = registry.view<char>() | registry.view<empty_type>();
    const auto pack = tmp | registry.view<const int>();

    {
        std::size_t next{};

        for(const auto entt: pack) {
            ASSERT_EQ(entt::to_integral(entt), ++next);
            ASSERT_TRUE((registry.has<int, char>(entt)));
        }
    }

    auto it = pack.begin();

    ASSERT_EQ(*it++, entities[1u]);
    ASSERT_EQ(++it, pack.end());
    ASSERT_TRUE(it == pack.end());
    ASSERT_FALSE(it != pack.end());

    pack.each([&registry, next = 0u](const auto entt, auto &&cv, auto &&iv) mutable {
        static_assert(std::is_same_v<decltype(entt), const entt::entity>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
        ASSERT_EQ(entt::to_integral(entt), ++next);
        ASSERT_EQ(&cv, registry.try_get<char>(entt));
        ASSERT_EQ(&iv, registry.try_get<int>(entt));
    });

    auto eit = pack.each().begin();

    ASSERT_EQ(std::get<0>(*eit++), entities[1u]);
    static_assert(std::is_same_v<decltype(std::get<1>(*eit)), char &>);
    static_assert(std::is_same_v<decltype(std::get<2>(*eit)), const int &>);
    ASSERT_EQ(++eit, pack.each().end());
    ASSERT_TRUE(eit == pack.each().end());
    ASSERT_FALSE(eit != pack.each().end());

    {
	    std::size_t next{};
	    
	    for(const auto [entt, cv, iv]: pack.each()) {
	        static_assert(std::is_same_v<decltype(entt), const entt::entity>);
	        static_assert(std::is_same_v<decltype(cv), char &>);
	        static_assert(std::is_same_v<decltype(iv), const int &>);
	        ASSERT_EQ(entt::to_integral(entt), ++next);
	        ASSERT_EQ(&cv, registry.try_get<char>(entt));
	        ASSERT_EQ(&iv, registry.try_get<int>(entt));
	    }
    }
}

TEST(ViewPack, LongestPool) {
    entt::registry registry;
    entt::entity entities[3];

    registry.create(std::begin(entities), std::end(entities));

    registry.insert<int>(std::begin(entities), std::end(entities));
    registry.insert<empty_type>(std::begin(entities), std::end(entities));
    registry.insert<char>(std::rbegin(entities), std::rend(entities) - 1u);

    const auto pack = registry.view<int>() | registry.view<empty_type>() | registry.view<const char>();

    {
        std::size_t next{2u};
        
        for(const auto entt: pack) {
            ASSERT_EQ(entt::to_integral(entt), next--);
            ASSERT_TRUE((registry.has<int, char>(entt)));
        }
    }

    auto it = pack.begin();

    ASSERT_EQ(*it++, entities[2u]);
    ASSERT_EQ(++it, pack.end());
    ASSERT_TRUE(it == pack.end());
    ASSERT_FALSE(it != pack.end());

    pack.each([&registry, next = 2u](const auto entt, auto &&iv, auto &&cv) mutable {
        static_assert(std::is_same_v<decltype(entt), const entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
        ASSERT_EQ(entt::to_integral(entt), next--);
        ASSERT_EQ(&iv, registry.try_get<int>(entt));
        ASSERT_EQ(&cv, registry.try_get<char>(entt));
    });

    auto eit = pack.each().begin();

    ASSERT_EQ(std::get<0>(*eit++), entities[2u]);
    static_assert(std::is_same_v<decltype(std::get<1>(*eit)), int &>);
    static_assert(std::is_same_v<decltype(std::get<2>(*eit)), const char &>);
    ASSERT_EQ(++eit, pack.each().end());
    ASSERT_TRUE(eit == pack.each().end());
    ASSERT_FALSE(eit != pack.each().end());

    {
        std::size_t next{2u};
        
        for(const auto [entt, iv, cv]: pack.each()) {
            static_assert(std::is_same_v<decltype(entt), const entt::entity>);
            static_assert(std::is_same_v<decltype(iv), int &>);
            static_assert(std::is_same_v<decltype(cv), const char &>);
            ASSERT_EQ(entt::to_integral(entt), next--);
            ASSERT_EQ(&iv, registry.try_get<int>(entt));
            ASSERT_EQ(&cv, registry.try_get<char>(entt));
        }
    }
}
