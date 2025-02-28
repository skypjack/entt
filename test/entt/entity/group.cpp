#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/group.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <entt/signal/sigh.hpp>
#include "../../common/boxed_type.h"
#include "../../common/config.h"
#include "../../common/empty.h"

TEST(NonOwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int, const char>);

    ASSERT_TRUE(group.empty());

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 4);
    registry.emplace<char>(e1, '2');

    ASSERT_FALSE(group.empty());
    ASSERT_NO_THROW(group.begin()++);
    ASSERT_NO_THROW(++cgroup.begin());
    ASSERT_NO_THROW([](auto it) { return it++; }(group.rbegin()));
    ASSERT_NO_THROW([](auto it) { return ++it; }(cgroup.rbegin()));

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
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 4);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<1>(entity), '2');
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

    const decltype(group) invalid{};

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
    ASSERT_NE(&handle, group.storage<int>());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_FALSE(handle.empty());
    ASSERT_TRUE(handle.contains(entity));
    ASSERT_EQ(&handle, &group.handle());
}

TEST(NonOwningGroup, Invalid) {
    entt::registry registry{};
    auto group = std::as_const(registry).group_if_exists(entt::get<const test::empty, const int>);

    const auto entity = registry.create();
    registry.emplace<test::empty>(entity);
    registry.emplace<int>(entity);

    ASSERT_FALSE(group);

    ASSERT_TRUE(group.empty());
    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.capacity(), 0u);
    ASSERT_NO_THROW(group.shrink_to_fit());

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
    std::array entity{registry.create(), registry.create()};

    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group_if_exists(entt::get<const int, const char>);

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], static_cast<char>(0));

    registry.emplace<int>(entity[1u], 1);
    registry.emplace<char>(entity[1u], static_cast<char>(1));

    auto iterable = group.each();
    auto citerable = cgroup.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_THROW(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(it.base(), group.begin());
    ASSERT_EQ((it++, ++it), iterable.end());
    ASSERT_EQ(it.base(), group.end());

    group.each([expected = 1](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cgroup.each([expected = 1](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    testing::StaticAssertTypeEq<decltype(std::get<1>(*iterable.begin())), int &>();
    testing::StaticAssertTypeEq<decltype(std::get<2>(*citerable.begin())), const char &>();

    // do not use iterable, make sure an iterable group works when created from a temporary
    for(auto [entt, ivalue, cvalue]: registry.group(entt::get<int, char>).each()) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), ivalue);
        ASSERT_EQ(static_cast<char>(entt::to_integral(entt)), cvalue);
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

    ASSERT_EQ((group.get<0, 1>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<0, 1>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<0, 1>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));

    group.sort<const int, unsigned int>([](const auto lhs, const auto rhs) {
        testing::StaticAssertTypeEq<decltype(std::get<0>(lhs)), const int &>();
        testing::StaticAssertTypeEq<decltype(std::get<1>(rhs)), unsigned int &>();
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

    registry.sort<unsigned int>(std::less{});
    const entt::sparse_set &other = *group.storage<unsigned int>();
    group.sort_as(other.begin(), other.end());

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<0, 1>(e1)), (std::make_tuple(1, 1u)));
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
    registry.emplace<int>(registry.create(), 4);

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
    auto group = registry.group(entt::get<int, test::empty, const char>);

    ASSERT_EQ(group.size(), 0u);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<test::empty>(entity);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    testing::StaticAssertTypeEq<decltype(group.get<0>({})), int &>();
    testing::StaticAssertTypeEq<decltype(group.get<int>({})), int &>();

    testing::StaticAssertTypeEq<decltype(group.get<1>({})), void>();
    testing::StaticAssertTypeEq<decltype(group.get<test::empty>({})), void>();

    testing::StaticAssertTypeEq<decltype(group.get<2>({})), const char &>();
    testing::StaticAssertTypeEq<decltype(group.get<const char>({})), const char &>();

    testing::StaticAssertTypeEq<decltype(group.get<int, test::empty, const char>({})), std::tuple<int &, const char &>>();
    testing::StaticAssertTypeEq<decltype(group.get<0, 1, 2>({})), std::tuple<int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(group.get({})), std::tuple<int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists(entt::get<int, char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists(entt::get<const int, char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists(entt::get<int, const char>)), decltype(std::as_const(registry).group_if_exists(entt::get<const int, const char>))>();

    group.each([](auto &&iv, auto &&cv) {
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
    });

    for([[maybe_unused]] auto [entt, iv, cv]: group.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
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
            ASSERT_EQ(group.get<0>(e2), 2);
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
            ASSERT_EQ(group.get<0>(e3), 3);
        }
    }
}

TEST(NonOwningGroup, EmptyAndNonEmptyTypes) {
    entt::registry registry;
    const auto group = registry.group(entt::get<int, test::empty>);

    const auto e0 = registry.create();
    registry.emplace<test::empty>(e0);
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<test::empty>(e1);
    registry.emplace<int>(e1);

    registry.emplace<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    for(auto [entt, iv]: group.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
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
    registry.emplace<test::empty>(entity);

    registry.group(entt::get<int, char, test::empty>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<int, char, test::empty>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
        ASSERT_EQ(entity, entt);
    }

    registry.group(entt::get<int, test::empty, char>).each([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<int, test::empty, char>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
        ASSERT_EQ(entity, entt);
    }

    registry.group(entt::get<test::empty, int, char>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group(entt::get<test::empty, int, char>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
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
    using type = decltype(std::declval<entt::registry>().group(entt::get<int, test::empty, char>).get({}));

    ASSERT_EQ(std::tuple_size_v<type>, 2u);

    testing::StaticAssertTypeEq<std::tuple_element_t<0, type>, int &>();
    testing::StaticAssertTypeEq<std::tuple_element_t<1, type>, char &>();

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 3);
    registry.emplace<char>(entity, 'c');

    const auto tup = registry.group(entt::get<int, char>).get(entity);

    ASSERT_EQ(std::get<0>(tup), 3);
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
    auto group = registry.group(entt::get<int, const char>, entt::exclude<double, const float>);

    testing::StaticAssertTypeEq<decltype(group.storage<0u>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<int>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const int>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<1u>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<char>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const char>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<2u>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<double>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const double>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<3u>()), const entt::storage_type_t<float> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<float>()), const entt::storage_type_t<float> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const float>()), const entt::storage_type_t<float> *>();

    ASSERT_TRUE(group);

    ASSERT_NE(group.storage<int>(), nullptr);
    ASSERT_NE(group.storage<1u>(), nullptr);
    ASSERT_NE(group.storage<double>(), nullptr);
    ASSERT_NE(group.storage<3u>(), nullptr);

    ASSERT_EQ(group.size(), 0u);

    group.storage<int>()->emplace(entity);
    group.storage<double>()->emplace(entity);
    registry.emplace<char>(entity);
    registry.emplace<float>(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.begin(), group.end());
    ASSERT_TRUE(group.storage<int>()->contains(entity));
    ASSERT_TRUE(group.storage<const char>()->contains(entity));
    ASSERT_TRUE(group.storage<double>()->contains(entity));
    ASSERT_TRUE(group.storage<const float>()->contains(entity));
    ASSERT_TRUE((registry.all_of<int, char, double, float>(entity)));

    group.storage<double>()->erase(entity);
    registry.erase<float>(entity);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_NE(group.begin(), group.end());
    ASSERT_TRUE(group.storage<const int>()->contains(entity));
    ASSERT_TRUE(group.storage<char>()->contains(entity));
    ASSERT_FALSE(group.storage<const double>()->contains(entity));
    ASSERT_FALSE(group.storage<float>()->contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));
    ASSERT_FALSE((registry.any_of<double, float>(entity)));

    group.storage<0u>()->erase(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.begin(), group.end());
    ASSERT_FALSE(group.storage<0u>()->contains(entity));
    ASSERT_TRUE(group.storage<1u>()->contains(entity));
    ASSERT_FALSE(group.storage<2u>()->contains(entity));
    ASSERT_FALSE(group.storage<3u>()->contains(entity));
    ASSERT_TRUE((registry.all_of<char>(entity)));
    ASSERT_FALSE((registry.any_of<int, double, float>(entity)));

    group = {};

    ASSERT_FALSE(group);

    ASSERT_EQ(group.storage<0u>(), nullptr);
    ASSERT_EQ(group.storage<const char>(), nullptr);
    ASSERT_EQ(group.storage<2u>(), nullptr);
    ASSERT_EQ(group.storage<const float>(), nullptr);
}

TEST(NonOwningGroup, Overlapping) {
    entt::registry registry;

    auto group = registry.group(entt::get<char>, entt::exclude<double>);
    auto other = registry.group<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(other.empty());

    const auto entity = registry.create();
    registry.emplace<char>(entity, '1');

    ASSERT_FALSE(group.empty());
    ASSERT_TRUE(other.empty());

    registry.emplace<int>(entity, 2);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE(other.empty());

    registry.emplace<double>(entity, 3.);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(other.empty());
}

TEST(OwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<const char>);

    ASSERT_TRUE(group.empty());

    const auto e0 = registry.create();
    registry.emplace<char>(e0, '1');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 4);
    registry.emplace<char>(e1, '2');

    ASSERT_FALSE(group.empty());
    ASSERT_NO_THROW(group.begin()++);
    ASSERT_NO_THROW(++cgroup.begin());
    ASSERT_NO_THROW([](auto it) { return it++; }(group.rbegin()));
    ASSERT_NO_THROW([](auto it) { return ++it; }(cgroup.rbegin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_NE(group.rbegin(), group.rend());
    ASSERT_NE(cgroup.rbegin(), cgroup.rend());
    ASSERT_EQ(group.size(), 1u);

    registry.emplace<int>(e0);

    ASSERT_EQ(group.size(), 2u);

    registry.erase<int>(e0);

    ASSERT_EQ(group.size(), 1u);

    ASSERT_EQ(cgroup.storage<const int>()->raw()[0u][0u], 4);
    ASSERT_EQ(group.storage<int>()->raw()[0u][0u], 4);

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 4);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<1>(entity), '2');
    }

    ASSERT_EQ(group.handle().data()[0u], e1);
    ASSERT_EQ(group.storage<int>()->raw()[0u][0u], 4);

    registry.erase<char>(e0);
    registry.erase<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.rbegin(), group.rend());
    ASSERT_EQ(cgroup.rbegin(), cgroup.rend());
    ASSERT_TRUE(group.empty());

    const decltype(group) invalid{};

    ASSERT_TRUE(group);
    ASSERT_TRUE(cgroup);
    ASSERT_FALSE(invalid);
}

