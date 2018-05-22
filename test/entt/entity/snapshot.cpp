#include <tuple>
#include <queue>
#include <vector>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

template<typename Storage>
struct OutputArchive {
    OutputArchive(Storage &storage)
        : storage{storage}
    {}

    template<typename... Value>
    void operator()(const Value &... value) {
        using accumulator_type = int[];
        accumulator_type accumulator = { (std::get<std::queue<Value>>(storage).push(value), 0)... };
        (void)accumulator;
    }

private:
    Storage &storage;
};

template<typename Storage>
struct InputArchive {
    InputArchive(Storage &storage)
        : storage{storage}
    {}

    template<typename... Value>
    void operator()(Value &... value) {
        auto assign = [this](auto &value) {
            auto &queue = std::get<std::queue<std::decay_t<decltype(value)>>>(storage);
            value = queue.front();
            queue.pop();
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { (assign(value), 0)... };
        (void)accumulator;
    }

private:
    Storage &storage;
};

struct AComponent {};

struct AnotherComponent {
    int key;
    int value;
};

struct WhatAComponent {
    entt::DefaultRegistry::entity_type bar;
    std::vector<entt::DefaultRegistry::entity_type> quux;
};

TEST(Snapshot, Dump) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    const auto e3 = registry.create();
    registry.assign<char>(e3, '0');
    registry.assign<float>(entt::tag_t{}, e3, .3f);

    const auto e4 = registry.create();
    registry.assign<AComponent>(entt::tag_t{}, e4);

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<entt::DefaultRegistry::entity_type>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<float>,
        std::queue<bool>,
        std::queue<AComponent>,
        std::queue<AnotherComponent>,
        std::queue<WhatAComponent>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<int, char, AnotherComponent, double>(output)
            .tag<float, bool, AComponent>(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .entities(input)
            .destroyed(input)
            .component<int, char, AnotherComponent, double>(input)
            .tag<float, bool, AComponent>(input)
            .orphans();

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_TRUE(registry.valid(e4));

    ASSERT_FALSE(registry.orphan(e0));
    ASSERT_FALSE(registry.orphan(e2));
    ASSERT_FALSE(registry.orphan(e3));
    ASSERT_FALSE(registry.orphan(e4));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_EQ(registry.get<double>(e0), .1);
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');

    ASSERT_TRUE(registry.has<float>());
    ASSERT_EQ(registry.attachee<float>(), e3);
    ASSERT_EQ(registry.get<float>(), .3f);

    ASSERT_TRUE(registry.has<AComponent>());
    ASSERT_EQ(registry.attachee<AComponent>(), e4);

    ASSERT_TRUE(registry.empty<AnotherComponent>());
    ASSERT_FALSE(registry.has<long int>());
}

TEST(Snapshot, Partial) {
    entt::DefaultRegistry registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 42);
    registry.assign<char>(e0, 'c');
    registry.assign<double>(e0, .1);

    const auto e1 = registry.create();

    const auto e2 = registry.create();
    registry.assign<int>(e2, 3);

    const auto e3 = registry.create();
    registry.assign<char>(e3, '0');
    registry.assign<float>(entt::tag_t{}, e3, .3f);

    const auto e4 = registry.create();
    registry.assign<AComponent>(entt::tag_t{}, e4);

    registry.destroy(e1);
    auto v1 = registry.current(e1);

