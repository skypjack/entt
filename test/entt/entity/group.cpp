#include <utility>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/group.hpp>

struct empty_type {};
struct boxed_int { int value; };

bool operator==(const boxed_int &lhs, const boxed_int &rhs) {
    return lhs.value == rhs.value;
}

TEST(NonOwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group(entt::get<const int, const char>);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE((group.empty<int, char>()));
    ASSERT_TRUE((cgroup.empty<const int, const char>()));

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE((group.empty<int>()));
    ASSERT_FALSE((cgroup.empty<const char>()));
    ASSERT_NO_THROW((group.begin()++));
    ASSERT_NO_THROW((++cgroup.begin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{1});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.assign<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{2});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.remove<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{1});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<const char>(entity), '2');
    }

    ASSERT_EQ(*(group.data() + 0), e1);

    ASSERT_EQ(*(group.data<int>() + 0), e1);
    ASSERT_EQ(*(group.data<char>() + 0), e0);
    ASSERT_EQ(*(cgroup.data<const char>() + 1), e1);

    ASSERT_EQ(*(group.raw<int>() + 0), 42);
    ASSERT_EQ(*(group.raw<char>() + 0), '1');
    ASSERT_EQ(*(cgroup.raw<const char>() + 1), '2');

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_TRUE(group.empty());

    ASSERT_TRUE(group.capacity());

    group.shrink_to_fit();

    ASSERT_FALSE(group.capacity());
}

TEST(NonOwningGroup, ElementAccess) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group(entt::get<const int, const char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    for(typename decltype(group)::size_type i{}; i < group.size(); ++i) {
        ASSERT_EQ(group[i], i ? e0 : e1);
        ASSERT_EQ(cgroup[i], i ? e0 : e1);
    }
}

TEST(NonOwningGroup, Contains) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    ASSERT_FALSE(group.contains(e0));
    ASSERT_TRUE(group.contains(e1));
}

TEST(NonOwningGroup, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    ASSERT_TRUE(registry.group(entt::get<char, int, float>).empty());
    ASSERT_TRUE(registry.group(entt::get<double, char, int, float>).empty());
}