TEST(OwningGroup, Handle) {
    entt::registry registry;
    const auto entity = registry.create();

    auto group = registry.group<int>(entt::get<char>);
    auto &&handle = group.handle();

    ASSERT_TRUE(handle.empty());
    ASSERT_FALSE(handle.contains(entity));
    ASSERT_EQ(&handle, &group.handle());
    ASSERT_EQ(&handle, group.storage<int>());

    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_FALSE(handle.empty());
    ASSERT_TRUE(handle.contains(entity));
    ASSERT_EQ(&handle, &group.handle());
}

TEST(OwningGroup, Invalid) {
    entt::registry registry{};
    auto group = std::as_const(registry).group_if_exists<const int>(entt::get<const test::empty>);

    const auto entity = registry.create();
    registry.emplace<test::empty>(entity);
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
    std::array entity{registry.create(), registry.create()};

    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<const char>);

    registry.emplace<int>(entity[0u], 0);
    registry.emplace<char>(entity[0u], static_cast<char>(0));

    registry.emplace<int>(entity[1u], 1);
    registry.emplace<char>(entity[1u], static_cast<char>(1));

    auto iterable = group.each();
    auto citerable = cgroup.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_THROW(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(it.base(), group.begin());
    ASSERT_EQ((it++, ++it), iterable.end());
    ASSERT_EQ(it.base(), group.end());

    group.each([expected = 1](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cgroup.each([expected = 1](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    testing::StaticAssertTypeEq<decltype(std::get<1>(*iterable.begin())), int &>();
    testing::StaticAssertTypeEq<decltype(std::get<2>(*citerable.begin())), const char &>();

    // do not use iterable, make sure an iterable group works when created from a temporary
    for(auto [entt, ivalue, cvalue]: registry.group<int>(entt::get<char>).each()) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), ivalue);
        ASSERT_EQ(static_cast<char>(entt::to_integral(entt)), cvalue);
    }
}

TEST(OwningGroup, SortOrdered) {
    entt::registry registry;
    auto group = registry.group<test::boxed_int, char>();

    const std::array value{test::boxed_int{16}, test::boxed_int{8}, test::boxed_int{4}, test::boxed_int{1}, test::boxed_int{2}};
    std::array<entt::entity, value.size()> entity{};
    const std::array other{'a', 'b', 'c'};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::boxed_int>(entity.begin(), entity.end(), value.begin());
    registry.insert<char>(entity.begin(), entity.begin() + other.size(), other.begin());

    group.sort([&group](const entt::entity lhs, const entt::entity rhs) {
        return group.get<test::boxed_int>(lhs).value < group.get<0>(rhs).value;
    });

    ASSERT_EQ(group.handle().data()[0u], entity[0]);
    ASSERT_EQ(group.handle().data()[1u], entity[1]);
    ASSERT_EQ(group.handle().data()[2u], entity[2]);
    ASSERT_EQ(group.handle().data()[3u], entity[3]);
    ASSERT_EQ(group.handle().data()[4u], entity[4]);

    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][0u], value[0]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][1u], value[1]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][2u], value[2]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][3u], value[3]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][4u], value[4]);

    ASSERT_EQ(group.storage<char>()->raw()[0u][0u], other[0]);
    ASSERT_EQ(group.storage<char>()->raw()[0u][1u], other[1]);
    ASSERT_EQ(group.storage<char>()->raw()[0u][2u], other[2]);

    ASSERT_EQ((group.get<test::boxed_int, char>(entity[0])), (std::make_tuple(value[0], other[0])));
    ASSERT_EQ((group.get<0, 1>(entity[1])), (std::make_tuple(value[1], other[1])));
    ASSERT_EQ((group.get<test::boxed_int, char>(entity[2])), (std::make_tuple(value[2], other[2])));

    ASSERT_FALSE(group.contains(entity[3]));
    ASSERT_FALSE(group.contains(entity[4]));
}

