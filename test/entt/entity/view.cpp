#include <utility>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

TEST(SingleComponentView, Functionalities) {
    entt::registry registry;
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

    view.get<char>(e0) = '1';
    view.get(e1) = '2';

    for(auto entity: view) {
        ASSERT_TRUE(cview.get<const char>(entity) == '1' || cview.get(entity) == '2');
    }

    ASSERT_EQ(*(view.data() + 0), e1);
    ASSERT_EQ(*(view.data() + 1), e0);

    ASSERT_EQ(*(view.raw() + 0), '2');
    ASSERT_EQ(*(cview.raw() + 1), '1');

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.empty());
}

TEST(SingleComponentView, ElementAccess) {
    entt::registry registry;
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
    entt::registry registry;

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
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});
    ASSERT_EQ(view.begin(), view.end());
}

TEST(SingleComponentView, Each) {
    entt::registry registry;

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
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_EQ(cview.size(), decltype(cview.size()){0});

    registry.assign<int>(registry.create(), 0);

    ASSERT_EQ(view.size(), decltype(view.size()){1});
    ASSERT_EQ(cview.size(), decltype(cview.size()){1});

    ASSERT_TRUE((std::is_same_v<typename decltype(view)::raw_type, int>));
    ASSERT_TRUE((std::is_same_v<typename decltype(cview)::raw_type, const int>));

    ASSERT_TRUE((std::is_same_v<decltype(view.get({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.raw()), int *>));
    ASSERT_TRUE((std::is_same_v<decltype(cview.get({})), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(cview.raw()), const int *>));

    view.each([](auto &&i) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
    });

    cview.each([](auto &&i) {
        ASSERT_TRUE((std::is_same_v<decltype(i), const int &>));
    });
}

TEST(SingleComponentView, Find) {
    entt::registry registry;
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

TEST(SingleComponentView, Less) {
    entt::registry registry;
    auto create = [&](auto... component) {
        const auto entt = registry.create();
        (registry.assign<decltype(component)>(entt, component), ...);
        return entt;
    };

    const auto entity = create(0, entt::tag<"empty"_hs>{});
    create('c');

    registry.view<entt::tag<"empty"_hs>>().less([entity](const auto entt) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<entt::tag<"empty"_hs>>().less([check = true]() mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    registry.view<int>().less([entity](const auto entt, int) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<int>().less([check = true](int) mutable {
        ASSERT_TRUE(check);
        check = false;
    });
}

TEST(SingleComponentView, FrontBack) {
    entt::registry registry;
    auto view = registry.view<const int>();

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_EQ(view.front(), e1);
    ASSERT_EQ(view.back(), e0);
}

TEST(MultiComponentView, Functionalities) {
    entt::registry registry;
    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();

    ASSERT_TRUE(view.empty());
    ASSERT_TRUE((view.empty<int, char>()));
    ASSERT_TRUE((cview.empty<const int, const char>()));

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);

    ASSERT_FALSE(view.empty());
    ASSERT_FALSE((view.empty<int>()));
    ASSERT_FALSE((cview.empty<const char>()));

    registry.assign<char>(e1);

    auto it = registry.view<int, char>().begin();

    ASSERT_EQ(*it, e1);
    ASSERT_EQ(++it, (registry.view<int, char>().end()));

    ASSERT_NO_THROW((registry.view<int, char>().begin()++));
    ASSERT_NO_THROW((++registry.view<int, char>().begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_EQ(view.size(), decltype(view.size()){1});
    ASSERT_EQ(view.size<int>(), decltype(view.size()){1});
    ASSERT_EQ(cview.size<const char>(), decltype(view.size()){2});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: view) {
        ASSERT_EQ(std::get<0>(cview.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<const char>(entity), '2');
    }

    ASSERT_EQ(*(view.data<int>() + 0), e1);
    ASSERT_EQ(*(view.data<char>() + 0), e0);
    ASSERT_EQ(*(cview.data<const char>() + 1), e1);

    ASSERT_EQ(*(view.raw<int>() + 0), 42);
    ASSERT_EQ(*(view.raw<char>() + 0), '1');
    ASSERT_EQ(*(cview.raw<const char>() + 1), '2');
}

TEST(MultiComponentView, Iterator) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    const auto view = registry.view<int, char>();
    using iterator = typename decltype(view)::iterator;

    iterator end{view.begin()};
    iterator begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, view.begin());
    ASSERT_EQ(begin--, view.end());

    ASSERT_EQ(++begin, view.end());
    ASSERT_EQ(--begin, view.begin());

    ASSERT_EQ(*begin, entity);
    ASSERT_EQ(*begin.operator->(), entity);
}

TEST(MultiComponentView, Contains) {
    entt::registry registry;

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

TEST(MultiComponentView, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    auto view = registry.view<char, int, float>();

    ASSERT_EQ(view.size(), entt::registry::size_type{1});
    ASSERT_EQ(view.begin(), view.end());
}

TEST(MultiComponentView, Each) {
    entt::registry registry;

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

TEST(MultiComponentView, EachWithType) {
    entt::registry registry;

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

TEST(MultiComponentView, EachWithHoles) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<char>(e0, '0');
    registry.assign<char>(e1, '1');

    registry.assign<int>(e0, 0);
    registry.assign<int>(e2, 2);

    auto view = registry.view<char, int>();

    view.each([e0](auto entity, const char &c, const int &i) {
        ASSERT_EQ(entity, e0);
        ASSERT_EQ(c, '0');
        ASSERT_EQ(i, 0);
    });
}

TEST(MultiComponentView, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto view = registry.view<int, const char>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), decltype(view.size()){1});

    ASSERT_TRUE((std::is_same_v<decltype(view.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<const char>({})), const char &>));
    ASSERT_TRUE((std::is_same_v<decltype(view.get<int, const char>({})), std::tuple<int &, const char &>>));
    ASSERT_TRUE((std::is_same_v<decltype(view.raw<const char>()), const char *>));
    ASSERT_TRUE((std::is_same_v<decltype(view.raw<int>()), int *>));

    view.each([](auto &&i, auto &&c) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
        ASSERT_TRUE((std::is_same_v<decltype(c), const char &>));
    });
}

TEST(MultiComponentView, Find) {
    entt::registry registry;
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

TEST(MultiComponentView, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1);

    const auto view = registry.view<int>(entt::exclude<char>);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);

    const auto e3 = registry.create();
    registry.assign<int>(e3, 3);
    registry.assign<char>(e3);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(view.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(view.get(e2), 2);
        }
    }

    registry.assign<char>(e0);
    registry.assign<char>(e2);
    registry.remove<char>(e1);
    registry.remove<char>(e3);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(view.get(e1), 1);
        } else if(entity == e3) {
            ASSERT_EQ(view.get<int>(e3), 3);
        }
    }
}

TEST(MultiComponentView, Less) {
    entt::registry registry;

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);
    registry.assign<double>(entity);
    registry.assign<entt::tag<"empty"_hs>>(entity);

    const auto other = registry.create();
    registry.assign<int>(other);
    registry.assign<char>(other);

    registry.view<int, char, entt::tag<"empty"_hs>>().less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<int, entt::tag<"empty"_hs>, char>().less([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    registry.view<entt::tag<"empty"_hs>, int, char>().less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<entt::tag<"empty"_hs>, int, char>().less<entt::tag<"empty"_hs>>([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<int, entt::tag<"empty"_hs>, char>().less<entt::tag<"empty"_hs>>([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    registry.view<int, char, double>().less([entity](const auto entt, int, char, double) {
        ASSERT_EQ(entity, entt);
    });
}

TEST(MultiComponentView, FrontBack) {
    entt::registry registry;
    auto view = registry.view<const int, const char>();

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    const auto entity = registry.create();
    registry.assign<char>(entity);

    ASSERT_EQ(view.front(), e1);
    ASSERT_EQ(view.back(), e0);
}