    using storage_type = std::tuple<
        std::queue<entt::DefaultRegistry::entity_type>,
        std::queue<int>,
        std::queue<char>,
        std::queue<double>,
        std::queue<float>,
        std::queue<bool>,
        std::queue<AComponent>,
        std::queue<WhatAComponent>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    registry.snapshot()
            .entities(output)
            .destroyed(output)
            .component<char, int>(output)
            .tag<bool, float>(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .entities(input)
            .destroyed(input)
            .component<char, int>(input)
            .tag<bool, float>(input);

    ASSERT_TRUE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_TRUE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_TRUE(registry.valid(e4));

    ASSERT_EQ(registry.get<int>(e0), 42);
    ASSERT_EQ(registry.get<char>(e0), 'c');
    ASSERT_FALSE(registry.has<double>(e0));
    ASSERT_EQ(registry.current(e1), v1);
    ASSERT_EQ(registry.get<int>(e2), 3);
    ASSERT_EQ(registry.get<char>(e3), '0');
    ASSERT_TRUE(registry.orphan(e4));

    ASSERT_TRUE(registry.has<float>());
    ASSERT_EQ(registry.attachee<float>(), e3);
    ASSERT_EQ(registry.get<float>(), .3f);
    ASSERT_FALSE(registry.has<long int>());

    registry.snapshot()
            .tag<float>(output)
            .destroyed(output)
            .entities(output);

    registry.reset();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_FALSE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));

    registry.restore()
            .tag<float>(input)
            .destroyed(input)
            .entities(input)
            .orphans();

    ASSERT_FALSE(registry.valid(e0));
    ASSERT_FALSE(registry.valid(e1));
    ASSERT_FALSE(registry.valid(e2));
    ASSERT_TRUE(registry.valid(e3));
    ASSERT_FALSE(registry.valid(e4));
}

TEST(Snapshot, Iterator) {
    entt::DefaultRegistry registry;

    for(auto i = 0; i < 50; ++i) {
        const auto entity = registry.create();
        registry.assign<AnotherComponent>(entity, i, i);

        if(i % 2) {
            registry.assign<AComponent>(entity);
        }
    }

    using storage_type = std::tuple<
        std::queue<entt::DefaultRegistry::entity_type>,
        std::queue<AnotherComponent>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    const auto view = registry.view<AComponent>();
    const auto size = view.size();

    registry.snapshot().component<AnotherComponent>(output, view.cbegin(), view.cend());
    registry.reset();
    registry.restore().component<AnotherComponent>(input);

    ASSERT_EQ(registry.view<AnotherComponent>().size(), size);

    registry.view<AnotherComponent>().each([](const auto entity, auto &&...) {
        ASSERT_TRUE(entity % 2);
    });
}

TEST(Snapshot, Continuous) {
    using entity_type = entt::DefaultRegistry::entity_type;

    entt::DefaultRegistry src;
    entt::DefaultRegistry dst;

    entt::ContinuousLoader<entity_type> loader{dst};

    std::vector<entity_type> entities;
    entity_type entity;

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<AComponent>,
        std::queue<AnotherComponent>,
        std::queue<WhatAComponent>,
        std::queue<double>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    for(int i = 0; i < 10; ++i) {
        src.create();
    }

    src.reset();

    for(int i = 0; i < 5; ++i) {
        entity = src.create();
        entities.push_back(entity);

        src.assign<AComponent>(entity);
        src.assign<AnotherComponent>(entity, i, i);

        if(i % 2) {
            src.assign<WhatAComponent>(entity, entity);
        } else if(i == 2) {
            src.assign<double>(entt::tag_t{}, entity, .3);
        }
    }

    src.view<WhatAComponent>().each([&entities](auto, auto &whatAComponent) {
        whatAComponent.quux.insert(whatAComponent.quux.begin(), entities.begin(), entities.end());
    });

    entity = dst.create();
    dst.assign<AComponent>(entity);
    dst.assign<AnotherComponent>(entity, -1, -1);

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, AnotherComponent, WhatAComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent, WhatAComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans();

    decltype(dst.size()) aComponentCnt{};
    decltype(dst.size()) anotherComponentCnt{};
    decltype(dst.size()) whatAComponentCnt{};

    dst.each([&dst, &aComponentCnt](auto entity) {
        ASSERT_TRUE(dst.has<AComponent>(entity));
        ++aComponentCnt;
    });

    dst.view<AnotherComponent>().each([&anotherComponentCnt](auto, const auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : component.key);
        ++anotherComponentCnt;
    });

    dst.view<WhatAComponent>().each([&dst, &whatAComponentCnt](auto entity, const auto &component) {
        ASSERT_EQ(entity, component.bar);

        for(auto entity: component.quux) {
            ASSERT_TRUE(dst.valid(entity));
        }

        ++whatAComponentCnt;
    });

    ASSERT_TRUE(dst.has<double>());
    ASSERT_EQ(dst.get<double>(), .3);

    src.view<AnotherComponent>().each([](auto, auto &component) {
        component.value = 2 * component.key;
    });

    auto size = dst.size();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, WhatAComponent, AnotherComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, WhatAComponent, AnotherComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.size<AComponent>(), aComponentCnt);
    ASSERT_EQ(dst.size<AnotherComponent>(), anotherComponentCnt);
    ASSERT_EQ(dst.size<WhatAComponent>(), whatAComponentCnt);
    ASSERT_TRUE(dst.has<double>());

    dst.view<AnotherComponent>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<WhatAComponent>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<WhatAComponent, AComponent, AnotherComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<WhatAComponent, AComponent, AnotherComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans();

    dst.view<WhatAComponent>().each([&loader, entity](auto, auto &component) {
        ASSERT_EQ(component.bar, loader.map(entity));
    });

    entities.clear();
    for(auto entity: src.view<AComponent>()) {
        entities.push_back(entity);
    }

    src.destroy(entity);
    loader.shrink();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, AnotherComponent, WhatAComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent, WhatAComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans()
            .shrink();

    dst.view<WhatAComponent>().each([&dst](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.has(entity));

    entity = src.create();

    src.view<WhatAComponent>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.reset<AComponent>();
    aComponentCnt = src.size<AComponent>();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, WhatAComponent, AnotherComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, WhatAComponent, AnotherComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans();

    ASSERT_EQ(dst.size<AComponent>(), aComponentCnt);
    ASSERT_TRUE(dst.has<double>());

    src.reset<AComponent>();
    src.remove<double>();
    aComponentCnt = {};

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<WhatAComponent, AComponent, AnotherComponent>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<WhatAComponent, AComponent, AnotherComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<double>(input)
            .orphans();

    ASSERT_EQ(dst.size<AComponent>(), aComponentCnt);
    ASSERT_FALSE(dst.has<double>());
}