TEST(OwningGroup, SortReverse) {
    entt::registry registry;
    auto group = registry.group<test::boxed_int, char>();

    const std::array value{test::boxed_int{4}, test::boxed_int{8}, test::boxed_int{16}, test::boxed_int{1}, test::boxed_int{2}};
    std::array<entt::entity, value.size()> entity{};
    const std::array other{'a', 'b', 'c'};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::boxed_int>(entity.begin(), entity.end(), value.begin());
    registry.insert<char>(entity.begin(), entity.begin() + other.size(), other.begin());

    group.sort<test::boxed_int>([](const auto &lhs, const auto &rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(group.handle().data()[0u], entity[2]);
    ASSERT_EQ(group.handle().data()[1u], entity[1]);
    ASSERT_EQ(group.handle().data()[2u], entity[0]);
    ASSERT_EQ(group.handle().data()[3u], entity[3]);
    ASSERT_EQ(group.handle().data()[4u], entity[4]);

    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][0u], value[2]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][1u], value[1]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][2u], value[0]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][3u], value[3]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][4u], value[4]);

    ASSERT_EQ(group.storage<char>()->raw()[0u][0u], other[2]);
    ASSERT_EQ(group.storage<char>()->raw()[0u][1u], other[1]);
    ASSERT_EQ(group.storage<char>()->raw()[0u][2u], other[0]);

    ASSERT_EQ((group.get<test::boxed_int, char>(entity[0])), (std::make_tuple(value[0], other[0])));
    ASSERT_EQ((group.get<0, 1>(entity[1])), (std::make_tuple(value[1], other[1])));
    ASSERT_EQ((group.get<test::boxed_int, char>(entity[2])), (std::make_tuple(value[2], other[2])));

    ASSERT_FALSE(group.contains(entity[3]));
    ASSERT_FALSE(group.contains(entity[4]));
}