TEST(NonOwningGroup, Each) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto cgroup = std::as_const(registry).group(entt::get<const int, const char>);
    std::size_t cnt = 0;

    group.each([&cnt](auto, int &, char &) { ++cnt; });
    group.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{4});

    cgroup.each([&cnt](auto, const int &, const char &) { --cnt; });
    cgroup.each([&cnt](const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(NonOwningGroup, Sort) {
    entt::registry registry;
    auto group = registry.group(entt::get<const int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();
    const auto e3 = registry.create();

    registry.assign<unsigned int>(e0, 0u);
    registry.assign<unsigned int>(e1, 1u);
    registry.assign<unsigned int>(e2, 2u);
    registry.assign<unsigned int>(e3, 3u);

    registry.assign<int>(e0, 0);
    registry.assign<int>(e1, 1);
    registry.assign<int>(e2, 2);

    ASSERT_EQ(*(group.raw<unsigned int>() + 0u), 0u);
    ASSERT_EQ(*(group.raw<unsigned int>() + 1u), 1u);
    ASSERT_EQ(*(group.raw<unsigned int>() + 2u), 2u);

    ASSERT_EQ(*(group.raw<const int>() + 0u), 0);
    ASSERT_EQ(*(group.raw<const int>() + 1u), 1);
    ASSERT_EQ(*(group.raw<const int>() + 2u), 2);

    ASSERT_EQ(*(group.data() + 0u), e0);
    ASSERT_EQ(*(group.data() + 1u), e1);
    ASSERT_EQ(*(group.data() + 2u), e2);

    group.sort([](const entt::entity lhs, const entt::entity rhs) {
        return std::underlying_type_t<entt::entity>(lhs) < std::underlying_type_t<entt::entity>(rhs);
    });

    ASSERT_EQ(*(group.raw<unsigned int>() + 0u), 0u);
    ASSERT_EQ(*(group.raw<unsigned int>() + 1u), 1u);
    ASSERT_EQ(*(group.raw<unsigned int>() + 2u), 2u);

    ASSERT_EQ(*(group.raw<const int>() + 0u), 0);
    ASSERT_EQ(*(group.raw<const int>() + 1u), 1);
    ASSERT_EQ(*(group.raw<const int>() + 2u), 2);

    ASSERT_EQ(*(group.data() + 0u), e2);
    ASSERT_EQ(*(group.data() + 1u), e1);
    ASSERT_EQ(*(group.data() + 2u), e0);

    ASSERT_EQ((group.get<const int, unsigned int>(e0)), (std::make_tuple(0, 0u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e1)), (std::make_tuple(1, 1u)));
    ASSERT_EQ((group.get<const int, unsigned int>(e2)), (std::make_tuple(2, 2u)));

    ASSERT_FALSE(group.contains(e3));

    group.sort<const int>([](const int lhs, const int rhs) {
        return lhs > rhs;
    });

    ASSERT_EQ(*(group.data() + 0u), e0);
    ASSERT_EQ(*(group.data() + 1u), e1);
    ASSERT_EQ(*(group.data() + 2u), e2);

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

    registry.assign<unsigned int>(e0, uval++);
    registry.assign<unsigned int>(e1, uval++);
    registry.assign<unsigned int>(e2, uval++);
    registry.assign<unsigned int>(e3, uval+1);

    registry.assign<int>(e0, ival++);
    registry.assign<int>(e1, ival++);
    registry.assign<int>(e2, ival++);

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

    registry.assign<unsigned int>(e0, 0u);
    registry.assign<unsigned int>(e1, 1u);

    registry.assign<int>(e0, 0);
    registry.assign<int>(e1, 1);

    registry.destroy(e0);
    registry.assign<int>(registry.create(), 42);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group[{}], e1);
    ASSERT_EQ(group.get<int>(e1), 1);
    ASSERT_EQ(group.get<unsigned int>(e1), 1u);

    group.each([e1](auto entity, auto ivalue, auto uivalue) {
        ASSERT_EQ(entity, e1);
        ASSERT_EQ(ivalue, 1);
        ASSERT_EQ(uivalue, 1u);
    });
}

TEST(NonOwningGroup, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, const char>);

    ASSERT_EQ(group.size(), decltype(group.size()){0});

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), decltype(group.size()){1});

    ASSERT_TRUE((std::is_same_v<decltype(group.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const char>({})), const char &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<int, const char>({})), std::tuple<int &, const char &>>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<const char>()), const char *>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<int>()), int *>));

    group.each([](auto &&i, auto &&c) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
        ASSERT_TRUE((std::is_same_v<decltype(c), const char &>));
    });
}

TEST(NonOwningGroup, Find) {
    entt::registry registry;
    auto group = registry.group(entt::get<int, const char>);

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
    registry.assign<int>(e5);
    registry.assign<char>(e5);

    ASSERT_NE(group.find(e5), group.end());
    ASSERT_EQ(group.find(e4), group.end());
}

TEST(NonOwningGroup, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1);

    const auto group = registry.group(entt::get<int>, entt::exclude<char>);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);

    const auto e3 = registry.create();
    registry.assign<int>(e3, 3);
    registry.assign<char>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(group.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(group.get<int>(e2), 2);
        }
    }

    registry.assign<char>(e0);
    registry.assign<char>(e2);

    ASSERT_TRUE(group.empty());

    registry.remove<char>(e1);
    registry.remove<char>(e3);

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
    registry.assign<empty_type>(e0);
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<empty_type>(e1);
    registry.assign<int>(e1);

    registry.assign<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});
}

TEST(NonOwningGroup, TrackEntitiesOnComponentDestruction) {
    entt::registry registry;
    const auto group = registry.group(entt::get<int>, entt::exclude<char>);
    const auto cgroup = std::as_const(registry).group(entt::get<const int>, entt::exclude<char>);

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(cgroup.empty());

    registry.remove<char>(entity);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE(cgroup.empty());
}

