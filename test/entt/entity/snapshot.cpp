#include <cstddef>
#include <map>
#include <queue>
#include <tuple>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>

struct noncopyable_component {
    noncopyable_component()
        : value{} {}

    explicit noncopyable_component(int v)
        : value{v} {}

    noncopyable_component(const noncopyable_component &) = delete;
    noncopyable_component(noncopyable_component &&) = default;

    noncopyable_component &operator=(const noncopyable_component &) = delete;
    noncopyable_component &operator=(noncopyable_component &&) = default;

    int value;
};

template<typename Storage>
struct output_archive {
    output_archive(Storage &instance)
        : storage{instance} {}

    template<typename... Value>
    void operator()(const Value &...value) {
        (std::get<std::queue<Value>>(storage).push(value), ...);
    }

    void operator()(const entt::entity &entity, const noncopyable_component &instance) {
        (*this)(entity, instance.value);
    }

private:
    Storage &storage;
};

template<typename Storage>
struct input_archive {
    input_archive(Storage &instance)
        : storage{instance} {}

    template<typename... Value>
    void operator()(Value &...value) {
        auto assign = [this](auto &val) {
            auto &queue = std::get<std::queue<std::decay_t<decltype(val)>>>(storage);
            val = queue.front();
            queue.pop();
        };

        (assign(value), ...);
    }

    void operator()(entt::entity &entity, noncopyable_component &instance) {
        (*this)(entity, instance.value);
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

struct map_component {
    std::map<entt::entity, int> keys;
    std::map<int, entt::entity> values;
    std::map<entt::entity, entt::entity> both;
};

TEST(Snapshot, Dump) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);
    registry.emplace<char>(e0, 'c');
    registry.emplace<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 3);

    const auto e3 = registry.create();
    registry.emplace<a_component>(e3);
    registry.emplace<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<a_component>,
        std::queue<another_component>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    entt::snapshot{registry}.entities(output).component<int, char, double, a_component, another_component>(output);
    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}.entities(input).component<int, char, double, a_component, another_component>(input).orphans();

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
    ASSERT_TRUE(registry.all_of<a_component>(e3));

    ASSERT_TRUE(registry.storage<another_component>().empty());
}

TEST(Snapshot, Partial) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    const auto e0 = registry.create();
    registry.emplace<int>(e0, 42);
    registry.emplace<char>(e0, 'c');
    registry.emplace<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.emplace<int>(e2, 3);

    const auto e3 = registry.create();
    registry.emplace<char>(e3, '0');

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    entt::snapshot{registry}.entities(output).component<char, int>(output);
    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}.entities(input).component<char, int>(input);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_FALSE(registry.all_of<double>(e0));
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');

    entt::snapshot{registry}.entities(output);
    registry.clear();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    entt::snapshot_loader{registry}.entities(input).orphans();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
}

TEST(Snapshot, Iterator) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;

    for(auto i = 0; i < 50; ++i) {
        const auto entity = registry.create();
        registry.emplace<another_component>(entity, i, i);
        registry.emplace<noncopyable_component>(entity, i);

        if(i % 2) {
            registry.emplace<a_component>(entity);
        }
    }

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<another_component>,
        std::queue<int>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    const auto view = registry.view<a_component>();
    const auto size = view.size();

    entt::snapshot{registry}.component<another_component, noncopyable_component>(output, view.begin(), view.end());
    registry.clear();
    entt::snapshot_loader{registry}.component<another_component, noncopyable_component>(input);

    ASSERT_EQ(registry.view<another_component>().size(), size);

    registry.view<another_component>().each([](const auto entity, const auto &) {
        ASSERT_NE(entt::to_integral(entity) % 2u, 0u);
    });
}

