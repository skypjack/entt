#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

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

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer.elapsed();
}

TEST(Benchmark, IterateCreateDeleteSingleComponent) {
    entt::DefaultRegistry registry;

    std::cout << "Looping 10000 times creating and deleting a random number of entities" << std::endl;

    Timer timer;

    auto view = registry.view<Position>();

    for(int i = 0; i < 10000; i++) {
        for(int j = 0; j < 10000; j++) {
            const auto entity = registry.create();
            registry.assign<Position>(entity);
        }

        for(auto entity: view) {
            if(rand() % 2 == 0) {
                registry.destroy(entity);
            }
        }
    }

    timer.elapsed();
}

TEST(Benchmark, IterateSingleComponent1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &) {});
    test([](auto, auto &position) {
        position.x = {};
    });
}

TEST(Benchmark, IterateSingleComponentRaw1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, one component, raw view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position>(entt::raw_t{}).each(func);
        timer.elapsed();
    };

    test([](const auto &) {});
    test([](auto &position) {
        position.x = {};
    });
}

TEST(Benchmark, IterateTwoComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity) {
        position.x = {};
        velocity.x = {};
    });
}

TEST(Benchmark, IterateTwoComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);

        if(i % 2) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity) {
        position.x = {};
        velocity.x = {};
    });
}

TEST(Benchmark, IterateTwoComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);

        if(i == 5000000L) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity) {
        position.x = {};
        velocity.x = {};
    });
}

TEST(Benchmark, IterateTwoComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity>();

    std::cout << "Iterating over 1000000 entities, two components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity>(entt::persistent_t{}).each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity) {
        position.x = {};
        velocity.x = {};
    });
}

TEST(Benchmark, IterateFiveComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
    });
}

TEST(Benchmark, IterateFiveComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);

        if(i % 2) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
    });
}

TEST(Benchmark, IterateFiveComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);

        if(i == 5000000L) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>();

    std::cout << "Iterating over 1000000 entities, five components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>>(entt::persistent_t{}).each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
    });
}

TEST(Benchmark, IterateTenComponents1M) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
        registry.assign<Comp<4>>(entity);
        registry.assign<Comp<5>>(entity);
        registry.assign<Comp<6>>(entity);
        registry.assign<Comp<7>>(entity);
        registry.assign<Comp<8>>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3, auto &comp4, auto &comp5, auto &comp6, auto &comp7, auto &comp8) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
        comp4.x = {};
        comp5.x = {};
        comp6.x = {};
        comp7.x = {};
        comp8.x = {};
    });
}

TEST(Benchmark, IterateTenComponents1MHalf) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
        registry.assign<Comp<4>>(entity);
        registry.assign<Comp<5>>(entity);
        registry.assign<Comp<6>>(entity);
        registry.assign<Comp<7>>(entity);
        registry.assign<Comp<8>>(entity);

        if(i % 2) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3, auto &comp4, auto &comp5, auto &comp6, auto &comp7, auto &comp8) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
        comp4.x = {};
        comp5.x = {};
        comp6.x = {};
        comp7.x = {};
        comp8.x = {};
    });
}

TEST(Benchmark, IterateTenComponents1MOne) {
    entt::DefaultRegistry registry;

    std::cout << "Iterating over 1000000 entities, ten components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
        registry.assign<Comp<4>>(entity);
        registry.assign<Comp<5>>(entity);
        registry.assign<Comp<6>>(entity);
        registry.assign<Comp<7>>(entity);
        registry.assign<Comp<8>>(entity);

        if(i == 5000000L) {
            registry.assign<Position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3, auto &comp4, auto &comp5, auto &comp6, auto &comp7, auto &comp8) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
        comp4.x = {};
        comp5.x = {};
        comp6.x = {};
        comp7.x = {};
        comp8.x = {};
    });
}

TEST(Benchmark, IterateTenComponentsPersistent1M) {
    entt::DefaultRegistry registry;
    registry.prepare<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>();

    std::cout << "Iterating over 1000000 entities, ten components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity);
        registry.assign<Velocity>(entity);
        registry.assign<Comp<1>>(entity);
        registry.assign<Comp<2>>(entity);
        registry.assign<Comp<3>>(entity);
        registry.assign<Comp<4>>(entity);
        registry.assign<Comp<5>>(entity);
        registry.assign<Comp<6>>(entity);
        registry.assign<Comp<7>>(entity);
        registry.assign<Comp<8>>(entity);
    }

    auto test = [&registry](auto func) {
        Timer timer;
        registry.view<Position, Velocity, Comp<1>, Comp<2>, Comp<3>, Comp<4>, Comp<5>, Comp<6>, Comp<7>, Comp<8>>(entt::persistent_t{}).each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &position, auto &velocity, auto &comp1, auto &comp2, auto &comp3, auto &comp4, auto &comp5, auto &comp6, auto &comp7, auto &comp8) {
        position.x = {};
        velocity.x = {};
        comp1.x = {};
        comp2.x = {};
        comp3.x = {};
        comp4.x = {};
        comp5.x = {};
        comp6.x = {};
        comp7.x = {};
        comp8.x = {};
    });
}

TEST(Benchmark, SortSingle) {
    entt::DefaultRegistry registry;

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity, i, i);
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
        const auto entity = registry.create();
        registry.assign<Position>(entity, i, i);
        registry.assign<Velocity>(entity, i, i);
    }

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    Timer timer;

    registry.sort<Velocity, Position>();

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedStdSort) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry::entity_type entities[3];

    std::cout << "Sort 150000 entities, almost sorted, std::sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.assign<Position>(entity, 50000 * i, 50000 * i);
    }

    Timer timer;

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedInsertionSort) {
    entt::DefaultRegistry registry;
    entt::DefaultRegistry::entity_type entities[3];

    std::cout << "Sort 150000 entities, almost sorted, insertion sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<Position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.assign<Position>(entity, 50000 * i, 50000 * i);
    }

    Timer timer;

    registry.sort<Position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    }, entt::InsertionSort{});

    timer.elapsed();
}
