#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/group.hpp>
#include <entt/entity/registry.hpp>

struct empty_type {};

struct boxed_int {
    int value;
};

inline bool operator==(const boxed_int &lhs, const boxed_int &rhs) {
    return lhs.value == rhs.value;
}

TEST(NonOwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int, const char>);

    ASSERT_TRUE(group.empty());

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 42);
    registry.emplace<char>(e1, '2');

    ASSERT_FALSE(group.empty());
    ASSERT_NO_FATAL_FAILURE(group.begin()++);
    ASSERT_NO_FATAL_FAILURE(++cgroup.begin());
    ASSERT_NO_FATAL_FAILURE([](auto it) { return it++; }(group.rbegin()));
    ASSERT_NO_FATAL_FAILURE([](auto it) { return ++it; }(cgroup.rbegin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_NE(group.rbegin(), group.rend());
    ASSERT_NE(cgroup.rbegin(), cgroup.rend());
    ASSERT_EQ(group.size(), 1u);

    registry.emplace<int>(e0);

    ASSERT_EQ(group.size(), 2u);

    registry.erase<int>(e0);

    ASSERT_EQ(group.size(), 1u);

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<const char>(entity), '2');
    }

    ASSERT_EQ(group.handle().data()[0u], e1);

    registry.erase<char>(e0);
    registry.erase<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.rbegin(), group.rend());
    ASSERT_EQ(cgroup.rbegin(), cgroup.rend());
    ASSERT_TRUE(group.empty());

    ASSERT_NE(group.capacity(), 0u);

    group.shrink_to_fit();

    ASSERT_EQ(group.capacity(), 0u);

    decltype(group) invalid{};

    ASSERT_TRUE(group);
    ASSERT_TRUE(cgroup);
    ASSERT_FALSE(invalid);
}

TEST(NonOwningGroup, Handle) {
    entt::registry registry;
    const auto entity = registry.create();

    auto group = registry.group(entt::get<int, char>);
    auto &&handle = group.handle();

    ASSERT_TRUE(handle.empty());
    ASSERT_FALSE(handle.contains(entity));
    ASSERT_EQ(&handle, &group.handle());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_FALSE(handle.empty());
    ASSERT_TRUE(handle.contains(entity));
    ASSERT_EQ(&handle, &group.handle());
}

TEST(NonOwningGroup, Invalid) {
    entt::registry registry{};
    auto group = std::as_const(registry).group_if_exists(entt::get<const empty_type, const int>);

    const auto entity = registry.create();
    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity);

    ASSERT_FALSE(group);

    ASSERT_TRUE(group.empty());
    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.capacity(), 0u);
    ASSERT_NO_FATAL_FAILURE(group.shrink_to_fit());

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(group.rbegin(), group.rend());

    ASSERT_FALSE(group.contains(entity));
    ASSERT_EQ(group.find(entity), group.end());
    ASSERT_EQ(group.front(), entt::entity{entt::null});
    ASSERT_EQ(group.back(), entt::entity{entt::null});
}

TEST(NonOwningGroup, ElementAccess) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int, const char>);

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    for(auto i = 0u; i < group.size(); ++i) {
        ASSERT_EQ(group[i], i ? e0 : e1);
        ASSERT_EQ(cgroup[i], i ? e0 : e1);
    }
}

TEST(NonOwningGroup, Contains) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    ASSERT_FALSE(group.contains(e0));
    ASSERT_TRUE(group.contains(e1));
}

TEST(NonOwningGroup, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    ASSERT_TRUE(registry.group(entt::get<char, int, float>).empty());
    ASSERT_TRUE(registry.group(entt::get<double, char, int, float>).empty());
}

