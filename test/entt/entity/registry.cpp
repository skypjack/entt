#include <unordered_set>
#include <functional>
#include <iterator>
#include <memory>
#include <cstdint>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>

struct empty_type {};

struct non_default_constructible {
    non_default_constructible(int v): value{v} {}
    int value;
};

struct aggregate {
    int value{};
};

struct listener {
    template<typename Component>
    static void sort(entt::registry &registry) {
        registry.sort<Component>([](auto lhs, auto rhs) { return lhs < rhs; });
    }

    template<typename Component>
    void incr(const entt::registry &registry, entt::entity entity) {
        ASSERT_TRUE(registry.valid(entity));
        ASSERT_TRUE(registry.has<Component>(entity));
        last = entity;
        ++counter;
    }

    template<typename Component>
    void decr(const entt::registry &registry, entt::entity entity) {
        ASSERT_TRUE(registry.valid(entity));
        ASSERT_TRUE(registry.has<Component>(entity));
        last = entity;
        --counter;
    }

    entt::entity last;
    int counter{0};
};

TEST(Registry, Context) {
    entt::registry registry;

    ASSERT_EQ(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<double>(), nullptr);

    registry.set<char>();
    registry.set<int>();
    registry.ctx_or_set<double>();

    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_NE(registry.try_ctx<int>(), nullptr);
    ASSERT_NE(registry.try_ctx<double>(), nullptr);

    registry.unset<int>();
    registry.unset<double>();

    auto count = 0;

    registry.ctx([&count](const auto var) {
        ASSERT_EQ(var, entt::type_info<char>::id());
        ++count;
    });

    ASSERT_EQ(count, 1);

    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<double>(), nullptr);

    registry.set<char>('c');
    registry.set<int>(0);
    registry.set<double>(1.);
    registry.set<int>(42);

    ASSERT_EQ(registry.ctx_or_set<char>('a'), 'c');
    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<char>(), &registry.ctx<char>());
    ASSERT_EQ(registry.ctx<char>(), std::as_const(registry).ctx<char>());

    ASSERT_EQ(registry.ctx<int>(), 42);
    ASSERT_NE(registry.try_ctx<int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<int>(), &registry.ctx<int>());
    ASSERT_EQ(registry.ctx<int>(), std::as_const(registry).ctx<int>());

    ASSERT_EQ(registry.ctx<double>(), 1.);
    ASSERT_NE(registry.try_ctx<double>(), nullptr);
    ASSERT_EQ(registry.try_ctx<double>(), &registry.ctx<double>());
    ASSERT_EQ(registry.ctx<double>(), std::as_const(registry).ctx<double>());

    ASSERT_EQ(registry.try_ctx<float>(), nullptr);
}