TEST(NonOwningGroup, Less) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.assign<int>(entity);
    registry.assign<char>(entity);
    registry.assign<entt::tag<"empty"_hs>>(entity);

    registry.group(entt::get<int, char, entt::tag<"empty"_hs>>).less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.group(entt::get<int, entt::tag<"empty"_hs>, char>).less([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    registry.group(entt::get<entt::tag<"empty"_hs>, int, char>).less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.group(entt::get<int, char, double>).less([](const auto, int, char, double) { FAIL(); });
}

TEST(NonOwningGroup, FrontBack) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<const int, const char>);

    ASSERT_EQ(group.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(group.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    const auto entity = registry.create();
    registry.assign<char>(entity);

    ASSERT_EQ(group.front(), e1);
    ASSERT_EQ(group.back(), e0);
}

TEST(NonOwningGroup, SignalRace) {
    entt::registry registry;
    registry.on_construct<double>().connect<&entt::registry::assign_or_replace<int>>();
    registry.group(entt::get<int, double>);

    auto entity = registry.create();
    registry.assign<double>(entity);

    ASSERT_EQ(registry.group(entt::get<int, double>).size(), 1u);
}

TEST(OwningGroup, Functionalities) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group<const int>(entt::get<const char>);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE((group.empty<int, char>()));
    ASSERT_TRUE((cgroup.empty<const int, const char>()));

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE((group.empty<int>()));
    ASSERT_FALSE((cgroup.empty<const char>()));
    ASSERT_NO_THROW((group.begin()++));
    ASSERT_NO_THROW((++cgroup.begin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{1});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.assign<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{2});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.remove<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group.size<int>(), typename decltype(group)::size_type{1});
    ASSERT_EQ(cgroup.size<const char>(), typename decltype(group)::size_type{2});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    ASSERT_EQ(*(cgroup.raw<const int>() + 0), 42);
    ASSERT_EQ(*(group.raw<int>() + 0), 42);

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<const char>(entity), '2');
    }

    ASSERT_EQ(*(group.data() + 0), e1);

    ASSERT_EQ(*(group.data<int>() + 0), e1);
    ASSERT_EQ(*(group.data<char>() + 0), e0);
    ASSERT_EQ(*(cgroup.data<const char>() + 1), e1);

    ASSERT_EQ(*(group.raw<int>() + 0), 42);
    ASSERT_EQ(*(group.raw<char>() + 0), '1');
    ASSERT_EQ(*(cgroup.raw<const char>() + 1), '2');

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_TRUE(group.empty());
}

TEST(OwningGroup, ElementAccess) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto cgroup = std::as_const(registry).group<const int>(entt::get<const char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    for(typename decltype(group)::size_type i{}; i < group.size(); ++i) {
        ASSERT_EQ(group[i], i ? e0 : e1);
        ASSERT_EQ(cgroup[i], i ? e0 : e1);
    }
}

TEST(OwningGroup, Contains) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    ASSERT_FALSE(group.contains(e0));
    ASSERT_TRUE(group.contains(e1));
}

TEST(OwningGroup, Empty) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    ASSERT_TRUE((registry.group<char, int>(entt::get<float>).empty()));
    ASSERT_TRUE((registry.group<double, float>(entt::get<char, int>).empty()));
}

