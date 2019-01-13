#include <tuple>
#include <queue>
#include <vector>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/entity.hpp>

template<typename Storage>
struct output_archive {
    output_archive(Storage &storage)
        : storage{storage}
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
    input_archive(Storage &storage)
        : storage{storage}
    {}

    template<typename... Value>
    void operator()(Value &... value) {
        auto assign = [this](auto &value) {
            auto &queue = std::get<std::queue<std::decay_t<decltype(value)>>>(storage);
            value = queue.front();
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
    entt::registry<>::entity_type bar;
    std::vector<entt::registry<>::entity_type> quux;
};

TEST(Snapshot, Dump) {
    entt::registry<> registry;

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
        std::queue<entt::registry<>::entity_type>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<another_component>
    >;

    storage_type storage;
    output_archive<storage_type> output{storage};
    input_archive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<int, char, another_component, double>(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));

    registry.loader()
            .entities(input)
            .destroyed(input)
            .component<int, char, another_component, double>(input)
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

    ASSERT_TRUE(registry.empty<another_component>());
}

TEST(Snapshot, Partial) {
    entt::registry<> registry;

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
        std::queue<entt::registry<>::entity_type>,
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
    entt::registry<> registry;

    for(auto i = 0; i < 50; ++i) {
        const auto entity = registry.create();
        registry.assign<another_component>(entity, i, i);

        if(i % 2) {
            registry.assign<a_component>(entity);
        }
    }

    using storage_type = std::tuple<
        std::queue<entt::registry<>::entity_type>,
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
        ASSERT_TRUE(entity % 2);
    });
}

TEST(Snapshot, Continuous) {
    using entity_type = entt::registry<>::entity_type;

    entt::registry<> src;
    entt::registry<> dst;

    entt::continuous_loader<entity_type> loader{dst};

    std::vector<entity_type> entities;
    entity_type entity;

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<a_component>,
        std::queue<another_component>,
        std::queue<what_a_component>,
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
        }
    }

    src.view<what_a_component>().each([&entities](auto, auto &what_a_component) {
        what_a_component.quux.insert(what_a_component.quux.begin(), entities.begin(), entities.end());
    });

    entity = dst.create();
    dst.assign<a_component>(entity);
    dst.assign<another_component>(entity, -1, -1);

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<a_component, another_component, what_a_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<a_component, another_component, what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans();

    decltype(dst.size()) a_component_cnt{};
    decltype(dst.size()) another_component_cnt{};
    decltype(dst.size()) what_a_component_cnt{};

    dst.each([&dst, &a_component_cnt](auto entity) {
        ASSERT_TRUE(dst.has<a_component>(entity));
        ++a_component_cnt;
    });

    dst.view<another_component>().each([&another_component_cnt](auto, const auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : component.key);
        ++another_component_cnt;
    });

    dst.view<what_a_component>().each([&dst, &what_a_component_cnt](auto entity, const auto &component) {
        ASSERT_EQ(entity, component.bar);

        for(auto entity: component.quux) {
            ASSERT_TRUE(dst.valid(entity));
        }

        ++what_a_component_cnt;
    });

    src.view<another_component>().each([](auto, auto &component) {
        component.value = 2 * component.key;
    });

    auto size = dst.size();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<a_component, what_a_component, another_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<a_component, what_a_component, another_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);
    ASSERT_EQ(dst.size<another_component>(), another_component_cnt);
    ASSERT_EQ(dst.size<what_a_component>(), what_a_component_cnt);

    dst.view<another_component>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<what_a_component, a_component, another_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<what_a_component, a_component, another_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans();

    dst.view<what_a_component>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    entities.clear();
    for(auto entity: src.view<a_component>()) {
        entities.push_back(entity);
    }

    src.destroy(entity);
    loader.shrink();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<a_component, another_component, what_a_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<a_component, another_component, what_a_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans()
            .shrink();

    dst.view<what_a_component>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.has(entity));

    entity = src.create();

    src.view<what_a_component>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.reset<a_component>();
    a_component_cnt = src.size<a_component>();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<a_component, what_a_component, another_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<a_component, what_a_component, another_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans();

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);

    src.reset<a_component>();
    a_component_cnt = {};

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<what_a_component, a_component, another_component>(output);

    loader.entities(input)
            .destroyed(input)
            .component<what_a_component, a_component, another_component>(input, &what_a_component::bar, &what_a_component::quux)
            .orphans();

    ASSERT_EQ(dst.size<a_component>(), a_component_cnt);
}

TEST(Snapshot, MoreOnShrink) {
    using entity_type = entt::registry<>::entity_type;

    entt::registry<> src;
    entt::registry<> dst;

    entt::continuous_loader<entity_type> loader{dst};

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<a_component>
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
    using entity_type = entt::registry<>::entity_type;

    entt::registry<> src;
    entt::registry<> dst;

    entt::continuous_loader<entity_type> loader{dst};

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<what_a_component>
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

    src.snapshot().entities(output).component<what_a_component>(output);
    loader.entities(input).component<what_a_component>(input, &what_a_component::bar, &what_a_component::quux);

    ASSERT_FALSE(dst.valid(parent));
    ASSERT_FALSE(dst.valid(child));

    ASSERT_TRUE(dst.has<what_a_component>(loader.map(parent)));
    ASSERT_TRUE(dst.has<what_a_component>(loader.map(child)));

    ASSERT_EQ(dst.get<what_a_component>(loader.map(parent)).bar, static_cast<entity_type>(entt::null));

    const auto &component = dst.get<what_a_component>(loader.map(child));

    ASSERT_EQ(component.bar, loader.map(parent));
    ASSERT_EQ(component.quux[0], loader.map(child));
}