TEST(Registry, Functionalities) {
    entt::registry registry;

    ASSERT_EQ(registry.size(), entt::registry::size_type{0});
    ASSERT_EQ(registry.alive(), entt::registry::size_type{0});
    ASSERT_NO_THROW((registry.reserve<int, char>(8)));
    ASSERT_NO_THROW(registry.reserve(42));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), entt::registry::size_type{42});
    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE((registry.empty<int, char>()));

    registry.prepare<double>();

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_TRUE(registry.has<>(e0));
    ASSERT_FALSE(registry.any<>(e1));

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e0, e1);

    ASSERT_FALSE((registry.has<int, char>(e0)));
    ASSERT_TRUE((registry.has<int, char>(e1)));
    ASSERT_FALSE((registry.any<int, double>(e0)));
    ASSERT_TRUE((registry.any<int, double>(e1)));

    ASSERT_EQ(registry.try_get<int>(e0), nullptr);
    ASSERT_NE(registry.try_get<int>(e1), nullptr);
    ASSERT_EQ(registry.try_get<char>(e0), nullptr);
    ASSERT_NE(registry.try_get<char>(e1), nullptr);
    ASSERT_EQ(registry.try_get<double>(e0), nullptr);
    ASSERT_EQ(registry.try_get<double>(e1), nullptr);

    ASSERT_EQ(registry.assign<int>(e0, 42), 42);
    ASSERT_EQ(registry.assign<char>(e0, 'c'), 'c');
    ASSERT_NO_THROW(registry.remove<int>(e1));
    ASSERT_NO_THROW(registry.remove<char>(e1));

    ASSERT_TRUE((registry.has<int, char>(e0)));
    ASSERT_FALSE((registry.has<int, char>(e1)));
    ASSERT_TRUE((registry.any<int, double>(e0)));
    ASSERT_FALSE((registry.any<int, double>(e1)));

    const auto e2 = registry.create();

    registry.assign_or_replace<int>(e2, registry.get<int>(e0));
    registry.assign_or_replace<char>(e2, registry.get<char>(e0));

    ASSERT_TRUE((registry.has<int, char>(e2)));
    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');

    ASSERT_NE(registry.try_get<int>(e0), nullptr);
    ASSERT_NE(registry.try_get<char>(e0), nullptr);
    ASSERT_EQ(registry.try_get<double>(e0), nullptr);
    ASSERT_EQ(*registry.try_get<int>(e0), 42);
    ASSERT_EQ(*registry.try_get<char>(e0), 'c');

    ASSERT_EQ(std::get<0>(registry.get<int, char>(e0)), 42);
    ASSERT_EQ(*std::get<0>(registry.try_get<int, char, double>(e0)), 42);
    ASSERT_EQ(std::get<1>(static_cast<const entt::registry &>(registry).get<int, char>(e0)), 'c');
    ASSERT_EQ(*std::get<1>(static_cast<const entt::registry &>(registry).try_get<int, char, double>(e0)), 'c');

    ASSERT_EQ(registry.get<int>(e0), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e0), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e0), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e0), &registry.get<char>(e2));

    ASSERT_NO_THROW(registry.replace<int>(e0, [](auto &instance) { instance = 0; }));
    ASSERT_EQ(registry.get<int>(e0), 0);

    ASSERT_NO_THROW(registry.assign_or_replace<int>(e0, 1));
    ASSERT_NO_THROW(registry.assign_or_replace<int>(e1, 1));
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.size(), entt::registry::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e2), entt::registry::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::registry::version_type{0});
    ASSERT_NO_THROW(registry.destroy(e2));
    ASSERT_EQ(registry.version(e2), entt::registry::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::registry::version_type{1});

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));

    ASSERT_EQ(registry.size(), entt::registry::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry::size_type{2});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.clear());

    ASSERT_EQ(registry.size(), entt::registry::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry::size_type{0});
    ASSERT_TRUE(registry.empty());

    const auto e3 = registry.create();

    ASSERT_EQ(registry.get_or_assign<int>(e3, 3), 3);
    ASSERT_EQ(registry.get_or_assign<char>(e3, 'c'), 'c');

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_TRUE((registry.has<int, char>(e3)));
    ASSERT_EQ(registry.get<int>(e3), 3);
    ASSERT_EQ(registry.get<char>(e3), 'c');

    ASSERT_NO_THROW(registry.clear<int>());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.clear());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE((registry.empty<int, char>()));

    const auto e4 = registry.create();
    const auto e5 = registry.create();

    registry.assign<int>(e4);

    ASSERT_NO_THROW(registry.remove_if_exists<int>(e4));
    ASSERT_NO_THROW(registry.remove_if_exists<int>(e5));

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());

    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{8});

    registry.shrink_to_fit<int, char>();

    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{});
}

TEST(Registry, AssignOrReplaceAggregates) {
    entt::registry registry;
    const auto entity = registry.create();
    auto &instance = registry.assign_or_replace<aggregate>(entity, 42);

    ASSERT_EQ(instance.value, 42);
}