TEST(NonOwningGroup, Each) {
    entt::registry registry;
    entt::entity entity[2]{registry.create(), registry.create()};

    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int, const char>);

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], 0);

    registry.emplace<int>(entity[1u], 1);
    registry.emplace<char>(entity[1u], 1);

    auto iterable = group.each();
    auto citerable = cgroup.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_FATAL_FAILURE(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ((it++, ++it), iterable.end());

    group.each([expected = 1u](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(entt::to_integral(entt), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cgroup.each([expected = 1u](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    static_assert(std::is_same_v<decltype(std::get<1>(*iterable.begin())), int &>);
    static_assert(std::is_same_v<decltype(std::get<2>(*citerable.begin())), const char &>);

    // do not use iterable, make sure an iterable group works when created from a temporary
    for(auto [entt, ivalue, cvalue]: registry.group(entt::get<int, char>).each()) {
        ASSERT_EQ(entt::to_integral(entt), ivalue);
        ASSERT_EQ(entt::to_integral(entt), cvalue);
    }
}

TEST(NonOwningGroup, Sort) {
    entt::registry registry;
    auto group = registry.group(entt::get<const int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();
    const auto e3 = registry.create();

    registry.emplace<unsigned int>(e0, 0u);
    registry.emplace<unsigned int>(e1, 1u);
    registry.emplace<unsigned int>(e2, 2u);
    registry.emplace<unsigned int>(e3, 3u);

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e1, 1);
    registry.emplace<int>(e2, 2);

    ASSERT_EQ(group.handle().data()[0u], e0);
    ASSERT_EQ(group.handle().data()[1u], e1);
    ASSERT_EQ(group.handle().data()[2u], e2);

    group.sort([](const entt::entity lhs, const entt::entity rhs) {
        return entt::to_integral(lhs) < entt::to_integral(rhs);
    });

    ASSERT_EQ(group.handle().data()[0u], e2);
    ASSERT_EQ(group.handle().data()[1u], e1);
    ASSERT_EQ(group.handle().data()[2u], e0);

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));

    group.sort<const int>([](const int lhs, const int rhs) {
        return lhs > rhs;
    });

    ASSERT_EQ(group.handle().data()[0u], e0);
    ASSERT_EQ(group.handle().data()[1u], e1);
    ASSERT_EQ(group.handle().data()[2u], e2);

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));

    group.sort<const int, unsigned int>([](const auto lhs, const auto rhs) {
        static_assert(std::is_same_v<decltype(std::get<0>(lhs)), const int &>);
        static_assert(std::is_same_v<decltype(std::get<1>(rhs)), unsigned int &>);
        return std::get<0>(lhs) < std::get<0>(rhs);
    });

    ASSERT_EQ(group.handle().data()[0u], e2);
    ASSERT_EQ(group.handle().data()[1u], e1);
    ASSERT_EQ(group.handle().data()[2u], e0);

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));
}

TEST(NonOwningGroup, SortAsAPool) {
    entt::registry registry;
    auto group = registry.group(entt::get<const int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();
    const auto e3 = registry.create();

    auto uval = 0u;
    auto ival = 0;

    registry.emplace<unsigned int>(e0, uval++);
    registry.emplace<unsigned int>(e1, uval++);
    registry.emplace<unsigned int>(e2, uval++);
    registry.emplace<unsigned int>(e3, uval + 1);

    registry.emplace<int>(e0, ival++);
    registry.emplace<int>(e1, ival++);
    registry.emplace<int>(e2, ival++);

    for(auto entity: group) {
        ASSERT_EQ(group.get<unsigned int>(entity), --uval);
        ASSERT_EQ(group.get<const int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less<unsigned int>{});
    group.sort<unsigned int>();

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));

    for(auto entity: group) {
        ASSERT_EQ(group.get<unsigned int>(entity), uval++);
        ASSERT_EQ(group.get<const int>(entity), ival++);
    }
}

TEST(NonOwningGroup, IndexRebuiltOnDestroy) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.emplace<unsigned int>(e0, 0u);
    registry.emplace<unsigned int>(e1, 1u);

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e1, 1);

    registry.destroy(e0);
    registry.emplace<int>(registry.create(), 42);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_EQ(group[{}], e1);
    ASSERT_EQ(group.get<int>(e1), 1);
    ASSERT_EQ(group.get<unsigned int>(e1), 1u);

    group.each([e1](auto entity, auto ivalue, auto uivalue) {
        ASSERT_EQ(entity, e1);
        ASSERT_EQ(ivalue, 1);
        ASSERT_EQ(uivalue, 1u);
    });

    for(auto &&curr: group.each()) {
        ASSERT_EQ(std::get<0>(curr), e1);
        ASSERT_EQ(std::get<1>(curr), 1);
        ASSERT_EQ(std::get<2>(curr), 1u);
    }
}