TEST(OwningGroup, Each) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto cgroup = std::as_const(registry).group<const int>(entt::get<const char>);
    std::size_t cnt = 0;

    group.each([&cnt](auto, int &, char &) { ++cnt; });
    group.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{4});

    cgroup.each([&cnt](auto, const int &, const char &) { --cnt; });
    cgroup.each([&cnt](const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(OwningGroup, SortOrdered) {
    entt::registry registry;
    auto group = registry.group<boxed_int, char>();

    entt::entity entities[5]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.assign<boxed_int>(entities[0], 12);
    registry.assign<char>(entities[0], 'a');

    registry.assign<boxed_int>(entities[1], 9);
    registry.assign<char>(entities[1], 'b');

    registry.assign<boxed_int>(entities[2], 6);
    registry.assign<char>(entities[2], 'c');

    registry.assign<boxed_int>(entities[3], 1);
    registry.assign<boxed_int>(entities[4], 2);

    group.sort([&group](const entt::entity lhs, const entt::entity rhs) {
        return group.get<boxed_int>(lhs).value < group.get<boxed_int>(rhs).value;
    });

    ASSERT_EQ(*(group.data() + 0u), entities[0]);
    ASSERT_EQ(*(group.data() + 1u), entities[1]);
    ASSERT_EQ(*(group.data() + 2u), entities[2]);
    ASSERT_EQ(*(group.data() + 3u), entities[3]);
    ASSERT_EQ(*(group.data() + 4u), entities[4]);

    ASSERT_EQ((group.raw<boxed_int>() + 0u)->value, 12);
    ASSERT_EQ((group.raw<boxed_int>() + 1u)->value, 9);
    ASSERT_EQ((group.raw<boxed_int>() + 2u)->value, 6);
    ASSERT_EQ((group.raw<boxed_int>() + 3u)->value, 1);
    ASSERT_EQ((group.raw<boxed_int>() + 4u)->value, 2);

    ASSERT_EQ(*(group.raw<char>() + 0u), 'a');
    ASSERT_EQ(*(group.raw<char>() + 1u), 'b');
    ASSERT_EQ(*(group.raw<char>() + 2u), 'c');

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

    registry.assign<boxed_int>(entities[0], 6);
    registry.assign<char>(entities[0], 'a');

    registry.assign<boxed_int>(entities[1], 9);
    registry.assign<char>(entities[1], 'b');

    registry.assign<boxed_int>(entities[2], 12);
    registry.assign<char>(entities[2], 'c');

    registry.assign<boxed_int>(entities[3], 1);
    registry.assign<boxed_int>(entities[4], 2);

    group.sort<boxed_int>([](const auto &lhs, const auto &rhs) {
        return lhs.value < rhs.value;
    });

    ASSERT_EQ(*(group.data() + 0u), entities[2]);
    ASSERT_EQ(*(group.data() + 1u), entities[1]);
    ASSERT_EQ(*(group.data() + 2u), entities[0]);
    ASSERT_EQ(*(group.data() + 3u), entities[3]);
    ASSERT_EQ(*(group.data() + 4u), entities[4]);

    ASSERT_EQ((group.raw<boxed_int>() + 0u)->value, 12);
    ASSERT_EQ((group.raw<boxed_int>() + 1u)->value, 9);
    ASSERT_EQ((group.raw<boxed_int>() + 2u)->value, 6);
    ASSERT_EQ((group.raw<boxed_int>() + 3u)->value, 1);
    ASSERT_EQ((group.raw<boxed_int>() + 4u)->value, 2);

    ASSERT_EQ(*(group.raw<char>() + 0u), 'c');
    ASSERT_EQ(*(group.raw<char>() + 1u), 'b');
    ASSERT_EQ(*(group.raw<char>() + 2u), 'a');

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

    registry.assign<boxed_int>(entities[0], 6);
    registry.assign<char>(entities[0], 'c');

    registry.assign<boxed_int>(entities[1], 3);
    registry.assign<char>(entities[1], 'b');

    registry.assign<boxed_int>(entities[2], 1);
    registry.assign<char>(entities[2], 'a');

    registry.assign<boxed_int>(entities[3], 9);
    registry.assign<char>(entities[3], 'd');

    registry.assign<boxed_int>(entities[4], 12);
    registry.assign<char>(entities[4], 'e');

    registry.assign<boxed_int>(entities[5], 4);
    registry.assign<boxed_int>(entities[6], 5);

    group.sort<char>([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(group.data() + 0u), entities[4]);
    ASSERT_EQ(*(group.data() + 1u), entities[3]);
    ASSERT_EQ(*(group.data() + 2u), entities[0]);
    ASSERT_EQ(*(group.data() + 3u), entities[1]);
    ASSERT_EQ(*(group.data() + 4u), entities[2]);
    ASSERT_EQ(*(group.data() + 5u), entities[5]);
    ASSERT_EQ(*(group.data() + 6u), entities[6]);

    ASSERT_EQ((group.raw<boxed_int>() + 0u)->value, 12);
    ASSERT_EQ((group.raw<boxed_int>() + 1u)->value, 9);
    ASSERT_EQ((group.raw<boxed_int>() + 2u)->value, 6);
    ASSERT_EQ((group.raw<boxed_int>() + 3u)->value, 3);
    ASSERT_EQ((group.raw<boxed_int>() + 4u)->value, 1);
    ASSERT_EQ((group.raw<boxed_int>() + 5u)->value, 4);
    ASSERT_EQ((group.raw<boxed_int>() + 6u)->value, 5);

    ASSERT_EQ(*(group.raw<char>() + 0u), 'c');
    ASSERT_EQ(*(group.raw<char>() + 1u), 'b');
    ASSERT_EQ(*(group.raw<char>() + 2u), 'a');
    ASSERT_EQ(*(group.raw<char>() + 3u), 'd');
    ASSERT_EQ(*(group.raw<char>() + 4u), 'e');

    ASSERT_EQ((group.get<boxed_int, char>(entities[0])), (std::make_tuple(boxed_int{6}, 'c')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[1])), (std::make_tuple(boxed_int{3}, 'b')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[2])), (std::make_tuple(boxed_int{1}, 'a')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[3])), (std::make_tuple(boxed_int{9}, 'd')));
    ASSERT_EQ((group.get<boxed_int, char>(entities[4])), (std::make_tuple(boxed_int{12}, 'e')));

    ASSERT_FALSE(group.contains(entities[5]));
    ASSERT_FALSE(group.contains(entities[6]));
}

