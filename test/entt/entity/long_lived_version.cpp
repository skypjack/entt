

#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/long_lived_versions.hpp>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iterator>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>



struct empty_type {};
struct stable_type { int value; };
using LVEntityId_t = entt::EntTypeWithLongTermVersionId;
//using traits_type = entt::entt_traits<LVEntityId_t>;
using Registry_t = entt::basic_registry<LVEntityId_t>;

template<>
struct entt::component_traits<stable_type>: basic_component_traits {
    using in_place_delete = std::true_type;
};

struct non_default_constructible {
    non_default_constructible(int v): value{v} {}
    int value;
};

struct aggregate {
    int value{};
};

struct listener {
    template<typename Component>
    static void sort(Registry_t &registry) {
        registry.sort<Component>([](auto lhs, auto rhs) { return lhs < rhs; });
    }

    template<typename Component>
    void incr(const Registry_t &, LVEntityId_t entity) {
        last = entity;
        ++counter;
    }

    template<typename Component>
    void decr(const Registry_t &, LVEntityId_t entity) {
        last = entity;
        --counter;
    }

    LVEntityId_t last{entt::null};
    int counter{0};
};

struct owner {
    void receive(const Registry_t &ref) {
        parent = &ref;
    }

    const Registry_t *parent{nullptr};
};


TEST(LongLivedVersionRegistry, CreateManyEntitiesAtOnceWithListener) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::entity_type entities[3];
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

TEST(LongLivedVersionRegistry, CreateWithHint) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    auto e3 = registry.create(Registry_t::entity_type{3});
    auto e2 = registry.create(Registry_t::entity_type{3});

    ASSERT_EQ(e2, Registry_t::entity_type{2});
    ASSERT_FALSE(registry.valid(Registry_t::entity_type{1}));
    ASSERT_EQ(e3, Registry_t::entity_type{3});

    registry.release(e2);

    ASSERT_EQ(registry.version(e2), Registry_t::entity_type::version_type{});
    ASSERT_EQ(registry.current(e2), Registry_t::entity_type::version_type{1});

    e2 = registry.create();
    auto e1 = registry.create(Registry_t::entity_type{2});

    ASSERT_EQ(registry.entity(e2), Registry_t::entity_type{2});
    ASSERT_EQ(registry.version(e2), Registry_t::entity_type::version_type{1});

    ASSERT_EQ(registry.entity(e1), Registry_t::entity_type{1});
    ASSERT_EQ(registry.version(e1), Registry_t::entity_type::version_type{});

    registry.release(e1);
    registry.release(e2);
    auto e0 = registry.create(Registry_t::entity_type{0});

    ASSERT_EQ(e0, Registry_t::entity_type{0});
    ASSERT_EQ(registry.version(e0), Registry_t::entity_type::version_type{});
}

TEST(LongLivedVersionRegistry, CreateClearCycle) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::entity_type pre{}, post{};

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

TEST(LongLivedVersionRegistry, CreateDestroyReleaseCornerCase) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.destroy(e0);
    registry.release(e1);

    registry.each([](auto) { FAIL(); });

    ASSERT_EQ(registry.current(e0), entt::registry::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{1});
}

TEST(LongLivedVersionRegistry, DestroyVersion) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    ASSERT_EQ(registry.current(e0), entt::registry::version_type{});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{});

    registry.destroy(e0);
    registry.destroy(e1, 3);

    ASSERT_DEATH(registry.destroy(e0), "");
    ASSERT_DEATH(registry.destroy(e1, 3), "");
    ASSERT_EQ(registry.current(e0), entt::registry::version_type{1});
    ASSERT_EQ(registry.current(e1), entt::registry::version_type{3});
}

