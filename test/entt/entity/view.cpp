#include <utility>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

struct empty_type {};

TEST(SingleComponentView, Functionalities) {
    entt::registry registry;
    auto view = registry.view<char>();
    auto cview = std::as_const(registry).view<const char>();

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_TRUE(view.empty());

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    ASSERT_NO_THROW(view.begin()++);
    ASSERT_NO_THROW(++cview.begin());
    ASSERT_NO_THROW([](auto it) { return it++; }(view.rbegin()));
    ASSERT_NO_THROW([](auto it) { return ++it; }(cview.rbegin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(view.rbegin(), view.rend());
    ASSERT_NE(cview.rbegin(), cview.rend());
    ASSERT_EQ(view.size(), typename decltype(view)::size_type{1});
    ASSERT_FALSE(view.empty());

    registry.emplace<char>(e0);

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
    ASSERT_EQ(view.rbegin(), view.rend());
    ASSERT_TRUE(view.empty());
}

TEST(SingleComponentView, ElementAccess) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    for(typename decltype(view)::size_type i{}; i < view.size(); ++i) {
        ASSERT_EQ(view[i], i ? e0 : e1);
        ASSERT_EQ(cview[i], i ? e0 : e1);
    }
}

TEST(SingleComponentView, Contains) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    registry.destroy(e0);

    auto view = registry.view<int>();

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(SingleComponentView, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<char>(e0);
    registry.emplace<double>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);

    auto view = registry.view<int>();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
}

TEST(SingleComponentView, EachAndProxy) {
    entt::registry registry;

    registry.emplace<int>(registry.create());
    registry.emplace<int>(registry.create());

    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &) { ++cnt; });
    view.each([&cnt](int &) { ++cnt; });

    for(auto &&[entt, iv]: view.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ++cnt;
    }

    ASSERT_EQ(cnt, std::size_t{6});

    cview.each([&cnt](auto, const int &) { --cnt; });
    cview.each([&cnt](const int &) { --cnt; });

    for(auto &&[entt, iv]: cview.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
        --cnt;
    }

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(SingleComponentView, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});
    ASSERT_EQ(cview.size(), decltype(cview.size()){0});

    registry.emplace<int>(registry.create(), 0);

    ASSERT_EQ(view.size(), decltype(view.size()){1});
    ASSERT_EQ(cview.size(), decltype(cview.size()){1});

    static_assert(std::is_same_v<typename decltype(view)::raw_type, int>);
    static_assert(std::is_same_v<typename decltype(cview)::raw_type, const int>);

    static_assert(std::is_same_v<decltype(view.get({})), int &>);
    static_assert(std::is_same_v<decltype(view.raw()), int *>);
    static_assert(std::is_same_v<decltype(cview.get({})), const int &>);
    static_assert(std::is_same_v<decltype(cview.raw()), const int *>);

    view.each([](auto &&i) {
        static_assert(std::is_same_v<decltype(i), int &>);
    });

    cview.each([](auto &&i) {
        static_assert(std::is_same_v<decltype(i), const int &>);
    });

    for(auto &&[entt, iv]: view.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
    }

    for(auto &&[entt, iv]: cview.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
    }
}

TEST(SingleComponentView, Find) {
    entt::registry registry;
    auto view = registry.view<int>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    const auto e2 = registry.create();
    registry.emplace<int>(e2);

    const auto e3 = registry.create();
    registry.emplace<int>(e3);

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
    registry.emplace<int>(e5);

    ASSERT_NE(view.find(e5), view.end());
    ASSERT_EQ(view.find(e4), view.end());
}