TEST(NonOwningGroup, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, empty_type, const char>);

    ASSERT_EQ(group.size(), 0u);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<empty_type>(entity);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    static_assert(std::is_same_v<decltype(group.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(group.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(group.get<int, const char>({})), std::tuple<int &, const char &>>);
    static_assert(std::is_same_v<decltype(group.get({})), std::tuple<int &, const char &>>);

    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists(entt::get<int, char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists(entt::get<const int, char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists(entt::get<int, const char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>);

    group.each([](auto &&i, auto &&c) {
        static_assert(std::is_same_v<decltype(i), int &>);
        static_assert(std::is_same_v<decltype(c), const char &>);
    });

    for(auto [entt, iv, cv]: group.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
    }
}

TEST(NonOwningGroup, Find) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, const char>);

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

    ASSERT_NE(group.find(e0), group.end());
    ASSERT_EQ(group.find(e1), group.end());
    ASSERT_NE(group.find(e2), group.end());
    ASSERT_NE(group.find(e3), group.end());

    auto it = group.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, group.end());
    ASSERT_EQ(++group.find(e0), group.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.emplace<int>(e5);
    registry.emplace<char>(e5);

    ASSERT_NE(group.find(e5), group.end());
    ASSERT_EQ(group.find(e4), group.end());
}

TEST(NonOwningGroup, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1);

    const auto group = registry.group(entt::get<int>, entt::exclude<char>);

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 2);

    const auto e3 = registry.create();
    registry.emplace<int>(e3, 3);
    registry.emplace<char>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(group.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(group.get<int>(e2), 2);
        }
    }

    registry.emplace<char>(e0);
    registry.emplace<char>(e2);

    ASSERT_TRUE(group.empty());

    registry.erase<char>(e1);
    registry.erase<char>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(group.get<int>(e1), 1);
        } else if(entity == e3) {
            ASSERT_EQ(group.get<int>(e3), 3);
        }
    }
}

TEST(NonOwningGroup, EmptyAndNonEmptyTypes) {
    entt::registry registry;
    const auto group = registry.group(entt::get<int, empty_type>);

    const auto e0 = registry.create();
    registry.emplace<empty_type>(e0);
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<empty_type>(e1);
    registry.emplace<int>(e1);

    registry.emplace<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    for(auto [entt, iv]: group.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_TRUE(entt == e0 || entt == e1);
    }

    ASSERT_EQ(group.size(), 2u);
}

TEST(NonOwningGroup, TrackEntitiesOnComponentDestruction) {
    entt::registry registry;
    const auto group = registry.group(entt::get<int>, entt::exclude<char>);
    const auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int>, entt::exclude<char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(cgroup.empty());

    registry.erase<char>(entity);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE(cgroup.empty());
}