TEST(LongLivedVersionRegistry, RangeDestroy) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, char>();
    Registry_t::entity_type entities[3u];

    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<int>(entities[0u]);
    registry.emplace<char>(entities[0u]);
    registry.emplace<double>(entities[0u]);

    registry.emplace<int>(entities[1u]);
    registry.emplace<char>(entities[1u]);

    registry.emplace<int>(entities[2u]);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    registry.destroy(icview.begin(), icview.end());
    registry.destroy(icview.rbegin(), icview.rend());

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    ASSERT_EQ(registry.size<int>(), 1u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_EQ(registry.size<double>(), 0u);

    registry.destroy(iview.begin(), iview.end());

    ASSERT_FALSE(registry.valid(entities[2u]));
    ASSERT_NO_FATAL_FAILURE(registry.destroy(iview.rbegin(), iview.rend()));
    ASSERT_EQ(iview.size(), 0u);
    ASSERT_EQ(icview.size_hint(), 0u);

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_EQ(registry.size<double>(), 0u);

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<int>(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));
    ASSERT_EQ(registry.size<int>(), 3u);

    registry.destroy(std::begin(entities), std::end(entities));

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_FALSE(registry.valid(entities[2u]));
    ASSERT_EQ(registry.size<int>(), 0u);
}

TEST(LongLivedVersionRegistry, StableDestroy) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    const auto iview = registry.view<int>();
    const auto icview = registry.view<int, stable_type>();
    Registry_t::entity_type entities[3u];

    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<int>(entities[0u]);
    registry.emplace<stable_type>(entities[0u]);
    registry.emplace<double>(entities[0u]);

    registry.emplace<int>(entities[1u]);
    registry.emplace<stable_type>(entities[1u]);

    registry.emplace<int>(entities[2u]);

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    registry.destroy(icview.begin(), icview.end());

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    ASSERT_EQ(registry.size<int>(), 1u);
    ASSERT_EQ(registry.size<stable_type>(), 2u);
    ASSERT_EQ(registry.size<double>(), 0u);

    registry.destroy(iview.begin(), iview.end());

    ASSERT_FALSE(registry.valid(entities[2u]));
    ASSERT_EQ(iview.size(), 0u);
    ASSERT_EQ(icview.size_hint(), 0u);

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<stable_type>(), 2u);
    ASSERT_EQ(registry.size<double>(), 0u);
}

TEST(LongLivedVersionRegistry, ReleaseVersion) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::entity_type entities[2u];

    registry.create(std::begin(entities), std::end(entities));

    ASSERT_EQ(registry.current(entities[0u]), entt::registry::version_type{});
    ASSERT_EQ(registry.current(entities[1u]), entt::registry::version_type{});

    registry.release(entities[0u]);
    registry.release(entities[1u], 3);

    ASSERT_DEATH(registry.release(entities[0u]), "");
    ASSERT_DEATH(registry.release(entities[1u], 3), "");
    ASSERT_EQ(registry.current(entities[0u]), entt::registry::version_type{1});
    ASSERT_EQ(registry.current(entities[1u]), entt::registry::version_type{3});
}

TEST(LongLivedVersionRegistry, RangeRelease) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::entity_type entities[3u];

    registry.create(std::begin(entities), std::end(entities));

    ASSERT_TRUE(registry.valid(entities[0u]));
    ASSERT_TRUE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    registry.release(std::begin(entities), std::end(entities) - 1u);

    ASSERT_FALSE(registry.valid(entities[0u]));
    ASSERT_FALSE(registry.valid(entities[1u]));
    ASSERT_TRUE(registry.valid(entities[2u]));

    registry.release(std::end(entities) - 1u, std::end(entities));

    ASSERT_FALSE(registry.valid(entities[2u]));
}
/*
TEST(LongLivedVersionRegistry, VersionOverflow) {
    using traits_type = entt::entt_traits<LVEntityId_t>;
    Registry_t registry;
    //entt::registry registry;
    const auto entity = registry.create();

    registry.release(entity);

    ASSERT_NE(registry.current(entity), registry.version(entity));
    ASSERT_NE(registry.current(entity), typename traits_type::version_type{});

    registry.release(registry.create(), traits_type::to_version(traits_type::construct()) - 1u);
    registry.release(registry.create());

    ASSERT_EQ(registry.current(entity), registry.version(entity));
    ASSERT_EQ(registry.current(entity), typename traits_type::version_type{});
}*/

