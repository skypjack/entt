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
    void incr(const entt::registry &, entt::entity entity) {
        last = entity;
        ++counter;
    }

    template<typename Component>
    void decr(const entt::registry &, entt::entity entity) {
        last = entity;
        --counter;
    }

    entt::entity last{entt::null};
    int counter{0};
};

struct owner {
    void receive(const entt::registry &ref) {
        parent = &ref;
    }

    const entt::registry *parent{nullptr};
};

TEST(Registry, Context) {
    entt::registry registry;

    ASSERT_EQ(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<const int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<double>(), nullptr);

    registry.set<char>();
    registry.set<int>();
    // suppress the warning due to the [[nodiscard]] attribute
    static_cast<void>(registry.ctx_or_set<double>());

    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_NE(registry.try_ctx<const int>(), nullptr);
    ASSERT_NE(registry.try_ctx<double>(), nullptr);

    registry.unset<int>();
    registry.unset<double>();

    auto count = 0;

    registry.ctx([&count](auto info) {
        ASSERT_EQ(info.hash(), entt::type_hash<char>::value());
        ++count;
    });

    ASSERT_EQ(count, 1);

    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<const int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<double>(), nullptr);

    registry.set<char>('c');
    registry.set<int>(0);
    registry.set<double>(1.);
    registry.set<int>(42);

    ASSERT_EQ(registry.ctx_or_set<char>('a'), 'c');
    ASSERT_NE(registry.try_ctx<char>(), nullptr);
    ASSERT_EQ(registry.try_ctx<char>(), &registry.ctx<char>());
    ASSERT_EQ(registry.ctx<char>(), std::as_const(registry).ctx<const char>());

    ASSERT_EQ(registry.ctx<const int>(), 42);
    ASSERT_NE(registry.try_ctx<int>(), nullptr);
    ASSERT_EQ(registry.try_ctx<const int>(), &registry.ctx<int>());
    ASSERT_EQ(registry.ctx<int>(), std::as_const(registry).ctx<const int>());

    ASSERT_EQ(registry.ctx<const double>(), 1.);
    ASSERT_NE(registry.try_ctx<double>(), nullptr);
    ASSERT_EQ(registry.try_ctx<const double>(), &registry.ctx<double>());
    ASSERT_EQ(registry.ctx<double>(), std::as_const(registry).ctx<const double>());

    ASSERT_EQ(registry.try_ctx<float>(), nullptr);
}

TEST(Registry, ContextAsRef) {
    entt::registry registry;
    int value{3};

    registry.set<int &>(value);

    ASSERT_NE(registry.try_ctx<int>(), nullptr);
    ASSERT_NE(registry.try_ctx<const int>(), nullptr);
    ASSERT_NE(std::as_const(registry).try_ctx<const int>(), nullptr);
    ASSERT_EQ(registry.ctx<const int>(), 3);
    ASSERT_EQ(registry.ctx<int>(), 3);

    registry.ctx<int>() = 42;

    ASSERT_EQ(registry.ctx<int>(), 42);
    ASSERT_EQ(value, 42);

    value = 3;

    ASSERT_EQ(std::as_const(registry).ctx<const int>(), 3);
}

TEST(Registry, ContextAsConstRef) {
    entt::registry registry;
    int value{3};

    registry.set<const int &>(value);

    ASSERT_EQ(registry.try_ctx<int>(), nullptr);
    ASSERT_NE(registry.try_ctx<const int>(), nullptr);
    ASSERT_NE(std::as_const(registry).try_ctx<const int>(), nullptr);
    ASSERT_EQ(registry.ctx<const int>(), 3);

    value = 42;

    ASSERT_EQ(std::as_const(registry).ctx<const int>(), 42);
}