TEST(SingleComponentView, EmptyTypes) {
    entt::registry registry;
    auto create = [&](auto... component) {
        const auto entt = registry.create();
        (registry.emplace<decltype(component)>(entt, component), ...);
        return entt;
    };

    const auto entity = create(0, empty_type{});
    create('c');

    registry.view<empty_type>().each([entity](const auto entt) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<empty_type>().each([check = true]() mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto &&[entt]: registry.view<empty_type>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int>().each([entity](const auto entt, int) {
        ASSERT_EQ(entity, entt);
    });

    registry.view<int>().each([check = true](int) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto &&[entt, iv]: registry.view<int>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_EQ(entity, entt);
    }
}

TEST(SingleComponentView, FrontBack) {
    entt::registry registry;
    auto view = registry.view<const int>();

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

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
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    ASSERT_FALSE(view.empty());
    ASSERT_FALSE((view.empty<int>()));
    ASSERT_FALSE((cview.empty<const char>()));

    registry.emplace<char>(e1);

    ASSERT_EQ(*view.begin(), e1);
    ASSERT_EQ(*view.rbegin(), e1);
    ASSERT_EQ(++view.begin(), (view.end()));
    ASSERT_EQ(++view.rbegin(), (view.rend()));

    ASSERT_NO_THROW((view.begin()++));
    ASSERT_NO_THROW((++cview.begin()));
    ASSERT_NO_THROW(view.rbegin()++);
    ASSERT_NO_THROW(++cview.rbegin());

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(view.rbegin(), view.rend());
    ASSERT_NE(cview.rbegin(), cview.rend());
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
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

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

TEST(MultiComponentView, ReverseIterator) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto view = registry.view<int, char>();
    using iterator = typename decltype(view)::reverse_iterator;

    iterator end{view.rbegin()};
    iterator begin{};
    begin = view.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.rbegin());
    ASSERT_EQ(end, view.rend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, view.rbegin());
    ASSERT_EQ(begin--, view.rend());

    ASSERT_EQ(++begin, view.rend());
    ASSERT_EQ(--begin, view.rbegin());

    ASSERT_EQ(*begin, entity);
    ASSERT_EQ(*begin.operator->(), entity);
}

TEST(MultiComponentView, Contains) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    auto view = registry.view<int, char>();

    ASSERT_FALSE(view.contains(e0));
    ASSERT_TRUE(view.contains(e1));
}

TEST(MultiComponentView, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    auto view = registry.view<char, int, float>();

    ASSERT_EQ(view.size(), entt::registry::size_type{1});
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
}

TEST(MultiComponentView, EachAndProxy) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();
    std::size_t cnt = 0;

    view.each([&cnt](auto, int &, char &) { ++cnt; });
    view.each([&cnt](int &, char &) { ++cnt; });

    for(auto &&[entt, iv, cv]: view.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ++cnt;
    }

    ASSERT_EQ(cnt, std::size_t{6});

    cview.each([&cnt](auto, const int &, const char &) { --cnt; });
    cview.each([&cnt](const int &, const char &) { --cnt; });

    for(auto &&[entt, iv, cv]: cview.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
        --cnt;
    }

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(MultiComponentView, EachAndProxyWithSuggestedType) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<int>(entity, i);
        registry.emplace<char>(entity);
    }

    // makes char a better candidate during iterations
    const auto entity = registry.create();
    registry.emplace<int>(entity, 99);

    registry.view<int, char>().each<int>([value = 2](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value--);
    });

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    registry.view<int, char>().each<int>([value = 0](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value++);
    });

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs > rhs;
    });

    auto value = registry.view<int, char>().size();

    for(auto &&curr: registry.view<int, char>().proxy()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(--value));
    }

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    value = {};

    for(auto &&curr: registry.view<int, char>().proxy<int>()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(value++));
    }
}

TEST(MultiComponentView, EachAndProxyWithHoles) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<char>(e0, '0');
    registry.emplace<char>(e1, '1');

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e2, 2);

    auto view = registry.view<char, int>();

    view.each([e0](auto entity, const char &c, const int &i) {
        ASSERT_EQ(entity, e0);
        ASSERT_EQ(c, '0');
        ASSERT_EQ(i, 0);
    });

    for(auto &&curr: view.proxy()) {
        ASSERT_EQ(std::get<0>(curr), e0);
        ASSERT_EQ(std::get<1>(curr), '0');
        ASSERT_EQ(std::get<2>(curr), 0);
    }
}

TEST(MultiComponentView, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto view = registry.view<int, const char>();

    ASSERT_EQ(view.size(), decltype(view.size()){0});

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(view.size(), decltype(view.size()){1});

    static_assert(std::is_same_v<decltype(view.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(view.get<int, const char>({})), std::tuple<int &, const char &>>);
    static_assert(std::is_same_v<decltype(view.raw<const char>()), const char *>);
    static_assert(std::is_same_v<decltype(view.raw<int>()), int *>);

    view.each([](auto &&i, auto &&c) {
        static_assert(std::is_same_v<decltype(i), int &>);
        static_assert(std::is_same_v<decltype(c), const char &>);
    });

    for(auto &&[entt, iv, cv]: view.proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
    }
}

TEST(MultiComponentView, Find) {
    entt::registry registry;
    auto view = registry.view<int, const char>();

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
    registry.emplace<int>(e5);
    registry.emplace<char>(e5);

    ASSERT_NE(view.find(e5), view.end());
    ASSERT_EQ(view.find(e4), view.end());
}

TEST(MultiComponentView, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1);

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 2);

    const auto e3 = registry.create();
    registry.emplace<int>(e3, 3);
    registry.emplace<char>(e3);

    const auto view = std::as_const(registry).view<const int>(entt::exclude<char>);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(view.get<const int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(view.get(e2), 2);
        }
    }

    registry.emplace<char>(e0);
    registry.emplace<char>(e2);
    registry.remove<char>(e1);
    registry.remove<char>(e3);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(view.get(e1), 1);
        } else if(entity == e3) {
            ASSERT_EQ(view.get<const int>(e3), 3);
        }
    }
}

