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

TEST(ViewPack, Functionalities) {
    entt::registry registry;
    const auto pack = registry.view<int>() | registry.view<char>();
    const auto cpack = registry.view<const int>() | registry.view<const char>();

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 42);
    registry.emplace<char>(e1, '2');

    ASSERT_EQ(*pack.begin(), e1);
    ASSERT_EQ(*pack.rbegin(), e1);
    ASSERT_EQ(++pack.begin(), (pack.end()));
    ASSERT_EQ(++pack.rbegin(), (pack.rend()));

    ASSERT_NO_THROW((pack.begin()++));
    ASSERT_NO_THROW((++cpack.begin()));
    ASSERT_NO_THROW(pack.rbegin()++);
    ASSERT_NO_THROW(++cpack.rbegin());

    ASSERT_NE(pack.begin(), pack.end());
    ASSERT_NE(cpack.begin(), cpack.end());
    ASSERT_NE(pack.rbegin(), pack.rend());
    ASSERT_NE(cpack.rbegin(), cpack.rend());

    for(auto entity: pack) {
        ASSERT_EQ(std::get<0>(cpack.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(pack.get<int, char>(entity)), '2');
        ASSERT_EQ(cpack.get<const char>(entity), '2');
    }
}

TEST(ViewPack, Iterator) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto pack = registry.view<int>() | registry.view<char>();

    ASSERT_NE(pack.begin(), pack.end());
    ASSERT_EQ(pack.begin()++, pack.begin());
    ASSERT_EQ(++pack.begin(), pack.end());
    ASSERT_EQ(*pack.begin(), entity);
}

TEST(ViewPack, ReverseIterator) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto pack = registry.view<int>() | registry.view<char>();

    ASSERT_NE(pack.rbegin(), pack.rend());
    ASSERT_EQ(pack.rbegin()++, pack.rbegin());
    ASSERT_EQ(++pack.rbegin(), pack.rend());
    ASSERT_EQ(*pack.rbegin(), entity);
}

TEST(ViewPack, Contains) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    const auto pack = registry.view<int>() | registry.view<char>();

    ASSERT_FALSE(pack.contains(e0));
    ASSERT_TRUE(pack.contains(e1));
}

TEST(ViewPack, Each) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1);

    auto pack = registry.view<int>() | registry.view<char>();
    auto cpack = registry.view<const int>() | registry.view<const char>();
    std::size_t cnt = 0;

    for(auto first = cpack.each().rbegin(), last = cpack.each().rend(); first != last; ++first) {
        static_assert(std::is_same_v<decltype(*first), std::tuple<entt::entity, const int &, const char &>>);
        ASSERT_EQ(std::get<1>(*first), cnt++);
    }

    pack.each([&cnt](auto, int &, char &) { ++cnt; });
    pack.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{6});

    cpack.each([&cnt](const int &, const char &) { --cnt; });
    cpack.each([&cnt](auto, const int &, const char &) { --cnt; });

    for(auto [entt, iv, cv]: pack.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(iv, --cnt);
    }

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(ViewPack, EachWithHoles) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<char>(e0, '0');
    registry.emplace<char>(e1, '1');

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e2, 2);

    auto pack = registry.view<char>() | registry.view<int>();

    pack.each([e0](auto entity, const char &c, const int &i) {
        ASSERT_EQ(entity, e0);
        ASSERT_EQ(c, '0');
        ASSERT_EQ(i, 0);
    });

    for(auto &&curr: pack.each()) {
        ASSERT_EQ(std::get<0>(curr), e0);
        ASSERT_EQ(std::get<1>(curr), '0');
        ASSERT_EQ(std::get<2>(curr), 0);
    }
}

TEST(ViewPack, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto pack = registry.view<int, empty_type>() | registry.view<const char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<empty_type>(entity);
    registry.emplace<char>(entity, 'c');

    static_assert(std::is_same_v<decltype(pack.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(pack.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(pack.get<int, const char>({})), std::tuple<int &, const char &>>);
    static_assert(std::is_same_v<decltype(pack.get({})), std::tuple<int &, const char &>>);

    pack.each([](auto &&i, auto &&c) {
        static_assert(std::is_same_v<decltype(i), int &>);
        static_assert(std::is_same_v<decltype(c), const char &>);
    });

    for(auto [entt, iv, cv]: pack.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
    }
}

TEST(ViewPack, Find) {
    entt::registry registry;
    auto pack = registry.view<int>() | registry.view<const char>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    const auto e2 = registry.create();
    registry.emplace<int>(e2);
    registry.emplace<char>(e2);

    const auto e3 = registry.create();
    registry.emplace<int>(e3);
    registry.emplace<char>(e3);

    registry.remove<int>(e1);

    ASSERT_NE(pack.find(e0), pack.end());
    ASSERT_EQ(pack.find(e1), pack.end());
    ASSERT_NE(pack.find(e2), pack.end());
    ASSERT_NE(pack.find(e3), pack.end());

    auto it = pack.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, pack.end());
    ASSERT_EQ(++pack.find(e0), pack.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.emplace<int>(e5);
    registry.emplace<char>(e5);

    ASSERT_NE(pack.find(e5), pack.end());
    ASSERT_EQ(pack.find(e4), pack.end());
}

TEST(ViewPack, FrontBack) {
    entt::registry registry;
    auto pack = registry.view<const int>() | registry.view<const char>();

    ASSERT_EQ(pack.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(pack.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    const auto entity = registry.create();
    registry.emplace<char>(entity);

    ASSERT_EQ(pack.front(), e1);
    ASSERT_EQ(pack.back(), e0);
}

TEST(ViewPack, ShortestPool) {
    entt::registry registry;
    entt::entity entities[4];

    registry.create(std::begin(entities), std::end(entities));

    registry.insert<int>(std::begin(entities), std::end(entities));
    registry.insert<empty_type>(std::begin(entities), std::end(entities));
    registry.insert<char>(std::rbegin(entities) + 1u, std::rend(entities) - 1u);

    const auto tmp = registry.view<char>() | registry.view<empty_type>();
    const auto pack = tmp | registry.view<const int>();

    {
        std::size_t next{};

        for(const auto entt: pack) {
            ASSERT_EQ(entt::to_integral(entt), ++next);
            ASSERT_TRUE((registry.all_of<int, char>(entt)));
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

    pack.each([](auto &&cv, auto &&iv) {
        static_assert(std::is_same_v<decltype(cv), char &>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
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
    entt::entity entities[4];

    registry.create(std::begin(entities), std::end(entities));

    registry.insert<int>(std::begin(entities), std::end(entities));
    registry.insert<empty_type>(std::begin(entities), std::end(entities));
    registry.insert<char>(std::rbegin(entities) + 1u, std::rend(entities) - 1u);

    const auto pack = registry.view<int>() | registry.view<empty_type>() | registry.view<const char>();

    {
        std::size_t next{2u};

        for(const auto entt: pack) {
            ASSERT_EQ(entt::to_integral(entt), next--);
            ASSERT_TRUE((registry.all_of<int, char>(entt)));
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

    pack.each([](auto &&iv, auto &&cv) {
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
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

TEST(ViewPack, RepeatedType) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 3);

    const auto view = registry.view<int>();
    const auto pack = view | view;

    for(auto [entt, i1, i2]: pack.each()) {
        ASSERT_EQ(entt, entity);
        ASSERT_EQ(i1, 3);
        ASSERT_EQ(i1, i2);
    }
}