TEST(Registry, Functionalities) {
    entt::registry registry;

    ASSERT_EQ(registry.size(), 0u);
    ASSERT_EQ(registry.alive(), 0u);
    ASSERT_NO_FATAL_FAILURE((registry.reserve<int, char>(8)));
    ASSERT_NO_FATAL_FAILURE(registry.reserve_pools(16));
    ASSERT_NO_FATAL_FAILURE(registry.reserve(42));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), 42u);
    ASSERT_EQ(registry.capacity<int>(), 8u);
    ASSERT_EQ(registry.capacity<char>(), 8u);
    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_TRUE((registry.empty<int, char>()));

    registry.prepare<double>();

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    ASSERT_TRUE(registry.all_of<>(e0));
    ASSERT_FALSE(registry.any_of<>(e1));

    ASSERT_EQ(registry.size<int>(), 1u);
    ASSERT_EQ(registry.size<char>(), 1u);
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e0, e1);

    ASSERT_FALSE((registry.all_of<int, char>(e0)));
    ASSERT_TRUE((registry.all_of<int, char>(e1)));
    ASSERT_FALSE((registry.any_of<int, double>(e0)));
    ASSERT_TRUE((registry.any_of<int, double>(e1)));

    ASSERT_EQ(registry.try_get<int>(e0), nullptr);
    ASSERT_NE(registry.try_get<int>(e1), nullptr);
    ASSERT_EQ(registry.try_get<char>(e0), nullptr);
    ASSERT_NE(registry.try_get<char>(e1), nullptr);
    ASSERT_EQ(registry.try_get<double>(e0), nullptr);
    ASSERT_EQ(registry.try_get<double>(e1), nullptr);

    ASSERT_EQ(registry.emplace<int>(e0, 42), 42);
    ASSERT_EQ(registry.emplace<char>(e0, 'c'), 'c');
    ASSERT_NO_FATAL_FAILURE(registry.remove<int>(e1));
    ASSERT_NO_FATAL_FAILURE(registry.remove<char>(e1));

    ASSERT_TRUE((registry.all_of<int, char>(e0)));
    ASSERT_FALSE((registry.all_of<int, char>(e1)));
    ASSERT_TRUE((registry.any_of<int, double>(e0)));
    ASSERT_FALSE((registry.any_of<int, double>(e1)));

    const auto e2 = registry.create();

    registry.emplace_or_replace<int>(e2, registry.get<int>(e0));
    registry.emplace_or_replace<char>(e2, registry.get<char>(e0));

    ASSERT_TRUE((registry.all_of<int, char>(e2)));
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

    ASSERT_EQ(registry.patch<int>(e0, [](auto &instance) { instance = 2; }), 2);
    ASSERT_EQ(registry.replace<int>(e0, 3), 3);

    ASSERT_NO_FATAL_FAILURE(registry.emplace_or_replace<int>(e0, 1));
    ASSERT_NO_FATAL_FAILURE(registry.emplace_or_replace<int>(e1, 1));
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const entt::registry &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.size(), 3u);
    ASSERT_EQ(registry.alive(), 3u);
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e2), 0u);
    ASSERT_EQ(registry.current(e2), 0u);
    ASSERT_NO_FATAL_FAILURE(registry.destroy(e2));
    ASSERT_EQ(registry.version(e2), 0u);
    ASSERT_EQ(registry.current(e2), 1u);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));

    ASSERT_EQ(registry.size(), 3u);
    ASSERT_EQ(registry.alive(), 2u);
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_FATAL_FAILURE(registry.clear());

    ASSERT_EQ(registry.size(), 3u);
    ASSERT_EQ(registry.alive(), 0u);
    ASSERT_TRUE(registry.empty());

    const auto e3 = registry.create();

    ASSERT_EQ(registry.get_or_emplace<int>(e3, 3), 3);
    ASSERT_EQ(registry.get_or_emplace<char>(e3, 'c'), 'c');

    ASSERT_EQ(registry.size<int>(), 1u);
    ASSERT_EQ(registry.size<char>(), 1u);
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_TRUE((registry.all_of<int, char>(e3)));
    ASSERT_EQ(registry.get<int>(e3), 3);
    ASSERT_EQ(registry.get<char>(e3), 'c');

    ASSERT_NO_FATAL_FAILURE(registry.clear<int>());

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 1u);
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_FATAL_FAILURE(registry.clear());

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_TRUE((registry.empty<int, char>()));

    const auto e4 = registry.create();
    const auto e5 = registry.create();

    registry.emplace<int>(e4);

    ASSERT_EQ(registry.remove_if_exists<int>(e4), 1u);
    ASSERT_EQ(registry.remove_if_exists<int>(e5), 0u);

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_TRUE(registry.empty<int>());

    ASSERT_EQ(registry.capacity<int>(), 8u);
    ASSERT_EQ(registry.capacity<char>(), 8u);

    registry.shrink_to_fit<int, char>();

    ASSERT_EQ(registry.capacity<int>(), 0u);
    ASSERT_EQ(registry.capacity<char>(), 0u);
}

