#include <gtest/gtest.h>
#include <registry.hpp>
#include <iostream>
#include <cstddef>
#include <chrono>
#include <vector>

struct Position {
    uint64_t x;
    uint64_t y;
};

struct Velocity {
    uint64_t x;
    uint64_t y;
};

struct Comp1 {};
struct Comp2 {};
struct Comp3 {};
struct Comp4 {};
struct Comp5 {};
struct Comp6 {};
struct Comp7 {};
struct Comp8 {};

struct Timer final {
    Timer(): start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

TEST(DefaultRegistry, Construct) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Constructing 10000000 entities" << std::endl;

    Timer timer;
    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create();
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, Destroy) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;
    std::vector<registry_type::entity_type> entities{};

    std::cout << "Destroying 10000000 entities" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        entities.push_back(registry.create());
    }

    Timer timer;

    for (auto entity: entities) {
        registry.destroy(entity);
    }

    timer.elapsed();
}

TEST(DefaultRegistry, IterateCreateDeleteSingleComponent) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Looping 10000 times creating and deleting a random number of entities" << std::endl;

    Timer timer;

    for(int i = 0; i < 10000; i++) {
        for(int j = 0; j < 10000; j++) {
            registry.create<Position>();
        }

        auto view = registry.view<Position>();

        for(auto entity: view) {
            if(rand() % 2 == 0) {
                registry.destroy(entity);
            }
        }
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateSingleComponent10M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, one component" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;

    auto view = registry.view<Position>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        (void)position;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, two components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10MHalf) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, two components, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10MOne) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, two components, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateSingleComponent50M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 50000000 entities, one component" << std::endl;

    for (uint64_t i = 0; i < 50000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;

    auto view = registry.view<Position>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        (void)position;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents50M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity>;

    registry_type registry;

    std::cout << "Iterating over 50000000 entities, two components" << std::endl;

    for (uint64_t i = 0; i < 50000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateFiveComponents10M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity, Comp1, Comp2, Comp3>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, five components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp1, Comp2, Comp3>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        auto &comp1 = registry.get<Comp1>(entity);
        auto &comp2 = registry.get<Comp2>(entity);
        auto &comp3 = registry.get<Comp3>(entity);
        (void)position;
        (void)velocity;
        (void)comp1;
        (void)comp2;
        (void)comp3;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTenComponents10M) {
    using registry_type = entt::DefaultRegistry<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, ten components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        auto &comp1 = registry.get<Comp1>(entity);
        auto &comp2 = registry.get<Comp2>(entity);
        auto &comp3 = registry.get<Comp3>(entity);
        auto &comp4 = registry.get<Comp4>(entity);
        auto &comp5 = registry.get<Comp5>(entity);
        auto &comp6 = registry.get<Comp6>(entity);
        auto &comp7 = registry.get<Comp7>(entity);
        auto &comp8 = registry.get<Comp8>(entity);
        (void)position;
        (void)velocity;
        (void)comp1;
        (void)comp2;
        (void)comp3;
        (void)comp4;
        (void)comp5;
        (void)comp6;
        (void)comp7;
        (void)comp8;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTenComponents10MHalf) {
    using registry_type = entt::DefaultRegistry<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, ten components, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        auto &comp1 = registry.get<Comp1>(entity);
        auto &comp2 = registry.get<Comp2>(entity);
        auto &comp3 = registry.get<Comp3>(entity);
        auto &comp4 = registry.get<Comp4>(entity);
        auto &comp5 = registry.get<Comp5>(entity);
        auto &comp6 = registry.get<Comp6>(entity);
        auto &comp7 = registry.get<Comp7>(entity);
        auto &comp8 = registry.get<Comp8>(entity);
        (void)position;
        (void)velocity;
        (void)comp1;
        (void)comp2;
        (void)comp3;
        (void)comp4;
        (void)comp5;
        (void)comp6;
        (void)comp7;
        (void)comp8;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTenComponents10MOne) {
    using registry_type = entt::DefaultRegistry<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>;

    registry_type registry;

    std::cout << "Iterating over 10000000 entities, ten components, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        auto &comp1 = registry.get<Comp1>(entity);
        auto &comp2 = registry.get<Comp2>(entity);
        auto &comp3 = registry.get<Comp3>(entity);
        auto &comp4 = registry.get<Comp4>(entity);
        auto &comp5 = registry.get<Comp5>(entity);
        auto &comp6 = registry.get<Comp6>(entity);
        auto &comp7 = registry.get<Comp7>(entity);
        auto &comp8 = registry.get<Comp8>(entity);
        (void)position;
        (void)velocity;
        (void)comp1;
        (void)comp2;
        (void)comp3;
        (void)comp4;
        (void)comp5;
        (void)comp6;
        (void)comp7;
        (void)comp8;
    }

    timer.elapsed();
    registry.reset();
}
