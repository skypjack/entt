#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entt_traits.hpp>
#include <entt/entity/registry.hpp>

ENTT_SHARED_TYPE(int)

struct listener {
    template<typename Component>
    void incr(entt::registry<> &registry, entt::registry<>::entity_type entity) {
        ASSERT_TRUE(registry.valid(entity));
        ASSERT_TRUE(registry.has<Component>(entity));
        last = entity;
        ++counter;
    }

    template<typename Component>
    void decr(entt::registry<> &registry, entt::registry<>::entity_type entity) {
        ASSERT_TRUE(registry.valid(entity));
        ASSERT_TRUE(registry.has<Component>(entity));
        last = entity;
        --counter;
    }

    entt::registry<>::entity_type last;
    int counter{0};
};

TEST(Registry, Types) {
    entt::registry<> registry;
    ASSERT_EQ(registry.type<int>(), registry.type<int>());
    ASSERT_NE(registry.type<double>(), registry.type<int>());
}

TEST(Registry, Functionalities) {
    entt::registry<> registry;

    ASSERT_EQ(registry.size(), entt::registry<>::size_type{0});
    ASSERT_EQ(registry.alive(), entt::registry<>::size_type{0});
    ASSERT_NO_THROW(registry.reserve(42));
    ASSERT_NO_THROW(registry.reserve<int>(8));
    ASSERT_NO_THROW(registry.reserve<char>(8));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), entt::registry<>::size_type{42});
    ASSERT_EQ(registry.capacity<int>(), entt::registry<>::size_type{8});
    ASSERT_EQ(registry.capacity<char>(), entt::registry<>::size_type{8});
    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_TRUE(registry.has<>(e0));
    ASSERT_TRUE(registry.has<>(e1));

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{1});
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
    ASSERT_EQ(std::get<1>(static_cast<const entt::registry<> &>(registry).get<int, char>(e0)), 'c');
    ASSERT_EQ(*std::get<1>(static_cast<const entt::registry<> &>(registry).try_get<int, char, double>(e0)), 'c');

    ASSERT_EQ(registry.get<int>(e0), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e0), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e0), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e0), &registry.get<char>(e2));

    ASSERT_NO_THROW(registry.replace<int>(e0, 0));
    ASSERT_EQ(registry.get<int>(e0), 0);

    ASSERT_NO_THROW(registry.assign_or_replace<int>(e0, 1));
    ASSERT_NO_THROW(registry.assign_or_replace<int>(e1, 1));
    ASSERT_EQ(static_cast<const entt::registry<> &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const entt::registry<> &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.size(), entt::registry<>::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry<>::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e2), entt::registry<>::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::registry<>::version_type{0});
    ASSERT_NO_THROW(registry.destroy(e2));
    ASSERT_EQ(registry.version(e2), entt::registry<>::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::registry<>::version_type{1});

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.fast(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_TRUE(registry.fast(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.fast(e2));

    ASSERT_EQ(registry.size(), entt::registry<>::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry<>::size_type{2});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size(), entt::registry<>::size_type{3});
    ASSERT_EQ(registry.alive(), entt::registry<>::size_type{0});
    ASSERT_TRUE(registry.empty());

    const auto e3 = registry.create();

    ASSERT_EQ(registry.get<int>(e3, 3), 3);
    ASSERT_EQ(registry.get<char>(e3, 'c'), 'c');

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());
    ASSERT_TRUE(registry.has<int>(e3));
    ASSERT_TRUE(registry.has<char>(e3));
    ASSERT_EQ(registry.get<int>(e3), 3);
    ASSERT_EQ(registry.get<char>(e3), 'c');

    ASSERT_NO_THROW(registry.reset<int>());

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{1});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    const auto e4 = registry.create();
    const auto e5 = registry.create();

    registry.assign<int>(e4);

    ASSERT_NO_THROW(registry.reset<int>(e4));
    ASSERT_NO_THROW(registry.reset<int>(e5));

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::registry<>::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
}