TEST(Registry, Identifiers) {
    entt::registry registry;
    const auto pre = registry.create();

    ASSERT_EQ(pre, registry.entity(pre));

    registry.destroy(pre);
    const auto post = registry.create();

    ASSERT_NE(pre, post);
    ASSERT_EQ(entt::registry::entity(pre), entt::registry::entity(post));
    ASSERT_NE(entt::registry::version(pre), entt::registry::version(post));
    ASSERT_NE(registry.version(pre), registry.current(pre));
    ASSERT_EQ(registry.version(post), registry.current(post));
}

TEST(Registry, RawData) {
    entt::registry registry;

    ASSERT_EQ(std::as_const(registry).data(), nullptr);

    const auto entity = registry.create();

    ASSERT_EQ(registry.raw<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).raw<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).data<int>(), nullptr);
    ASSERT_EQ(*std::as_const(registry).data(), entity);

    registry.assign<int>(entity, 42);

    ASSERT_EQ(*registry.raw<int>(), 42);
    ASSERT_EQ(*std::as_const(registry).raw<int>(), 42);
    ASSERT_EQ(*std::as_const(registry).data<int>(), entity);

    const auto other = registry.create();
    registry.destroy(entity);

    ASSERT_NE(*std::as_const(registry).data(), entity);
    ASSERT_EQ(*(std::as_const(registry).data() + 1u), other);
}

TEST(Registry, CreateManyEntitiesAtOnce) {
    entt::registry registry;
    entt::entity entities[3];

    const auto entity = registry.create();
    registry.destroy(registry.create());
    registry.destroy(entity);
    registry.destroy(registry.create());

    registry.create(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.valid(entities[0]));
    ASSERT_TRUE(registry.valid(entities[1]));
    ASSERT_TRUE(registry.valid(entities[2]));

    ASSERT_EQ(registry.entity(entities[0]), entt::entity{0});
    ASSERT_EQ(registry.version(entities[0]), entt::registry::version_type{2});

    ASSERT_EQ(registry.entity(entities[1]), entt::entity{1});
    ASSERT_EQ(registry.version(entities[1]), entt::registry::version_type{1});

    ASSERT_EQ(registry.entity(entities[2]), entt::entity{2});
    ASSERT_EQ(registry.version(entities[2]), entt::registry::version_type{0});
}

TEST(Registry, CreateManyEntitiesAtOnceWithListener) {
    entt::registry registry;
    entt::entity entities[3];
    listener listener;

    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.create(std::begin(entities), std::end(entities));
    registry.assign(std::begin(entities), std::end(entities), 42);
    registry.assign(std::begin(entities), std::end(entities), 'c');

    ASSERT_EQ(registry.get<int>(entities[0]), 42);
    ASSERT_EQ(registry.get<char>(entities[1]), 'c');
    ASSERT_EQ(listener.counter, 3);

    registry.on_construct<int>().disconnect<&listener::incr<int>>(listener);
    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.create(std::begin(entities), std::end(entities));
    registry.assign(std::begin(entities), std::end(entities), 'a');
    registry.assign<empty_type>(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.has<empty_type>(entities[0]));
    ASSERT_EQ(registry.get<char>(entities[2]), 'a');
    ASSERT_EQ(listener.counter, 6);
}

TEST(Registry, CreateWithHint) {
    entt::registry registry;
    auto e3 = registry.create(entt::entity{3});
    auto e2 = registry.create(entt::entity{3});

    ASSERT_EQ(e2, entt::entity{2});
    ASSERT_FALSE(registry.valid(entt::entity{1}));
    ASSERT_EQ(e3, entt::entity{3});

    registry.destroy(e2);

    ASSERT_EQ(registry.version(e2), 0);
    ASSERT_EQ(registry.current(e2), 1);

    e2 = registry.create();
    auto e1 = registry.create(entt::entity{2});

    ASSERT_EQ(registry.entity(e2), entt::entity{2});
    ASSERT_EQ(registry.version(e2), 1);

    ASSERT_EQ(registry.entity(e1), entt::entity{1});
    ASSERT_EQ(registry.version(e1), 0);

    registry.destroy(e1);
    registry.destroy(e2);
    auto e0 = registry.create(entt::entity{0});

    ASSERT_EQ(e0, entt::entity{0});
    ASSERT_EQ(registry.version(e0), 0);
}

