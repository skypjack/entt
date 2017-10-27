#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

TEST(View, SingleComponent) {
    entt::DefaultRegistry registry;

    auto e1 = registry.create();
    auto e2 = registry.create<int, char>();

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());

    auto view = registry.view<char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<char>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    view.get(e1) = '1';
    view.get(e2) = '2';

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_TRUE(cview.get(entity) == '1' || cview.get(entity) == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e2);
    ASSERT_EQ(*(view.data() + 1), e1);

    ASSERT_EQ(*(view.raw() + 0), '2');
    ASSERT_EQ(*(static_cast<const decltype(view) &>(view).raw() + 1), '1');

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
}

TEST(View, SingleComponentEmpty) {
    entt::DefaultRegistry registry;

    registry.create<char, double>();
    registry.create<char>();

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{0});

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(View, SingleComponentEach) {
    entt::DefaultRegistry registry;

    registry.create<int, char>();
    registry.create<int, char>();

    auto view = registry.view<int>();
    const auto &cview = static_cast<const decltype(view) &>(view);
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});

    cview.each([&cnt](auto, const int &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(View, MultipleComponent) {
    entt::DefaultRegistry registry;

    auto e1 = registry.create<char>();
    auto e2 = registry.create<int, char>();

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));

    auto view = registry.view<int, char>();

    ASSERT_NE(view.begin(), view.end());

    view.get<char>(e1) = '1';
    view.get<char>(e2) = '2';

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_TRUE(cview.get<char>(entity) == '2');
    }

    registry.remove<char>(e1);
    registry.remove<char>(e2);
    view.reset();

    ASSERT_EQ(view.begin(), view.end());
}

TEST(View, MultipleComponentEmpty) {
    entt::DefaultRegistry registry;

    registry.create<double, int, float>();
    registry.create<char, float>();

    auto view = registry.view<char, int, float>();

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(View, MultipleComponentEach) {
    entt::DefaultRegistry registry;

    registry.create<int, char>();
    registry.create<int, char>();

    auto view = registry.view<int, char>();
    const auto &cview = static_cast<const decltype(view) &>(view);
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});

    cview.each([&cnt](auto, const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(PersistentView, Prepare) {
    entt::DefaultRegistry registry;
    registry.prepare<int, char>();

    auto e1 = registry.create<char>();
    auto e2 = registry.create<int, char>();

    ASSERT_NO_THROW((registry.persistent<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.persistent<int, char>().begin()));

    auto view = registry.persistent<int, char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<int>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.remove<int>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    view.get<char>(e1) = '1';
    view.get<char>(e2) = '2';

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_TRUE(cview.get<char>(entity) == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e2);

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
}

TEST(PersistentView, NoPrepare) {
    entt::DefaultRegistry registry;

    auto e1 = registry.create<char>();
    auto e2 = registry.create<int, char>();

    ASSERT_NO_THROW((registry.persistent<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.persistent<int, char>().begin()));

    auto view = registry.persistent<int, char>();

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<int>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.remove<int>(e1);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    view.get<char>(e1) = '1';
    view.get<char>(e2) = '2';

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_TRUE(cview.get<char>(entity) == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e2);

    registry.remove<char>(e1);
    registry.remove<char>(e2);

    ASSERT_EQ(view.begin(), view.end());
}

TEST(PersistentView, Empty) {
    entt::DefaultRegistry registry;

    registry.create<double, int, float>();
    registry.create<char, float>();

    for(auto entity: registry.persistent<char, int, float>()) {
        (void)entity;
        FAIL();
    }

    for(auto entity: registry.persistent<double, char, int, float>()) {
        (void)entity;
        FAIL();
    }
}

TEST(PersistentView, Each) {
    entt::DefaultRegistry registry;
    registry.prepare<int, char>();

    registry.create<int, char>();
    registry.create<int, char>();

    auto view = registry.persistent<int, char>();
    const auto &cview = static_cast<const decltype(view) &>(view);
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});

    cview.each([&cnt](auto, const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(PersistentView, Sort) {
    entt::DefaultRegistry registry;
    registry.prepare<int, unsigned int>();

    auto e1 = registry.create();
    auto e2 = registry.create();
    auto e3 = registry.create();

    auto uval = 0u;
    auto ival = 0;

    registry.assign<unsigned int>(e1, uval++);
    registry.assign<unsigned int>(e2, uval++);
    registry.assign<unsigned int>(e3, uval++);

    registry.assign<int>(e1, ival++);
    registry.assign<int>(e2, ival++);
    registry.assign<int>(e3, ival++);

    auto view = registry.persistent<int, unsigned int>();

    for(auto entity: view) {
        ASSERT_EQ(view.get<unsigned int>(entity), --uval);
        ASSERT_EQ(view.get<int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less<unsigned int>{});
    view.sort<unsigned int>();

    for(auto entity: view) {
        ASSERT_EQ(view.get<unsigned int>(entity), uval++);
        ASSERT_EQ(view.get<int>(entity), ival++);
    }
}