TEST(Registry, Identifiers) {
    entt::registry<> registry;
    const auto pre = registry.create();

    ASSERT_EQ(pre, registry.entity(pre));

    registry.destroy(pre);
    const auto post = registry.create();

    ASSERT_NE(pre, post);
    ASSERT_EQ(registry.entity(pre), registry.entity(post));
    ASSERT_NE(registry.version(pre), registry.version(post));
    ASSERT_NE(registry.version(pre), registry.current(pre));
    ASSERT_EQ(registry.version(post), registry.current(post));
}

TEST(Registry, RawData) {
    entt::registry<> registry;
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
    entt::registry<> registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.destroy(e0);
    registry.destroy(e1);

    registry.each([](auto) { FAIL(); });

    ASSERT_EQ(registry.current(e0), entt::registry<>::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::registry<>::version_type{1});
}

TEST(Registry, VersionOverflow) {
    entt::registry<> registry;

    const auto entity = registry.create();
    registry.destroy(entity);

    ASSERT_EQ(registry.version(entity), entt::registry<>::version_type{});

    for(auto i = entt::entt_traits<entt::registry<>::entity_type>::version_mask; i; --i) {
        ASSERT_NE(registry.current(entity), registry.version(entity));
        registry.destroy(registry.create());
    }

    ASSERT_EQ(registry.current(entity), registry.version(entity));
}

TEST(Registry, Each) {
    entt::registry<> registry;
    entt::registry<>::size_type tot;
    entt::registry<>::size_type match;

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
    entt::registry<> registry;
    entt::registry<>::size_type tot{};

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
    entt::registry<> registry;
    entt::registry<>::entity_type pre{}, post{};

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
    entt::registry<> registry;
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
    entt::registry<> registry;

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
    entt::registry<> registry;
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
    entt::registry<> registry;

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
    entt::registry<> registry;
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
    entt::registry<> registry;

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
    entt::registry<> registry;
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
    entt::registry<> registry;
    auto view = registry.view<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry<>::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(view.size(), entt::registry<>::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(view.size(), entt::registry<>::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(view.size(), entt::registry<>::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(view.size(), entt::registry<>::size_type{1});

    registry.reset();

    ASSERT_EQ(view.size(), entt::registry<>::size_type{0});
}

TEST(Registry, CleanNonOwningGroupViewAfterReset) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});
}

TEST(Registry, CleanFullOwningGroupViewAfterReset) {
    entt::registry<> registry;
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});
}

TEST(Registry, CleanPartialOwningGroupViewAfterReset) {
    entt::registry<> registry;
    auto group = registry.group<int>(entt::get<char>);

    const auto entity = registry.create();
    registry.assign<int>(entity, 0);
    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<char>(entity);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<char>(entity, 'c');

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset<int>();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});

    registry.assign<int>(entity, 0);

    ASSERT_EQ(group.size(), entt::registry<>::size_type{1});

    registry.reset();

    ASSERT_EQ(group.size(), entt::registry<>::size_type{0});
}

TEST(Registry, SortSingle) {
    entt::registry<> registry;

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
    entt::registry<> registry;

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

TEST(Registry, ComponentsWithTypesFromStandardTemplateLibrary) {
    // see #37 - the test shouldn't crash, that's all
    entt::registry<> registry;
    const auto entity = registry.create();
    registry.assign<std::unordered_set<int>>(entity).insert(42);
    registry.destroy(entity);
}

TEST(Registry, ConstructWithComponents) {
    // it should compile, that's all
    entt::registry<> registry;
    const auto value = 0;
    registry.assign<int>(registry.create(), value);
}

TEST(Registry, MergeTwoRegistries) {
    using entity_type = entt::registry<>::entity_type;

    entt::registry<> src;
    entt::registry<> dst;

    std::unordered_map<entity_type, entity_type> ref;

    auto merge = [&ref](const auto &view, auto &dst) {
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

    merge(src.view<int>(), dst);
    merge(src.view<char>(), dst);
    merge(src.view<double>(), dst);
    merge(src.view<float>(), dst);

    ne(dst.view<int, float, double>().begin(), dst.view<int, float, double>().end());
    ne(dst.view<char, float, int>().begin(), dst.view<char, float, int>().end());
}

TEST(Registry, Signals) {
    entt::registry<> registry;
    listener listener;

    registry.construction<int>().connect<&listener::incr<int>>(&listener);
    registry.destruction<int>().connect<&listener::decr<int>>(&listener);

    auto e0 = registry.create();
    auto e1 = registry.create();

    registry.assign<int>(e0);
    registry.assign<int>(e1);

    ASSERT_EQ(listener.counter, 2);
    ASSERT_EQ(listener.last, e1);

    registry.remove<int>(e0);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e0);

    registry.destruction<int>().disconnect<&listener::decr<int>>(&listener);
    registry.remove<int>(e1);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e0);

    registry.construction<int>().disconnect<&listener::incr<int>>(&listener);
    registry.assign<int>(e1);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e0);

    registry.construction<int>().connect<&listener::incr<int>>(&listener);
    registry.destruction<int>().connect<&listener::decr<int>>(&listener);
    registry.assign<int>(e0);
    registry.reset<int>(e1);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e1);

    registry.reset<int>();

    ASSERT_EQ(listener.counter, 0);
    ASSERT_EQ(listener.last, e0);

    registry.assign<int>(e0);
    registry.assign<int>(e1);
    registry.destroy(e1);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e1);
}