TEST(Snapshot, Continuous) {
    using traits_type = entt::entt_traits<entt::entity>;

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
        std::queue<map_component>,
        std::queue<int>,
        std::queue<double>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    for(int i = 0; i < 10; ++i) {
        static_cast<void>(src.create());
    }

    src.clear();

    for(int i = 0; i < 5; ++i) {
        entity = src.create();
        entities.push_back(entity);

        src.emplace<a_component>(entity);
        src.emplace<another_component>(entity, i, i);
        src.emplace<noncopyable_component>(entity, i);

        if(i % 2) {
            src.emplace<what_a_component>(entity, entity);
        } else {
            src.emplace<map_component>(entity);
        }
    }

    src.view<what_a_component>().each([&entities](auto, auto &what_a_component) {
        what_a_component.quux.insert(what_a_component.quux.begin(), entities.begin(), entities.end());
    });

    src.view<map_component>().each([&entities](auto, auto &map_component) {
        for(std::size_t i = 0; i < entities.size(); ++i) {
            map_component.keys.insert({entities[i], int(i)});
            map_component.values.insert({int(i), entities[i]});
            map_component.both.insert({entities[entities.size() - i - 1], entities[i]});
        }
    });

    entity = dst.create();
    dst.emplace<a_component>(entity);
    dst.emplace<another_component>(entity, -1, -1);
    dst.emplace<noncopyable_component>(entity, -1);

    entt::snapshot{src}.entities(output).component<a_component, another_component, what_a_component, map_component, noncopyable_component>(output);

    loader.entities(input)
        .component<a_component, another_component, what_a_component, map_component, noncopyable_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans();

    decltype(dst.size()) a_component_cnt{};
    decltype(dst.size()) another_component_cnt{};
    decltype(dst.size()) what_a_component_cnt{};
    decltype(dst.size()) map_component_cnt{};
    decltype(dst.size()) noncopyable_component_cnt{};

    dst.each([&dst, &a_component_cnt](auto entt) {
        ASSERT_TRUE(dst.all_of<a_component>(entt));
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

    dst.view<map_component>().each([&dst, &map_component_cnt](const auto &component) {
        for(auto child: component.keys) {
            ASSERT_TRUE(dst.valid(child.first));
        }

        for(auto child: component.values) {
            ASSERT_TRUE(dst.valid(child.second));
        }

        for(auto child: component.both) {
            ASSERT_TRUE(dst.valid(child.first));
            ASSERT_TRUE(dst.valid(child.second));
        }

        ++map_component_cnt;
    });

    dst.view<noncopyable_component>().each([&dst, &noncopyable_component_cnt](auto, const auto &component) {
        ++noncopyable_component_cnt;
        ASSERT_EQ(component.value, static_cast<int>(dst.storage<noncopyable_component>().size() - noncopyable_component_cnt - 1u));
    });

    src.view<another_component>().each([](auto, auto &component) {
        component.value = 2 * component.key;
    });

    auto size = dst.size();

    entt::snapshot{src}.entities(output).component<a_component, what_a_component, map_component, another_component>(output);

    loader.entities(input)
        .component<a_component, what_a_component, map_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);
    ASSERT_EQ(dst.storage<another_component>().size(), another_component_cnt);
    ASSERT_EQ(dst.storage<what_a_component>().size(), what_a_component_cnt);
    ASSERT_EQ(dst.storage<map_component>().size(), map_component_cnt);
    ASSERT_EQ(dst.storage<noncopyable_component>().size(), noncopyable_component_cnt);

    dst.view<another_component>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    entt::snapshot{src}.entities(output).component<what_a_component, map_component, a_component, another_component>(output);

    loader.entities(input)
        .component<what_a_component, map_component, a_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans();

    dst.view<what_a_component>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    entities.clear();
    for(auto entt: src.view<a_component>()) {
        entities.push_back(entt);
    }

    src.destroy(entity);
    loader.shrink();

    entt::snapshot{src}.entities(output).component<a_component, another_component, what_a_component, map_component>(output);

    loader.entities(input)
        .component<a_component, another_component, what_a_component, map_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans()
        .shrink();

    dst.view<what_a_component>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.contains(entity));

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.clear<a_component>();
    a_component_cnt = src.storage<a_component>().size();

    entt::snapshot{src}.entities(output).component<a_component, what_a_component, map_component, another_component>(output);

    loader.entities(input)
        .component<a_component, what_a_component, map_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans();

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);

    src.clear<a_component>();
    a_component_cnt = {};

    entt::snapshot{src}.entities(output).component<what_a_component, map_component, a_component, another_component>(output);

    loader.entities(input)
        .component<what_a_component, map_component, a_component, another_component>(
            input,
            &what_a_component::bar,
            &what_a_component::quux,
            &map_component::keys,
            &map_component::values,
            &map_component::both)
        .orphans();

    ASSERT_EQ(dst.storage<a_component>().size(), a_component_cnt);
}

TEST(Snapshot, MoreOnShrink) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    auto entity = src.create();
    entt::snapshot{src}.entities(output);
    loader.entities(input).shrink();

    ASSERT_TRUE(dst.valid(entity));

    loader.shrink();

    ASSERT_FALSE(dst.valid(entity));
}

TEST(Snapshot, SyncDataMembers) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry src;
    entt::registry dst;

    entt::continuous_loader loader{dst};

    using storage_type = std::tuple<
        std::queue<typename traits_type::entity_type>,
        std::queue<entt::entity>,
        std::queue<what_a_component>,
        std::queue<map_component>>;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    static_cast<void>(src.create());
    static_cast<void>(src.create());

    src.clear();

    auto parent = src.create();
    auto child = src.create();

    src.emplace<what_a_component>(parent, entt::null);
    src.emplace<what_a_component>(child, parent).quux.push_back(child);

    src.emplace<map_component>(
        child,
        decltype(map_component::keys){{{child, 10}}},
        decltype(map_component::values){{{10, child}}},
        decltype(map_component::both){{{child, child}}});

    entt::snapshot{src}.entities(output).component<what_a_component, map_component>(output);

    loader.entities(input).component<what_a_component, map_component>(
        input,
        &what_a_component::bar,
        &what_a_component::quux,
        &map_component::keys,
        &map_component::values,
        &map_component::both);

    ASSERT_FALSE(dst.valid(parent));
    ASSERT_FALSE(dst.valid(child));

    ASSERT_TRUE(dst.all_of<what_a_component>(loader.map(parent)));
    ASSERT_TRUE(dst.all_of<what_a_component>(loader.map(child)));

    ASSERT_EQ(dst.get<what_a_component>(loader.map(parent)).bar, static_cast<entt::entity>(entt::null));

    const auto &component = dst.get<what_a_component>(loader.map(child));

    ASSERT_EQ(component.bar, loader.map(parent));
    ASSERT_EQ(component.quux[0], loader.map(child));

    const auto &foobar = dst.get<map_component>(loader.map(child));
    ASSERT_EQ(foobar.keys.at(loader.map(child)), 10);
    ASSERT_EQ(foobar.values.at(10), loader.map(child));
    ASSERT_EQ(foobar.both.at(loader.map(child)), loader.map(child));
}
