#include <map>
#include <tuple>
#include <queue>
#include <vector>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>

template<typename Storage>
struct output_archive {
    output_archive(Storage &instance)
        : storage{instance}
    {}

    template<typename... Value>
    void operator()(const Value &... value) {
        (std::get<std::queue<Value>>(storage).push(value), ...);
    }

private:
    Storage &storage;
};

template<typename Storage>
struct input_archive {
    input_archive(Storage &instance)
        : storage{instance}
    {}

    template<typename... Value>
    void operator()(Value &... value) {
        auto assign = [this](auto &val) {
            auto &queue = std::get<std::queue<std::decay_t<decltype(val)>>>(storage);
            val = queue.front();
            queue.pop();
        };

        (assign(value), ...);
    }

private:
    Storage &storage;
};

struct a_component {};

struct another_component {
    int key;
    int value;
};

struct what_a_component {
    entt::entity bar;
    std::vector<entt::entity> quux;
};

struct what_a_map_component {
    entt::entity bar;
    std::map<entt::entity, int> entity_keys;
    std::map<int, entt::entity> entity_values;
    std::map<entt::entity, entt::entity> entity_keys_and_values;
};

TEST(Snapshot, Dump) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    const auto e3 = registry.create();
    registry.assign<a_component>(e3);
    registry.assign<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<a_component>,
        std::queue<another_component>
    >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<int, char, double, a_component, another_component>(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    registry.loader()
            .entities(input)
            .destroyed(input)
            .component<int, char, double, a_component, another_component>(input)
            .orphans();

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e2));
    ASSERT_FALSE(registry.orphan(e3));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<double>(e0), .1);
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');
    ASSERT_TRUE(registry.has<a_component>(e3));

    ASSERT_TRUE(registry.empty<another_component>());
}

TEST(Snapshot, Partial) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    const auto e3 = registry.create();
    registry.assign<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>
    >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<char, int>(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    registry.loader()
            .entities(input)
            .destroyed(input)
            .component<char, int>(input);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_FALSE(registry.has<double>(e0));
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');

    registry.snapshot()
            .entities(output)
            .destroyed(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    registry.loader()
            .entities(input)
            .destroyed(input)
            .orphans();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
}

TEST(Snapshot, Iterator) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry registry;

    for(auto i = 0; i < 50; ++i) {
        const auto entity = registry.create();
        registry.assign<another_component>(entity, i, i);

        if(i % 2) {
            registry.assign<a_component>(entity);
        }
    }

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<another_component>
    >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    const auto view = registry.view<a_component>();
    const auto size = view.size();

    registry.snapshot().component<another_component>(output, view.begin(), view.end());
    registry.reset();
    registry.loader().component<another_component>(input);

    ASSERT_EQ(registry.view<another_component>().size(), size);

    registry.view<another_component>().each([](const auto entity, const auto &) {
        ASSERT_TRUE(entt::to_integer(entity) % 2);
    });
}