TEST(NonOwningGroup, EmptyTypes) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<empty_type>(entity);

    registry.group(entt::get<int, char, empty_type>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<int, char, empty_type>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.group(entt::get<int, empty_type, char>).each([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<int, empty_type, char>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.group(entt::get<empty_type, int, char>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<empty_type, int, char>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    auto iterable = registry.group(entt::get<int, char, double>).each();

    ASSERT_EQ(iterable.begin(), iterable.end());
}

TEST(NonOwningGroup, FrontBack) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<const int, const char>);

    ASSERT_EQ(group.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(group.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    const auto entity = registry.create();
    registry.emplace<char>(entity);

    ASSERT_EQ(group.front(), e1);
    ASSERT_EQ(group.back(), e0);
}

TEST(NonOwningGroup, SignalRace) {
    entt::registry registry;
    registry.on_construct<double>().connect<&entt::registry::emplace_or_replace<int>>();
    const auto group = registry.group(entt::get<int, double>);

    auto entity = registry.create();
    registry.emplace<double>(entity);

    ASSERT_EQ(group.size(), 1u);
}

TEST(NonOwningGroup, ExtendedGet) {
    using type = decltype(std::declval<entt::registry>().group(entt::get<int, empty_type, char>).get({}));

    static_assert(std::tuple_size_v<type> == 2u);
    static_assert(std::is_same_v<std::tuple_element_t<0, type>, int &>);
    static_assert(std::is_same_v<std::tuple_element_t<1, type>, char &>);

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    const auto tup = registry.group(entt::get<int, char>).get(entity);

    ASSERT_EQ(std::get<0>(tup), 42);
    ASSERT_EQ(std::get<1>(tup), 'c');
}

TEST(NonOwningGroup, IterableGroupAlgorithmCompatibility) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto group = registry.group(entt::get<int, char>);
    const auto iterable = group.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(NonOwningGroup, Storage) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto group = registry.group(entt::get<int, const char>);

    static_assert(std::is_same_v<decltype(group.storage<0u>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(group.storage<int>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(group.storage<1u>()), const entt::storage_type_t<char> &>);
    static_assert(std::is_same_v<decltype(group.storage<const char>()), const entt::storage_type_t<char> &>);

    ASSERT_EQ(group.size(), 0u);

    group.storage<int>().emplace(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_TRUE(group.storage<int>().contains(entity));
    ASSERT_TRUE(group.storage<const char>().contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));

    group.storage<0u>().erase(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_TRUE(group.storage<1u>().contains(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
}

TEST(OwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<const char>);

    ASSERT_TRUE(group.empty());

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 42);
    registry.emplace<char>(e1, '2');

    ASSERT_FALSE(group.empty());
    ASSERT_NO_FATAL_FAILURE(group.begin()++);
    ASSERT_NO_FATAL_FAILURE(++cgroup.begin());
    ASSERT_NO_FATAL_FAILURE([](auto it) { return it++; }(group.rbegin()));
    ASSERT_NO_FATAL_FAILURE([](auto it) { return ++it; }(cgroup.rbegin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_NE(group.rbegin(), group.rend());
    ASSERT_NE(cgroup.rbegin(), cgroup.rend());
    ASSERT_EQ(group.size(), 1u);

    registry.emplace<int>(e0);

    ASSERT_EQ(group.size(), 2u);

    registry.erase<int>(e0);

    ASSERT_EQ(group.size(), 1u);

    ASSERT_EQ(cgroup.storage<const int>().raw()[0u][0u], 42);
    ASSERT_EQ(group.storage<int>().raw()[0u][0u], 42);

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<const char>(entity), '2');
    }

    ASSERT_EQ(group.storage<int>().raw()[0u][0u], 42);

    registry.erase<char>(e0);
    registry.erase<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.rbegin(), group.rend());
    ASSERT_EQ(cgroup.rbegin(), cgroup.rend());
    ASSERT_TRUE(group.empty());

    decltype(group) invalid{};

    ASSERT_TRUE(group);
    ASSERT_TRUE(cgroup);
    ASSERT_FALSE(invalid);
}

TEST(OwningGroup, Invalid) {
    entt::registry registry{};
    auto group = std::as_const(registry).group_if_exists<const int>(entt::get<const empty_type>);

    const auto entity = registry.create();
    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity);

    ASSERT_FALSE(group);

    ASSERT_TRUE(group.empty());
    ASSERT_EQ(group.size(), 0u);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(group.rbegin(), group.rend());

    ASSERT_FALSE(group.contains(entity));
    ASSERT_EQ(group.find(entity), group.end());
    ASSERT_EQ(group.front(), entt::entity{entt::null});
    ASSERT_EQ(group.back(), entt::entity{entt::null});
}

TEST(OwningGroup, ElementAccess) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<const char>);

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    for(auto i = 0u; i < group.size(); ++i) {
        ASSERT_EQ(group[i], i ? e0 : e1);
        ASSERT_EQ(cgroup[i], i ? e0 : e1);
    }
}

TEST(OwningGroup, Contains) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.destroy(e0);

    ASSERT_FALSE(group.contains(e0));
    ASSERT_TRUE(group.contains(e1));
}

TEST(OwningGroup, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<double>(e0);
    registry.emplace<int>(e0);
    registry.emplace<float>(e0);

    const auto e1 = registry.create();
    registry.emplace<char>(e1);
    registry.emplace<float>(e1);

    ASSERT_TRUE((registry.group<char, int>(entt::get<float>).empty()));
    ASSERT_TRUE((registry.group<double, float>(entt::get<char, int>).empty()));
}

