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

    test([](auto, const auto &) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateSingleComponentRaw1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, one component, raw view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.raw_view<position>().each(func);
        timer.elapsed();
    };

    test([](const auto &) {});
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

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
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

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
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

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsPersistent1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, two components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.persistent_view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
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

TEST(Benchmark, IterateFiveComponents1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsPersistent1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.persistent_view<position, velocity, comp<1>, comp<2>, comp<3>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
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
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
    });
}

TEST(Benchmark, IterateTenComponents1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>, comp<4>, comp<5>, comp<6>, comp<7>, comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTenComponents1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>, comp<4>, comp<5>, comp<6>, comp<7>, comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTenComponents1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.view<position, velocity, comp<1>, comp<2>, comp<3>, comp<4>, comp<5>, comp<6>, comp<7>, comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTenComponentsPersistent1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, persistent view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);
    }

    auto test = [&registry](auto func) {
        timer timer;
        registry.persistent_view<position, velocity, comp<1>, comp<2>, comp<3>, comp<4>, comp<5>, comp<6>, comp<7>, comp<8>>().each(func);
        timer.elapsed();
    };

    test([](auto, const auto &...) {});
    test([](auto, auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTenComponentsRuntime1M) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<position>(entity);
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>(),
            registry.type<comp<4>>(),
            registry.type<comp<5>>(),
            registry.type<comp<6>>(),
            registry.type<comp<7>>(),
            registry.type<comp<8>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
        registry.get<comp<4>>(entity).x = {};
        registry.get<comp<5>>(entity).x = {};
        registry.get<comp<6>>(entity).x = {};
        registry.get<comp<7>>(entity).x = {};
        registry.get<comp<8>>(entity).x = {};
    });
}

TEST(Benchmark, IterateTenComponentsRuntime1MHalf) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);

        if(i % 2) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>(),
            registry.type<comp<4>>(),
            registry.type<comp<5>>(),
            registry.type<comp<6>>(),
            registry.type<comp<7>>(),
            registry.type<comp<8>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
        registry.get<comp<4>>(entity).x = {};
        registry.get<comp<5>>(entity).x = {};
        registry.get<comp<6>>(entity).x = {};
        registry.get<comp<7>>(entity).x = {};
        registry.get<comp<8>>(entity).x = {};
    });
}

TEST(Benchmark, IterateTenComponentsRuntime1MOne) {
    entt::registry<> registry;

    std::cout << "Iterating over 1000000 entities, ten components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.assign<velocity>(entity);
        registry.assign<comp<1>>(entity);
        registry.assign<comp<2>>(entity);
        registry.assign<comp<3>>(entity);
        registry.assign<comp<4>>(entity);
        registry.assign<comp<5>>(entity);
        registry.assign<comp<6>>(entity);
        registry.assign<comp<7>>(entity);
        registry.assign<comp<8>>(entity);

        if(i == 5000000L) {
            registry.assign<position>(entity);
        }
    }

    auto test = [&registry](auto func) {
        using component_type = typename entt::registry<>::component_type;
        component_type types[] = {
            registry.type<position>(),
            registry.type<velocity>(),
            registry.type<comp<1>>(),
            registry.type<comp<2>>(),
            registry.type<comp<3>>(),
            registry.type<comp<4>>(),
            registry.type<comp<5>>(),
            registry.type<comp<6>>(),
            registry.type<comp<7>>(),
            registry.type<comp<8>>()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([](auto) {});
    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
        registry.get<comp<3>>(entity).x = {};
        registry.get<comp<4>>(entity).x = {};
        registry.get<comp<5>>(entity).x = {};
        registry.get<comp<6>>(entity).x = {};
        registry.get<comp<7>>(entity).x = {};
        registry.get<comp<8>>(entity).x = {};
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
