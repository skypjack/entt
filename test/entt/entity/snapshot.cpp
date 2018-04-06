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

    template<typename Value>
    void operator()(const Value &value) {
        std::get<std::queue<Value>>(storage).push(value);
    }

private:
    Storage &storage;
};

template<typename Storage>
struct InputArchive {
    InputArchive(Storage &storage)
        : storage{storage}
    {}

    template<typename Value>
    void operator()(Value &value) {
        auto &queue = std::get<std::queue<Value>>(storage);
        value = queue.front();
        queue.pop();
    }

private:
    Storage &storage;
};

struct AComponent {};

struct AnotherComponent {
    int key;
    int value;
};

struct Foo {
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
    registry.attach<float>(e3, .3f);

    const auto e4 = registry.create();
    registry.attach<AComponent>(e4);

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
        std::queue<Foo>
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
    registry.attach<float>(e3, .3f);

    const auto e4 = registry.create();
    registry.attach<AComponent>(e4);

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
        std::queue<Foo>
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
        std::queue<Foo>,
        std::queue<double>
    >;

    storage_type storage;
    OutputArchive<storage_type> output{storage};
    InputArchive<storage_type> input{storage};

    for(int i = 0; i < 10; ++i) {
        src.create();
    }

    src.each([&src](auto entity) {
        src.destroy(entity);
    });

    for(int i = 0; i < 5; ++i) {
        entity = src.create();
        entities.push_back(entity);

        src.assign<AComponent>(entity);
        src.assign<AnotherComponent>(entity, i, i);

        if(i % 2) {
            src.assign<Foo>(entity, entity);
        } else if(i == 2) {
            src.attach<double>(entity, .3);
        }
    }

    src.view<Foo>().each([&entities](auto, auto &foo) {
        foo.quux.insert(foo.quux.begin(), entities.begin(), entities.end());
    });

    entity = dst.create();
    dst.assign<AComponent>(entity);
    dst.assign<AnotherComponent>(entity, -1, -1);

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
            .tag<double>(input)
            .orphans();

    decltype(dst.size()) aComponentCnt{};
    decltype(dst.size()) anotherComponentCnt{};
    decltype(dst.size()) fooCnt{};

    dst.each([&dst, &aComponentCnt](auto entity) {
        ASSERT_TRUE(dst.has<AComponent>(entity));
        ++aComponentCnt;
    });

    dst.view<AnotherComponent>().each([&anotherComponentCnt](auto, const auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : component.key);
        ++anotherComponentCnt;
    });

    dst.view<Foo>().each([&dst, &fooCnt](auto entity, const auto &component) {
        ASSERT_EQ(entity, component.bar);

        for(auto entity: component.quux) {
            ASSERT_TRUE(dst.valid(entity));
        }

        ++fooCnt;
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
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
            .tag<double>(input)
            .orphans();

    ASSERT_EQ(size, dst.size());

    ASSERT_EQ(dst.size<AComponent>(), aComponentCnt);
    ASSERT_EQ(dst.size<AnotherComponent>(), anotherComponentCnt);
    ASSERT_EQ(dst.size<Foo>(), fooCnt);
    ASSERT_TRUE(dst.has<double>());

    dst.view<AnotherComponent>().each([](auto, auto &component) {
        ASSERT_EQ(component.value, component.key < 0 ? -1 : (2 * component.key));
    });

    entity = src.create();

    src.view<Foo>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
            .tag<double>(input)
            .orphans();

    dst.view<Foo>().each([&loader, entity](auto, auto &component) {
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
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
            .tag<double>(input)
            .orphans()
            .shrink();

    dst.view<Foo>().each([&dst, &loader, entity](auto, auto &component) {
        ASSERT_FALSE(dst.valid(component.bar));
    });

    ASSERT_FALSE(loader.has(entity));

    entity = src.create();

    src.view<Foo>().each([entity](auto, auto &component) {
        component.bar = entity;
    });

    dst.reset<AComponent>();
    aComponentCnt = src.size<AComponent>();

    src.snapshot()
            .entities(output)
            .destroyed(output)
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
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
            .component<AComponent, AnotherComponent, Foo>(output)
            .tag<double>(output);

    loader.entities(input)
            .destroyed(input)
            .component<AComponent, AnotherComponent>(input)
            .component<Foo>(input, &Foo::bar, &Foo::quux)
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