TEST(OwningGroup, Each) {
    entt::registry registry;
    entt::entity entity[2]{registry.create(), registry.create()};

    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<const char>);

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], 0);

    registry.emplace<int>(entity[1u], 1);
    registry.emplace<char>(entity[1u], 1);

    auto iterable = group.each();
    auto citerable = cgroup.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_FATAL_FAILURE(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ((it++, ++it), iterable.end());

    group.each([expected = 1u](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(entt::to_integral(entt), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cgroup.each([expected = 1u](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    static_assert(std::is_same_v<decltype(std::get<1>(*iterable.begin())), int &>);
    static_assert(std::is_same_v<decltype(std::get<2>(*citerable.begin())), const char &>);

    // do not use iterable, make sure an iterable group works when created from a temporary
    for(auto [entt, ivalue, cvalue]: registry.group<int>(entt::get<char>).each()) {
        ASSERT_EQ(entt::to_integral(entt), ivalue);
        ASSERT_EQ(entt::to_integral(entt), cvalue);
    }
}

TEST(OwningGroup, SortOrdered) {
    entt::registry registry;
    auto group = registry.group<boxed_int, char>();

    entt::entity entities[5]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<boxed_int>(entities[0], 12);
    registry.emplace<char>(entities[0], 'a');

    registry.emplace<boxed_int>(entities[1], 9);
    registry.emplace<char>(entities[1], 'b');

    registry.emplace<boxed_int>(entities[2], 6);
    registry.emplace<char>(entities[2], 'c');

    registry.emplace<boxed_int>(entities[3], 1);
    registry.emplace<boxed_int>(entities[4], 2);

    group.sort([&group](const entt::entity lhs, const entt::entity rhs) {
        return group.get<boxed_int>(lhs).value < group.get<boxed_int>(rhs).value;
    });

    ASSERT_EQ(group.storage<boxed_int>().data()[0u], entities[0]);
    ASSERT_EQ(group.storage<boxed_int>().data()[1u], entities[1]);
    ASSERT_EQ(group.storage<boxed_int>().data()[2u], entities[2]);
    ASSERT_EQ(group.storage<boxed_int>().data()[3u], entities[3]);
    ASSERT_EQ(group.storage<boxed_int>().data()[4u], entities[4]);

    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][0u].value, 12);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][1u].value, 9);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][2u].value, 6);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][3u].value, 1);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][4u].value, 2);

    ASSERT_EQ(group.storage<char>().raw()[0u][0u], 'a');
    ASSERT_EQ(group.storage<char>().raw()[0u][1u], 'b');
    ASSERT_EQ(group.storage<char>().raw()[0u][2u], 'c');

    ASSERT_EQ((group.get<boxed_int, char>(entities[0])), (std::make_tuple(boxed_int{12}, 'a')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[1])), (std::make_tuple(boxed_int{9}, 'b')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[2])), (std::make_tuple(boxed_int{6}, 'c')));

    ASSERT_FALSE(group.contains(entities[3]));
    ASSERT_FALSE(group.contains(entities[4]));
}

TEST(OwningGroup, SortReverse) {
    entt::registry registry;
    auto group = registry.group<boxed_int, char>();

    entt::entity entities[5]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<boxed_int>(entities[0], 6);
    registry.emplace<char>(entities[0], 'a');

    registry.emplace<boxed_int>(entities[1], 9);
    registry.emplace<char>(entities[1], 'b');

    registry.emplace<boxed_int>(entities[2], 12);
    registry.emplace<char>(entities[2], 'c');

    registry.emplace<boxed_int>(entities[3], 1);
    registry.emplace<boxed_int>(entities[4], 2);

    group.sort<boxed_int>([](const auto &lhs, const auto &rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(group.storage<boxed_int>().data()[0u], entities[2]);
    ASSERT_EQ(group.storage<boxed_int>().data()[1u], entities[1]);
    ASSERT_EQ(group.storage<boxed_int>().data()[2u], entities[0]);
    ASSERT_EQ(group.storage<boxed_int>().data()[3u], entities[3]);
    ASSERT_EQ(group.storage<boxed_int>().data()[4u], entities[4]);

    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][0u].value, 12);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][1u].value, 9);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][2u].value, 6);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][3u].value, 1);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][4u].value, 2);

    ASSERT_EQ(group.storage<char>().raw()[0u][0u], 'c');
    ASSERT_EQ(group.storage<char>().raw()[0u][1u], 'b');
    ASSERT_EQ(group.storage<char>().raw()[0u][2u], 'a');

    ASSERT_EQ((group.get<boxed_int, char>(entities[0])), (std::make_tuple(boxed_int{6}, 'a')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[1])), (std::make_tuple(boxed_int{9}, 'b')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[2])), (std::make_tuple(boxed_int{12}, 'c')));

    ASSERT_FALSE(group.contains(entities[3]));
    ASSERT_FALSE(group.contains(entities[4]));
}