TEST(OwningGroup, SortWithExclusionList) {
    entt::registry registry;
    auto group = registry.group<boxed_int>(entt::exclude<char>);

    entt::entity entities[5]{};
    registry.create(std::begin(entities), std::end(entities));

    registry.assign<boxed_int>(entities[0], 0);
    registry.assign<boxed_int>(entities[1], 1);
    registry.assign<boxed_int>(entities[2], 2);
    registry.assign<boxed_int>(entities[3], 3);
    registry.assign<boxed_int>(entities[4], 4);

    registry.assign<char>(entities[2]);

    group.sort([](const entt::entity lhs, const entt::entity rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(group.data() + 0u), entities[4]);
    ASSERT_EQ(*(group.data() + 1u), entities[3]);
    ASSERT_EQ(*(group.data() + 2u), entities[1]);
    ASSERT_EQ(*(group.data() + 3u), entities[0]);

    ASSERT_EQ((group.raw<boxed_int>() + 0u)->value, 4);
    ASSERT_EQ((group.raw<boxed_int>() + 1u)->value, 3);
    ASSERT_EQ((group.raw<boxed_int>() + 2u)->value, 1);
    ASSERT_EQ((group.raw<boxed_int>() + 3u)->value, 0);

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

    registry.assign<unsigned int>(e0, 0u);
    registry.assign<unsigned int>(e1, 1u);

    registry.assign<int>(e0, 0);
    registry.assign<int>(e1, 1);

    registry.destroy(e0);
    registry.assign<int>(registry.create(), 42);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group[{}], e1);
    ASSERT_EQ(group.get<int>(e1), 1);
    ASSERT_EQ(group.get<unsigned int>(e1), 1u);

    group.each([e1](auto entity, auto ivalue, auto uivalue) {
        ASSERT_EQ(entity, e1);
        ASSERT_EQ(ivalue, 1);
        ASSERT_EQ(uivalue, 1u);
    });
}

TEST(OwningGroup, ConstNonConstAndAllInBetween) {
    entt::registry registry;
    auto group = registry.group<int, const char>(entt::get<double, const float>);

    ASSERT_EQ(group.size(), decltype(group.size()){0});

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');
    registry.assign<double>(entity, 0.);
    registry.assign<float>(entity, 0.f);

    ASSERT_EQ(group.size(), decltype(group.size()){1});

    ASSERT_TRUE((std::is_same_v<decltype(group.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const char>({})), const char &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<double>({})), double &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const float>({})), const float &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<int, const char, double, const float>({})), std::tuple<int &, const char &, double &, const float &>>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<const float>()), const float *>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<double>()), double *>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<const char>()), const char *>));
    ASSERT_TRUE((std::is_same_v<decltype(group.raw<int>()), int *>));

    group.each([](auto &&i, auto &&c, auto &&d, auto &&f) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
        ASSERT_TRUE((std::is_same_v<decltype(c), const char &>));
        ASSERT_TRUE((std::is_same_v<decltype(d), double &>));
        ASSERT_TRUE((std::is_same_v<decltype(f), const float &>));
    });
}