TEST(Registry, Move) {
    entt::registry registry;
    const auto entity = registry.create();
    owner test{};

    registry.on_construct<int>().connect<&owner::receive>(test);
    registry.on_destroy<int>().connect<&owner::receive>(test);

    ASSERT_EQ(test.parent, nullptr);

    registry.emplace<int>(entity);

    ASSERT_EQ(test.parent, &registry);

    entt::registry other{std::move(registry)};
    other.remove<int>(entity);
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &other);

    registry = std::move(other);
    registry.emplace<int>(entity);
    registry.emplace<int>(registry.create(entity));

    ASSERT_EQ(test.parent, &registry);
}

TEST(Registry, ReplaceAggregate) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<aggregate>(entity, 0);
    auto &instance = registry.replace<aggregate>(entity, 42);

    ASSERT_EQ(instance.value, 42);
}

TEST(Registry, EmplaceOrReplaceAggregate) {
    entt::registry registry;
    const auto entity = registry.create();
    auto &instance = registry.emplace_or_replace<aggregate>(entity, 42);

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

TEST(Registry, Data) {
    entt::registry registry;

    ASSERT_EQ(std::as_const(registry).data(), nullptr);

    const auto entity = registry.create();

    ASSERT_EQ(*std::as_const(registry).data(), entity);

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
    registry.insert(std::begin(entities), std::end(entities), 42);
    registry.insert(std::begin(entities), std::end(entities), 'c');

    ASSERT_EQ(registry.get<int>(entities[0]), 42);
    ASSERT_EQ(registry.get<char>(entities[1]), 'c');
    ASSERT_EQ(listener.counter, 3);

    registry.on_construct<int>().disconnect<&listener::incr<int>>(listener);
    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.create(std::begin(entities), std::end(entities));
    registry.insert(std::begin(entities), std::end(entities), 'a');
    registry.insert<empty_type>(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.all_of<empty_type>(entities[0]));
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

    ASSERT_EQ(registry.version(e2), entt::registry::version_type{});
    ASSERT_EQ(registry.current(e2), entt::registry::version_type{1});

    e2 = registry.create();
    auto e1 = registry.create(entt::entity{2});

    ASSERT_EQ(registry.entity(e2), entt::entity{2});
    ASSERT_EQ(registry.version(e2), entt::registry::version_type{1});

    ASSERT_EQ(registry.entity(e1), entt::entity{1});
    ASSERT_EQ(registry.version(e1), entt::registry::version_type{});

    registry.destroy(e1);
    registry.destroy(e2);
    auto e0 = registry.create(entt::entity{0});

    ASSERT_EQ(e0, entt::entity{0});
    ASSERT_EQ(registry.version(e0), entt::registry::version_type{});
}

TEST(Registry, DestroyWithVersion) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_EQ(registry.current(e0), entt::registry::version_type{});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{});

    registry.destroy(e0);
    registry.destroy(e1, 3);

    ASSERT_EQ(registry.current(e0), entt::registry::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{3});
}

TEST(Registry, CreateDestroyEntities) {
    entt::registry registry;
    entt::entity pre{}, post{};

    for(int i = 0; i < 10; ++i) {
        const auto entity = registry.create();
        registry.emplace<double>(entity);
    }

    registry.clear();

    for(int i = 0; i < 7; ++i) {
        const auto entity = registry.create();
        registry.emplace<int>(entity);
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
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    const auto entity = registry.create();

    registry.destroy(entity, typename traits_type::version_type(traits_type::version_mask));
    registry.destroy(registry.create());

    ASSERT_EQ(registry.current(entity), registry.version(entity));
    ASSERT_EQ(registry.current(entity), typename traits_type::version_type{});
}

TEST(Registry, Each) {
    entt::registry registry;
    entt::registry::size_type tot;
    entt::registry::size_type match;

    registry.create();
    registry.emplace<int>(registry.create());
    registry.create();
    registry.emplace<int>(registry.create());
    registry.create();

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.all_of<int>(entity)) { ++match; }
        registry.create();
        ++tot;
    });

    ASSERT_EQ(tot, 5u);
    ASSERT_EQ(match, 2u);

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.all_of<int>(entity)) {
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
        if(registry.all_of<int>(entity)) { ++match; }
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

    registry.emplace<int>(registry.create());
    registry.create();
    registry.emplace<int>(registry.create());

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 1u);
    tot = {};

    registry.each([&](auto entity) { registry.remove_all(entity); });
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
    registry.emplace<int>(e0, 0);
    registry.emplace<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 0);

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 0);
    registry.emplace<char>(e2, 'c');

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
        (registry.emplace<decltype(component)>(entity, component), ...);
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