TEST(OwningGroup, SortUnordered) {
    entt::registry registry;
    auto group = registry.group<boxed_int>(entt::get<char>);

    entt::entity entities[7]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<boxed_int>(entities[0], 6);
    registry.emplace<char>(entities[0], 'c');

    registry.emplace<boxed_int>(entities[1], 3);
    registry.emplace<char>(entities[1], 'b');

    registry.emplace<boxed_int>(entities[2], 1);
    registry.emplace<char>(entities[2], 'a');

    registry.emplace<boxed_int>(entities[3], 9);
    registry.emplace<char>(entities[3], 'd');

    registry.emplace<boxed_int>(entities[4], 12);
    registry.emplace<char>(entities[4], 'e');

    registry.emplace<boxed_int>(entities[5], 4);
    registry.emplace<boxed_int>(entities[6], 5);

    group.sort<boxed_int, char>([](const auto lhs, const auto rhs) {
        static_assert(std::is_same_v<decltype(std::get<0>(lhs)), boxed_int &>);
        static_assert(std::is_same_v<decltype(std::get<1>(rhs)), char &>);
        return std::get<1>(lhs) < std::get<1>(rhs);
    });

    ASSERT_EQ(group.storage<boxed_int>().data()[0u], entities[4]);
    ASSERT_EQ(group.storage<boxed_int>().data()[1u], entities[3]);
    ASSERT_EQ(group.storage<boxed_int>().data()[2u], entities[0]);
    ASSERT_EQ(group.storage<boxed_int>().data()[3u], entities[1]);
    ASSERT_EQ(group.storage<boxed_int>().data()[4u], entities[2]);
    ASSERT_EQ(group.storage<boxed_int>().data()[5u], entities[5]);
    ASSERT_EQ(group.storage<boxed_int>().data()[6u], entities[6]);

    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][0u].value, 12);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][1u].value, 9);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][2u].value, 6);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][3u].value, 3);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][4u].value, 1);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][5u].value, 4);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][6u].value, 5);

    ASSERT_EQ(group.get<char>(group.storage<boxed_int>().data()[0u]), 'e');
    ASSERT_EQ(group.get<char>(group.storage<boxed_int>().data()[1u]), 'd');
    ASSERT_EQ(group.get<char>(group.storage<boxed_int>().data()[2u]), 'c');
    ASSERT_EQ(group.get<char>(group.storage<boxed_int>().data()[3u]), 'b');
    ASSERT_EQ(group.get<char>(group.storage<boxed_int>().data()[4u]), 'a');

    ASSERT_FALSE(group.contains(entities[5]));
    ASSERT_FALSE(group.contains(entities[6]));
}

TEST(OwningGroup, SortWithExclusionList) {
    entt::registry registry;
    auto group = registry.group<boxed_int>({}, entt::exclude<char>);

    entt::entity entities[5]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<boxed_int>(entities[0], 0);
    registry.emplace<boxed_int>(entities[1], 1);
    registry.emplace<boxed_int>(entities[2], 2);
    registry.emplace<boxed_int>(entities[3], 3);
    registry.emplace<boxed_int>(entities[4], 4);

    registry.emplace<char>(entities[2]);

    group.sort([](const entt::entity lhs, const entt::entity rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(group.storage<boxed_int>().data()[0u], entities[4]);
    ASSERT_EQ(group.storage<boxed_int>().data()[1u], entities[3]);
    ASSERT_EQ(group.storage<boxed_int>().data()[2u], entities[1]);
    ASSERT_EQ(group.storage<boxed_int>().data()[3u], entities[0]);

    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][0u].value, 4);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][1u].value, 3);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][2u].value, 1);
    ASSERT_EQ(group.storage<boxed_int>().raw()[0u][3u].value, 0);

    ASSERT_EQ(group.get<boxed_int>(entities[0]).value, 0);
    ASSERT_EQ(group.get<boxed_int>(entities[1]).value, 1);
    ASSERT_EQ(group.get<boxed_int>(entities[3]).value, 3);
    ASSERT_EQ(group.get<boxed_int>(entities[4]).value, 4);

    ASSERT_FALSE(group.contains(entities[2]));
}

TEST(OwningGroup, IndexRebuiltOnDestroy) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.emplace<unsigned int>(e0, 0u);
    registry.emplace<unsigned int>(e1, 1u);

    registry.emplace<int>(e0, 0);
    registry.emplace<int>(e1, 1);

    registry.destroy(e0);
    registry.emplace<int>(registry.create(), 42);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_EQ(group[{}], e1);
    ASSERT_EQ(group.get<int>(e1), 1);
    ASSERT_EQ(group.get<unsigned int>(e1), 1u);

    group.each([e1](auto entity, auto ivalue, auto uivalue) {
        ASSERT_EQ(entity, e1);
        ASSERT_EQ(ivalue, 1);
        ASSERT_EQ(uivalue, 1u);
    });

    for(auto &&curr: group.each()) {
        ASSERT_EQ(std::get<0>(curr), e1);
        ASSERT_EQ(std::get<1>(curr), 1);
        ASSERT_EQ(std::get<2>(curr), 1u);
    }
}