TEST(Registry, CreateDestroyEntities) {
    entt::registry registry;
    entt::entity pre{}, post{};

    for(int i = 0; i < 10; ++i) {
        const auto entity = registry.create();
        registry.assign<double>(entity);
    }

    registry.clear();

    for(int i = 0; i < 7; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity);
        if(i == 3) { pre = entity; }
    }

    registry.clear();

    for(int i = 0; i < 5; ++i) {
        const auto entity = registry.create();
        if(i == 3) { post = entity; }
    }

    ASSERT_FALSE(registry.valid(pre));
    ASSERT_TRUE(registry.valid(post));
    ASSERT_NE(registry.version(pre), registry.version(post));
    ASSERT_EQ(registry.version(pre) + 1, registry.version(post));
    ASSERT_EQ(registry.current(pre), registry.current(post));
}

TEST(Registry, CreateDestroyCornerCase) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.destroy(e0);
    registry.destroy(e1);

    registry.each([](auto) { FAIL(); });

    ASSERT_EQ(registry.current(e0), entt::registry::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{1});
}

TEST(Registry, VersionOverflow) {
    entt::registry registry;

    const auto entity = registry.create();
    registry.destroy(entity);

    ASSERT_EQ(registry.version(entity), entt::registry::version_type{});

    for(auto i = entt::entt_traits<std::underlying_type_t<entt::entity>>::version_mask; i; --i) {
        ASSERT_NE(registry.current(entity), registry.version(entity));
        registry.destroy(registry.create());
    }

    ASSERT_EQ(registry.current(entity), registry.version(entity));
}

TEST(Registry, Each) {
    entt::registry registry;
    entt::registry::size_type tot;
    entt::registry::size_type match;

    registry.create();
    registry.assign<int>(registry.create());
    registry.create();
    registry.assign<int>(registry.create());
    registry.create();

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.has<int>(entity)) { ++match; }
        registry.create();
        ++tot;
    });

    ASSERT_EQ(tot, 5u);
    ASSERT_EQ(match, 2u);

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.has<int>(entity)) {
            registry.destroy(entity);
            ++match;
        }

        ++tot;
    });

    ASSERT_EQ(tot, 10u);
    ASSERT_EQ(match, 2u);

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.has<int>(entity)) { ++match; }
        registry.destroy(entity);
        ++tot;
    });

    ASSERT_EQ(tot, 8u);
    ASSERT_EQ(match, 0u);

    registry.each([&](auto) { FAIL(); });
}

TEST(Registry, Orphans) {
    entt::registry registry;
    entt::registry::size_type tot{};

    registry.assign<int>(registry.create());
    registry.create();
    registry.assign<int>(registry.create());

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 1u);
    tot = {};

    registry.each([&](auto entity) { registry.remove_if_exists<int>(entity); });
    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 3u);
    registry.clear();
    tot = {};

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 0u);
}

