#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iterator>
#include <memory>
#include <cstdint>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>

ENTT_NAMED_TYPE(int)

struct empty_type {};

struct listener {
    template<typename Component>
    void incr(entt::entity entity, entt::registry &registry, const Component &) {
        ASSERT_TRUE(registry.valid(entity));
        ASSERT_TRUE(registry.has<Component>(entity));
        last = entity;
        ++counter;
    }

    template<typename Component>
    void decr(entt::entity entity, entt::registry &registry) {
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

TEST(Registry, Types) {
    entt::registry registry;
    ASSERT_EQ(registry.type<int>(), registry.type<int>());
    ASSERT_NE(registry.type<double>(), registry.type<int>());
}

TEST(Registry, Functionalities) {
    entt::registry registry;

    ASSERT_EQ(registry.size(), entt::registry::size_type{0});
    ASSERT_EQ(registry.alive(), entt::registry::size_type{0});
    ASSERT_NO_THROW(registry.reserve(42));
    ASSERT_NO_THROW(registry.reserve<int>(8));
    ASSERT_NO_THROW(registry.reserve<char>(8));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), entt::registry::size_type{42});
    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_TRUE(registry.has<>(e0));
    ASSERT_TRUE(registry.has<>(e1));

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e0, e1);

    ASSERT_FALSE(registry.has<int>(e0));
    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_FALSE(registry.has<char>(e0));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE((registry.has<int, char>(e0)));
    ASSERT_TRUE((registry.has<int, char>(e1)));

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

    ASSERT_TRUE(registry.has<int>(e0));
    ASSERT_FALSE(registry.has<int>(e1));
    ASSERT_TRUE(registry.has<char>(e0));
    ASSERT_FALSE(registry.has<char>(e1));
    ASSERT_TRUE((registry.has<int, char>(e0)));
    ASSERT_FALSE((registry.has<int, char>(e1)));

    const auto e2 = registry.create();

    registry.assign_or_replace<int>(e2, registry.get<int>(e0));
    registry.assign_or_replace<char>(e2, registry.get<char>(e0));

    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e2));
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

    ASSERT_NO_THROW(registry.replace<int>(e0, 0));
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

    ASSERT_NO_THROW(registry.reset());

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
    ASSERT_TRUE(registry.has<int>(e3));
    ASSERT_TRUE(registry.has<char>(e3));
    ASSERT_EQ(registry.get<int>(e3), 3);
    ASSERT_EQ(registry.get<char>(e3), 'c');

    ASSERT_NO_THROW(registry.reset<int>());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    const auto e4 = registry.create();
    const auto e5 = registry.create();

    registry.assign<int>(e4);

    ASSERT_NO_THROW(registry.reset<int>(e4));
    ASSERT_NO_THROW(registry.reset<int>(e5));

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());

    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{8});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{8});

    registry.shrink_to_fit<int>();
    registry.shrink_to_fit<char>();

    ASSERT_EQ(registry.capacity<int>(), entt::registry::size_type{});
    ASSERT_EQ(registry.capacity<char>(), entt::registry::size_type{});
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
    const auto entity = registry.create();

    ASSERT_EQ(registry.raw<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).raw<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).data<int>(), nullptr);

    registry.assign<int>(entity, 42);

    ASSERT_NE(registry.raw<int>(), nullptr);
    ASSERT_NE(std::as_const(registry).raw<int>(), nullptr);
    ASSERT_NE(std::as_const(registry).data<int>(), nullptr);

    ASSERT_EQ(*registry.raw<int>(), 42);
    ASSERT_EQ(*std::as_const(registry).raw<int>(), 42);
    ASSERT_EQ(*std::as_const(registry).data<int>(), entity);
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

    registry.each([&](auto entity) { registry.reset<int>(entity); });
    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 3u);
    registry.reset();
    tot = {};

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 0u);
}

TEST(Registry, CreateDestroyEntities) {
    entt::registry registry;
    entt::entity pre{}, post{};

    for(int i = 0; i < 10; ++i) {
        const auto entity = registry.create();
        registry.assign<double>(entity);
    }

    registry.reset();

    for(int i = 0; i < 7; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity);
        if(i == 3) { pre = entity; }
    }

    registry.reset();

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

    ASSERT_EQ(iview.size(), decltype(iview)::size_type{3});
    ASSERT_EQ(cview.size(), decltype(cview)::size_type{2});

    decltype(mview)::size_type cnt{0};
    mview.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(mview)::size_type{2});
}