TEST(Registry, NonOwningGroupInitOnEmplace) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.emplace<decltype(component)>(entity, component), ...);
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
        (registry.emplace<decltype(component)>(entity, component), ...);
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

TEST(Registry, FullOwningGroupInitOnEmplace) {
    entt::registry registry;
    auto group = registry.group<int, char>();
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.emplace<decltype(component)>(entity, component), ...);
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
        (registry.emplace<decltype(component)>(entity, component), ...);
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

TEST(Registry, PartialOwningGroupInitOnEmplace) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);
    auto create = [&](auto... component) {
        const auto entity = registry.create();
        (registry.emplace<decltype(component)>(entity, component), ...);
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
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.remove<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.clear<int>();

    ASSERT_EQ(view.size_hint(), 0u);

    registry.emplace<int>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.clear();

    ASSERT_EQ(view.size_hint(), 0u);
}

TEST(Registry, CleanNonOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

TEST(Registry, CleanFullOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

TEST(Registry, CleanPartialOwningGroupViewAfterRemoveAndClear) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.remove<char>(entity);

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.clear<int>();

    ASSERT_EQ(group.size(), 0u);

    registry.emplace<int>(entity, 0);

    ASSERT_EQ(group.size(), 1u);

    registry.clear();

    ASSERT_EQ(group.size(), 0u);
}

TEST(Registry, NestedGroups) {
    entt::registry registry;
    entt::entity entities[10];

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<int>(std::begin(entities), std::end(entities));
    registry.insert<char>(std::begin(entities), std::end(entities));
    const auto g1 = registry.group<int>(entt::get<char>, entt::exclude<double>);

    ASSERT_TRUE(registry.sortable(g1));
    ASSERT_EQ(g1.size(), 10u);

    const auto g2 = registry.group<int>(entt::get<char>);

    ASSERT_TRUE(registry.sortable(g1));
    ASSERT_FALSE(registry.sortable(g2));
    ASSERT_EQ(g1.size(), 10u);
    ASSERT_EQ(g2.size(), 10u);

    for(auto i = 0u; i < 5u; ++i) {
        ASSERT_TRUE(g1.contains(entities[i*2+1]));
        ASSERT_TRUE(g1.contains(entities[i*2]));
        ASSERT_TRUE(g2.contains(entities[i*2+1]));
        ASSERT_TRUE(g2.contains(entities[i*2]));
        registry.emplace<double>(entities[i*2]);
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

    ASSERT_FALSE(registry.sortable(g1));
    ASSERT_FALSE(registry.sortable(g2));
    ASSERT_TRUE(registry.sortable(g3));

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
        registry.emplace<int>(entities[i*2+1]);
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
        registry.emplace<float>(entities[i*2]);
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

    registry.emplace<int>(registry.create(), val++);
    registry.emplace<int>(registry.create(), val++);
    registry.emplace<int>(registry.create(), val++);

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
        registry.emplace<unsigned int>(entity, uval++);
        registry.emplace<int>(entity, ival++);
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

    registry.emplace<empty_type>(registry.create());
    registry.emplace<empty_type>(registry.create());
    registry.emplace<empty_type>(registry.create());

    ASSERT_LT(registry.view<empty_type>().data()[0], registry.view<empty_type>().data()[1]);
    ASSERT_LT(registry.view<empty_type>().data()[1], registry.view<empty_type>().data()[2]);

    registry.sort<empty_type>(std::less<entt::entity>{});

    ASSERT_GT(registry.view<empty_type>().data()[0], registry.view<empty_type>().data()[1]);
    ASSERT_GT(registry.view<empty_type>().data()[1], registry.view<empty_type>().data()[2]);
}

TEST(Registry, ComponentsWithTypesFromStandardTemplateLibrary) {
    // see #37 - the test shouldn't crash, that's all
    entt::registry registry;
    const auto entity = registry.create();
    registry.emplace<std::unordered_set<int>>(entity).insert(42);
    registry.destroy(entity);
}

TEST(Registry, ConstructWithComponents) {
    // it should compile, that's all
    entt::registry registry;
    const auto value = 0;
    registry.emplace<int>(registry.create(), value);
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

    registry.emplace<empty_type>(e0);
    registry.emplace<empty_type>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.emplace<int>(e1);
    registry.emplace<int>(e0);

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

    registry.emplace<empty_type>(e1);
    registry.emplace<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.on_destroy<int>().connect<&listener::decr<int>>(listener);

    registry.emplace<int>(e0);
    registry.remove_if_exists<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_destroy<empty_type>().connect<&listener::decr<empty_type>>(listener);

    registry.remove_if_exists<empty_type>(e1);
    registry.emplace<empty_type>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.clear<empty_type>();
    registry.clear<int>();

    ASSERT_EQ(listener.counter, 0);
    ASSERT_EQ(listener.last, e0);

    registry.emplace<empty_type>(e0);
    registry.emplace<empty_type>(e1);
    registry.emplace<int>(e0);
    registry.emplace<int>(e1);

    registry.destroy(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.remove<int>(e0);
    registry.remove<empty_type>(e0);
    registry.emplace_or_replace<int>(e0);
    registry.emplace_or_replace<empty_type>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_destroy<empty_type>().disconnect<&listener::decr<empty_type>>(listener);
    registry.on_destroy<int>().disconnect<&listener::decr<int>>(listener);

    registry.emplace_or_replace<empty_type>(e0);
    registry.emplace_or_replace<int>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.on_update<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_update<int>().connect<&listener::incr<int>>(listener);

    registry.emplace_or_replace<empty_type>(e0);
    registry.emplace_or_replace<int>(e0);

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

    registry.emplace<int>(e0);
    registry.emplace<char>(e0);
    registry.emplace<double>(e0);

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.emplace<int>(e2);

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

TEST(Registry, Insert) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<int>(e0);
    registry.emplace<char>(e0);
    registry.emplace<double>(e0);

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.emplace<int>(e2);

    ASSERT_FALSE(registry.all_of<float>(e0));
    ASSERT_FALSE(registry.all_of<float>(e1));
    ASSERT_FALSE(registry.all_of<float>(e2));

    const auto icview = registry.view<int, char>();
    registry.insert(icview.begin(), icview.end(), 3.f);

    ASSERT_EQ(registry.get<float>(e0), 3.f);
    ASSERT_EQ(registry.get<float>(e1), 3.f);
    ASSERT_FALSE(registry.all_of<float>(e2));

    registry.clear<float>();
    float value[3]{0.f, 1.f, 2.f};

    const auto iview = registry.view<int>();
    registry.insert<float>(iview.data(), iview.data() + iview.size(), value, value + iview.size());

    ASSERT_EQ(registry.get<float>(e0), 0.f);
    ASSERT_EQ(registry.get<float>(e1), 1.f);
    ASSERT_EQ(registry.get<float>(e2), 2.f);
}

TEST(Registry, Remove) {
    entt::registry registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    registry.emplace<int>(e0);
    registry.emplace<char>(e0);
    registry.emplace<double>(e0);

    registry.emplace<int>(e1);
    registry.emplace<char>(e1);

    registry.emplace<int>(e2);

    ASSERT_TRUE(registry.all_of<int>(e0));
    ASSERT_TRUE(registry.all_of<int>(e1));
    ASSERT_TRUE(registry.all_of<int>(e2));

    const auto view = registry.view<int, char>();
    registry.remove<int>(view.begin(), view.end());

    ASSERT_FALSE(registry.all_of<int>(e0));
    ASSERT_FALSE(registry.all_of<int>(e1));
    ASSERT_TRUE(registry.all_of<int>(e2));
}

TEST(Registry, NonOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto group = registry.group<>(entt::get<int, char>);

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, FullOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto group = registry.group<int, char>();

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, PartialOwningGroupInterleaved) {
    entt::registry registry;
    typename entt::entity entity = entt::null;

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    const auto group = registry.group<int>(entt::get<char>);

    entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    std::size_t cnt{};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(Registry, NonOwningGroupSortInterleaved) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 0);
    registry.emplace<char>(e0, '0');

    const auto e1 = registry.create();
    registry.emplace<int>(e1, 1);
    registry.emplace<char>(e1, '1');

    registry.sort<int>(std::greater{});
    registry.sort<char>(std::less{});

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 2);
    registry.emplace<char>(e2, '2');

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

TEST(Registry, GetOrEmplace) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto value = registry.get_or_emplace<int>(entity, 3);
    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), value);
    ASSERT_EQ(registry.get<int>(entity), 3);
}