TEST(Registry, View) {
    entt::registry registry;
    auto mview = registry.view<int, char>();
    auto iview = registry.view<int>();
    auto cview = registry.view<char>();

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_EQ(iview.size(), 3u);
    ASSERT_EQ(cview.size(), 2u);

    std::size_t cnt{};
    mview.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, NonOwningGroupInitOnFirstUse) {
    entt::registry registry;
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    auto group = registry.group<>(entt::get<int, char>);
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE((registry.sortable<int, char>()));
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, NonOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE((registry.sortable<int, char>()));
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInitOnFirstUse) {
    entt::registry registry;
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    auto group = registry.group<int, char>();
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE(registry.sortable<int>());
    ASSERT_FALSE(registry.sortable<char>());
    ASSERT_TRUE(registry.sortable<double>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<int, char>();
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE(registry.sortable<int>());
    ASSERT_FALSE(registry.sortable<char>());
    ASSERT_TRUE(registry.sortable<double>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, PartialOwningGroupInitOnFirstUse) {
    entt::registry registry;
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    auto group = registry.group<int>(entt::get<char>);
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE((registry.sortable<int, char>()));
    ASSERT_FALSE(registry.sortable<int>());
    ASSERT_TRUE(registry.sortable<char>());
    ASSERT_EQ(cnt, 2u);

}

TEST(Registry, PartialOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.assign<decltype(component)>(entity, component), ...);
    };

    create(0, 'c');
    create(0);
    create(0, 'c');

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE((registry.sortable<int, char>()));
    ASSERT_FALSE(registry.sortable<int>());
    ASSERT_TRUE(registry.sortable<char>());
    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, CleanViewAfterRemoveAndClear) {
    entt::registry registry;
    auto view = registry.view<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.remove<char>(entity);

    ASSERT_EQ(view.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.clear<int>();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.clear();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanNonOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanFullOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanPartialOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.clear();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
}

TEST(Registry, NestedGroups) {
    entt::registry registry;
    entt::entity entities[10];

    registry.create(std::begin(entities), std::end(entities));
    registry.assign<int>(std::begin(entities), std::end(entities));
    registry.assign<char>(std::begin(entities), std::end(entities));
    const auto g1 = registry.group<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_TRUE(g1.sortable());
    ASSERT_EQ(g1.size(), 10u);

    const auto g2 = registry.group<int>(entt::get<char>);

    ASSERT_TRUE(g1.sortable());
    ASSERT_FALSE(g2.sortable());
    ASSERT_EQ(g1.size(), 10u);
    ASSERT_EQ(g2.size(), 10u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_TRUE(g1.contains(entities[i*2+1]));
        ASSERT_TRUE(g1.contains(entities[i*2]));
        ASSERT_TRUE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        registry.assign<double>(entities[i*2]);
    }

    ASSERT_EQ(g1.size(), 5u);
    ASSERT_EQ(g2.size(), 10u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_TRUE(g1.contains(entities[i*2+1]));
        ASSERT_FALSE(g1.contains(entities[i*2]));
        ASSERT_TRUE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        registry.remove<int>(entities[i*2+1]);
    }

    ASSERT_EQ(g1.size(), 0u);
    ASSERT_EQ(g2.size(), 5u);

    const auto g3= registry.group<int, float>(entt::get<char>, entt::exclude<double>);

    ASSERT_FALSE(g1.sortable());
    ASSERT_FALSE(g2.sortable());
    ASSERT_TRUE(g3.sortable());

    ASSERT_EQ(g1.size(), 0u);
    ASSERT_EQ(g2.size(), 5u);
    ASSERT_EQ(g3.size(), 0u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_FALSE(g1.contains(entities[i*2+1]));
        ASSERT_FALSE(g1.contains(entities[i*2]));
        ASSERT_FALSE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        ASSERT_FALSE(g3.contains(entities[i*2+1]));
        ASSERT_FALSE(g3.contains(entities[i*2]));
        registry.assign<int>(entities[i*2+1]);
    }

    ASSERT_EQ(g1.size(), 5u);
    ASSERT_EQ(g2.size(), 10u);
    ASSERT_EQ(g3.size(), 0u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_TRUE(g1.contains(entities[i*2+1]));
        ASSERT_FALSE(g1.contains(entities[i*2]));
        ASSERT_TRUE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        ASSERT_FALSE(g3.contains(entities[i*2+1]));
        ASSERT_FALSE(g3.contains(entities[i*2]));
        registry.assign<float>(entities[i*2]);
    }

    ASSERT_EQ(g1.size(), 5u);
    ASSERT_EQ(g2.size(), 10u);
    ASSERT_EQ(g3.size(), 0u);

    for(auto i = 0u; i < 5u; ++i) {
        registry.remove<double>(entities[i*2]);
    }

    ASSERT_EQ(g1.size(), 10u);
    ASSERT_EQ(g2.size(), 10u);
    ASSERT_EQ(g3.size(), 5u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_TRUE(g1.contains(entities[i*2+1]));
        ASSERT_TRUE(g1.contains(entities[i*2]));
        ASSERT_TRUE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        ASSERT_FALSE(g3.contains(entities[i*2+1]));
        ASSERT_TRUE(g3.contains(entities[i*2]));
        registry.remove<int>(entities[i*2+1]);
        registry.remove<int>(entities[i*2]);
    }

    ASSERT_EQ(g1.size(), 0u);
    ASSERT_EQ(g2.size(), 0u);
    ASSERT_EQ(g3.size(), 0u);
}

TEST(Registry, SortSingle) {
    entt::registry registry;

    int val = 0;

    registry.assign<int>(registry.create(), val++);
    registry.assign<int>(registry.create(), val++);
    registry.assign<int>(registry.create(), val++);

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --val);
    }

    registry.sort<int>(std::less<int>{});

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), val++);
    }
}

