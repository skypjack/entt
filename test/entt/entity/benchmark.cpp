#include <gtest/gtest.h>
#include <iostream>
#include <cstddef>
#include <chrono>
#include <vector>
#include <entt/entity/registry.hpp>

struct Position {
    uint64_t x;
    uint64_t y;
};

struct Velocity {
    uint64_t x;
    uint64_t y;
};

template<std::size_t>
struct Comp {};

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
    entt::DefaultRegistry registry;

    std::cout << "Constructing 10000000 entities" << std::endl;

    Timer timer;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create();
    }

    timer.elapsed();
}

TEST(DefaultRegistry, Destroy) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

    std::cout << "Destroying 10000000 entities" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        entities.push_back(registry.create());
    }

    Timer timer;

    for(auto entity: entities) {
        registry.destroy(entity);
    }

    timer.elapsed();
}

TEST(DefaultRegistry, IterateCreateDeleteSingleComponent) {
    entt::DefaultRegistry registry;

    std::cout << "Looping 10000 times creating and deleting a random number of entities" << std::endl;

    Timer timer;

    auto view = registry.view<Position>();

    for(int i = 0; i < 10000; i++) {
        for(int j = 0; j < 10000; j++) {
            registry.create<Position>();
        }

        for(auto entity: view) {
            if(rand() % 2 == 0) {
                registry.destroy(entity);
            }
        }
    }

    timer.elapsed();
}

TEST(DefaultRegistry, IterateSingleComponent10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, one component" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;
    registry.view<Position>().each([](auto, auto &) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponents10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponents10MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components, half of the entities have all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponents10MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components, only one entity has all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;
    registry.persistent<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10MHalf) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view, half of the entities have all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.persistent<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10MOne) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view, only one entity has all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.persistent<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateFiveComponents10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, five components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponents10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponents10MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components, half of the entities have all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponents10MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components, only one entity has all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateFiveComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    std::cout << "Iterating over 10000000 entities, five components, persistent view" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponentsPersistent10MHalf) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view, half of the entities have all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, IterateTenComponentsPersistent10MOne) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view, only one entity has all the components" << std::endl;

    for(uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(DefaultRegistry, SortSingle) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(uint64_t i = 0; i < 150000L; i++) {
        auto entity = registry.create<Position>({ i, i });
        entities.push_back(entity);
    }

    Timer timer;

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(DefaultRegistry, SortMulti) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for(uint64_t i = 0; i < 150000L; i++) {
        auto entity = registry.create<Position, Velocity>({ i, i }, { i, i });
        entities.push_back(entity);
    }

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    Timer timer;

    registry.sort<Velocity, Position>();

    timer.elapsed();
}
