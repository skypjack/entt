#include <algorithm>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

struct empty_type {};

struct stable_type {
    static constexpr auto in_place_delete = true;
    int value;
};

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

    ASSERT_EQ(view.get<0u>(e0), '1');
    ASSERT_EQ(cview.get<0u>(e0), view.get<char>(e0));
    ASSERT_EQ(view.get<char>(e1), '2');

    for(auto entity: view) {
        ASSERT_TRUE(entity == e0 || entity == e1);
        ASSERT_TRUE(entity != e0 || cview.get<const char>(entity) == '1');
        ASSERT_TRUE(entity != e1 || std::get<const char &>(cview.get(entity)) == '2');
    }

    registry.erase<char>(e0);
    registry.erase<char>(e1);

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
    ASSERT_TRUE(view.empty());

    decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(SingleComponentView, Constructors) {
    entt::storage<int> storage{};

    entt::view<entt::get_t<int>> invalid{};
    entt::basic_view from_storage{storage};
    entt::basic_view from_tuple{std::forward_as_tuple(storage)};

    ASSERT_FALSE(invalid);
    ASSERT_TRUE(from_storage);
    ASSERT_TRUE(from_tuple);

    ASSERT_EQ(&from_storage.handle(), &from_tuple.handle());
}

TEST(SingleComponentView, Handle) {
    entt::registry registry;
    const auto entity = registry.create();

    auto view = registry.view<int>();
    auto &&handle = view.handle();

    ASSERT_TRUE(handle.empty());
    ASSERT_FALSE(handle.contains(entity));
    ASSERT_EQ(&handle, &view.handle());

    registry.emplace<int>(entity);

    ASSERT_FALSE(handle.empty());
    ASSERT_TRUE(handle.contains(entity));
    ASSERT_EQ(&handle, &view.handle());
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

    ASSERT_TRUE(cview.empty());
    ASSERT_EQ(eview.size(), 0u);
    ASSERT_FALSE(cview.contains(entity));

    ASSERT_EQ(cview.begin(), cview.end());
    ASSERT_EQ(eview.rbegin(), eview.rend());
    ASSERT_EQ(eview.find(entity), eview.end());
    ASSERT_NE(cview.front(), entity);
    ASSERT_NE(eview.back(), entity);
}

TEST(SingleComponentView, ElementAccess) {
    entt::registry registry;
    auto view = registry.view<int>();
    auto cview = std::as_const(registry).view<const int>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 3);

    for(auto i = 0u; i < view.size(); ++i) {
        ASSERT_EQ(view[i], i ? e0 : e1);
        ASSERT_EQ(cview[i], i ? e0 : e1);
    }

    ASSERT_EQ(view[e0], 42);
    ASSERT_EQ(cview[e1], 3);
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
    entt::entity entity[2]{registry.create(), registry.create()};

    auto view = registry.view<int>(entt::exclude<double>);
    auto cview = std::as_const(registry).view<const int>();

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<int>(entity[1u], 1);

    auto iterable = view.each();
    auto citerable = cview.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_FATAL_FAILURE(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ((it++, ++it), iterable.end());

    view.each([expected = 1u](auto entt, int &value) mutable {
        ASSERT_EQ(entt::to_integral(entt), expected);
        ASSERT_EQ(value, expected);
        --expected;
    });

    cview.each([expected = 1u](const int &value) mutable {
        ASSERT_EQ(value, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    static_assert(std::is_same_v<decltype(std::get<1>(*iterable.begin())), int &>);
    static_assert(std::is_same_v<decltype(std::get<1>(*citerable.begin())), const int &>);

    // do not use iterable, make sure an iterable view works when created from a temporary
    for(auto [entt, value]: view.each()) {
        ASSERT_EQ(entt::to_integral(entt), value);
    }
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

    static_assert(std::is_same_v<decltype(view.get<0u>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<int &>>);

    static_assert(std::is_same_v<decltype(cview.get<0u>({})), const int &>);
    static_assert(std::is_same_v<decltype(cview.get<const int>({})), const int &>);
    static_assert(std::is_same_v<decltype(cview.get({})), std::tuple<const int &>>);

    static_assert(std::is_same_v<decltype(std::as_const(registry).view<int>()), decltype(cview)>);

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

    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<>>);
    static_assert(std::is_same_v<decltype(cview.get({})), std::tuple<>>);

    static_assert(std::is_same_v<decltype(std::as_const(registry).view<empty_type>()), decltype(cview)>);

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

    registry.erase<int>(e1);

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
    entt::entity entities[2u];

    registry.create(std::begin(entities), std::end(entities));
    registry.emplace<int>(entities[0u], 0);
    registry.emplace<empty_type>(entities[0u]);
    registry.emplace<char>(entities[1u], 'c');

    registry.view<empty_type>().each([&](const auto entt) {
        ASSERT_EQ(entities[0u], entt);
    });

    registry.view<empty_type>().each([check = true]() mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt]: registry.view<empty_type>().each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        ASSERT_EQ(entities[0u], entt);
    }

    registry.view<int>().each([&](const auto entt, int) {
        ASSERT_EQ(entities[0u], entt);
    });

    registry.view<int>().each([check = true](int) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv]: registry.view<int>().each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_EQ(entities[0u], entt);
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
    entt::storage_type_t<int> istorage;
    entt::storage_type_t<stable_type> sstorage;

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>>, entt::exclude_t<>>, decltype(entt::basic_view{istorage})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::as_const(istorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<stable_type>>, entt::exclude_t<>>, decltype(entt::basic_view{sstorage})>);

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(istorage), std::make_tuple()})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::as_const(istorage))})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<stable_type>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(sstorage)})>);
}