TEST(Snapshot, ContinuousMoreOnShrink) {
    using entity_type = entt::DefaultRegistry::entity_type;

    entt::DefaultRegistry src;
    entt::DefaultRegistry dst;

    entt::ContinuousLoader<entity_type> loader{dst};

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<AComponent>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    auto entity = src.create();
    src.snapshot().entities(output);
    loader.entities(input).shrink();

    ASSERT_TRUE(dst.valid(entity));

    loader.shrink();

    ASSERT_FALSE(dst.valid(entity));
}

TEST(Snapshot, SyncDataMembers) {
    using entity_type = entt::DefaultRegistry::entity_type;

    entt::DefaultRegistry src;
    entt::DefaultRegistry dst;

    entt::ContinuousLoader<entity_type> loader{dst};

    using storage_type = std::tuple<
        std::queue<entity_type>,
        std::queue<WhatAComponent>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    src.create();
    src.create();

    src.reset();

    auto parent = src.create();
    auto child = src.create();

    src.assign<WhatAComponent>(entt::tag_t{}, child, parent).quux.push_back(parent);
    src.assign<WhatAComponent>(child, child).quux.push_back(child);

    src.snapshot().entities(output)
            .component<WhatAComponent>(output)
            .tag<WhatAComponent>(output);

    loader.entities(input)
            .component<WhatAComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux)
            .tag<WhatAComponent>(input, &WhatAComponent::bar, &WhatAComponent::quux);

    ASSERT_FALSE(dst.valid(parent));
    ASSERT_FALSE(dst.valid(child));

    ASSERT_TRUE(dst.has<WhatAComponent>());
    ASSERT_EQ(dst.attachee<WhatAComponent>(), loader.map(child));
    ASSERT_EQ(dst.get<WhatAComponent>().bar, loader.map(parent));
    ASSERT_EQ(dst.get<WhatAComponent>().quux[0], loader.map(parent));
    ASSERT_TRUE(dst.has<WhatAComponent>(loader.map(child)));

    const auto &component = dst.get<WhatAComponent>(loader.map(child));

    ASSERT_EQ(component.bar, loader.map(child));
    ASSERT_EQ(component.quux[0], loader.map(child));
}