TEST(LongLivedVersionRegistry, NullEntity) {
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    //entt::registry registry;
    const Registry_t::entity_type entity = entt::null;

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_NE(registry.create(entity), entity);
}

TEST(LongLivedVersionRegistry, TombstoneVersion) {
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    using traits_type = entt::entt_traits<LVEntityId_t>;

    //entt::registry registry;
    const Registry_t::entity_type entity = entt::tombstone;

    ASSERT_FALSE(registry.valid(entity));

    const auto other = registry.create();
    const auto vers = traits_type::to_version(entity);
    const auto required = traits_type::construct(traits_type::to_entity(other), vers);

    ASSERT_NE(registry.release(other, vers), vers);
    ASSERT_NE(registry.create(required), required);
}

TEST(LongLivedVersionRegistry, Each) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::size_type tot;
    Registry_t::size_type match;

    static_cast<void>(registry.create());
    registry.emplace<int>(registry.create());
    static_cast<void>(registry.create());
    registry.emplace<int>(registry.create());
    static_cast<void>(registry.create());

    tot = 0u;
    match = 0u;

    registry.each([&](auto entity) {
        if(registry.all_of<int>(entity)) { ++match; }
        static_cast<void>(registry.create());
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

TEST(LongLivedVersionRegistry, Orphans) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    Registry_t::size_type tot{};
    Registry_t::entity_type entities[3u]{};

    registry.create(std::begin(entities), std::end(entities));
    registry.emplace<int>(entities[0u]);
    registry.emplace<int>(entities[2u]);

    registry.orphans([&](auto) { ++tot; });

    ASSERT_EQ(tot, 1u);

    registry.erase<int>(entities[0u]);
    registry.erase<int>(entities[2u]);

    tot = {};
    registry.orphans([&](auto) { ++tot; });

    ASSERT_EQ(tot, 3u);

    registry.clear();
    tot = {};

    registry.orphans([&](auto) { ++tot; });
    ASSERT_EQ(tot, 0u);
}

TEST(LongLivedVersionRegistry, View) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    auto mview = registry.view<int, char>();
    auto iview = registry.view<int>();
    auto cview = registry.view<char>();
    LVEntityId_t entities[3u];

    registry.create(std::begin(entities), std::end(entities));

    registry.emplace<int>(entities[0u], 0);
    registry.emplace<char>(entities[0u], 'c');

    registry.emplace<int>(entities[1u], 0);

    registry.emplace<int>(entities[2u], 0);
    registry.emplace<char>(entities[2u], 'c');

    ASSERT_EQ(iview.size(), 3u);
    ASSERT_EQ(cview.size(), 2u);

    std::size_t cnt{};
    mview.each([&cnt](auto...) { ++cnt; });

    ASSERT_EQ(cnt, 2u);
}

TEST(LongLivedVersionRegistry, NonOwningGroupInitOnFirstUse) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, NonOwningGroupInitOnEmplace) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, FullOwningGroupInitOnFirstUse) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, FullOwningGroupInitOnEmplace) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, PartialOwningGroupInitOnFirstUse) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, PartialOwningGroupInitOnEmplace) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
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

TEST(LongLivedVersionRegistry, CleanViewAfterRemoveAndClear) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    auto view = registry.view<int, char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity);
    registry.emplace<char>(entity);

    ASSERT_EQ(view.size_hint(), 1u);

    registry.erase<char>(entity);

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

TEST(LongLivedVersionRegistry, CleanNonOwningGroupViewAfterRemoveAndClear) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.erase<char>(entity);

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