TEST(SingleComponentView, IterableViewAlgorithmCompatibility) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);

    const auto view = registry.view<int>();
    const auto iterable = view.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(SingleComponentView, StableType) {
    entt::registry registry;
    auto view = registry.view<stable_type>();

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<stable_type>(entity);
    registry.emplace<stable_type>(other);
    registry.destroy(entity);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    ASSERT_EQ(view.front(), other);
    ASSERT_EQ(view.back(), other);

    ASSERT_EQ(*view.begin(), other);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([other](const auto entt, stable_type) {
        ASSERT_EQ(other, entt);
    });

    view.each([check = true](stable_type) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, st]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(st), stable_type &>);
        ASSERT_EQ(other, entt);
    }

    registry.compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TEST(SingleComponentView, Storage) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto view = registry.view<int>();
    const auto cview = registry.view<const char>();

    static_assert(std::is_same_v<decltype(view.storage()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(view.storage<0u>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(view.storage<int>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(cview.storage()), const entt::storage_type_t<char> &>);
    static_assert(std::is_same_v<decltype(cview.storage<0u>()), const entt::storage_type_t<char> &>);
    static_assert(std::is_same_v<decltype(cview.storage<const char>()), const entt::storage_type_t<char> &>);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    view.storage().emplace(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);
    ASSERT_TRUE(view.storage<int>().contains(entity));
    ASSERT_TRUE(cview.storage<0u>().contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));

    view.storage().erase(entity);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 1u);
    ASSERT_FALSE(view.storage<0u>().contains(entity));
    ASSERT_TRUE(cview.storage<const char>().contains(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
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
    ASSERT_EQ(*cview.begin(), e1);
    ASSERT_EQ(++view.begin(), (view.end()));
    ASSERT_EQ(++cview.begin(), (cview.end()));

    ASSERT_NO_FATAL_FAILURE((view.begin()++));
    ASSERT_NO_FATAL_FAILURE((++cview.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_EQ(view.size_hint(), 1u);

    for(auto entity: view) {
        ASSERT_EQ(std::get<0>(cview.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<0>(cview.get<0u, 1u>(entity)), 42);

        ASSERT_EQ(std::get<1>(view.get<int, char>(entity)), '2');
        ASSERT_EQ(std::get<1>(view.get<0u, 1u>(entity)), '2');

        ASSERT_EQ(cview.get<const char>(entity), '2');
        ASSERT_EQ(cview.get<1u>(entity), '2');
    }

    decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(MultiComponentView, Constructors) {
    entt::storage<int> storage{};

    entt::view<entt::get_t<int, int>> invalid{};
    entt::basic_view from_storage{storage, storage};
    entt::basic_view from_tuple{std::forward_as_tuple(storage, storage)};

    ASSERT_FALSE(invalid);
    ASSERT_TRUE(from_storage);
    ASSERT_TRUE(from_tuple);

    ASSERT_EQ(&from_storage.handle(), &from_tuple.handle());
}

TEST(MultiComponentView, Handle) {
    entt::registry registry;
    const auto entity = registry.create();

    auto view = registry.view<int, char>();
    auto &&handle = view.handle();

    ASSERT_TRUE(handle.empty());
    ASSERT_FALSE(handle.contains(entity));
    ASSERT_EQ(&handle, &view.handle());

    registry.emplace<int>(entity);

    ASSERT_FALSE(handle.empty());
    ASSERT_TRUE(handle.contains(entity));
    ASSERT_EQ(&handle, &view.handle());

    view = view.refresh();
    auto &&other = view.handle();

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(other.contains(entity));
    ASSERT_EQ(&other, &view.handle());
    ASSERT_NE(&handle, &other);

    view = view.use<int>();

    ASSERT_NE(&other, &view.handle());
    ASSERT_EQ(&handle, &view.handle());
}

TEST(MultiComponentView, LazyTypesFromConstRegistry) {
    entt::registry registry{};
    auto view = std::as_const(registry).view<const empty_type, const int>();

    const auto entity = registry.create();
    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity);

    ASSERT_TRUE(view);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_FALSE(view.contains(entity));

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.find(entity), view.end());
    ASSERT_NE(view.front(), entity);
    ASSERT_NE(view.back(), entity);
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
    const entt::entity entity[2]{registry.create(), registry.create()};

    registry.insert<int>(std::begin(entity), std::end(entity));
    registry.insert<char>(std::begin(entity), std::end(entity));

    const auto view = registry.view<int, char>();
    using iterator = typename decltype(view)::iterator;

    iterator end{view.begin()};
    iterator begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(*begin, entity[1u]);
    ASSERT_EQ(*begin.operator->(), entity[1u]);
    ASSERT_EQ(begin++, view.begin());

    ASSERT_EQ(*begin, entity[0u]);
    ASSERT_EQ(*begin.operator->(), entity[0u]);
    ASSERT_EQ(++begin, view.end());
}

TEST(MultiComponentView, ElementAccess) {
    entt::registry registry;
    auto view = registry.view<int, char>();
    auto cview = std::as_const(registry).view<const int, const char>();

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);
    registry.emplace<char>(e0, '0');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 3);
    registry.emplace<char>(e1, '1');

    ASSERT_EQ(view[e0], std::make_tuple(42, '0'));
    ASSERT_EQ(cview[e1], std::make_tuple(3, '1'));
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
}

TEST(MultiComponentView, Each) {
    entt::registry registry;
    entt::entity entity[2]{registry.create(), registry.create()};

    auto view = registry.view<int, char>(entt::exclude<double>);
    auto cview = std::as_const(registry).view<const int, const char>();

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], 0);

    registry.emplace<int>(entity[1u], 1);
    registry.emplace<char>(entity[1u], 1);

    auto iterable = view.each();
    auto citerable = cview.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_FATAL_FAILURE(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ((it++, ++it), iterable.end());

    view.each([expected = 1u](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(entt::to_integral(entt), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cview.each([expected = 1u](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    static_assert(std::is_same_v<decltype(std::get<1>(*iterable.begin())), int &>);
    static_assert(std::is_same_v<decltype(std::get<2>(*citerable.begin())), const char &>);

    // do not use iterable, make sure an iterable view works when created from a temporary
    for(auto [entt, ivalue, cvalue]: registry.view<int, char>().each()) {
        ASSERT_EQ(entt::to_integral(entt), ivalue);
        ASSERT_EQ(entt::to_integral(entt), cvalue);
    }
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

    registry.view<int, char>().use<int>().each([value = 2](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value--);
    });

    registry.sort<int>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    registry.view<int, char>().use<0u>().each([value = 0](const auto curr, const auto) mutable {
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

    for(auto &&curr: registry.view<int, char>().use<int>().each()) {
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

    static_assert(std::is_same_v<decltype(view.get<0u>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get<2u>({})), const char &>);
    static_assert(std::is_same_v<decltype(view.get<0u, 2u>({})), std::tuple<int &, const char &>>);

    static_assert(std::is_same_v<decltype(view.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(view.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(view.get<int, const char>({})), std::tuple<int &, const char &>>);

    static_assert(std::is_same_v<decltype(view.get({})), std::tuple<int &, const char &>>);

    static_assert(std::is_same_v<decltype(std::as_const(registry).view<char, int>()), decltype(std::as_const(registry).view<const char, const int>())>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).view<char, const int>()), decltype(std::as_const(registry).view<const char, const int>())>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).view<const char, int>()), decltype(std::as_const(registry).view<const char, const int>())>);

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

    registry.erase<int>(e1);

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
            ASSERT_EQ(view.get<0u>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(std::get<0>(view.get(e2)), 2);
        }
    }

    registry.emplace<char>(e0);
    registry.emplace<char>(e2);
    registry.erase<char>(e1);
    registry.erase<char>(e3);

    for(const auto entity: view) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(std::get<0>(view.get(e1)), 1);
        } else if(entity == e3) {
            ASSERT_EQ(view.get<const int>(e3), 3);
            ASSERT_EQ(view.get<0u>(e3), 3);
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

    registry.view<empty_type, int, char>(entt::exclude<double>).use<empty_type>().each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.view<empty_type, int, char>(entt::exclude<double>).use<0u>().each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.view<int, empty_type, char>(entt::exclude<double>).use<1u>().each([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, cv]: registry.view<int, empty_type, char>(entt::exclude<double>).use<empty_type>().each()) {
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
    entt::storage_type_t<int> istorage;
    entt::storage_type_t<double> dstorage;
    entt::storage_type_t<stable_type> sstorage;

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{istorage, dstorage})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>, entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::as_const(istorage), dstorage})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, const entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{istorage, std::as_const(dstorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>, const entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::as_const(istorage), std::as_const(dstorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, entt::storage_type_t<stable_type>>, entt::exclude_t<>>, decltype(entt::basic_view{istorage, sstorage})>);

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(istorage, dstorage), std::make_tuple()})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>, entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::as_const(istorage), dstorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, const entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(istorage, std::as_const(dstorage))})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>, const entt::storage_type_t<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::as_const(istorage), std::as_const(dstorage))})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, entt::storage_type_t<stable_type>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(istorage, sstorage)})>);

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>>, entt::exclude_t<entt::storage_type_t<double>>>, decltype(entt::basic_view{std::forward_as_tuple(istorage), std::forward_as_tuple(dstorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>>, entt::exclude_t<entt::storage_type_t<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::as_const(istorage)), std::forward_as_tuple(dstorage)})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>>, entt::exclude_t<const entt::storage_type_t<double>>>, decltype(entt::basic_view{std::forward_as_tuple(istorage), std::forward_as_tuple(std::as_const(dstorage))})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<int>>, entt::exclude_t<const entt::storage_type_t<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::as_const(istorage)), std::forward_as_tuple(std::as_const(dstorage))})>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>>, entt::exclude_t<entt::storage_type_t<stable_type>>>, decltype(entt::basic_view{std::forward_as_tuple(istorage), std::forward_as_tuple(sstorage)})>);
}

