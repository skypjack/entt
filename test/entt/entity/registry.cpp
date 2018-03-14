#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entt_traits.hpp>
#include <entt/entity/registry.hpp>

TEST(DefaultRegistry, Functionalities) {
    entt::DefaultRegistry registry;

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{0});
    ASSERT_NO_THROW(registry.reserve(42));
    ASSERT_NO_THROW(registry.reserve<int>(8));
    ASSERT_NO_THROW(registry.reserve<char>(8));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    auto e0 = registry.create();
    auto e1 = registry.create<int, char>();

    ASSERT_TRUE(registry.has<>(e0));
    ASSERT_TRUE(registry.has<>(e1));

    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{2});
    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NE(e0, e1);

    ASSERT_FALSE(registry.has<int>(e0));
    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_FALSE(registry.has<char>(e0));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE((registry.has<int, char>(e0)));
    ASSERT_TRUE((registry.has<int, char>(e1)));

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

    auto e2 = registry.create();

    registry.accommodate<int>(e2, registry.get<int>(e0));
    registry.accommodate<char>(e2, registry.get<char>(e0));

    ASSERT_TRUE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e2));
    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');

    ASSERT_EQ(std::get<0>(registry.get<int, char>(e0)), 42);
    ASSERT_EQ(std::get<1>(static_cast<const entt::DefaultRegistry &>(registry).get<int, char>(e0)), 'c');

    ASSERT_EQ(registry.get<int>(e0), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e0), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e0), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e0), &registry.get<char>(e2));

    ASSERT_NO_THROW(registry.replace<int>(e0, 0));
    ASSERT_EQ(registry.get<int>(e0), 0);

    ASSERT_NO_THROW(registry.accommodate<int>(e0, 1));
    ASSERT_NO_THROW(registry.accommodate<int>(e1, 1));
    ASSERT_EQ(static_cast<const entt::DefaultRegistry &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const entt::DefaultRegistry &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{3});
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e2), entt::DefaultRegistry::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::DefaultRegistry::version_type{0});
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{3});
    ASSERT_NO_THROW(registry.destroy(e2));
    ASSERT_EQ(registry.capacity(), entt::DefaultRegistry::size_type{3});
    ASSERT_EQ(registry.version(e2), entt::DefaultRegistry::version_type{0});
    ASSERT_EQ(registry.current(e2), entt::DefaultRegistry::version_type{1});

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_TRUE(registry.fast(e0));
    ASSERT_TRUE(registry.valid(e1));
    ASSERT_TRUE(registry.fast(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.fast(e2));

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{2});
    ASSERT_FALSE(registry.empty());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty());

    registry.create<int, char>();

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{1});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset<int>());

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{1});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty<char>());

    ASSERT_NO_THROW(registry.reset());

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(registry.empty<char>());

    e0 = registry.create<int>();
    e1 = registry.create();

    ASSERT_NO_THROW(registry.reset<int>(e0));
    ASSERT_NO_THROW(registry.reset<int>(e1));

    ASSERT_EQ(registry.size<int>(), entt::DefaultRegistry::size_type{0});
    ASSERT_EQ(registry.size<char>(), entt::DefaultRegistry::size_type{0});
    ASSERT_TRUE(registry.empty<int>());
}

TEST(DefaultRegistry, CreateDestroyCornerCase) {
    entt::DefaultRegistry registry;

    auto e0 = registry.create();
    auto e1 = registry.create();

    registry.destroy(e0);
    registry.destroy(e1);

    registry.each([](auto) { FAIL(); });

    ASSERT_EQ(registry.current(e0), entt::DefaultRegistry::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::DefaultRegistry::version_type{1});
}

TEST(DefaultRegistry, VersionOverflow) {
    entt::DefaultRegistry registry;

    auto entity = registry.create();
    registry.destroy(entity);

    ASSERT_EQ(registry.version(entity), entt::DefaultRegistry::version_type{});

    for(auto i = entt::entt_traits<entt::DefaultRegistry::entity_type>::version_mask; i; --i) {
        ASSERT_NE(registry.current(entity), registry.version(entity));
        registry.destroy(registry.create());
    }

    ASSERT_EQ(registry.current(entity), registry.version(entity));
}

TEST(DefaultRegistry, Each) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry::size_type tot;
    entt::DefaultRegistry::size_type match;

    registry.create();
    registry.create<int>();
    registry.create();
    registry.create<int>();
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

TEST(DefaultRegistry, Orphans) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry::size_type tot{};

    registry.create<int>();
    registry.create();
    registry.create<int>();
    registry.create();
    registry.attach<double>(registry.create());

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 2u);
    tot = 0u;

    registry.each([&](auto entity) { registry.reset<int>(entity); });
    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 4u);
    registry.reset();
    tot = 0u;

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 0u);
}

TEST(DefaultRegistry, Types) {
    entt::DefaultRegistry registry;

    ASSERT_EQ(registry.tag<int>(), registry.tag<int>());
    ASSERT_EQ(registry.component<int>(), registry.component<int>());

    ASSERT_NE(registry.tag<int>(), registry.tag<double>());
    ASSERT_NE(registry.component<int>(), registry.component<double>());
}