TEST(Registry, NonOwningGroupInitOnFirstUse) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());

    auto group = registry.group<>(entt::get<int, char>);
    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});
}

TEST(Registry, NonOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});
}

TEST(Registry, FullOwningGroupInitOnFirstUse) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());

    auto group = registry.group<int, char>();
    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_TRUE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});
}

TEST(Registry, FullOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<int, char>();

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_TRUE(registry.owned<char>());

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_TRUE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});
}

TEST(Registry, PartialOwningGroupInitOnFirstUse) {
    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);
    registry.assign<char>(e2, 'c');

    ASSERT_FALSE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());

    auto group = registry.group<int>(entt::get<char>);
    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});

}

TEST(Registry, PartialOwningGroupInitOnAssign) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<char>(e0, 'c');

    const auto e1 = registry.create();
    registry.assign<int>(e1, 0);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 0);
    registry.assign<char>(e2, 'c');

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_TRUE(registry.owned<int>());
    ASSERT_FALSE(registry.owned<char>());
    ASSERT_EQ(cnt, decltype(group)::size_type{2});
}

TEST(Registry, CleanViewAfterReset) {
    entt::registry registry;
    auto view = registry.view<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(view.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(view.size(), entt::registry::size_type{1});

    registry.reset();

    ASSERT_EQ(view.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanNonOwningGroupViewAfterReset) {
    entt::registry registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanFullOwningGroupViewAfterReset) {
    entt::registry registry;
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
}

TEST(Registry, CleanPartialOwningGroupViewAfterReset) {
    entt::registry registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry::size_type{0});
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

TEST(Registry, SortOwned) {
    entt::registry registry;
    registry.group<int>(entt::get<char>);

    for(auto i = 0; i < 5; ++i) {
        const auto entity = registry.create();
        registry.assign<int>(entity, i);

        if(i < 2) {
            registry.assign<char>(entity);
        }
    }

    ASSERT_TRUE((registry.has<int, char>(*(registry.data<int>()+0))));
    ASSERT_TRUE((registry.has<int, char>(*(registry.data<int>()+1))));

    ASSERT_EQ(*(registry.raw<int>()+0), 0);
    ASSERT_EQ(*(registry.raw<int>()+1), 1);
    ASSERT_EQ(*(registry.raw<int>()+2), 2);
    ASSERT_EQ(*(registry.raw<int>()+3), 3);
    ASSERT_EQ(*(registry.raw<int>()+4), 4);

    registry.sort<int>([](const int lhs, const int rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(registry.raw<int>()+0), 0);
    ASSERT_EQ(*(registry.raw<int>()+1), 1);
    ASSERT_EQ(*(registry.raw<int>()+2), 4);
    ASSERT_EQ(*(registry.raw<int>()+3), 3);
    ASSERT_EQ(*(registry.raw<int>()+4), 2);

    registry.reset<char>();
    registry.sort<int>([](const int lhs, const int rhs) {
        return lhs < rhs;
    });

    ASSERT_EQ(*(registry.raw<int>()+0), 4);
    ASSERT_EQ(*(registry.raw<int>()+1), 3);
    ASSERT_EQ(*(registry.raw<int>()+2), 2);
    ASSERT_EQ(*(registry.raw<int>()+3), 1);
    ASSERT_EQ(*(registry.raw<int>()+4), 0);

    registry.each([&registry](const auto entity) {
        registry.assign<char>(entity);
    });

    registry.sort<int>([](const int lhs, const int rhs) {
        return lhs > rhs;
    });

    ASSERT_EQ(*(registry.raw<int>()+0), 4);
    ASSERT_EQ(*(registry.raw<int>()+1), 3);
    ASSERT_EQ(*(registry.raw<int>()+2), 2);
    ASSERT_EQ(*(registry.raw<int>()+3), 1);
    ASSERT_EQ(*(registry.raw<int>()+4), 0);

    registry.reset<char>();
    registry.sort<int>([](const int lhs, const int rhs) {
        return lhs > rhs;
    });

    ASSERT_EQ(*(registry.raw<int>()+0), 0);
    ASSERT_EQ(*(registry.raw<int>()+1), 1);
    ASSERT_EQ(*(registry.raw<int>()+2), 2);
    ASSERT_EQ(*(registry.raw<int>()+3), 3);
    ASSERT_EQ(*(registry.raw<int>()+4), 4);
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

TEST(Registry, MergeTwoRegistries) {
    entt::registry src;
    entt::registry dst;

    std::unordered_map<entt::entity, entt::entity> ref;

    auto merge = [&ref, &dst](const auto &view) {
        view.each([&](auto entity, const auto &component) {
            if(ref.find(entity) == ref.cend()) {
                const auto other = dst.create();
                dst.template assign<std::decay_t<decltype(component)>>(other, component);
                ref.emplace(entity, other);
            } else {
                using component_type = std::decay_t<decltype(component)>;
                dst.template assign<component_type>(ref[entity], component);
            }
        });
    };

    auto e0 = src.create();
    src.assign<int>(e0);
    src.assign<float>(e0);
    src.assign<double>(e0);

    auto e1 = src.create();
    src.assign<char>(e1);
    src.assign<float>(e1);
    src.assign<int>(e1);

    auto e2 = dst.create();
    dst.assign<int>(e2);
    dst.assign<char>(e2);
    dst.assign<double>(e2);

    auto e3 = dst.create();
    dst.assign<float>(e3);
    dst.assign<int>(e3);

    auto eq = [](auto begin, auto end) { ASSERT_EQ(begin, end); };
    auto ne = [](auto begin, auto end) { ASSERT_NE(begin, end); };

    eq(dst.view<int, float, double>().begin(), dst.view<int, float, double>().end());
    eq(dst.view<char, float, int>().begin(), dst.view<char, float, int>().end());

    merge(src.view<int>());
    merge(src.view<char>());
    merge(src.view<double>());
    merge(src.view<float>());

    ne(dst.view<int, float, double>().begin(), dst.view<int, float, double>().end());
    ne(dst.view<char, float, int>().begin(), dst.view<char, float, int>().end());
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
    registry.reset<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.on_destroy<empty_type>().connect<&listener::decr<empty_type>>(listener);

    registry.reset<empty_type>(e1);
    registry.assign<empty_type>(e0);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e0);

    registry.reset<empty_type>();
    registry.reset<int>();

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

TEST(Registry, DestroyByComponents) {
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

TEST(Registry, CreateAnEntityWithComponents) {
    entt::registry registry;
    auto &&[entity, ivalue, cvalue, evalue] = registry.create<int, char, empty_type>();
    // suppress warnings
    (void)evalue;

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_FALSE(registry.empty<empty_type>());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{1});
    ASSERT_EQ(registry.size<empty_type>(), entt::registry::size_type{1});

    ASSERT_TRUE((registry.has<int, char, empty_type>(entity)));

    ivalue = 42;
    cvalue = 'c';

    ASSERT_EQ(registry.get<int>(entity), 42);
    ASSERT_EQ(registry.get<char>(entity), 'c');
}

TEST(Registry, CreateManyEntitiesWithComponentsAtOnce) {
    entt::registry registry;
    entt::entity entities[3];

    const auto entity = registry.create();
    registry.destroy(registry.create());
    registry.destroy(entity);
    registry.destroy(registry.create());

    const auto [iptr, cptr, eptr] = registry.create<int, char, empty_type>(std::begin(entities), std::end(entities));

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_FALSE(registry.empty<empty_type>());

    ASSERT_EQ(registry.size<int>(), entt::registry::size_type{3});
    ASSERT_EQ(registry.size<char>(), entt::registry::size_type{3});
    ASSERT_EQ(registry.size<empty_type>(), entt::registry::size_type{3});

    ASSERT_TRUE(registry.valid(entities[0]));
    ASSERT_TRUE(registry.valid(entities[1]));
    ASSERT_TRUE(registry.valid(entities[2]));

    ASSERT_EQ(registry.entity(entities[0]), entt::entity{0});
    ASSERT_EQ(registry.version(entities[0]), entt::registry::version_type{2});

    ASSERT_EQ(registry.entity(entities[1]), entt::entity{1});
    ASSERT_EQ(registry.version(entities[1]), entt::registry::version_type{1});

    ASSERT_EQ(registry.entity(entities[2]), entt::entity{2});
    ASSERT_EQ(registry.version(entities[2]), entt::registry::version_type{0});

    ASSERT_TRUE((registry.has<int, char, empty_type>(entities[0])));
    ASSERT_TRUE((registry.has<int, char, empty_type>(entities[1])));
    ASSERT_TRUE((registry.has<int, char, empty_type>(entities[2])));

    for(auto i = 0; i < 3; ++i) {
        iptr[i] = i;
        cptr[i] = char('a'+i);
    }

    for(auto i = 0; i < 3; ++i) {
        ASSERT_EQ(registry.get<int>(entities[i]), i);
        ASSERT_EQ(registry.get<char>(entities[i]), char('a'+i));
    }
}

TEST(Registry, CreateManyEntitiesWithComponentsAtOnceWithListener) {
    entt::registry registry;
    entt::entity entities[3];
    listener listener;

    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.create<int, char>(std::begin(entities), std::end(entities));

    ASSERT_EQ(listener.counter, 3);

    registry.on_construct<int>().disconnect<&listener::incr<int>>(listener);
    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.create<char, empty_type>(std::begin(entities), std::end(entities));

    ASSERT_EQ(listener.counter, 6);
}

TEST(Registry, CreateFromPrototype) {
    entt::registry registry;

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');

    const auto full = registry.create(prototype, registry);

    ASSERT_TRUE((registry.has<int, char>(full)));
    ASSERT_EQ(registry.get<int>(full), 3);
    ASSERT_EQ(registry.get<char>(full), 'c');

    const auto partial = registry.create<int>(prototype, registry);

    ASSERT_TRUE(registry.has<int>(partial));
    ASSERT_FALSE(registry.has<char>(partial));
    ASSERT_EQ(registry.get<int>(partial), 3);

    const auto exclude = registry.create(prototype, registry, entt::exclude<int>);

    ASSERT_FALSE(registry.has<int>(exclude));
    ASSERT_TRUE(registry.has<char>(exclude));
    ASSERT_EQ(registry.get<char>(exclude), 'c');
}

TEST(Registry, CreateManyFromPrototype) {
    entt::registry registry;
    entt::entity entities[2];

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');

    registry.create(std::begin(entities), std::end(entities), prototype, registry);

    ASSERT_TRUE((registry.has<int, char>(entities[0])));
    ASSERT_TRUE((registry.has<int, char>(entities[1])));
    ASSERT_EQ(registry.get<int>(entities[0]), 3);
    ASSERT_EQ(registry.get<char>(entities[1]), 'c');

    registry.create<int>(std::begin(entities), std::end(entities), prototype, registry);

    ASSERT_TRUE(registry.has<int>(entities[0]));
    ASSERT_FALSE(registry.has<char>(entities[1]));
    ASSERT_EQ(registry.get<int>(entities[0]), 3);

    registry.create(std::begin(entities), std::end(entities), prototype, registry, entt::exclude<int>);

    ASSERT_FALSE(registry.has<int>(entities[0]));
    ASSERT_TRUE(registry.has<char>(entities[1]));
    ASSERT_EQ(registry.get<char>(entities[0]), 'c');
}

TEST(Registry, CreateFromPrototypeWithListener) {
    entt::registry registry;
    entt::entity entities[3];
    listener listener;

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');
    registry.assign<empty_type>(prototype);

    registry.on_construct<int>().connect<&listener::incr<int>>(listener);
    registry.create<int, char>(std::begin(entities), std::end(entities), prototype, registry);

    ASSERT_EQ(listener.counter, 3);

    registry.on_construct<int>().disconnect<&listener::incr<int>>(listener);
    registry.on_construct<empty_type>().connect<&listener::incr<empty_type>>(listener);
    registry.create<char, empty_type>(std::begin(entities), std::end(entities), prototype, registry);

    ASSERT_EQ(listener.counter, 6);
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

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(group)::size_type{2});
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

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(group)::size_type{2});
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

    decltype(group)::size_type cnt{0};
    group.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(group)::size_type{2});
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

    registry.sort<int>([](auto lhs, auto rhs) { return lhs > rhs; });
    registry.sort<char>([](auto lhs, auto rhs) { return lhs < rhs; });

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

TEST(Registry, Clone) {
    entt::registry registry;
    entt::registry other;

    registry.destroy(registry.create());

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);
    registry.assign<double>(e0, 0.0);

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1, '1');
    registry.assign<double>(e1, 1.1);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);
    registry.assign<char>(e2, '2');

    registry.destroy(e1);

    ASSERT_EQ((other.group<int, char>().size()), entt::registry::size_type{0});

    other = registry.clone<int, char, float>();

    ASSERT_EQ((other.group<int, char>().size()), entt::registry::size_type{1});
    ASSERT_EQ(other.size(), registry.size());
    ASSERT_EQ(other.alive(), registry.alive());

    ASSERT_TRUE(other.valid(e0));
    ASSERT_FALSE(other.valid(e1));
    ASSERT_TRUE(other.valid(e2));

    ASSERT_TRUE((other.has<int>(e0)));
    ASSERT_FALSE((other.has<double>(e0)));
    ASSERT_TRUE((other.has<int, char>(e2)));

    ASSERT_EQ(other.get<int>(e0), 0);
    ASSERT_EQ(other.get<int>(e2), 2);
    ASSERT_EQ(other.get<char>(e2), '2');

    const auto e3 = other.create();

    ASSERT_NE(e1, e3);
    ASSERT_EQ(registry.entity(e1), registry.entity(e3));
    ASSERT_EQ(other.entity(e1), other.entity(e3));

    other.assign<int>(e3, 3);
    other.assign<char>(e3, '3');

    ASSERT_EQ((registry.group<int, char>().size()), entt::registry::size_type{1});
    ASSERT_EQ((other.group<int, char>().size()), entt::registry::size_type{2});

    other = registry.clone();

    ASSERT_EQ(other.size(), registry.size());
    ASSERT_EQ(other.alive(), registry.alive());

    ASSERT_TRUE(other.valid(e0));
    ASSERT_FALSE(other.valid(e1));
    ASSERT_TRUE(other.valid(e2));
    ASSERT_FALSE(other.valid(e3));

    ASSERT_TRUE((other.has<int, double>(e0)));
    ASSERT_TRUE((other.has<int, char>(e2)));

    ASSERT_EQ(other.get<int>(e0), 0);
    ASSERT_EQ(other.get<double>(e0), 0.);
    ASSERT_EQ(other.get<int>(e2), 2);
    ASSERT_EQ(other.get<char>(e2), '2');

    other = other.clone<char>();

    ASSERT_EQ(other.size(), registry.size());
    ASSERT_EQ(other.alive(), registry.alive());

    ASSERT_TRUE(other.valid(e0));
    ASSERT_FALSE(other.valid(e1));
    ASSERT_TRUE(other.valid(e2));
    ASSERT_FALSE(other.valid(e3));

    ASSERT_FALSE((other.has<int>(e0)));
    ASSERT_FALSE((other.has<double>(e0)));
    ASSERT_FALSE((other.has<int>(e2)));
    ASSERT_TRUE((other.has<char>(e2)));

    ASSERT_TRUE(other.orphan(e0));
    ASSERT_EQ(other.get<char>(e2), '2');

    // the remove erased function must be available after cloning
    other.reset();
}

