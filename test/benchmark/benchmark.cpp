#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/space.hpp>

struct Position {
    std::uint64_t x;
    std::uint64_t y;
};

struct Velocity {
    std::uint64_t x;
    std::uint64_t y;
};

template<std::size_t>
struct Comp { int x; };

struct Timer final {
    Timer(): start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

TEST(Benchmark, Construct) {
    entt::DefaultRegistry registry;

    std::cout << "Constructing 1000000 entities" << std::endl;

    Timer timer;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create();
    }

    timer.elapsed();
}

TEST(Benchmark, Destroy) {
    entt::DefaultRegistry registry;

    std::cout << "Destroying 1000000 entities" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create();
    }

    Timer timer;

    for(auto entity: registry.view<int>()) {
        registry.destroy(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, IterateCreateDeleteSingleComponent) {
    entt::DefaultRegistry registry;

    std::cout << "Looping 10000 times creating and deleting a random number of entities" << std::endl;

    Timer timer;

    auto view = registry.view<Position>();

    for(int i = 0; i < 10000; i++) {
        for(int j = 0; j < 10000; j++) {
            registry.create<Position>();
        }

        view.each([&registry](auto entity, auto &&...) {
            if(rand() % 2 == 0) {
                registry.destroy(entity);
            }
        });
    }

    timer.elapsed();
}

TEST(Benchmark, IterateSingleComponent1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position>();
    }

    Timer timer;
    registry.view<Position>().each([](auto, auto &) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTwoComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTwoComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTwoComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTwoComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 1000000 entities, two components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity>();
    }

    Timer timer;
    registry.persistent<Position, Velocity>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateFiveComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateFiveComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateFiveComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateFiveComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    std::cout << "Iterating over 1000000 entities, five components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTenComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTenComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i % 2) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTenComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        auto entity = registry.create<Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        if(i == 5000000L) { registry.assign<Position>(entity); }
    }

    Timer timer;
    registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, IterateTenComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 1000000 entities, ten components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
    }

    Timer timer;
    registry.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SortSingle) {
    entt::DefaultRegistry registry;

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        registry.create<Position>({ i, i });
    }

    Timer timer;

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, SortMulti) {
    entt::DefaultRegistry registry;

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        registry.create<Position, Velocity>({ i, i }, { i, i });
    }

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    Timer timer;

    registry.sort<Velocity, Position>();

    timer.elapsed();
}

TEST(Benchmark, SpaceConstruct) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Constructing 1000000 entities" << std::endl;

    Timer timer;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        space.create();
    }

    timer.elapsed();
}

TEST(Benchmark, SpaceAssign) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Assigning 1000000 entities" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create<int>();
    }

    Timer timer;

    for(auto entity: registry.view<int>()) {
        space.assign(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, SpaceIterateSingleComponent1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position>();
        space.assign(entity);
    }

    Timer timer;
    space.view<Position>([](auto, auto &) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateTwoComponents1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity>();
        space.assign(entity);
    }

    Timer timer;
    space.view<Position, Velocity>([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateTwoComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 1000000 entities, two components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity>();
        space.assign(entity);
    }

    Timer timer;
    space.persistent<Position, Velocity>([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateFiveComponents1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
        space.assign(entity);
    }

    Timer timer;
    space.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateFiveComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    std::cout << "Iterating over 1000000 entities, five components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();
        space.assign(entity);
    }

    Timer timer;
    space.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateTenComponents1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};

    std::cout << "Iterating over 1000000 entities, ten components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        space.assign(entity);
    }

    Timer timer;
    space.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>([](auto, auto &...) {});
    timer.elapsed();
}

TEST(Benchmark, SpaceIterateTenComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    entt::DefaultSpace space{registry};
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 1000000 entities, ten components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();
        space.assign(entity);
    }

    Timer timer;
    space.persistent<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>([](auto, auto &...) {});
    timer.elapsed();
}
