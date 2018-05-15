#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

TEST(View, SingleComponent) {
    entt::DefaultRegistry registry;
    auto view = registry.view<char>();

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_TRUE(view.empty());

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});
    ASSERT_FALSE(view.empty());

    registry.assign<char>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    view.get(e0) = '1';
    view.get(e1) = '2';

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_TRUE(cview.get(entity) == '1' || cview.get(entity) == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e1);
    ASSERT_EQ(*(view.data() + 1), e0);

    ASSERT_EQ(*(view.raw() + 0), '2');
    ASSERT_EQ(*(static_cast<const decltype(view) &>(view).raw() + 1), '1');

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.empty());
}

TEST(View, SingleComponentBeginEnd) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int>();
    const auto &cview = view;

    for(auto i = 0; i < 3; ++i) {
        registry.assign<int>(registry.create());
    }

    auto test = [](auto begin, auto end) {
        ASSERT_NE(begin, end);
        ASSERT_NE(++begin, end);
        ASSERT_NE(begin++, end);
        ASSERT_EQ(begin+1, end);
        ASSERT_NE(begin, end);
        ASSERT_EQ((begin += 1), end);
        ASSERT_EQ(begin, end);
    };

    test(view.begin(), view.end());
    test(cview.begin(), cview.end());
    test(view.cbegin(), view.cend());
}

TEST(View, SingleComponentContains) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    registry.destroy(e0);

    auto view = registry.view<int>();

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(View, SingleComponentEmpty) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{0});

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(View, SingleComponentEach) {
    entt::DefaultRegistry registry;

    registry.assign<int>(registry.create());
    registry.assign<int>(registry.create());

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
    auto view = registry.view<int, char>();

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_FALSE(view.empty());

    registry.assign<char>(e1);

    auto it = registry.view<char>().begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(*(it+1), e0);
    ASSERT_EQ(it += 2, registry.view<char>().end());

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.begin()+1, view.end());
    ASSERT_EQ(view.size(), decltype(view.size()){1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_EQ(std::get<0>(cview.get<int, char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<char>(entity), '2');
    }

    registry.remove<char>(e0);
    registry.remove<char>(e1);
}

TEST(View, MultipleComponentBeginEnd) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int, char>();
    const auto &cview = view;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity);
        registry.assign<char>(entity);
    }

    auto test = [](auto begin, auto end) {
        ASSERT_NE(begin, end);
        ASSERT_NE(++begin, end);
        ASSERT_NE(begin++, end);
        ASSERT_EQ(begin+1, end);
        ASSERT_NE(begin, end);
        ASSERT_EQ((begin += 1), end);
        ASSERT_EQ(begin, end);
    };

    test(cview.begin(), cview.end());
    test(view.begin(), view.end());
    test(view.cbegin(), view.cend());
}

TEST(View, MultipleComponentContains) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    auto view = registry.view<int, char>();

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(View, MultipleComponentEmpty) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    auto view = registry.view<char, int, float>();

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(View, MultipleComponentEach) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto view = registry.view<int, char>();
    const auto &cview = static_cast<const decltype(view) &>(view);
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});

    cview.each([&cnt](auto, const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(View, MultipleComponentEachWithHoles) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<char>(e0, '0');
    registry.assign<char>(e1, '1');

    registry.assign<int>(e0, 0);
    registry.assign<int>(e2, 2);

    auto view = registry.view<char, int>();

    view.each([&](auto entity, const char &c, const int &i) {
        if(entity == e0) {
            ASSERT_EQ(c, '0');
            ASSERT_EQ(i, 0);
        } else {
            FAIL();
        }
    });
}

TEST(PersistentView, Prepare) {
    entt::DefaultRegistry registry;
    registry.prepare<int, char>();
    auto view = registry.view<int, char>(entt::persistent_t{});

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(view.empty());
    ASSERT_NO_THROW((registry.view<int, char>(entt::persistent_t{}).begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>(entt::persistent_t{}).begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<int>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.remove<int>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_EQ(std::get<0>(cview.get<int, char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<char>(entity), '2');
    }

    ASSERT_EQ(*(view.data() + 0), e1);

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.empty());
}

TEST(PersistentView, NoPrepare) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int, char>(entt::persistent_t{});

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(view.empty());
    ASSERT_NO_THROW((registry.view<int, char>(entt::persistent_t{}).begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>(entt::persistent_t{}).begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<int>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.remove<int>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        const auto &cview = static_cast<const decltype(view) &>(view);
        ASSERT_EQ(std::get<0>(cview.get<int, char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<char>(entity), '2');
    }

    ASSERT_EQ(*(view.data() + 0), e1);

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.empty());
}

TEST(PersistentView, BeginEnd) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int, char>(entt::persistent_t{});
    const auto &cview = view;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity);
        registry.assign<char>(entity);
    }

    auto test = [](auto begin, auto end) {
        ASSERT_NE(begin, end);
        ASSERT_NE(++begin, end);
        ASSERT_NE(begin++, end);
        ASSERT_EQ(begin+1, end);
        ASSERT_NE(begin, end);
        ASSERT_EQ((begin += 1), end);
        ASSERT_EQ(begin, end);
    };

    test(cview.begin(), cview.end());
    test(view.begin(), view.end());
    test(view.cbegin(), view.cend());
}