TEST(Registry, DestroyByComponents) {
    entt::registry<> registry;

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
        const auto view = registry.view<int, char, double>();
        registry.destroy(view.begin(), view.end());
    }

    ASSERT_FALSE(registry.valid(e0));
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

TEST(Registry, SignalsOnAccommodate) {
    entt::registry<> registry;
    const auto entity = registry.create();
    const auto group = registry.group<>(entt::get<int, char>);

    registry.assign<int>(entity);
    registry.assign_or_replace<char>(entity);

    ASSERT_FALSE((group.empty()));
}

TEST(Registry, CreateManyEntitiesAtOnce) {
    entt::registry<> registry;
    entt::registry<>::entity_type entities[3];

    const auto entity = registry.create();
    registry.destroy(registry.create());
    registry.destroy(entity);
    registry.destroy(registry.create());

    registry.create(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.valid(entities[0]));
    ASSERT_TRUE(registry.valid(entities[1]));
    ASSERT_TRUE(registry.valid(entities[2]));

    ASSERT_EQ(registry.entity(entities[0]), entt::registry<>::entity_type{0});
    ASSERT_EQ(registry.version(entities[0]), entt::registry<>::version_type{2});

    ASSERT_EQ(registry.entity(entities[1]), entt::registry<>::entity_type{1});
    ASSERT_EQ(registry.version(entities[1]), entt::registry<>::version_type{1});

    ASSERT_EQ(registry.entity(entities[2]), entt::registry<>::entity_type{2});
    ASSERT_EQ(registry.version(entities[2]), entt::registry<>::version_type{0});
}

TEST(Registry, CreateAnEntityWithComponents) {
    entt::registry<> registry;
    const auto &[entity, ivalue, cvalue] = registry.create<int, char>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{1});
    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{1});

    ASSERT_TRUE((registry.has<int, char>(entity)));

    ivalue = 42;
    cvalue = 'c';

    ASSERT_EQ(registry.get<int>(entity), 42);
    ASSERT_EQ(registry.get<char>(entity), 'c');
}

TEST(Registry, CreateManyEntitiesWithComponentsAtOnce) {
    entt::registry<> registry;
    entt::registry<>::entity_type entities[3];

    const auto entity = registry.create();
    registry.destroy(registry.create());
    registry.destroy(entity);
    registry.destroy(registry.create());

    const auto [iptr, cptr] = registry.create<int, char>(std::begin(entities), std::end(entities));

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{3});
    ASSERT_EQ(registry.size<int>(), entt::registry<>::size_type{3});

    ASSERT_TRUE(registry.valid(entities[0]));
    ASSERT_TRUE(registry.valid(entities[1]));
    ASSERT_TRUE(registry.valid(entities[2]));

    ASSERT_EQ(registry.entity(entities[0]), entt::registry<>::entity_type{0});
    ASSERT_EQ(registry.version(entities[0]), entt::registry<>::version_type{2});

    ASSERT_EQ(registry.entity(entities[1]), entt::registry<>::entity_type{1});
    ASSERT_EQ(registry.version(entities[1]), entt::registry<>::version_type{1});

    ASSERT_EQ(registry.entity(entities[2]), entt::registry<>::entity_type{2});
    ASSERT_EQ(registry.version(entities[2]), entt::registry<>::version_type{0});

    ASSERT_TRUE((registry.has<int, char>(entities[0])));
    ASSERT_TRUE((registry.has<int, char>(entities[1])));
    ASSERT_TRUE((registry.has<int, char>(entities[2])));

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
    entt::registry<> registry;
    entt::registry<>::entity_type entities[3];
    listener listener;

    registry.construction<int>().connect<&listener::incr<int>>(&listener);
    registry.create<int, char>(std::begin(entities), std::end(entities));

    ASSERT_EQ(listener.counter, 3);
}

