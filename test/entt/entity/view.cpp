#include <tuple>
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

    ASSERT_NO_FATAL_FAILURE(view.begin()++);
    ASSERT_NO_FATAL_FAILURE(++cview.begin());
    ASSERT_NO_FATAL_FAILURE([](auto it) { return it++; }(view.rbegin()));
    ASSERT_NO_FATAL_FAILURE([](auto it) { return ++it; }(cview.rbegin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(view.rbegin(), view.rend());
    ASSERT_NE(cview.rbegin(), cview.rend());
    ASSERT_EQ(view.size(), 1u);
    ASSERT_FALSE(view.empty());

    registry.emplace<char>(e0);

    ASSERT_EQ(view.size(), 2u);

    view.get<char>(e0) = '1';
    std::get<0>(view.get(e1)) = '2';

    for(auto entity: view) {
        ASSERT_TRUE(cview.get<const char>(entity) == '1' || std::get<const char &>(cview.get(entity)) == '2');
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

    decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(SingleComponentView, RawData) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    const auto entity = registry.create();

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);
    ASSERT_EQ(view.raw(), nullptr);
    ASSERT_EQ(cview.raw(), nullptr);
    ASSERT_EQ(view.data(), nullptr);
    ASSERT_EQ(cview.data(), nullptr);

    registry.emplace<int>(entity, 42);

    ASSERT_NE(view.size(), 0u);
    ASSERT_NE(cview.size(), 0u);
    ASSERT_EQ(*view.raw(), 42);
    ASSERT_EQ(*cview.raw(), 42);
    ASSERT_EQ(*view.data(), entity);
    ASSERT_EQ(*cview.data(), entity);

    registry.destroy(entity);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);
}

TEST(SingleComponentView, LazyTypeFromConstRegistry) {
    entt::registry registry{};
    auto eview = std::as_const(registry).view<const empty_type>();
    auto cview = std::as_const(registry).view<const int>();

    const auto entity = registry.create();
    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity);

    ASSERT_TRUE(cview);
    ASSERT_TRUE(eview);

    ASSERT_NE(cview.raw(), nullptr);
    ASSERT_NE(eview.data(), nullptr);

    ASSERT_FALSE(cview.empty());
    ASSERT_EQ(eview.size(), 1u);
    ASSERT_TRUE(cview.contains(entity));

    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(eview.rbegin(), eview.rend());
    ASSERT_NE(eview.find(entity), eview.end());
    ASSERT_EQ(cview.front(), entity);
    ASSERT_EQ(eview.back(), entity);
}

TEST(SingleComponentView, ElementAccess) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);

    for(auto i = 0u; i < view.size(); ++i) {
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

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
}

TEST(SingleComponentView, Each) {
    entt::registry registry;

    registry.emplace<int>(registry.create(), 0);
    registry.emplace<int>(registry.create(), 1);

    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();
    std::size_t cnt = 0;

    for(auto first = cview.each().rbegin(), last = cview.each().rend(); first != last; ++first) {
        static_assert(std::is_same_v<decltype(*first), std::tuple<entt::entity, const int &>>);
        ASSERT_EQ(std::get<1>(*first), cnt++);
    }

    view.each([&cnt](auto, int &) { ++cnt; });
    view.each([&cnt](int &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{6});

    cview.each([&cnt](const int &) { --cnt; });
    cview.each([&cnt](auto, const int &) { --cnt; });

    for(auto [entt, iv]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_EQ(iv, --cnt);
    }

    ASSERT_EQ(cnt, std::size_t{0});

    auto it = view.each().begin();
    auto rit = view.each().rbegin();

    ASSERT_EQ((it++, ++it), view.each().end());
    ASSERT_EQ((rit++, ++rit), view.each().rend());
}

TEST(SingleComponentView, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    registry.emplace<int>(registry.create(), 0);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);

    static_assert(std::is_same_v<typename decltype(view)::raw_type, int>);
    static_assert(std::is_same_v<typename decltype(cview)::raw_type, const int>);

    static_assert(std::is_same_v<decltype(view.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<int &>>);
    static_assert(std::is_same_v<decltype(view.raw()), int *>);
    static_assert(std::is_same_v<decltype(cview.get<const int>({})), const int &>);
    static_assert(std::is_same_v<decltype(cview.get({})), std::tuple<const int &>>);
    static_assert(std::is_same_v<decltype(cview.raw()), const int *>);

    view.each([](auto &&i) {
        static_assert(std::is_same_v<decltype(i), int &>);
    });

    cview.each([](auto &&i) {
        static_assert(std::is_same_v<decltype(i), const int &>);
    });

    for(auto [entt, iv]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
    }

    for(auto [entt, iv]: cview.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), const int &>);
    }
}