TEST(MultiComponentView, IterableViewAlgorithmCompatibility) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto view = registry.view<int, char>();
    const auto iterable = view.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(MultiComponentView, StableType) {
    entt::registry registry;
    auto view = registry.view<int, stable_type>();

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<int>(other);
    registry.emplace<stable_type>(entity);
    registry.emplace<stable_type>(other);
    registry.destroy(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    view = view.use<stable_type>();

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    ASSERT_EQ(view.front(), other);
    ASSERT_EQ(view.back(), other);

    ASSERT_EQ(*view.begin(), other);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([other](const auto entt, int, stable_type) {
        ASSERT_EQ(other, entt);
    });

    view.each([check = true](int, stable_type) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, st]: view.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(st), stable_type &>);
        ASSERT_EQ(other, entt);
    }

    registry.compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TEST(MultiComponentView, StableTypeWithExcludedComponent) {
    entt::registry registry;
    auto view = registry.view<stable_type>(entt::exclude<int>).use<stable_type>();

    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<stable_type>(entity, 0);
    registry.emplace<stable_type>(other, 42);
    registry.emplace<int>(entity);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    registry.destroy(entity);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity));
    ASSERT_TRUE(view.contains(other));

    for(auto entt: view) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    }

    for(auto [entt, comp]: view.each()) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
        ASSERT_EQ(comp.value, 42);
    }

    view.each([other](const auto entt, auto &&...) {
        constexpr entt::entity tombstone = entt::tombstone;
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, other);
    });
}