TEST(Registry, SortMulti) {
    entt::registry registry;

    unsigned int uval = 0u;
    int ival = 0;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<unsigned int>(entity, uval++);
        registry.assign<int>(entity, ival++);
    }

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), --uval);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less<unsigned int>{});
    registry.sort<int, unsigned int>();

    for(auto entity: registry.view<unsigned int>()) {
        ASSERT_EQ(registry.get<unsigned int>(entity), uval++);
    }

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), ival++);
    }
}

TEST(Registry, SortEmpty) {
    entt::registry registry;

    registry.assign<empty_type>(registry.create());
    registry.assign<empty_type>(registry.create());
    registry.assign<empty_type>(registry.create());

    ASSERT_LT(registry.data<empty_type>()[0], registry.data<empty_type>()[1]);
    ASSERT_LT(registry.data<empty_type>()[1], registry.data<empty_type>()[2]);

    registry.sort<empty_type>(std::less<entt::entity>{});

    ASSERT_GT(registry.data<empty_type>()[0], registry.data<empty_type>()[1]);
    ASSERT_GT(registry.data<empty_type>()[1], registry.data<empty_type>()[2]);
}

TEST(Registry, ComponentsWithTypesFromStandardTemplateLibrary) {
    // see #37 - the test shouldn't crash, that's all
    entt::registry registry;
    const auto entity = registry.create();
    registry.assign<std::unordered_set<int>>(entity).insert(42);
    registry.destroy(entity);
}

TEST(Registry, ConstructWithComponents) {
    // it should compile, that's all
    entt::registry registry;
    const auto value = 0;
    registry.assign<int>(registry.create(), value);
}

TEST(Registry, Signals) {
    entt::registry registry;
    listener listener;

    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_destroy<empty_type>().connect<&listener::decr<empty_type>>(listener);
    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.on_destroy<int>().connect<&listener::decr<int>>(listener);

    auto e0 = registry.create();
    auto e1 = registry.create();

    registry.assign<empty_type>(e0);
    registry.assign<empty_type>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.assign<int>(e1);
    registry.assign<int>(e0);

    ASSERT_EQ(listener.counter, 4);
    ASSERT_EQ(listener.last, e0);

    registry.remove<empty_type>(e0);
    registry.remove<int>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_destroy<empty_type>().disconnect<&listener::decr<empty_type>>(listener);
    registry.on_destroy<int>().disconnect<&listener::decr<int>>(listener);

    registry.remove<empty_type>(e1);
    registry.remove<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_construct<empty_type>().disconnect<&listener::incr<empty_type>>(listener);
    registry.on_construct<int>().disconnect<&listener::incr<int>>(listener);

    registry.assign<empty_type>(e1);
    registry.assign<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.on_destroy<int>().connect<&listener::decr<int>>(listener);

    registry.assign<int>(e0);
    registry.remove_if_exists<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_destroy<empty_type>().connect<&listener::decr<empty_type>>(listener);

    registry.remove_if_exists<empty_type>(e1);
    registry.assign<empty_type>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.clear<empty_type>();
    registry.clear<int>();

    ASSERT_EQ(listener.counter, 0);
    ASSERT_EQ(listener.last, e0);

    registry.assign<empty_type>(e0);
    registry.assign<empty_type>(e1);
    registry.assign<int>(e0);
    registry.assign<int>(e1);

    registry.destroy(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.remove<int>(e0);
    registry.remove<empty_type>(e0);
    registry.assign_or_replace<int>(e0);
    registry.assign_or_replace<empty_type>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_destroy<empty_type>().disconnect<&listener::decr<empty_type>>(listener);
    registry.on_destroy<int>().disconnect<&listener::decr<int>>(listener);

    registry.assign_or_replace<empty_type>(e0);
    registry.assign_or_replace<int>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_replace<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_replace<int>().connect<&listener::incr<int>>(listener);

    registry.assign_or_replace<empty_type>(e0);
    registry.assign_or_replace<int>(e0);

    ASSERT_EQ(listener.counter, 4);
    ASSERT_EQ(listener.last, e0);

    registry.replace<empty_type>(e0);
    registry.replace<int>(e0);

    ASSERT_EQ(listener.counter, 6);
    ASSERT_EQ(listener.last, e0);
}

TEST(Registry, RangeDestroy) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<int>(e0);
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.assign<int>(e2);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));

    {
        const auto view = registry.view<int, char>();
        registry.destroy(view.begin(), view.end());
    }

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));

    {
        const auto view = registry.view<int>();
        registry.destroy(view.begin(), view.end());
    }

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
}

