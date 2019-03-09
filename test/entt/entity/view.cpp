#include <utility>
#include <iterator>
#include <algorithm>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

TEST(SingleComponentView, Functionalities) {
    entt::registry<> registry;
    auto view = registry.view<char>();
    auto cview = std::as_const(registry).view<const char>();

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_TRUE(view.empty());

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_NO_THROW(registry.view<char>().begin()++);
    ASSERT_NO_THROW(++registry.view<char>().begin());

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});
    ASSERT_FALSE(view.empty());

    registry.assign<char>(e0);

    ASSERT_EQ(view.size(), typename decltype(view)::size_type{2});

    view.get(e0) = '1';
    view.get(e1) = '2';

    for(auto entity: view) {
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

TEST(SingleComponentView, ElementAccess) {
    entt::registry<> registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    for(typename decltype(view)::size_type i{}; i < view.size(); ++i) {
        ASSERT_EQ(view[i], i ? e0 : e1);
        ASSERT_EQ(cview[i], i ? e0 : e1);
    }
}

TEST(SingleComponentView, Contains) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    registry.destroy(e0);

    auto view = registry.view<int>();

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(SingleComponentView, Empty) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), entt::registry<>::size_type{0});

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(SingleComponentView, Each) {
    entt::registry<> registry;

    registry.assign<int>(registry.create());
    registry.assign<int>(registry.create());

    auto view = registry.view<int>();
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &) { ++cnt; });
    view.each([&cnt](int &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{4});

    std::as_const(view).each([&cnt](auto, const int &) { --cnt; });
    std::as_const(view).each([&cnt](const int &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(SingleComponentView, ConstNonConstAndAllInBetween) {
    entt::registry<> registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_EQ(cview.size(), decltype(cview.size()){0});

    registry.assign<int>(registry.create(), 0);

    ASSERT_EQ(view.size(), decltype(view.size()){1});
    ASSERT_EQ(cview.size(), decltype(cview.size()){1});

    ASSERT_TRUE((std::is_same_v<typename decltype(view)::raw_type, int>));
    ASSERT_TRUE((std::is_same_v<typename decltype(cview)::raw_type, const int>));

    ASSERT_TRUE((std::is_same_v<decltype(view.get(0)), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.raw()), int *>));
    ASSERT_TRUE((std::is_same_v<decltype(cview.get(0)), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(cview.raw()), const int *>));

    view.each([](auto, auto &&i) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
    });

    cview.each([](auto, auto &&i) {
        ASSERT_TRUE((std::is_same_v<decltype(i), const int &>));
    });
}

TEST(SingleComponentView, Find) {
    entt::registry<> registry;
    auto view = registry.view<int>();

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    const auto e2 = registry.create();
    registry.assign<int>(e2);

    const auto e3 = registry.create();
    registry.assign<int>(e3);

    registry.remove<int>(e1);

    ASSERT_NE(view.find(e0), view.end());
    ASSERT_EQ(view.find(e1), view.end());
    ASSERT_NE(view.find(e2), view.end());
    ASSERT_NE(view.find(e3), view.end());

    auto it = view.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, view.end());
    ASSERT_EQ(++view.find(e0), view.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.assign<int>(e5);

    ASSERT_NE(view.find(e5), view.end());
    ASSERT_EQ(view.find(e4), view.end());
}

TEST(MultipleComponentView, Functionalities) {
    entt::registry<> registry;
    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_FALSE(view.empty());

    registry.assign<char>(e1);

    auto it = registry.view<int, char>().begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (registry.view<int, char>().end()));

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_EQ(view.size(), decltype(view.size()){1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(std::get<0>(cview.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<const char>(entity), '2');
    }
}

TEST(MultipleComponentView, Iterator) {
    entt::registry<> registry;
    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    const auto view = registry.view<int, char>();
    using iterator_type = typename decltype(view)::iterator_type;

    iterator_type end{view.begin()};
    iterator_type begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(view.begin()++, view.begin());
    ASSERT_EQ(++view.begin(), view.end());
}

TEST(MultipleComponentView, Contains) {
    entt::registry<> registry;

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

TEST(MultipleComponentView, Empty) {
    entt::registry<> registry;

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

TEST(MultipleComponentView, Each) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &, char &) { ++cnt; });
    view.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{4});

    cview.each([&cnt](auto, const int &, const char &) { --cnt; });
    cview.each([&cnt](const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(MultipleComponentView, EachWithType) {
    entt::registry<> registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity, i);
        registry.assign<char>(entity);
    }

    // makes char a better candidate during iterations
    const auto entity = registry.create();
    registry.assign<int>(entity, 99);

    registry.view<int, char>().each<int>([value = 2](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value--);
    });

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    registry.view<int, char>().each<int>([value = 0](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value++);
    });
}