TEST(MultiComponentView, SameComponentTypes) {
    entt::registry registry;
    entt::storage_type_t<int> storage;
    entt::storage_type_t<int> other;
    entt::basic_view view{storage, other};

    storage.bind(entt::forward_as_any(registry));
    other.bind(entt::forward_as_any(registry));

    const entt::entity e0{42u};
    const entt::entity e1{3u};

    storage.emplace(e0, 7);
    other.emplace(e0, 9);
    other.emplace(e1, 1);

    ASSERT_TRUE(view.contains(e0));
    ASSERT_FALSE(view.contains(e1));

    ASSERT_EQ((view.get<0u, 1u>(e0)), (std::make_tuple(7, 9)));
    ASSERT_EQ(view.get<1u>(e0), 9);

    for(auto entt: view) {
        ASSERT_EQ(entt, e0);
    }

    view.each([&](auto entt, auto &&first, auto &&second) {
        ASSERT_EQ(entt, e0);
        ASSERT_EQ(first, 7);
        ASSERT_EQ(second, 9);
    });

    for(auto [entt, first, second]: view.each()) {
        ASSERT_EQ(entt, e0);
        ASSERT_EQ(first, 7);
        ASSERT_EQ(second, 9);
    }

    ASSERT_EQ(&view.handle(), &storage);
    ASSERT_EQ(&view.use<1u>().handle(), &other);
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
    registry.emplace<stable_type>(other);

    const auto view1 = registry.view<int>(entt::exclude<const double>);
    const auto view2 = registry.view<const char>(entt::exclude<float>);
    const auto view3 = registry.view<empty_type>();
    const auto view4 = registry.view<stable_type>();

    static_assert(std::is_same_v<entt::basic_view<entt::get_t<entt::storage_type_t<int>, const entt::storage_type_t<char>>, entt::exclude_t<const entt::storage_type_t<double>, entt::storage_type_t<float>>>, decltype(view1 | view2)>);
    static_assert(std::is_same_v<entt::basic_view<entt::get_t<const entt::storage_type_t<char>, entt::storage_type_t<int>>, entt::exclude_t<entt::storage_type_t<float>, const entt::storage_type_t<double>>>, decltype(view2 | view1)>);
    static_assert(std::is_same_v<decltype((view3 | view2) | view1), decltype(view3 | (view2 | view1))>);

    ASSERT_FALSE((view1 | view2).contains(entity));
    ASSERT_TRUE((view1 | view2).contains(other));

    ASSERT_TRUE((view3 | view2).contains(entity));
    ASSERT_FALSE((view3 | view2).contains(other));

    ASSERT_FALSE((view1 | view2 | view3).contains(entity));
    ASSERT_FALSE((view1 | view2 | view3).contains(other));

    ASSERT_FALSE((view1 | view4 | view2).contains(entity));
    ASSERT_TRUE((view1 | view4 | view2).contains(other));
}

TEST(MultiComponentView, Storage) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto view = registry.view<int, const char>();

    static_assert(std::is_same_v<decltype(view.storage<0u>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(view.storage<int>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(view.storage<1u>()), const entt::storage_type_t<char> &>);
    static_assert(std::is_same_v<decltype(view.storage<const char>()), const entt::storage_type_t<char> &>);

    ASSERT_EQ(view.size_hint(), 0u);

    view.storage<int>().emplace(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_TRUE(view.storage<const char>().contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));

    view.storage<0u>().erase(entity);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_TRUE(view.storage<1u>().contains(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
}