TEST(Registry, NonOwningGroupInterleaved) {
    entt::registry<> registry;
    typename entt::registry<>::entity_type entity = entt::null;

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
    entt::registry<> registry;
    typename entt::registry<>::entity_type entity = entt::null;

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
    entt::registry<> registry;
    typename entt::registry<>::entity_type entity = entt::null;

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
    entt::registry<> registry;
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
    entt::registry<> registry;
    entt::registry<> other;

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

    other = registry.clone<int, char>();

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

    other = registry.clone();

    ASSERT_EQ(other.size(), registry.size());
    ASSERT_EQ(other.alive(), registry.alive());

    ASSERT_TRUE(other.valid(e0));
    ASSERT_FALSE(other.valid(e1));
    ASSERT_TRUE(other.valid(e2));

    ASSERT_TRUE((other.has<int, double>(e0)));
    ASSERT_TRUE((other.has<int, char>(e2)));

    ASSERT_EQ(other.get<int>(e0), 0);
    ASSERT_EQ(other.get<double>(e0), 0.);
    ASSERT_EQ(other.get<int>(e2), 2);
    ASSERT_EQ(other.get<char>(e2), '2');

    other = registry.clone<char>();

    ASSERT_EQ(other.size(), registry.size());
    ASSERT_EQ(other.alive(), registry.alive());

    ASSERT_TRUE(other.valid(e0));
    ASSERT_FALSE(other.valid(e1));
    ASSERT_TRUE(other.valid(e2));

    ASSERT_FALSE((other.has<int>(e0)));
    ASSERT_FALSE((other.has<double>(e0)));
    ASSERT_FALSE((other.has<int>(e2)));
    ASSERT_TRUE((other.has<char>(e2)));

    ASSERT_TRUE(other.orphan(e0));
    ASSERT_EQ(other.get<char>(e2), '2');

    const auto entity = registry.create();
    listener listener;

    ASSERT_NE(e1, entity);
    ASSERT_EQ(registry.entity(e1), registry.entity(entity));

    registry.construction<char>().connect<&listener::incr<char>>(&listener);
    registry.destruction<char>().connect<&listener::decr<char>>(&listener);
    registry.assign<char>(entity, 'e');
    registry.assign<char>(e0, '0');
    registry.remove<char>(e0);

    ASSERT_EQ(listener.counter, 1);
    ASSERT_EQ(listener.last, e0);
}

TEST(Registry, GetOrAssign) {
    entt::registry<> registry;
    const auto entity = registry.create();
    const auto value = registry.get<int>(entity, 3);
    ASSERT_TRUE(registry.has<int>(entity));
    ASSERT_EQ(registry.get<int>(entity), value);
    ASSERT_EQ(registry.get<int>(entity), 3);
}

TEST(Registry, Constness) {
    entt::registry<> registry;

    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int>({})), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.get<int, char>({})), std::tuple<int &, char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int>({})), int *>));
    ASSERT_TRUE((std::is_same_v<decltype(registry.try_get<int, char>({})), std::tuple<int *, char *>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int>({})), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).get<int, char>({})), std::tuple<const int &, const char &>>));

    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int>({})), const int *>));
    ASSERT_TRUE((std::is_same_v<decltype(std::as_const(registry).try_get<int, char>({})), std::tuple<const int *, const char *>>));
}