TEST(OwningGroup, Find) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<const char>);

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
    registry.assign<int>(e5);
    registry.assign<char>(e5);

    ASSERT_NE(group.find(e5), group.end());
    ASSERT_EQ(group.find(e4), group.end());
}

TEST(OwningGroup, ExcludedComponents) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1);

    const auto group = registry.group<int>(entt::exclude<char, double>);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);

    const auto e3 = registry.create();
    registry.assign<int>(e3, 3);
    registry.assign<double>(e3);

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e2);

        if(entity == e0) {
            ASSERT_EQ(group.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(group.get<int>(e2), 2);
        }
    }

    registry.assign<char>(e0);
    registry.assign<double>(e2);

    ASSERT_TRUE(group.empty());

    registry.remove<char>(e1);
    registry.remove<double>(e3);

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
    registry.assign<empty_type>(e0);
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<empty_type>(e1);
    registry.assign<int>(e1);

    registry.assign<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});
}

TEST(OwningGroup, TrackEntitiesOnComponentDestruction) {
    entt::registry registry;
    const auto group = registry.group<int>(entt::exclude<char>);
    const auto cgroup = std::as_const(registry).group<const int>(entt::exclude<char>);

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    ASSERT_TRUE(group.empty());
    ASSERT_TRUE(cgroup.empty());

    registry.remove<char>(entity);

    ASSERT_FALSE(group.empty());
    ASSERT_FALSE(cgroup.empty());
}

TEST(OwningGroup, Less) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.assign<int>(entity);
    registry.assign<char>(entity);
    registry.assign<entt::tag<"empty"_hs>>(entity);

    registry.group<int>(entt::get<char, entt::tag<"empty"_hs>>).less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.group<char>(entt::get<entt::tag<"empty"_hs>, int>).less([check = true](int, char) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    registry.group<entt::tag<"empty"_hs>>(entt::get<int, char>).less([entity](const auto entt, int, char) {
        ASSERT_EQ(entity, entt);
    });

    registry.group<double>(entt::get<int, char>).less([](const auto, double, int, char) { FAIL(); });
}

TEST(OwningGroup, FrontBack) {
    entt::registry registry;
    auto group = registry.group<const char>(entt::get<const int>);

    ASSERT_EQ(group.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(group.back(), static_cast<entt::entity>(entt::null));

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    const auto entity = registry.create();
    registry.assign<char>(entity);

    ASSERT_EQ(group.front(), e1);
    ASSERT_EQ(group.back(), e0);
}

TEST(OwningGroup, SignalRace) {
    entt::registry registry;
    registry.on_construct<double>().connect<&entt::registry::assign_or_replace<int>>();
    registry.group<int>(entt::get<double>);

    auto entity = registry.create();
    registry.assign<double>(entity);

    ASSERT_EQ(registry.group<int>(entt::get<double>).size(), 1u);
}

TEST(OwningGroup, StableLateInitialization) {
    entt::registry registry;

    for(std::size_t i{}; i < 30u; ++i) {
        auto entity = registry.create();
        if(!(i % 2u)) registry.assign<int>(entity);
        if(!(i % 3u)) registry.assign<char>(entity);
    }

    // thanks to @pgruenbacher for pointing out this corner case
    ASSERT_EQ((registry.group<int, char>().size()), 5u);
}

TEST(OwningGroup, PreventEarlyOptOut) {
    entt::registry registry;

    registry.assign<int>(registry.create(), 3);

    const auto entity = registry.create();
    registry.assign<char>(entity, 'c');
    registry.assign<int>(entity, 2);

    // thanks to @pgruenbacher for pointing out this corner case
    registry.group<char, int>().each([entity](const auto entt, const auto &c, const auto &i) {
        ASSERT_EQ(entity, entt);
        ASSERT_EQ(c, 'c');
        ASSERT_EQ(i, 2);
    });
}