TEST(MultipleComponentView, EachWithHoles) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<char>(e0, '0');
    registry.assign<char>(e1, '1');

    registry.assign<int>(e0, 0);
    registry.assign<int>(e2, 2);

    auto view = registry.view<char, int>();

    view.each([e0](auto entity, const char &c, const int &i) {
        if(e0 == entity) {
            ASSERT_EQ(c, '0');
            ASSERT_EQ(i, 0);
        } else {
            FAIL();
        }
    });
}

TEST(MultipleComponentView, ConstNonConstAndAllInBetween) {
    entt::registry<> registry;
    auto view = registry.view<int, const char>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), decltype(view.size()){1});

    ASSERT_TRUE((std::is_same_v<decltype(view.get<int>(0)), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<const int>(0)), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<const char>(0)), const char &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<int, const char>(0)), std::tuple<int &, const char &>>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<const int, const char>(0)), std::tuple<const int &, const char &>>));

    view.each([](auto, auto &&i, auto &&c) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
        ASSERT_TRUE((std::is_same_v<decltype(c), const char &>));
    });
}

TEST(MultipleComponentView, Find) {
    entt::registry<> registry;
    auto view = registry.view<int, const char>();

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    const auto e2 = registry.create();
    registry.assign<int>(e2);
    registry.assign<char>(e2);

    const auto e3 = registry.create();
    registry.assign<int>(e3);
    registry.assign<char>(e3);

    registry.remove<int>(e1);

    ASSERT_NE(view.find(e0), view.end());
    ASSERT_EQ(view.find(e1), view.end());
    ASSERT_NE(view.find(e2), view.end());
    ASSERT_NE(view.find(e3), view.end());

    auto it = view.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, view.end());
    ASSERT_EQ(++view.find(e0), view.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.assign<int>(e5);
    registry.assign<char>(e5);

    ASSERT_NE(view.find(e5), view.end());
    ASSERT_EQ(view.find(e4), view.end());
}

TEST(RuntimeView, Functionalities) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    // forces the creation of the pools
    registry.reserve<int>(0);
    registry.reserve<char>(0);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_TRUE(view.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_FALSE(view.empty());

    registry.assign<char>(e1);

    auto it = registry.runtime_view(std::begin(types), std::end(types)).begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (registry.runtime_view(std::begin(types), std::end(types)).end()));

    ASSERT_NO_THROW((registry.runtime_view(std::begin(types), std::end(types)).begin()++));
    ASSERT_NO_THROW((++registry.runtime_view(std::begin(types), std::end(types)).begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.size(), decltype(view.size()){1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(registry.get<int>(entity), 42);
        ASSERT_EQ(registry.get<char>(entity), '2');
    }
}

TEST(RuntimeView, Iterator) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));
    using iterator_type = typename decltype(view)::iterator_type;

    iterator_type end{view.begin()};
    iterator_type begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(view.begin()++, view.begin());
    ASSERT_EQ(++view.begin(), view.end());
}

TEST(RuntimeView, Contains) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(RuntimeView, Empty) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    component_type types[] = { registry.type<char>(), registry.type<int>(), registry.type<float>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(RuntimeView, Each) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));
    std::size_t cnt = 0;

    view.each([&cnt](auto) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{2});
}

TEST(RuntimeView, EachWithHoles) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<char>(e0, '0');
    registry.assign<char>(e1, '1');

    registry.assign<int>(e0, 0);
    registry.assign<int>(e2, 2);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    view.each([e0](auto entity) {
        ASSERT_EQ(e0, entity);
    });
}

TEST(RuntimeView, MissingPool) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    component_type types[] = { registry.type<int>(), registry.type<char>() };
    auto view = registry.runtime_view(std::begin(types), std::end(types));

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});

    registry.assign<char>(e0);

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}

TEST(RuntimeView, EmptyRange) {
    entt::registry<> registry;
    using component_type = typename decltype(registry)::component_type;

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const component_type *ptr = nullptr;
    auto view = registry.runtime_view(ptr, ptr);

    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_FALSE(view.contains(e0));

    view.each([](auto) { FAIL(); });

    for(auto entity: view) {
        (void)entity;
        FAIL();
    }
}