TEST(OwningGroup, SortUnordered) {
    entt::registry registry;
    auto group = registry.group<test::boxed_int>(entt::get<char>);

    const std::array value{test::boxed_int{16}, test::boxed_int{2}, test::boxed_int{1}, test::boxed_int{32}, test::boxed_int{64}, test::boxed_int{4}, test::boxed_int{8}};
    std::array<entt::entity, value.size()> entity{};
    const std::array other{'c', 'b', 'a', 'd', 'e'};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::boxed_int>(entity.begin(), entity.end(), value.begin());
    registry.insert<char>(entity.begin(), entity.begin() + other.size(), other.begin());

    group.sort<test::boxed_int, char>([](const auto lhs, const auto rhs) {
        testing::StaticAssertTypeEq<decltype(std::get<0>(lhs)), test::boxed_int &>();
        testing::StaticAssertTypeEq<decltype(std::get<1>(rhs)), char &>();
        return std::get<1>(lhs) < std::get<1>(rhs);
    });

    ASSERT_EQ(group.handle().data()[0u], entity[4]);
    ASSERT_EQ(group.handle().data()[1u], entity[3]);
    ASSERT_EQ(group.handle().data()[2u], entity[0]);
    ASSERT_EQ(group.handle().data()[3u], entity[1]);
    ASSERT_EQ(group.handle().data()[4u], entity[2]);
    ASSERT_EQ(group.handle().data()[5u], entity[5]);
    ASSERT_EQ(group.handle().data()[6u], entity[6]);

    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][0u], value[4]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][1u], value[3]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][2u], value[0]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][3u], value[1]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][4u], value[2]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][5u], value[5]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][6u], value[6]);

    ASSERT_EQ(group.get<char>(group.handle().data()[0u]), other[4]);
    ASSERT_EQ(group.get<1>(group.handle().data()[1u]), other[3]);
    ASSERT_EQ(group.get<char>(group.handle().data()[2u]), other[0]);
    ASSERT_EQ(group.get<1>(group.handle().data()[3u]), other[1]);
    ASSERT_EQ(group.get<char>(group.handle().data()[4u]), other[2]);

    ASSERT_FALSE(group.contains(entity[5]));
    ASSERT_FALSE(group.contains(entity[6]));
}

