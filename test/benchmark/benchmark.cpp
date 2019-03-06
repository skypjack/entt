#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iterator>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

struct position {
    std::uint64_t x;
    std::uint64_t y;
};

struct velocity {
    std::uint64_t x;
    std::uint64_t y;
};

template<std::size_t>
struct comp { int x; };

struct timer final {
    timer(): start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

TEST(Benchmark, Construct) {
    entt::registry<> registry;

    std::cout << "Constructing 1000000 entities" << std::endl;

    timer timer;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create();
    }

    timer.elapsed();
}

TEST(Benchmark, ConstructMany) {
    entt::registry<> registry;
    std::vector<entt::registry<>::entity_type> entities(1000000);

    std::cout << "Constructing 1000000 entities at once" << std::endl;

    timer timer;
    registry.create(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(Benchmark, ConstructManyAndAssignComponents) {
    entt::registry<> registry;
    std::vector<entt::registry<>::entity_type> entities(1000000);

    std::cout << "Constructing 1000000 entities at once and assign components" << std::endl;

    timer timer;

    registry.create(entities.begin(), entities.end());

    for(const auto entity: entities) {
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, ConstructManyWithComponents) {
    entt::registry<> registry;
    std::vector<entt::registry<>::entity_type> entities(1000000);

    std::cout << "Constructing 1000000 entities at once with components" << std::endl;

    timer timer;
    registry.create<position, velocity>(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(Benchmark, Destroy) {
    entt::registry<> registry;

    std::cout << "Destroying 1000000 entities" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create();
    }

    timer timer;

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer.elapsed();
}

TEST(Benchmark, IterateCreateDeleteSingleComponent) {
    entt::registry<> registry;

    std::cout << "Looping 10000 times creating and deleting a random number of entities" << std::endl;

    timer timer;

    auto view = registry.view<position>();

    for(int i = 0; i < 10000; i++) {
        for(int j = 0; j < 10000; j++) {
            const auto entity = registry.create();
            registry.assign<position>(entity);
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
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateSingleComponentRuntime1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, one component, runtime view" << std::endl;

     for(std::uint64_t i = 0; i < 1000000L; i++) {
         const auto entity = registry.create();
         registry.assign<position>(entity);
     }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponents1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponents1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponents1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsNonOwningGroup1M) {
    entt::registry<> registry;
    registry.group<>(entt::get<position, velocity>);

    std::cout << "Iterating over 1000000 entities, two components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<>(entt::get<position, velocity>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsFullOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity>();

    std::cout << "Iterating over 1000000 entities, two components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsPartialOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position>(entt::get<velocity>);

    std::cout << "Iterating over 1000000 entities, two components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position>(entt::get<velocity>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponents1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponents1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponents1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsNonOwningGroup1M) {
    entt::registry<> registry;
    registry.group<>(entt::get<position, velocity>);

    std::cout << "Iterating over 1000000 entities, three components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<>(entt::get<position, velocity, comp<0>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsFullOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity, comp<0>>();

    std::cout << "Iterating over 1000000 entities, three components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsPartialOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity>(entt::get<comp<0>>);

    std::cout << "Iterating over 1000000 entities, three components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity>(entt::get<comp<0>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>(), registry.type<comp<0>>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>(), registry.type<comp<0>>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = { registry.type<position>(), registry.type<velocity>(), registry.type<comp<0>>() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponents1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsNonOwningGroup1M) {
    entt::registry<> registry;
    registry.group<>(entt::get<position, velocity, comp<0>, comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<>(entt::get<position, velocity, comp<0>, comp<1>, comp<2>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsFullOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity, comp<0>, comp<1>, comp<2>>();

    std::cout << "Iterating over 1000000 entities, five components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsPartialFourOfFiveOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity, comp<0>, comp<1>>(entt::get<comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (4 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity, comp<0>, comp<1>>(entt::get<comp<2>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsPartialThreeOfFiveOwningGroup1M) {
    entt::registry<> registry;
    registry.group<position, velocity, comp<0>>(entt::get<comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (3 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity, comp<0>>(entt::get<comp<1>, comp<2>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<0>>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<0>>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<0>>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IteratePathological) {
    entt::registry<> registry;

    std::cout << "Pathological case" << std::endl;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(i % 7) { registry.remove<position>(entity); }
            if(i % 11) { registry.remove<velocity>(entity); }
            if(i % 13) { registry.remove<comp<0>>(entity); }
            if(i % 17) { registry.destroy(entity); }
        });

        for(std::uint64_t i = 0; i < 50000L; i++) {
            const auto entity = registry.create();
            registry.assign<position>(entity);
            registry.assign<velocity>(entity);
            registry.assign<comp<0>>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IteratePathologicalNonOwningGroup) {
    entt::registry<> registry;
    registry.group<>(entt::get<position, velocity, comp<0>>);

    std::cout << "Pathological case" << std::endl;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(i % 7) { registry.remove<position>(entity); }
            if(i % 11) { registry.remove<velocity>(entity); }
            if(i % 13) { registry.remove<comp<0>>(entity); }
            if(i % 17) { registry.destroy(entity); }
        });

        for(std::uint64_t i = 0; i < 50000L; i++) {
            const auto entity = registry.create();
            registry.assign<position>(entity);
            registry.assign<velocity>(entity);
            registry.assign<comp<0>>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<>(entt::get<position, velocity, comp<0>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IteratePathologicalFullOwningGroup) {
    entt::registry<> registry;
    registry.group<position, velocity, comp<0>>();

    std::cout << "Pathological case" << std::endl;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(i % 7) { registry.remove<position>(entity); }
            if(i % 11) { registry.remove<velocity>(entity); }
            if(i % 13) { registry.remove<comp<0>>(entity); }
            if(i % 17) { registry.destroy(entity); }
        });

        for(std::uint64_t i = 0; i < 50000L; i++) {
            const auto entity = registry.create();
            registry.assign<position>(entity);
            registry.assign<velocity>(entity);
            registry.assign<comp<0>>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IteratePathologicalPartialOwningGroup) {
    entt::registry<> registry;
    registry.group<position, velocity>(entt::get<comp<0>>);

    std::cout << "Pathological case" << std::endl;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(i % 7) { registry.remove<position>(entity); }
            if(i % 11) { registry.remove<velocity>(entity); }
            if(i % 13) { registry.remove<comp<0>>(entity); }
            if(i % 17) { registry.destroy(entity); }
        });

        for(std::uint64_t i = 0; i < 50000L; i++) {
            const auto entity = registry.create();
            registry.assign<position>(entity);
            registry.assign<velocity>(entity);
            registry.assign<comp<0>>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.group<position, velocity>(entt::get<comp<0>>).each(func);
        timer.elapsed();
    };

    test([](const auto &...) {});
    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, SortSingle) {
    entt::registry<> registry;

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, SortMulti) {
    entt::registry<> registry;

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);
        registry.assign<velocity>(entity, i, i);
    }

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer timer;

    registry.sort<velocity, position>();

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedStdSort) {
    entt::registry<> registry;
    entt::registry<>::entity_type entities[3];

    std::cout << "Sort 150000 entities, almost sorted, std::sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.assign<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedInsertionSort) {
    entt::registry<> registry;
    entt::registry<>::entity_type entities[3];

    std::cout << "Sort 150000 entities, almost sorted, insertion sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.assign<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    }, entt::insertion_sort{});

    timer.elapsed();
}