TEST(DefaultRegistry, CreateDestroyEntities) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry::entity_type pre{}, post{};

    for(int i = 0; i < 10; ++i) {
        registry.create<double>();
    }

    registry.reset();

    for(int i = 0; i < 7; ++i) {
        auto entity = registry.create<int>();
        if(i == 3) { pre = entity; }
    }

    registry.reset();

    for(int i = 0; i < 5; ++i) {
        auto entity = registry.create();
        if(i == 3) { post = entity; }
    }

    ASSERT_FALSE(registry.valid(pre));
    ASSERT_TRUE(registry.valid(post));
    ASSERT_NE(registry.version(pre), registry.version(post));
    ASSERT_EQ(registry.version(pre) + 1, registry.version(post));
    ASSERT_EQ(registry.current(pre), registry.current(post));
}

TEST(DefaultRegistry, AttachRemoveTags) {
    entt::DefaultRegistry registry;
    const auto &cregistry = registry;

    ASSERT_FALSE(registry.has<int>());

    auto entity = registry.create();
    registry.attach<int>(entity, 42);

    ASSERT_TRUE(registry.has<int>());
    ASSERT_EQ(registry.get<int>(), 42);
    ASSERT_EQ(cregistry.get<int>(), 42);
    ASSERT_EQ(registry.attachee<int>(), entity);

    registry.remove<int>();

    ASSERT_FALSE(registry.has<int>());

    registry.attach<int>(entity, 42);
    registry.destroy(entity);

    ASSERT_FALSE(registry.has<int>());
}

TEST(DefaultRegistry, StandardViews) {
    entt::DefaultRegistry registry;
    auto mview = registry.view<int, char>();
    auto iview = registry.view<int>();
    auto cview = registry.view<char>();

    registry.create(0, 'c');
    registry.create(0);
    registry.create(0, 'c');

    ASSERT_EQ(iview.size(), decltype(iview)::size_type{3});
    ASSERT_EQ(cview.size(), decltype(cview)::size_type{2});

    decltype(mview)::size_type cnt{0};
    mview.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(mview)::size_type{2});
}

TEST(DefaultRegistry, PersistentViews) {
    entt::DefaultRegistry registry;
    auto view = registry.persistent<int, char>();

    ASSERT_TRUE((registry.contains<int, char>()));
    ASSERT_FALSE((registry.contains<int, double>()));

    registry.prepare<int, double>();

    ASSERT_TRUE((registry.contains<int, double>()));

    registry.discard<int, double>();

    ASSERT_FALSE((registry.contains<int, double>()));

    registry.create(0, 'c');
    registry.create(0);
    registry.create(0, 'c');

    decltype(view)::size_type cnt{0};
    view.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, decltype(view)::size_type{2});
}

TEST(DefaultRegistry, CleanStandardViewsAfterReset) {
    entt::DefaultRegistry registry;
    auto view = registry.view<int>();
    registry.create(0);

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{1});

    registry.reset();

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{0});
}

TEST(DefaultRegistry, CleanPersistentViewsAfterReset) {
    entt::DefaultRegistry registry;
    auto view = registry.persistent<int, char>();
    registry.create(0, 'c');

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{1});

    registry.reset();

    ASSERT_EQ(view.size(), entt::DefaultRegistry::size_type{0});
}

TEST(DefaultRegistry, CleanTagsAfterReset) {
    entt::DefaultRegistry registry;
    auto entity = registry.create();
    registry.attach<int>(entity);

    ASSERT_TRUE(registry.has<int>());

    registry.reset();

    ASSERT_FALSE(registry.has<int>());
}

TEST(DefaultRegistry, SortSingle) {
    entt::DefaultRegistry registry;

    int val = 0;

    registry.create(val++);
    registry.create(val++);
    registry.create(val++);

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), --val);
    }

    registry.sort<int>(std::less<int>{});

    for(auto entity: registry.view<int>()) {
        ASSERT_EQ(registry.get<int>(entity), val++);
    }
}

TEST(DefaultRegistry, SortMulti) {
    entt::DefaultRegistry registry;

    unsigned int uval = 0u;
    int ival = 0;

    registry.create(uval++, ival++);
    registry.create(uval++, ival++);
    registry.create(uval++, ival++);

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

TEST(DefaultRegistry, ComponentsWithTypesFromStandardTemplateLibrary) {
    // see #37 - the test shouldn't crash, that's all
    entt::DefaultRegistry registry;
    auto entity = registry.create();
    registry.assign<std::unordered_set<int>>(entity).insert(42);
    registry.destroy(entity);
}

TEST(DefaultRegistry, ConstructWithComponents) {
    // it should compile, that's all
    entt::DefaultRegistry registry;
    const auto value = 0;
    registry.create(value);
}

TEST(DefaultRegistry, MergeTwoRegistries) {
    using entity_type = entt::DefaultRegistry::entity_type;

    entt::DefaultRegistry src;
    entt::DefaultRegistry dst;

    std::unordered_map<entity_type, entity_type> ref;

    auto merge = [&ref](const auto &view, auto &dst) {
        view.each([&](auto entity, const auto &component) {
            if(ref.find(entity) == ref.cend()) {
                ref.emplace(entity, dst.create(component));
            } else {
                using component_type = std::decay_t<decltype(component)>;
                dst.template assign<component_type>(ref[entity], component);
            }
        });
    };

    src.create<int, float, double>();
    src.create<char, float, int>();

    dst.create<int, char, double>();
    dst.create<float, int>();

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