TEST(LongLivedVersionRegistry, CleanFullOwningGroupViewAfterRemoveAndClear) {
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;
    auto group = registry.group<int, char>();

    const auto entity = registry.create();
    registry.emplace<int>(entity, 0);
    registry.emplace<char>(entity, 'c');

    ASSERT_EQ(group.size(), 1u);

    registry.erase<char>(entity);

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


TEST(LongLivedVersionRegistry, Functionalities) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;

    ASSERT_EQ(registry.size(), 0u);
    ASSERT_EQ(registry.alive(), 0u);
    ASSERT_NO_FATAL_FAILURE((registry.reserve<int, char>(8)));
    ASSERT_NO_FATAL_FAILURE(registry.reserve(42));
    ASSERT_TRUE(registry.empty());

    ASSERT_EQ(registry.capacity(), 42u);
    ASSERT_EQ(registry.capacity<int>(), ENTT_PACKED_PAGE);
    ASSERT_EQ(registry.capacity<char>(), ENTT_PACKED_PAGE);
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
    ASSERT_NO_FATAL_FAILURE(registry.erase<int>(e1));
    ASSERT_NO_FATAL_FAILURE(registry.erase<char>(e1));

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
    ASSERT_EQ(std::get<1>(static_cast<const Registry_t &>(registry).get<int, char>(e0)), 'c');
    ASSERT_EQ(*std::get<1>(static_cast<const Registry_t &>(registry).try_get<int, char, double>(e0)), 'c');

    ASSERT_EQ(registry.get<int>(e0), registry.get<int>(e2));
    ASSERT_EQ(registry.get<char>(e0), registry.get<char>(e2));
    ASSERT_NE(&registry.get<int>(e0), &registry.get<int>(e2));
    ASSERT_NE(&registry.get<char>(e0), &registry.get<char>(e2));

    ASSERT_EQ(registry.patch<int>(e0, [](auto &instance) { instance = 2; }), 2);
    ASSERT_EQ(registry.replace<int>(e0, 3), 3);

    ASSERT_NO_FATAL_FAILURE(registry.emplace_or_replace<int>(e0, 1));
    ASSERT_NO_FATAL_FAILURE(registry.emplace_or_replace<int>(e1, 1));
    ASSERT_EQ(static_cast<const Registry_t &>(registry).get<int>(e0), 1);
    ASSERT_EQ(static_cast<const Registry_t &>(registry).get<int>(e1), 1);

    ASSERT_EQ(registry.size(), 3u);
    ASSERT_EQ(registry.alive(), 3u);
    ASSERT_FALSE(registry.empty());

    ASSERT_EQ(registry.version(e2), Registry_t::version_type(0));
    ASSERT_EQ(registry.current(e2), Registry_t::version_type(0));
    ASSERT_DEATH(registry.release(e2), "");
    ASSERT_NO_FATAL_FAILURE(registry.destroy(e2));
    ASSERT_DEATH(registry.destroy(e2), "");
    ASSERT_EQ(registry.version(e2), Registry_t::version_type(0));
    ASSERT_EQ(registry.current(e2), Registry_t::version_type(1));

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

    ASSERT_EQ(registry.remove<int>(e4), 1u);
    ASSERT_EQ(registry.remove<int>(e5), 0u);

    ASSERT_EQ(registry.size<int>(), 0u);
    ASSERT_EQ(registry.size<char>(), 0u);
    ASSERT_TRUE(registry.empty<int>());

    ASSERT_EQ(registry.capacity<int>(), ENTT_PACKED_PAGE);
    ASSERT_EQ(registry.capacity<char>(), ENTT_PACKED_PAGE);

    registry.shrink_to_fit<int, char>();

    ASSERT_EQ(registry.capacity<int>(), 0u);
    ASSERT_EQ(registry.capacity<char>(), 0u);
}