TEST(OwningGroup, SortWithExclusionList) {
    entt::registry registry;
    auto group = registry.group<test::boxed_int>(entt::get<>, entt::exclude<char>);

    const std::array value{test::boxed_int{1}, test::boxed_int{2}, test::boxed_int{4}, test::boxed_int{8}, test::boxed_int{16}};
    std::array<entt::entity, value.size()> entity{};

    registry.create(entity.begin(), entity.end());
    registry.insert<test::boxed_int>(entity.begin(), entity.end(), value.begin());
    registry.emplace<char>(entity[2]);

    group.sort([](const entt::entity lhs, const entt::entity rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(group.handle().data()[0u], entity[4]);
    ASSERT_EQ(group.handle().data()[1u], entity[3]);
    ASSERT_EQ(group.handle().data()[2u], entity[1]);
    ASSERT_EQ(group.handle().data()[3u], entity[0]);

    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][0u], value[4]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][1u], value[3]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][2u], value[1]);
    ASSERT_EQ(group.storage<test::boxed_int>()->raw()[0u][3u], value[0]);

    ASSERT_EQ(group.get<test::boxed_int>(entity[0]), value[0]);
    ASSERT_EQ(group.get<0>(entity[1]), value[1]);
    ASSERT_EQ(group.get<test::boxed_int>(entity[3]), value[3]);
    ASSERT_EQ(group.get<0>(entity[4]), value[4]);

    ASSERT_FALSE(group.contains(entity[2]));
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
    registry.emplace<int>(registry.create(), 4);

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
    auto group = registry.group<int, const char>(entt::get<test::empty, double, const float>);

    ASSERT_EQ(group.size(), 0u);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');
    registry.emplace<test::empty>(entity);
    registry.emplace<double>(entity, 0.);
    registry.emplace<float>(entity, 0.f);

    ASSERT_EQ(group.size(), 1u);

    testing::StaticAssertTypeEq<decltype(group.get<0>({})), int &>();
    testing::StaticAssertTypeEq<decltype(group.get<int>({})), int &>();

    testing::StaticAssertTypeEq<decltype(group.get<1>({})), const char &>();
    testing::StaticAssertTypeEq<decltype(group.get<const char>({})), const char &>();

    testing::StaticAssertTypeEq<decltype(group.get<2>({})), void>();
    testing::StaticAssertTypeEq<decltype(group.get<test::empty>({})), void>();

    testing::StaticAssertTypeEq<decltype(group.get<3>({})), double &>();
    testing::StaticAssertTypeEq<decltype(group.get<double>({})), double &>();

    testing::StaticAssertTypeEq<decltype(group.get<4>({})), const float &>();
    testing::StaticAssertTypeEq<decltype(group.get<const float>({})), const float &>();

    testing::StaticAssertTypeEq<decltype(group.get<int, const char, test::empty, double, const float>({})), std::tuple<int &, const char &, double &, const float &>>();
    testing::StaticAssertTypeEq<decltype(group.get<0, 1, 2, 3, 4>({})), std::tuple<int &, const char &, double &, const float &>>();

    testing::StaticAssertTypeEq<decltype(group.get({})), std::tuple<int &, const char &, double &, const float &>>();

    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists<int>(entt::get<char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists<const int>(entt::get<char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>();
    testing::StaticAssertTypeEq<decltype(std::as_const(registry).group_if_exists<int>(entt::get<const char>)), decltype(std::as_const(registry).group_if_exists<const int>(entt::get<const char>))>();

    group.each([](auto &&iv, auto &&cv, auto &&dv, auto &&fv) {
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
        testing::StaticAssertTypeEq<decltype(dv), double &>();
        testing::StaticAssertTypeEq<decltype(fv), const float &>();
    });

    for([[maybe_unused]] auto [entt, iv, cv, dv, fv]: group.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
        testing::StaticAssertTypeEq<decltype(dv), double &>();
        testing::StaticAssertTypeEq<decltype(fv), const float &>();
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

    const auto group = registry.group<int>(entt::get<>, entt::exclude<char, double>);

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
            ASSERT_EQ(group.get<0>(e2), 2);
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
            ASSERT_EQ(group.get<0>(e3), 3);
        }
    }
}