TEST(SingleComponentView, ConstNonConstAndAllInBetweenWithEmptyType) {
    entt::registry registry;
    auto view = registry.view<empty_type>();
    auto cview = std::as_const(registry).view<const empty_type>();

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    registry.emplace<empty_type>(registry.create());

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);

    static_assert(std::is_same_v<typename decltype(view)::raw_type, empty_type>);
    static_assert(std::is_same_v<typename decltype(cview)::raw_type, const empty_type>);

    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<>>);
    static_assert(std::is_same_v<decltype(cview.get({})), std::tuple<>>);

    for(auto [entt]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
    }

    for(auto [entt]: cview.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
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

    for(auto [entt]: registry.view<empty_type>().each()) {
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

    for(auto [entt, iv]: registry.view<int>().each()) {
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

TEST(SingleComponentView, DeductionGuide) {
    entt::registry registry;
    typename entt::storage_traits<entt::entity, int>::storage_type storage;

    static_assert(std::is_same_v<decltype(entt::basic_view{storage}), entt::basic_view<entt::entity, entt::exclude_t<>, int>>);
    static_assert(std::is_same_v<decltype(entt::basic_view{std::as_const(storage)}), entt::basic_view<entt::entity, entt::exclude_t<>, const int>>);
}

TEST(MultiComponentView, Functionalities) {
    entt::registry registry;
    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 42);
    registry.emplace<char>(e1, '2');

    ASSERT_EQ(*view.begin(), e1);
    ASSERT_EQ(*view.rbegin(), e1);
    ASSERT_EQ(++view.begin(), (view.end()));
    ASSERT_EQ(++view.rbegin(), (view.rend()));

    ASSERT_NO_FATAL_FAILURE((view.begin()++));
    ASSERT_NO_FATAL_FAILURE((++cview.begin()));
    ASSERT_NO_FATAL_FAILURE(view.rbegin()++);
    ASSERT_NO_FATAL_FAILURE(++cview.rbegin());

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(view.rbegin(), view.rend());
    ASSERT_NE(cview.rbegin(), cview.rend());
    ASSERT_EQ(view.size_hint(), 1u);

    for(auto entity: view) {
        ASSERT_EQ(std::get<0>(cview.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(cview.get<const char>(entity), '2');
    }

    decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(MultiComponentView, LazyTypesFromConstRegistry) {
    entt::registry registry{};
    auto view = std::as_const(registry).view<const empty_type, const int>();

    const auto entity = registry.create();
    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity);

    ASSERT_TRUE(view);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_TRUE(view.contains(entity));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(view.find(entity), view.end());
    ASSERT_EQ(view.front(), entity);
    ASSERT_EQ(view.back(), entity);
}

TEST(MultiComponentView, LazyExcludedTypeFromConstRegistry) {
    entt::registry registry;

    auto entity = registry.create();
    registry.emplace<int>(entity);

    auto view = std::as_const(registry).view<const int>(entt::exclude<char>);

    ASSERT_TRUE(view);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_TRUE(view.contains(entity));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(view.find(entity), view.end());
    ASSERT_EQ(view.front(), entity);
    ASSERT_EQ(view.back(), entity);
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

    registry.emplace<int>(registry.create());
    registry.emplace<char>(registry.create());

    const auto other = registry.create();
    registry.emplace<int>(other);
    registry.emplace<char>(other);

    begin = view.begin();

    ASSERT_EQ(*(begin++), other);
    ASSERT_EQ(*(begin++), entity);
    ASSERT_EQ(begin--, end);
    ASSERT_EQ(*(begin--), entity);
    ASSERT_EQ(*begin, other);
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

TEST(MultiComponentView, SizeHint) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    auto view = registry.view<char, int, float>();

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
}

TEST(MultiComponentView, Each) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1);

    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();
    std::size_t cnt = 0;

    for(auto first = cview.each().rbegin(), last = cview.each().rend(); first != last; ++first) {
        static_assert(std::is_same_v<decltype(*first), std::tuple<entt::entity, const int &, const char &>>);
        ASSERT_EQ(std::get<1>(*first), cnt++);
    }

    view.each([&cnt](auto, int &, char &) { ++cnt; });
    view.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{6});

    cview.each([&cnt](const int &, const char &) { --cnt; });
    cview.each([&cnt](auto, const int &, const char &) { --cnt; });

    for(auto [entt, iv, cv]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(iv, --cnt);
    }

    ASSERT_EQ(cnt, std::size_t{0});

    auto it = view.each().begin();
    auto rit = view.each().rbegin();

    ASSERT_EQ((it++, ++it), view.each().end());
    ASSERT_EQ((rit++, ++rit), view.each().rend());
}

TEST(MultiComponentView, EachWithSuggestedType) {
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

    auto value = registry.view<int, char>().size_hint();

    for(auto &&curr: registry.view<int, char>().each()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(--value));
    }

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    value = {};

    for(auto &&curr: registry.view<int, char>().each<int>()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(value++));
    }
}