TEST(Registry, RangeAssign) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<int>(e0);
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.assign<int>(e2);

    ASSERT_FALSE(registry.has<float>(e0));
    ASSERT_FALSE(registry.has<float>(e1));
    ASSERT_FALSE(registry.has<float>(e2));

    const auto view = registry.view<int, char>();
    registry.assign(view.begin(), view.end(), 3.f);

    ASSERT_EQ(registry.get<float>(e0), 3.f);
    ASSERT_EQ(registry.get<float>(e1), 3.f);
    ASSERT_FALSE(registry.has<float>(e2));

    registry.clear<float>();
    float value[3]{0.f, 1.f, 2.f};
    registry.assign<float>(registry.data<int>(), registry.data<int>() + registry.size<int>(), value);

    ASSERT_EQ(registry.get<float>(e0), 0.f);
    ASSERT_EQ(registry.get<float>(e1), 1.f);
    ASSERT_EQ(registry.get<float>(e2), 2.f);
}

TEST(Registry, RangeRemove) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.assign<int>(e0);
    registry.assign<char>(e0);
    registry.assign<double>(e0);

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.assign<int>(e2);

    ASSERT_TRUE(registry.has<int>(e0));
    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<int>(e2));

    const auto view = registry.view<int, char>();
    registry.remove<int>(view.begin(), view.end());

    ASSERT_FALSE(registry.has<int>(e0));
    ASSERT_FALSE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<int>(e2));
}

TEST(Registry, NonOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    const auto group = registry.group<>(entt::get<int, char>);

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    const auto group = registry.group<int, char>();

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, PartialOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    const auto group = registry.group<int>(entt::get<char>);

    entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, NonOwningGroupSortInterleaved) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, '0');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1, '1');

    registry.sort<int>(std::greater{});
    registry.sort<char>(std::less{});

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);
    registry.assign<char>(e2, '2');

    group.each([e0, e1, e2](const auto entity, const auto &i, const auto &c) {
        if(entity == e0) {
            ASSERT_EQ(i, 0);
            ASSERT_EQ(c, '0');
        } else if(entity == e1) {
            ASSERT_EQ(i, 1);
            ASSERT_EQ(c, '1');
        } else if(entity == e2) {
            ASSERT_EQ(i, 2);
            ASSERT_EQ(c, '2');
        }
    });
}

TEST(Registry, GetOrAssign) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto value = registry.get_or_assign<int>(entity, 3);
    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), value);
    ASSERT_EQ(registry.get<int>(entity), 3);
}