TEST(OwningGroup, EmptyAndNonEmptyTypes) {
    entt::registry registry;
    const auto group = registry.group<int>(entt::get<test::empty>);

    const auto e0 = registry.create();
    registry.emplace<test::empty>(e0);
    registry.emplace<int>(e0);

    const auto e1 = registry.create();
    registry.emplace<test::empty>(e1);
    registry.emplace<int>(e1);

    registry.emplace<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    for(auto [entt, iv]: group.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        ASSERT_TRUE(entt == e0 || entt == e1);
    }

    ASSERT_EQ(group.size(), 2u);
}

TEST(OwningGroup, TrackEntitiesOnComponentDestruction) {
    entt::registry registry;
    const auto group = registry.group<int>(entt::get<>, entt::exclude<char>);
    const auto cgroup = std::as_const(registry).group_if_exists<const int>(entt::get<>, entt::exclude<char>);

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
    registry.emplace<test::empty>(entity);

    registry.group<int>(entt::get<char, test::empty>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group<int>(entt::get<char, test::empty>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
        ASSERT_EQ(entity, entt);
    }

    registry.group<char>(entt::get<test::empty, int>).each([check = true](char, int) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, cv, iv]: registry.group<char>(entt::get<test::empty, int>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        ASSERT_EQ(entity, entt);
    }

    registry.group<test::empty>(entt::get<int, char>).each([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    for(auto [entt, iv, cv]: registry.group<test::empty>(entt::get<int, char>).each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), char &>();
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
    constexpr auto number_of_entities = 30u;

    for(std::size_t i{}; i < number_of_entities; ++i) {
        auto entity = registry.create();
        if((i % 2u) == 0u) { registry.emplace<int>(entity); }
        if((i % 3u) == 0u) { registry.emplace<char>(entity); }
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
    registry.group<char, int>().each([entity](const auto entt, const auto &cv, const auto &iv) {
        ASSERT_EQ(entity, entt);
        ASSERT_EQ(cv, 'c');
        ASSERT_EQ(iv, 2);
    });
}

TEST(OwningGroup, SwapElements) {
    entt::registry registry;
    std::array entity{registry.create(), registry.create(), registry.create()};

    registry.emplace<int>(entity[1u]);
    registry.emplace<int>(entity[0u]);

    registry.emplace<char>(entity[2u]);
    registry.emplace<char>(entity[0u]);

    ASSERT_EQ(registry.storage<int>().index(entity[0u]), 1u);
    ASSERT_EQ(registry.storage<char>().index(entity[0u]), 1u);

    registry.group<int>(entt::get<char>);

    ASSERT_EQ(registry.storage<int>().index(entity[0u]), 0u);
    ASSERT_EQ(registry.storage<char>().index(entity[0u]), 1u);
}

TEST(OwningGroup, SwappingValuesIsAllowed) {
    entt::registry registry;
    const auto group = registry.group<test::boxed_int>(entt::get<test::empty>);

    for(std::size_t i{}; i < 2u; ++i) {
        const auto entity = registry.create();
        registry.emplace<test::boxed_int>(entity, static_cast<int>(i));
        registry.emplace<test::empty>(entity);
    }

    registry.destroy(group.back());

    // thanks to @andranik3949 for pointing out this missing test
    registry.view<const test::boxed_int>().each([](const auto entity, const auto &value) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entity)), value.value);
    });
}