TEST(MultiComponentView, EachWithHoles) {
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

    for(auto &&curr: view.each()) {
        ASSERT_EQ(std::get<0>(curr), e0);
        ASSERT_EQ(std::get<1>(curr), '0');
        ASSERT_EQ(std::get<2>(curr), 0);
    }
}

TEST(MultiComponentView, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto view = registry.view<int, empty_type, const char>();

    ASSERT_EQ(view.size_hint(), 0u);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<empty_type>(entity);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(view.size_hint(), 1u);

    static_assert(std::is_same_v<decltype(view.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(view.get<int, const char>({})), std::tuple<int &, const char &>>);
    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<int &, const char &>>);

    view.each([](auto &&i, auto &&c) {
        static_assert(std::is_same_v<decltype(i), int &>);
        static_assert(std::is_same_v<decltype(c), const char &>);
    });

    for(auto [entt, iv, cv]: view.each()) {
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
            ASSERT_EQ(std::get<0>(view.get(e2)), 2);
        }
    }

    registry.emplace<char>(e0);
    registry.emplace<char>(e2);
    registry.remove<char>(e1);
    registry.remove<char>(e3);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(std::get<0>(view.get(e1)), 1);
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
    registry.emplace<empty_type>(entity);

    const auto other = registry.create();
    registry.emplace<int>(other);
    registry.emplace<char>(other);
    registry.emplace<double>(other);
    registry.emplace<empty_type>(other);

    const auto ignored = registry.create();
    registry.emplace<int>(ignored);
    registry.emplace<char>(ignored);

    registry.view<int, char, empty_type>(entt::exclude<double>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.view<int, char, empty_type>(entt::exclude<double>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, empty_type, char>(entt::exclude<double>).each([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, cv]: registry.view<int, empty_type, char>(entt::exclude<double>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<empty_type, int, char>(entt::exclude<double>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.view<empty_type, int, char>(entt::exclude<double>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<empty_type, int, char>(entt::exclude<double>).each<empty_type>([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.view<empty_type, int, char>(entt::exclude<double>).each<empty_type>()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, empty_type, char>(entt::exclude<double>).each<empty_type>([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, cv]: registry.view<int, empty_type, char>(entt::exclude<double>).each<empty_type>()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
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

TEST(MultiComponentView, ExtendedGet) {
    using type = decltype(std::declval<entt::registry>().view<int, empty_type, char>().get({}));
    static_assert(std::tuple_size_v<type> == 2u);
    static_assert(std::is_same_v<std::tuple_element_t<0, type>, int &>);
    static_assert(std::is_same_v<std::tuple_element_t<1, type>, char &>);
}

TEST(MultiComponentView, DeductionGuide) {
    entt::registry registry;
    typename entt::storage_traits<entt::entity, int>::storage_type istorage;
    typename entt::storage_traits<entt::entity, double>::storage_type dstorage;

    static_assert(std::is_same_v<decltype(entt::basic_view{istorage, dstorage}), entt::basic_view<entt::entity, entt::exclude_t<>, int, double>>);
    static_assert(std::is_same_v<decltype(entt::basic_view{std::as_const(istorage), dstorage}), entt::basic_view<entt::entity, entt::exclude_t<>, const int, double>>);
    static_assert(std::is_same_v<decltype(entt::basic_view{istorage, std::as_const(dstorage)}), entt::basic_view<entt::entity, entt::exclude_t<>, int, const double>>);
    static_assert(std::is_same_v<decltype(entt::basic_view{std::as_const(istorage), std::as_const(dstorage)}), entt::basic_view<entt::entity, entt::exclude_t<>, const int, const double>>);
}

TEST(View, Pipe) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<double>(entity);
    registry.emplace<empty_type>(entity);

    registry.emplace<int>(other);
    registry.emplace<char>(other);

    const auto view1 = registry.view<int>(entt::exclude<double>);
    const auto view2 = registry.view<const char>(entt::exclude<float>);
    const auto view3 = registry.view<empty_type>();

    static_assert(std::is_same_v<decltype(view1 | view2), entt::basic_view<entt::entity, entt::exclude_t<double, float>, int, const char>>);
    static_assert(std::is_same_v<decltype(view2 | view1), entt::basic_view<entt::entity, entt::exclude_t<float, double>, const char, int>>);
    static_assert(std::is_same_v<decltype((view1 | view2) | view3), decltype(view1 | (view2 | view3))>);

    ASSERT_FALSE((view1 | view2).contains(entity));
    ASSERT_TRUE((view1 | view2).contains(other));

    ASSERT_TRUE((view2 | view3).contains(entity));
    ASSERT_FALSE((view2 | view3).contains(other));

    ASSERT_FALSE((view1 | view2 | view3).contains(entity));
    ASSERT_FALSE((view1 | view2 | view3).contains(other));
}