TEST(Registry, CloneExclude) {
    entt::registry registry;
    entt::registry other;

    const auto entity = registry.create();
    registry.assign<int>(entity);
    registry.assign<char>(entity);

    other = registry.clone<int, char>(entt::exclude<char>);

    ASSERT_TRUE(other.has(entity));
    ASSERT_TRUE(other.has<int>(entity));
    ASSERT_FALSE(other.has<char>(entity));

    other = registry.clone(entt::exclude<int>);

    ASSERT_TRUE(other.has(entity));
    ASSERT_FALSE(other.has<int>(entity));
    ASSERT_TRUE(other.has<char>(entity));

    other = registry.clone(entt::exclude<int, char>);

    ASSERT_TRUE(other.has(entity));
    ASSERT_TRUE(other.orphan(entity));
}

TEST(Registry, CloneMoveOnlyComponent) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.assign<std::unique_ptr<int>>(entity);
    registry.assign<char>(entity);

    auto other = registry.clone();

    ASSERT_TRUE(other.valid(entity));
    ASSERT_TRUE(other.has<char>(entity));
    ASSERT_FALSE(other.has<std::unique_ptr<int>>(entity));
}

TEST(Registry, Stomp) {
    entt::registry registry;

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');

    auto entity = registry.create();
    registry.stomp<int, char, double>(entity, prototype, registry);

    ASSERT_TRUE((registry.has<int, char>(entity)));
    ASSERT_EQ(registry.get<int>(entity), 3);
    ASSERT_EQ(registry.get<char>(entity), 'c');

    registry.replace<int>(prototype, 42);
    registry.replace<char>(prototype, 'a');
    registry.stomp<int>(entity, prototype, registry);

    ASSERT_EQ(registry.get<int>(entity), 42);
    ASSERT_EQ(registry.get<char>(entity), 'c');
}