TEST(Snapshot, Continuous) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    std::vector<entt::entity> entities;
    entt::entity entity;

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<another_component>,
        std::queue<what_a_component>,
        std::queue<what_a_map_component>,
        std::queue<double>
    >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    for(int i = 0; i < 10; ++i) {
        src.create();
    }

    src.reset();

    for(int i = 0; i < 5; ++i) {
        entity = src.create();
        entities.push_back(entity);

        src.assign<a_component>(entity);
        src.assign<another_component>(entity, i, i);

        if(i % 2) {
            src.assign<what_a_component>(entity, entity);
        } else {
            src.assign<what_a_map_component>(entity, entity);
        }
    }

    src.view<what_a_component>().each([&entities](auto, auto &what_a_component) {
        what_a_component.quux.insert(what_a_component.quux.begin(), entities.begin(), entities.end());
    });

    src.view<what_a_map_component>().each([&entities](auto, auto &what_a_map_component) {
        std::map<entt::entity, int> m1;
        std::map<int, entt::entity> m2;
        std::map<entt::entity, entt::entity> m3;
        for( size_t i = 0; i < entities.size(); ++i)
        {
            what_a_map_component.entity_keys.insert({entities[i], int(i)});
            what_a_map_component.entity_values.insert({int(i), entities[i]});
            what_a_map_component.entity_keys_and_values.insert({entities[entities.size() - i - 1], entities[i]});
        }
    });

    entity = dst.create();
    dst.assign<a_component>(entity);
    dst.assign<another_component>(entity, -1, -1);

    src.snapshot()
       .entities(output)
       .destroyed(output)
       .component<a_component, another_component, what_a_component, what_a_map_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<a_component, another_component, what_a_component, what_a_map_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
            )
        .orphans();

    decltype(dst.size()) a_component_cnt{};
    decltype(dst.size()) another_component_cnt{};
    decltype(dst.size()) what_a_component_cnt{};
    decltype(dst.size()) what_a_map_component_cnt{};

    dst.each([&dst, &a_component_cnt](auto entt) {
        ASSERT_TRUE(dst.has<a_component>(entt));
        ++a_component_cnt;
    });

    dst.view<another_component>().each([&another_component_cnt](auto, const auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : component.key);
        ++another_component_cnt;
    });

    dst.view<what_a_component>().each([&dst, &what_a_component_cnt](auto entt, const auto &component) {
        ASSERT_EQ(entt, component.bar);

        for(auto child: component.quux) {
            ASSERT_TRUE(dst.valid(child));
        }

        ++what_a_component_cnt;
    });

    dst.view<what_a_map_component>().each([&dst, &what_a_map_component_cnt](auto entt, const auto &component) {
        ASSERT_EQ(entt, component.bar);

        for(auto child: component.entity_keys) {
            ASSERT_TRUE(dst.valid(child.first));
        }
        for(auto child: component.entity_values) {
            ASSERT_TRUE(dst.valid(child.second));
        }
        for(auto child: component.entity_keys_and_values) {
            ASSERT_TRUE(dst.valid(child.first));
            ASSERT_TRUE(dst.valid(child.second));
        }
        ++what_a_map_component_cnt;
    });

    src.view<another_component>().each([](auto, auto &component) {
        component.value = 2 * component.key;
    });

    auto size = dst.size();

    src.snapshot()
        .entities(output)
        .destroyed(output)
        .component<a_component, what_a_component, what_a_map_component, another_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<a_component, what_a_component, what_a_map_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
            )
        .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);
    ASSERT_EQ(dst.size<another_component>(), another_component_cnt);
    ASSERT_EQ(dst.size<what_a_component>(), what_a_component_cnt);
    ASSERT_EQ(dst.size<what_a_map_component>(), what_a_map_component_cnt);

    dst.view<another_component>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.view<what_a_map_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.snapshot()
        .entities(output)
        .destroyed(output)
        .component<what_a_component, what_a_map_component, a_component, another_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<what_a_component, what_a_map_component, a_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
          )
        .orphans();

    dst.view<what_a_component>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    dst.view<what_a_map_component>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    entities.clear();
    for(auto entt: src.view<a_component>()) {
        entities.push_back(entt);
    }

    src.destroy(entity);
    loader.shrink();

    src.snapshot()
        .entities(output)
        .destroyed(output)
        .component<a_component, another_component, what_a_component, what_a_map_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<a_component, another_component, what_a_component, what_a_map_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
            )
        .orphans()
        .shrink();

    dst.view<what_a_component>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    dst.view<what_a_map_component>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.has(entity));

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.view<what_a_map_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.reset<a_component>();
    a_component_cnt = src.size<a_component>();

    src.snapshot()
        .entities(output)
        .destroyed(output)
        .component<a_component, what_a_component, what_a_map_component, another_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<a_component, what_a_component, what_a_map_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
            )
        .orphans();

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);

    src.reset<a_component>();
    a_component_cnt = {};

    src.snapshot()
        .entities(output)
        .destroyed(output)
        .component<what_a_component, what_a_map_component, a_component, another_component>(output);

    loader.entities(input)
        .destroyed(input)
        .component<what_a_component, what_a_map_component, a_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &what_a_map_component::bar,
            &what_a_map_component::entity_keys,
            &what_a_map_component::entity_values,
            &what_a_map_component::entity_keys_and_values
            )
        .orphans();

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);
}

TEST(Snapshot, MoreOnShrink) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>
        >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    auto entity = src.create();
    src.snapshot().entities(output);
    loader.entities(input).shrink();

    ASSERT_TRUE(dst.valid(entity));

    loader.shrink();

    ASSERT_FALSE(dst.valid(entity));
}

TEST(Snapshot, SyncDataMembers) {
    using traits_type = entt::entt_traits<std::underlying_type_t<entt::entity>>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<what_a_component>,
        std::queue<what_a_map_component>
        >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    src.create();
    src.create();

    src.reset();

    auto parent = src.create();
    auto child = src.create();


    src.assign<what_a_component>(parent, entt::null);
    src.assign<what_a_component>(child, parent).quux.push_back(child);

    src.assign<what_a_map_component>(parent, entt::null);
    auto map1 = decltype(what_a_map_component::entity_keys){{{child,10}}};
    auto map2 = decltype(what_a_map_component::entity_values){{{10,child}}};
    auto map3 = decltype(what_a_map_component::entity_keys_and_values){{{child,child}}};
    src.assign<what_a_map_component>(child, parent, map1, map2, map3);

    src.snapshot().entities(output).component<what_a_component, what_a_map_component>(output);
    loader.entities(input).component<what_a_component, what_a_map_component>(
        input,
        &what_a_component::bar,
        &what_a_component::quux,
        &what_a_map_component::bar,
        &what_a_map_component::entity_keys,
        &what_a_map_component::entity_values,
        &what_a_map_component::entity_keys_and_values
        );

    ASSERT_FALSE(dst.valid(parent));
    ASSERT_FALSE(dst.valid(child));

    ASSERT_TRUE(dst.has<what_a_component>(loader.map(parent)));
    ASSERT_TRUE(dst.has<what_a_component>(loader.map(child)));


    ASSERT_EQ(dst.get<what_a_component>(loader.map(parent)).bar, static_cast<entt::entity>(entt::null));
    ASSERT_EQ(dst.get<what_a_map_component>(loader.map(parent)).bar, static_cast<entt::entity>(entt::null));



    const auto &component = dst.get<what_a_component>(loader.map(child));

    ASSERT_EQ(component.bar, loader.map(parent));
    ASSERT_EQ(component.quux[0], loader.map(child));

    const auto &map_component = dst.get<what_a_map_component>(loader.map(child));
    ASSERT_EQ(map_component.bar, loader.map(parent));
    ASSERT_EQ(map_component.entity_keys.at(loader.map(child)), 10);
    ASSERT_EQ(map_component.entity_values.at(10), loader.map(child));
    ASSERT_EQ(map_component.entity_keys_and_values.at(loader.map(child)), loader.map(child));
}
