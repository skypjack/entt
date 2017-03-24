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

struct Timer final {
    Timer(): start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

using registry_type = entt::DefaultRegistry<Position, Velocity>;

TEST(DefaultRegistry, Construct) {
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

TEST(DefaultRegistry, IterateSingleComponent) {
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

TEST(DefaultRegistry, IterateCreateDeleteSingleComponent) {
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

TEST(DefaultRegistry, IterateTwoComponents) {
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