TEST(Registry, StompExclude) {
    entt::registry registry;

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');

    const auto entity = registry.create();
    registry.stomp(entity, prototype, registry, entt::exclude<char>);

    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_FALSE(registry.has<char>(entity));
    ASSERT_EQ(registry.get<int>(entity), 3);

    registry.replace<int>(prototype, 42);
    registry.stomp(entity, prototype, registry, entt::exclude<int>);

    ASSERT_TRUE((registry.has<int, char>(entity)));
    ASSERT_EQ(registry.get<int>(entity), 3);
    ASSERT_EQ(registry.get<char>(entity), 'c');

    registry.remove<int>(entity);
    registry.remove<char>(entity);
    registry.stomp(entity, prototype, registry, entt::exclude<int, char>);

    ASSERT_TRUE(registry.orphan(entity));
}

TEST(Registry, StompMulti) {
    entt::registry registry;

    const auto prototype = registry.create();
    registry.assign<int>(prototype, 3);
    registry.assign<char>(prototype, 'c');

    entt::entity entities[2];
    registry.create(std::begin(entities), std::end(entities));
    registry.stomp(std::begin(entities), std::end(entities), prototype, registry);

    ASSERT_TRUE((registry.has<int, char>(entities[0])));
    ASSERT_TRUE((registry.has<int, char>(entities[1])));
    ASSERT_EQ(registry.get<int>(entities[0]), 3);
    ASSERT_EQ(registry.get<char>(entities[1]), 'c');
}