TEST(OwningGroup, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto group = registry.group<int, const char>(entt::get<empty_type, double, const float>);

    ASSERT_EQ(group.size(), 0u);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');
    registry.emplace<empty_type>(entity);
    registry.emplace<double>(entity, 0.);
    registry.emplace<float>(entity, 0.f);

    ASSERT_EQ(group.size(), 1u);

    static_assert(std::is_same_v<decltype(group.get<int>({})), int &>);
    static_assert(std::is_same_v<decltype(group.get<const char>({})), const char &>);
    static_assert(std::is_same_v<decltype(group.get<double>({})), double &>);
    static_assert(std::is_same_v<decltype(group.get<const float>({})), const float &>);
    static_assert(std::is_same_v<decltype(group.get<int, const char, double, const float>({})), std::tuple<int &, const char &, double &, const float &>>);
    static_assert(std::is_same_v<decltype(group.get({})), std::tuple<int &, const char &, double &, const float &>>);

    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists<int>(entt::get<char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists<const int>(entt::get<char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>);
    static_assert(std::is_same_v<decltype(std::as_const(registry).group_if_exists<int>(entt::get<const char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>);

    group.each([](auto &&i, auto &&c, auto &&d, auto &&f) {
        static_assert(std::is_same_v<decltype(i), int &>);
        static_assert(std::is_same_v<decltype(c), const char &>);
        static_assert(std::is_same_v<decltype(d), double &>);
        static_assert(std::is_same_v<decltype(f), const float &>);
    });

    for(auto [entt, iv, cv, dv, fv]: group.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), const char &>);
        static_assert(std::is_same_v<decltype(dv), double &>);
        static_assert(std::is_same_v<decltype(fv), const float &>);
    }
}

TEST(OwningGroup, Find) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<const char>);

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

    ASSERT_NE(group.find(e0), group.end());
    ASSERT_EQ(group.find(e1), group.end());
    ASSERT_NE(group.find(e2), group.end());
    ASSERT_NE(group.find(e3), group.end());

    auto it = group.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, group.end());
    ASSERT_EQ(++group.find(e0), group.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.emplace<int>(e5);
    registry.emplace<char>(e5);

    ASSERT_NE(group.find(e5), group.end());
    ASSERT_EQ(group.find(e4), group.end());
}

TEST(OwningGroup, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1);

    const auto group = registry.group<int>({}, entt::exclude<char, double>);

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 2);

    const auto e3 = registry.create();
    registry.emplace<int>(e3, 3);
    registry.emplace<double>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(group.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(group.get<int>(e2), 2);
        }
    }

    registry.emplace<char>(e0);
    registry.emplace<double>(e2);

    ASSERT_TRUE(group.empty());

    registry.erase<char>(e1);
    registry.erase<double>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e1 || entity == e3);

        if(entity == e1) {
            ASSERT_EQ(group.get<int>(e1), 1);
        } else if(entity == e3) {
            ASSERT_EQ(group.get<int>(e3), 3);
        }
    }
}

TEST(OwningGroup, EmptyAndNonEmptyTypes) {
    entt::registry registry;
    const auto group = registry.group<int>(entt::get<empty_type>);

    const auto e0 = registry.create();
    registry.emplace<empty_type>(e0);
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<empty_type>(e1);
    registry.emplace<int>(e1);

    registry.emplace<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    for(auto [entt, iv]: group.each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_TRUE(entt == e0 || entt == e1);
    }

    ASSERT_EQ(group.size(), 2u);
}