TEST(Registry, Constness) {
    entt::registry registry;

    ASSERT_TRUE((std::is_same_v<decltype(registry.assign<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.assign<empty_type>({})), empty_type>));

    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int, char>({})), std::tuple<int &, char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int>({})), int *>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int, char>({})), std::tuple<int *, char *>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int>({})), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int, char>({})), std::tuple<const int &, const char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int>({})), const int *>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int, char>({})), std::tuple<const int *, const char *>>));
}

TEST(Registry, BatchCreateAmbiguousCall) {
    struct ambiguous { std::uint32_t foo; std::uint64_t bar; };
    entt::registry registry;
    const auto entity = registry.create();
    std::uint32_t foo = 32u;
    std::uint64_t bar = 64u;
    // this should work, no other tests required
    registry.assign<ambiguous>(entity, foo, bar);
}

TEST(Registry, MoveOnlyComponent) {
    entt::registry registry;
    // the purpose is to ensure that move only types are always accepted
    registry.assign<std::unique_ptr<int>>(registry.create());
}

TEST(Registry, NonDefaultConstructibleComponent) {
    entt::registry registry;
    // the purpose is to ensure that non default constructible type are always accepted
    registry.assign<non_default_constructible>(registry.create(), 42);
}

TEST(Registry, Dependencies) {
    entt::registry registry;
    const auto entity = registry.create();

    // required because of an issue of VS2019
    constexpr auto assign_or_replace = &entt::registry::assign_or_replace<double>;
    constexpr auto remove = &entt::registry::remove<double>;

    registry.on_construct<int>().connect<assign_or_replace>();
    registry.on_destroy<int>().connect<remove>();
    registry.assign<double>(entity, .3);

    ASSERT_FALSE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);

    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);

    registry.remove<int>(entity);

    ASSERT_FALSE((registry.any<int, double>(entity)));

    registry.on_construct<int>().disconnect<assign_or_replace>();
    registry.on_destroy<int>().disconnect<remove>();
    registry.assign<int>(entity);

    ASSERT_TRUE((registry.any<int, double>(entity)));
    ASSERT_FALSE(registry.has<double>(entity));
}

TEST(Registry, StableAssign) {
    entt::registry registry;
    registry.on_construct<int>().connect<&listener::sort<int>>();
    registry.assign<int>(registry.create(), 0);

    ASSERT_EQ(registry.assign<int>(registry.create(), 1), 1);
}

TEST(Registry, AssignEntities) {
    entt::registry registry;
    entt::entity entities[3];
    registry.create(std::begin(entities), std::end(entities));
    registry.destroy(entities[1]);
    registry.destroy(entities[2]);

    entt::registry other;
    other.assign(registry.data(), registry.data() + registry.size());

    ASSERT_EQ(registry.size(), other.size());
    ASSERT_TRUE(other.valid(entities[0]));
    ASSERT_FALSE(other.valid(entities[1]));
    ASSERT_FALSE(other.valid(entities[2]));
    ASSERT_EQ(registry.create(), other.create());
    ASSERT_EQ(other.entity(other.create()), entities[1]);
}

TEST(Registry, Visit) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.assign<int>(entity);
    registry.assign<double>(other);
    registry.assign<char>(entity);

    auto total = 0;
    auto esize = 0;
    auto osize = 0;

    registry.visit([&total](const auto component) {
        ASSERT_TRUE(total != 0 || component == entt::type_info<char>::id());
        ASSERT_TRUE(total != 1 || component == entt::type_info<double>::id());
        ASSERT_TRUE(total != 2 || component == entt::type_info<int>::id());
        ++total;
    });

    registry.visit(entity, [&esize](const auto component) {
        ASSERT_TRUE(esize != 0 || component == entt::type_info<char>::id());
        ASSERT_TRUE(esize != 1 || component == entt::type_info<int>::id());
        ++esize;
    });

    registry.visit(other, [&osize](const auto component) {
        ASSERT_TRUE(osize != 0 || component == entt::type_info<double>::id());
        ++osize;
    });

    ASSERT_EQ(total, 3);
    ASSERT_EQ(esize, 2);
    ASSERT_EQ(osize, 1);
}