TEST(MultiComponentView, EmptyTypes) {
    entt::registry registry;

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<double>(entity);
    registry.emplace<empty_type>(entity);

    const auto other = registry.create();
    registry.emplace<int>(other);
    registry.emplace<char>(other);

    registry.view<int, char, empty_type>().each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto &&[entt, iv, cv]: registry.view<int, char, empty_type>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, empty_type, char>().each([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto &&[entt, iv, cv]: registry.view<int, empty_type, char>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<empty_type, int, char>().each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto &&[entt, iv, cv]: registry.view<empty_type, int, char>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<empty_type, int, char>().each<empty_type>([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto &&[entt, iv, cv]: registry.view<empty_type, int, char>().proxy<empty_type>()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, empty_type, char>().each<empty_type>([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto &&[entt, iv, cv]: registry.view<int, empty_type, char>().proxy<empty_type>()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, char, double>().each([entity](const auto entt, int, char, double) {
        ASSERT_EQ(entity, entt);
    });

    for(auto &&[entt, iv, cv, dv]: registry.view<int, char, double>().proxy()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        static_assert(std::is_same_v<decltype(dv), double &>);
        ASSERT_EQ(entity, entt);
    }
}

TEST(MultiComponentView, FrontBack) {
    entt::registry registry;
    auto view = registry.view<const int, const char>();

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    const auto entity = registry.create();
    registry.emplace<char>(entity);

    ASSERT_EQ(view.front(), e1);
    ASSERT_EQ(view.back(), e0);
}

TEST(MultiComponentView, ChunkedEmpty) {
    entt::registry registry;
    auto view = registry.view<const entt::id_type, const char>();

    view.chunked([](auto...) { FAIL(); });

    registry.emplace<entt::id_type>(registry.create());
    registry.emplace<char>(registry.create());

    view.chunked([](auto...) { FAIL(); });
}

TEST(MultiComponentView, ChunkedContiguous) {
    entt::registry registry;
    auto view = registry.view<const entt::id_type, const char>();

    for(auto i = 0; i < 5; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
    }

    view.chunked([](auto *entity, auto *id, auto *, auto sz) {
        ASSERT_EQ(sz, 5u);

        for(decltype(sz) i{}; i < sz; ++i) {
            ASSERT_EQ(entt::to_integral(*(entity + i)), *(id + i));
        }
    });
}

TEST(MultiComponentView, ChunkedSpread) {
    entt::registry registry;
    auto view = registry.view<const entt::id_type, const char>();

    registry.emplace<entt::id_type>(registry.create());

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
    }

    registry.emplace<char>(registry.create());

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
    }

    registry.emplace<entt::id_type>(registry.create());
    registry.emplace<entt::id_type>(registry.create());
    registry.emplace<char>(registry.create());

    view.chunked([](auto *entity, auto *id, auto *, auto sz) {
        ASSERT_EQ(sz, 3u);

        for(decltype(sz) i{}; i < sz; ++i) {
            ASSERT_EQ(entt::to_integral(*(entity + i)), *(id + i));
        }
    });
}

TEST(MultiComponentView, ChunkedWithExcludedComponents) {
    entt::registry registry;
    auto view = registry.view<const entt::id_type, const char>(entt::exclude<double>);

    registry.emplace<entt::id_type>(registry.create());

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
    }

    registry.emplace<char>(registry.create());

    for(auto i = 0; i < 2; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
        registry.emplace<double>(entity);
    }

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
    }

    for(auto i = 0; i < 2; ++i) {
        const auto entity = registry.create();
        registry.emplace<entt::id_type>(entity, entt::to_integral(entity));
        registry.emplace<char>(entity);
        registry.emplace<double>(entity);
    }

    registry.emplace<entt::id_type>(registry.create());
    registry.emplace<entt::id_type>(registry.create());
    registry.emplace<char>(registry.create());

    view.chunked([](auto *entity, auto *id, auto *, auto sz) {
        ASSERT_EQ(sz, 3u);

        for(decltype(sz) i{}; i < sz; ++i) {
            ASSERT_EQ(entt::to_integral(*(entity + i)), *(id + i));
        }
    });
}