TEST(OwningGroup, TrackEntitiesOnComponentDestruction) {
    entt::registry registry;
    const auto group = registry.group<int>({}, entt::exclude<char>);
    const auto cgroup = std::as_const(registry).group_if_exists<const int>({}, entt::exclude<char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(cgroup.empty());

    registry.erase<char>(entity);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE(cgroup.empty());
}

TEST(OwningGroup, EmptyTypes) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);
    registry.emplace<empty_type>(entity);

    registry.group<int>(entt::get<char, empty_type>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group<int>(entt::get<char, empty_type>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    registry.group<char>(entt::get<empty_type, int>).each([check = true](char, int) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, cv, iv]: registry.group<char>(entt::get<empty_type, int>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        ASSERT_EQ(entity, entt);
    }

    registry.group<empty_type>(entt::get<int, char>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group<empty_type>(entt::get<int, char>).each()) {
        static_assert(std::is_same_v<decltype(entt), entt::entity>);
        static_assert(std::is_same_v<decltype(iv), int &>);
        static_assert(std::is_same_v<decltype(cv), char &>);
        ASSERT_EQ(entity, entt);
    }

    auto iterable = registry.group<double>(entt::get<int, char>).each();

    ASSERT_EQ(iterable.begin(), iterable.end());
}

TEST(OwningGroup, FrontBack) {
    entt::registry registry;
    auto group = registry.group<const char>(entt::get<const int>);

    ASSERT_EQ(group.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(group.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.emplace<int>(e0);
    registry.emplace<char>(e0);

    const auto e1 = registry.create();
    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    const auto entity = registry.create();
    registry.emplace<char>(entity);

    ASSERT_EQ(group.front(), e1);
    ASSERT_EQ(group.back(), e0);
}

TEST(OwningGroup, SignalRace) {
    entt::registry registry;
    registry.on_construct<double>().connect<&entt::registry::emplace_or_replace<int>>();
    const auto group = registry.group<int>(entt::get<double>);

    auto entity = registry.create();
    registry.emplace<double>(entity);

    ASSERT_EQ(group.size(), 1u);
}

TEST(OwningGroup, StableLateInitialization) {
    entt::registry registry;

    for(std::size_t i{}; i < 30u; ++i) {
        auto entity = registry.create();
        if(!(i % 2u)) registry.emplace<int>(entity);
        if(!(i % 3u)) registry.emplace<char>(entity);
    }

    // thanks to @pgruenbacher for pointing out this corner case
    ASSERT_EQ((registry.group<int, char>().size()), 5u);
}

TEST(OwningGroup, PreventEarlyOptOut) {
    entt::registry registry;

    registry.emplace<int>(registry.create(), 3);

    const auto entity = registry.create();
    registry.emplace<char>(entity, 'c');
    registry.emplace<int>(entity, 2);

    // thanks to @pgruenbacher for pointing out this corner case
    registry.group<char, int>().each([entity](const auto entt, const auto &c, const auto &i) {
        ASSERT_EQ(entity, entt);
        ASSERT_EQ(c, 'c');
        ASSERT_EQ(i, 2);
    });
}

TEST(OwningGroup, SwappingValuesIsAllowed) {
    entt::registry registry;
    const auto group = registry.group<boxed_int>(entt::get<empty_type>);

    for(std::size_t i{}; i < 2u; ++i) {
        const auto entity = registry.create();
        registry.emplace<boxed_int>(entity, static_cast<int>(i));
        registry.emplace<empty_type>(entity);
    }

    registry.destroy(group.back());

    // thanks to @andranik3949 for pointing out this missing test
    registry.view<const boxed_int>().each([](const auto entity, const auto &value) {
        ASSERT_EQ(entt::to_integral(entity), value.value);
    });
}

TEST(OwningGroup, ExtendedGet) {
    using type = decltype(std::declval<entt::registry>().group<int, empty_type>(entt::get<char>).get({}));

    static_assert(std::tuple_size_v<type> == 2u);
    static_assert(std::is_same_v<std::tuple_element_t<0, type>, int &>);
    static_assert(std::is_same_v<std::tuple_element_t<1, type>, char &>);

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    const auto tup = registry.group<int>(entt::get<char>).get(entity);

    ASSERT_EQ(std::get<0>(tup), 42);
    ASSERT_EQ(std::get<1>(tup), 'c');
}

TEST(OwningGroup, IterableGroupAlgorithmCompatibility) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto group = registry.group<int>(entt::get<char>);
    const auto iterable = group.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(OwningGroup, Storage) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto group = registry.group<int>(entt::get<const char>);

    static_assert(std::is_same_v<decltype(group.storage<0u>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(group.storage<int>()), entt::storage_type_t<int> &>);
    static_assert(std::is_same_v<decltype(group.storage<1u>()), const entt::storage_type_t<char> &>);
    static_assert(std::is_same_v<decltype(group.storage<const char>()), const entt::storage_type_t<char> &>);

    ASSERT_EQ(group.size(), 0u);

    group.storage<int>().emplace(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_TRUE(group.storage<int>().contains(entity));
    ASSERT_TRUE(group.storage<const char>().contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));

    group.storage<0u>().erase(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_TRUE(group.storage<1u>().contains(entity));
    ASSERT_FALSE((registry.all_of<int, char>(entity)));
}