TEST(LongLivedVersionRegistry, Identifiers) {
    //entt::registry registry;
    entt::LongLivedVersionIdType ll_root;
    entt::LongLivedVersionIdType::setIfUnSetAndGetRoot(&ll_root);
    Registry_t registry;

    const auto pre = registry.create();

    ASSERT_EQ(pre, registry.entity(pre));

    registry.release(pre);
    const auto post = registry.create();

    ASSERT_NE(pre, post);
    ASSERT_EQ(Registry_t::entity(pre), Registry_t::entity(post));
    ASSERT_NE(Registry_t::version(pre), Registry_t::version(post));
    ASSERT_NE(registry.version(pre), registry.current(pre));
    ASSERT_EQ(registry.version(post), registry.current(post));
}





struct position {
    std::uint64_t x;
    std::uint64_t y;
};

struct velocity: position {};
struct stable_position: position {};

template<std::size_t>
struct comp { int x; };

template<>
struct entt::component_traits<stable_position>: basic_component_traits {
    using in_place_delete = std::true_type;
};

struct timer final {
    timer(): start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

template<typename Func>
void pathological(Func func) {
    entt::registry registry;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(!(++i % 7)) { registry.remove<position>(entity); }
            if(!(++i % 11)) { registry.remove<velocity>(entity); }
            if(!(++i % 13)) { registry.remove<comp<0>>(entity); }
            if(!(++i % 17)) { registry.destroy(entity); }
        });

        for(std::uint64_t j = 0; j < 50000L; j++) {
            const auto entity = registry.create();
            registry.emplace<position>(entity);
            registry.emplace<velocity>(entity);
            registry.emplace<comp<0>>(entity);
        }
    }

    func(registry, [](auto &... comp) {
        ((comp.x = {}), ...);
    });
}


TEST(LongLivedVersion, BasicLinkedList) {

  entt::LongLivedVersionIdType ll_root;

}