TEST(Registry, Constness) {
    entt::registry registry;

    ASSERT_TRUE((std::is_same_v<decltype(registry.emplace<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.emplace<empty_type>({})), void>));

    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int, const char>({})), std::tuple<int &, const char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int>({})), int *>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int, const char>({})), std::tuple<int *, const char *>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int>({})), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int, const char>({})), std::tuple<const int &, const char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int>({})), const int *>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int, const char>({})), std::tuple<const int *, const char *>>));
}

TEST(Registry, MoveOnlyComponent) {
    entt::registry registry;
    // the purpose is to ensure that move only types are always accepted
    registry.emplace<std::unique_ptr<int>>(registry.create());
}

TEST(Registry, NonDefaultConstructibleComponent) {
    entt::registry registry;
    // the purpose is to ensure that non default constructible type are always accepted
    registry.emplace<non_default_constructible>(registry.create(), 42);
}

TEST(Registry, Dependencies) {
    entt::registry registry;
    const auto entity = registry.create();

    // required because of an issue of VS2019
    constexpr auto emplace_or_replace = &entt::registry::emplace_or_replace<double>;
    constexpr auto remove = &entt::registry::remove<double>;

    registry.on_construct<int>().connect<emplace_or_replace>();
    registry.on_destroy<int>().connect<remove>();
    registry.emplace<double>(entity, .3);

    ASSERT_FALSE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);

    registry.emplace<int>(entity);

    ASSERT_TRUE(registry.all_of<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);

    registry.remove<int>(entity);

    ASSERT_FALSE((registry.any_of<int, double>(entity)));

    registry.on_construct<int>().disconnect<emplace_or_replace>();
    registry.on_destroy<int>().disconnect<remove>();
    registry.emplace<int>(entity);

    ASSERT_TRUE((registry.any_of<int, double>(entity)));
    ASSERT_FALSE(registry.all_of<double>(entity));
}