TEST(Registry, StompMoveOnlyComponent) {
    entt::registry registry;

    const auto prototype = registry.create();
    registry.assign<std::unique_ptr<int>>(prototype);
    registry.assign<char>(prototype);

    const auto entity = registry.create();
    registry.stomp(entity, prototype, registry);

    ASSERT_TRUE(registry.has<char>(entity));
    ASSERT_FALSE(registry.has<std::unique_ptr<int>>(entity));
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
    // the purpose is to ensure that move only components are always accepted
    entt::registry registry;
    const auto entity = registry.create();
    registry.assign<std::unique_ptr<int>>(entity);
}

TEST(Registry, Dependencies) {
    entt::registry registry;
    const auto entity = registry.create();

    // required because of an issue of VS2019
    constexpr auto assign_or_replace = &entt::registry::assign_or_replace<double>;
    constexpr auto remove = &entt::registry::remove<double>;

    registry.on_construct<int>().connect<assign_or_replace>(registry);
    registry.on_destroy<int>().connect<remove>(registry);
    registry.assign<double>(entity, .3);

    ASSERT_FALSE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);

    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);

    registry.remove<int>(entity);

    ASSERT_FALSE(registry.has<int>(entity));
    ASSERT_FALSE(registry.has<double>(entity));

    registry.on_construct<int>().disconnect<assign_or_replace>(registry);
    registry.on_destroy<int>().disconnect<remove>(registry);
    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_FALSE(registry.has<double>(entity));
}