/*
TEST(LongLivedVersionBenchmark, Create) {
    entt::registry registry;

    std::cout << "Creating 1000000 entities" << std::endl;

    timer timer;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        static_cast<void>(registry.create());
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, CreateMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once" << std::endl;

    timer timer;
    registry.create(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, CreateManyAndEmplaceComponents) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once and emplace components" << std::endl;

    timer timer;

    registry.create(entities.begin(), entities.end());

    for(const auto entity: entities) {
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, CreateManyWithComponents) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once with components" << std::endl;

    timer timer;
    registry.create(entities.begin(), entities.end());
    registry.insert<position>(entities.begin(), entities.end());
    registry.insert<velocity>(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, Erase) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Erasing 1000000 components from their entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;

    for(auto entity: registry.view<int>()) {
        registry.erase<int>(entity);
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, EraseMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Erasing 1000000 components from their entities at once" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.erase<int>(view.begin(), view.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, Remove) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Removing 1000000 components from their entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;

    for(auto entity: registry.view<int>()) {
        registry.remove<int>(entity);
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, RemoveMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Removing 1000000 components from their entities at once" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.remove<int>(view.begin(), view.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, Clear) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Clearing 1000000 components from their entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    registry.clear<int>();
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, Recycle) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Recycling 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer timer;

    for(auto next = entities.size(); next; --next) {
        static_cast<void>(registry.create());
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, RecycleMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Recycling 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer timer;
    registry.create(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, Destroy) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Destroying 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;

    for(auto entity: registry.view<int>()) {
        registry.destroy(entity);
    }

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, DestroyMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Destroying 1000000 entities at once" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.destroy(view.begin(), view.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, DestroyManyFastPath) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Destroying 1000000 entities at once, fast path" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    registry.destroy(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, IterateSingleComponent1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateSingleComponentTombstonePolicy1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, one component, tombstone policy" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<stable_position>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<stable_position>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateSingleComponentRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, one component, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = { entt::type_hash<position>::value() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTombstonePolicyTwoComponentsTombstonePolicy1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, tombstone policy" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<stable_position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<stable_position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity>);

    std::cout << "Iterating over 1000000 entities, two components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity>();

    std::cout << "Iterating over 1000000 entities, two components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsPartialOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position>(entt::get<velocity>);

    std::cout << "Iterating over 1000000 entities, two components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateTwoComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsTombstonePolicy1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, tombstone policy" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<stable_position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<stable_position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity, comp<0>>);

    std::cout << "Iterating over 1000000 entities, three components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>>();

    std::cout << "Iterating over 1000000 entities, three components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsPartialOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity>(entt::get<comp<0>>);

    std::cout << "Iterating over 1000000 entities, three components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateThreeComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsTombstonePolicy1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, tombstone policy" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<stable_position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<stable_position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity, comp<0>, comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>, comp<1>, comp<2>>();

    std::cout << "Iterating over 1000000 entities, five components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsPartialFourOfFiveOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>, comp<1>>(entt::get<comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (4 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsPartialThreeOfFiveOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>>(entt::get<comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (3 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IterateFiveComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(LongLivedVersionBenchmark, IteratePathological) {
    std::cout << "Pathological case" << std::endl;

    pathological([](auto &registry, auto func) {
        timer timer;
        registry.template view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    });
}

TEST(LongLivedVersionBenchmark, IteratePathologicalNonOwningGroup) {
    std::cout << "Pathological case (non-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<>(entt::get<position, velocity, comp<0>>);

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(LongLivedVersionBenchmark, IteratePathologicalFullOwningGroup) {
    std::cout << "Pathological case (full-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<position, velocity, comp<0>>();

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(LongLivedVersionBenchmark, IteratePathologicalPartialOwningGroup) {
    std::cout << "Pathological case (partial-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<position, velocity>(entt::get<comp<0>>);

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(LongLivedVersionBenchmark, SortSingle) {
    entt::registry registry;

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, SortMulti) {
    entt::registry registry;

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);
        registry.emplace<velocity>(entity, i, i);
    }

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer timer;

    registry.sort<velocity, position>();

    timer.elapsed();
}

TEST(LongLivedVersionBenchmark, AlmostSortedStdSort) {
    entt::registry registry;
    entt::entity entities[3]{};

    std::cout << "Sort 150000 entities, almost sorted, std::sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.emplace<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    });

    timer.elapsed();
}



TEST(LongLivedVersionBenchmark, AlmostSortedInsertionSort) {
    entt::registry registry;
    entt::entity entities[3]{};

    std::cout << "Sort 150000 entities, almost sorted, insertion sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.emplace<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    }, entt::insertion_sort{});

    timer.elapsed();
}*/


TEST(LongLivedVersion, Traits) {
    using traits_type = entt::entt_traits<entt::entity>;
    entt::registry registry{};

    registry.destroy(registry.create());
    const auto entity = registry.create();
    const auto other = registry.create();

    ASSERT_EQ(entt::to_integral(entity), traits_type::to_integral(entity));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral<entt::entity>(entt::null));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral(entt::entity{}));

    ASSERT_EQ(traits_type::to_entity(entity), 0u);
    ASSERT_EQ(traits_type::to_version(entity), 1u);
    ASSERT_EQ(traits_type::to_entity(other), 1u);
    ASSERT_EQ(traits_type::to_version(other), 0u);

    ASSERT_EQ(traits_type::construct(traits_type::to_entity(entity), traits_type::to_version(entity)), entity);
    ASSERT_EQ(traits_type::construct(traits_type::to_entity(other), traits_type::to_version(other)), other);
    ASSERT_NE(traits_type::construct(traits_type::to_entity(entity), {}), entity);

    ASSERT_EQ(traits_type::construct(), entt::tombstone | static_cast<entt::entity>(entt::null));
    ASSERT_EQ(traits_type::construct(), entt::null | static_cast<entt::entity>(entt::tombstone));

    ASSERT_EQ(traits_type::construct(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(traits_type::construct(), static_cast<entt::entity>(entt::tombstone));
    ASSERT_EQ(traits_type::construct(), entt::entity{~entt::id_type{}});
}