TEST(Registry, StableEmplace) {
    entt::registry registry;
    registry.on_construct<int>().connect<&listener::sort<int>>();
    registry.emplace<int>(registry.create(), 0);

    ASSERT_EQ(registry.emplace<int>(registry.create(), 1), 1);
}

TEST(Registry, AssignEntities) {
    entt::registry registry;
    entt::entity entities[3];
    registry.create(std::begin(entities), std::end(entities));
    registry.destroy(entities[1]);
    registry.destroy(entities[2]);

    entt::registry other;
    const auto *data = registry.data();
    other.assign(data, data + registry.size(), registry.destroyed());

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

    registry.emplace<int>(entity);
    registry.emplace<double>(other);
    registry.emplace<char>(entity);

    bool hasType[3]{};

    registry.visit([&hasType](auto info) {
        hasType[0] = hasType[0] || (info.hash() == entt::type_hash<int>::value());
        hasType[1] = hasType[1] || (info.hash() == entt::type_hash<double>::value());
        hasType[2] = hasType[2] || (info.hash() == entt::type_hash<char>::value());
    });

    ASSERT_TRUE(hasType[0] && hasType[1] && hasType[2]);

    hasType[0] = hasType[1] = hasType[2] = false;

    registry.visit(entity, [&hasType](auto info) {
        hasType[0] = hasType[0] || (info.hash() == entt::type_hash<int>::value());
        hasType[1] = hasType[1] || (info.hash() == entt::type_hash<double>::value());
        hasType[2] = hasType[2] || (info.hash() == entt::type_hash<char>::value());
    });

    ASSERT_TRUE(hasType[0] && !hasType[1] && hasType[2]);

    hasType[0] = hasType[2] = false;

    registry.visit(other, [&hasType](auto info) {
        hasType[0] = hasType[0] || (info.hash() == entt::type_hash<int>::value());
        hasType[1] = hasType[1] || (info.hash() == entt::type_hash<double>::value());
        hasType[2] = hasType[2] || (info.hash() == entt::type_hash<char>::value());
    });

    ASSERT_TRUE(!hasType[0] && hasType[1] && !hasType[2]);

    hasType[1] = false;
}

TEST(Registry, ScramblingPoolsIsAllowed) {
    entt::registry registry;
    registry.on_destroy<int>().connect<&listener::sort<int>>();

    for(std::size_t i{}; i < 2u; ++i) {
        const auto entity = registry.create();
        registry.emplace<int>(entity, static_cast<int>(i));
    }

    registry.destroy(registry.view<int>().back());

    // thanks to @andranik3949 for pointing out this missing test
    registry.view<const int>().each([](const auto entity, const auto &value) {
        ASSERT_EQ(entt::to_integral(entity), value);
    });
}