TEST(PersistentView, Contains) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    auto view = registry.view<int, char>(entt::persistent_t{});

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(PersistentView, Empty) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    for(auto entity: registry.view<char, int, float>(entt::persistent_t{})) {
        (void)entity;
        FAIL();
    }

    for(auto entity: registry.view<double, char, int, float>(entt::persistent_t{})) {
        (void)entity;
        FAIL();
    }
}

TEST(PersistentView, Each) {
    entt::DefaultRegistry registry;
    registry.prepare<int, char>();

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto view = registry.view<int, char>(entt::persistent_t{});
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

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    auto uval = 0u;
    auto ival = 0;

    registry.assign<unsigned int>(e0, uval++);
    registry.assign<unsigned int>(e1, uval++);
    registry.assign<unsigned int>(e2, uval++);

    registry.assign<int>(e0, ival++);
    registry.assign<int>(e1, ival++);
    registry.assign<int>(e2, ival++);

    auto view = registry.view<int, unsigned int>(entt::persistent_t{});

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

TEST(RawView, Functionalities) {
    entt::DefaultRegistry registry;
    auto view = registry.view<char>(entt::raw_t{});

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(view.empty());
    ASSERT_NO_THROW(registry.view<char>(entt::raw_t{}).begin()++);
    ASSERT_NO_THROW(++registry.view<char>(entt::raw_t{}).begin());

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});

    registry.assign<char>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';

    for(auto &&component: view) {
        ASSERT_TRUE(component == '1' || component == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e1);
    ASSERT_EQ(*(view.data() + 1), e0);

    ASSERT_EQ(*(view.raw() + 0), '2');
    ASSERT_EQ(*(static_cast<const decltype(view) &>(view).raw() + 1), '1');

    for(auto &&component: view) {
        // verifies that iterators return references to components
        component = '0';
    }

    for(auto &&component: view) {
        ASSERT_TRUE(component == '0');
    }

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.empty());
}

TEST(RawView, BeginEnd) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int>(entt::raw_t{});
    const auto &cview = view;

    for(auto i = 0; i < 3; ++i) {
        registry.assign<int>(registry.create());
    }

    auto test = [](auto begin, auto end) {
        ASSERT_NE(begin, end);
        ASSERT_NE(++begin, end);
        ASSERT_NE(begin++, end);
        ASSERT_EQ(begin+1, end);
        ASSERT_NE(begin, end);
        ASSERT_EQ((begin += 1), end);
        ASSERT_EQ(begin, end);
    };

    test(cview.begin(), cview.end());
    test(view.begin(), view.end());
    test(view.cbegin(), view.cend());
}

TEST(RawView, Empty) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);

    auto view = registry.view<int>(entt::raw_t{});

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{0});

    for(auto &&component: view) {
        (void)component;
        FAIL();
    }
}

TEST(RawView, Each) {
    entt::DefaultRegistry registry;

    registry.assign<int>(registry.create(), 1);
    registry.assign<int>(registry.create(), 3);

    auto view = registry.view<int>(entt::raw_t{});
    const auto &cview = static_cast<const decltype(view) &>(view);
    std::size_t cnt = 0;

    view.each([&cnt](int &v) { cnt += (v % 2); });

    ASSERT_EQ(cnt, std::size_t{2});

    cview.each([&cnt](const int &v) { cnt -= (v % 2); });

    ASSERT_EQ(cnt, std::size_t{0});
}