TEST(OwningGroup, ExtendedGet) {
    using type = decltype(std::declval<entt::registry>().group<int, test::empty>(entt::get<char>).get({}));

    ASSERT_EQ(std::tuple_size_v<type>, 2u);

    testing::StaticAssertTypeEq<std::tuple_element_t<0, type>, int &>();
    testing::StaticAssertTypeEq<std::tuple_element_t<1, type>, char &>();

    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 3);
    registry.emplace<char>(entity, 'c');

    const auto tup = registry.group<int>(entt::get<char>).get(entity);

    ASSERT_EQ(std::get<0>(tup), 3);
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
    auto group = registry.group<int>(entt::get<const char>, entt::exclude<double, const float>);

    testing::StaticAssertTypeEq<decltype(group.storage<0u>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<int>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const int>()), entt::storage_type_t<int> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<1u>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<char>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const char>()), const entt::storage_type_t<char> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<2u>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<double>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const double>()), entt::storage_type_t<double> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<3u>()), const entt::storage_type_t<float> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<float>()), const entt::storage_type_t<float> *>();
    testing::StaticAssertTypeEq<decltype(group.storage<const float>()), const entt::storage_type_t<float> *>();

    ASSERT_TRUE(group);

    ASSERT_NE(group.storage<int>(), nullptr);
    ASSERT_NE(group.storage<1u>(), nullptr);
    ASSERT_NE(group.storage<double>(), nullptr);
    ASSERT_NE(group.storage<3u>(), nullptr);

    ASSERT_EQ(group.size(), 0u);

    group.storage<int>()->emplace(entity);
    group.storage<double>()->emplace(entity);
    registry.emplace<char>(entity);
    registry.emplace<float>(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.begin(), group.end());
    ASSERT_TRUE(group.storage<int>()->contains(entity));
    ASSERT_TRUE(group.storage<const char>()->contains(entity));
    ASSERT_TRUE(group.storage<double>()->contains(entity));
    ASSERT_TRUE(group.storage<const float>()->contains(entity));
    ASSERT_TRUE((registry.all_of<int, char, double, float>(entity)));

    group.storage<double>()->erase(entity);
    registry.erase<float>(entity);

    ASSERT_EQ(group.size(), 1u);
    ASSERT_NE(group.begin(), group.end());
    ASSERT_TRUE(group.storage<const int>()->contains(entity));
    ASSERT_TRUE(group.storage<char>()->contains(entity));
    ASSERT_FALSE(group.storage<const double>()->contains(entity));
    ASSERT_FALSE(group.storage<float>()->contains(entity));
    ASSERT_TRUE((registry.all_of<int, char>(entity)));
    ASSERT_FALSE((registry.any_of<double, float>(entity)));

    group.storage<0u>()->erase(entity);

    ASSERT_EQ(group.size(), 0u);
    ASSERT_EQ(group.begin(), group.end());
    ASSERT_FALSE(group.storage<0u>()->contains(entity));
    ASSERT_TRUE(group.storage<1u>()->contains(entity));
    ASSERT_FALSE(group.storage<2u>()->contains(entity));
    ASSERT_FALSE(group.storage<3u>()->contains(entity));
    ASSERT_TRUE((registry.all_of<char>(entity)));
    ASSERT_FALSE((registry.any_of<int, double, float>(entity)));

    group = {};

    ASSERT_FALSE(group);

    ASSERT_EQ(group.storage<0u>(), nullptr);
    ASSERT_EQ(group.storage<const char>(), nullptr);
    ASSERT_EQ(group.storage<2u>(), nullptr);
    ASSERT_EQ(group.storage<const float>(), nullptr);
}

ENTT_DEBUG_TEST(OwningGroupDeathTest, Overlapping) {
    entt::registry registry;
    registry.group<char>(entt::get<int>, entt::exclude<double>);

    ASSERT_DEATH((registry.group<char, float>(entt::get<float>, entt::exclude<double>)), "");
    ASSERT_DEATH(registry.group<char>(entt::get<int, float>, entt::exclude<double>), "");
    ASSERT_DEATH(registry.group<char>(entt::get<int>, entt::exclude<double, float>), "");
}
