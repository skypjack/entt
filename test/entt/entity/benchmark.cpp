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

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create();
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, Destroy) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

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
    registry.reset();
}

TEST(DefaultRegistry, IterateSingleComponent10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, one component" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;

    auto view = registry.view<Position>();

    for(auto entity: view) {
        auto &position = view.get(entity);
        (void)position;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents10MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, two components, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10MHalf) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent10MOne) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 10000000 entities, two components, persistent view, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateSingleComponent50M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 50000000 entities, one component" << std::endl;

    for (uint64_t i = 0; i < 50000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;

    auto view = registry.view<Position>();

    for(auto entity: view) {
        auto &position = view.get(entity);
        (void)position;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponents50M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 50000000 entities, two components" << std::endl;

    for (uint64_t i = 0; i < 50000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTwoComponentsPersistent50M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 50000000 entities, two components, persistent view" << std::endl;

    for (uint64_t i = 0; i < 50000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        (void)position;
        (void)velocity;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateFiveComponents10M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, five components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
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
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 10000000 entities, ten components, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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

TEST(DefaultRegistry, IterateFiveComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    std::cout << "Iterating over 10000000 entities, five components, persistent view" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        (void)position;
        (void)velocity;
        (void)comp1;
        (void)comp2;
        (void)comp3;
    }

    timer.elapsed();
    registry.reset();
}

TEST(DefaultRegistry, IterateTenComponentsPersistent10M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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

TEST(DefaultRegistry, IterateTenComponentsPersistent10MHalf) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view, half of the entities have all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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

TEST(DefaultRegistry, IterateTenComponentsPersistent10MOne) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 10000000 entities, ten components, persistent view, only one entity has all the components" << std::endl;

    for (uint64_t i = 0; i < 10000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;

    auto view = registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        auto &comp1 = view.get<Comp<1>>(entity);
        auto &comp2 = view.get<Comp<2>>(entity);
        auto &comp3 = view.get<Comp<3>>(entity);
        auto &comp4 = view.get<Comp<4>>(entity);
        auto &comp5 = view.get<Comp<5>>(entity);
        auto &comp6 = view.get<Comp<6>>(entity);
        auto &comp7 = view.get<Comp<7>>(entity);
        auto &comp8 = view.get<Comp<8>>(entity);
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

TEST(DefaultRegistry, SortSingle) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for (uint64_t i = 0; i < 150000L; i++) {
        auto entity = registry.create();
        entities.push_back(entity);
        registry.assign<Position>(entity, i, i);
    }

    Timer timer;

    registry.sort<Position>([&registry](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(DefaultRegistry, SortMulti) {
    entt::DefaultRegistry registry;
    std::vector<entt::DefaultRegistry::entity_type> entities{};

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for (uint64_t i = 0; i < 150000L; i++) {
        auto entity = registry.create();
        entities.push_back(entity);
        registry.assign<Position>(entity, i, i);
        registry.assign<Velocity>(entity, i, i);
    }

    registry.sort<Position>([&registry](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    Timer timer;

    registry.sort<Velocity, Position>();

    timer.elapsed();
}
