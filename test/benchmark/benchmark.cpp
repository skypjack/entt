#include <iostream>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iterator>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
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

template<typename Func>
void pathological(Func func) {
    entt::registry registry;

    for(std::uint64_t i = 0; i < 500000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    for(auto i = 0; i < 10; ++i) {
        registry.each([i = 0, &registry](const auto entity) mutable {
            if(!(++i % 7)) { registry.remove_if_exists<position>(entity); }
            if(!(++i % 11)) { registry.remove_if_exists<velocity>(entity); }
            if(!(++i % 13)) { registry.remove_if_exists<comp<0>>(entity); }
            if(!(++i % 17)) { registry.destroy(entity); }
        });

        for(std::uint64_t j = 0; j < 50000L; j++) {
            const auto entity = registry.create();
            registry.emplace<position>(entity);
            registry.emplace<velocity>(entity);
            registry.emplace<comp<0>>(entity);
        }
    }

    func(registry, [](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, Create) {
    entt::registry registry;

    std::cout << "Creating 1000000 entities" << std::endl;

    timer timer;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        registry.create();
    }

    timer.elapsed();
}

TEST(Benchmark, CreateMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once" << std::endl;

    timer timer;
    registry.create(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(Benchmark, CreateManyAndEmplaceComponents) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once and emplace components" << std::endl;

    timer timer;

    registry.create(entities.begin(), entities.end());

    for(const auto entity: entities) {
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, CreateManyWithComponents) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Creating 1000000 entities at once with components" << std::endl;

    timer timer;
    registry.create(entities.begin(), entities.end());
    registry.insert<position>(entities.begin(), entities.end());
    registry.insert<velocity>(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(Benchmark, Remove) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Removing 1000000 components from their entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;

    for(auto entity: registry.view<int>()) {
        registry.remove<int>(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, RemoveMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Removing 999999 components from their entities at once" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.remove<int>(++view.begin(), view.end());
    timer.elapsed();
}

TEST(Benchmark, RemoveAll) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Removing 1000000 components from their entities at once" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.remove<int>(view.begin(), view.end());
    timer.elapsed();
}

TEST(Benchmark, Recycle) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Recycling 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer timer;

    for(auto next = entities.size(); next; --next) {
        registry.create();
    }

    timer.elapsed();
}

TEST(Benchmark, RecycleMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Recycling 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());

    registry.each([&registry](auto entity) {
        registry.destroy(entity);
    });

    timer timer;
    registry.create(entities.begin(), entities.end());
    timer.elapsed();
}

TEST(Benchmark, Destroy) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Destroying 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;

    for(auto entity: registry.view<int>()) {
        registry.destroy(entity);
    }

    timer.elapsed();
}

TEST(Benchmark, DestroyMany) {
    entt::registry registry;
    std::vector<entt::entity> entities(1000000);

    std::cout << "Destroying 1000000 entities" << std::endl;

    registry.create(entities.begin(), entities.end());
    registry.insert<int>(entities.begin(), entities.end());

    timer timer;
    auto view = registry.view<int>();
    registry.destroy(view.begin(), view.end());
    timer.elapsed();
}

TEST(Benchmark, IterateSingleComponent1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateSingleComponentRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, one component, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = { entt::type_hash<position>::value() };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity>);

    std::cout << "Iterating over 1000000 entities, two components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity>();

    std::cout << "Iterating over 1000000 entities, two components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsPartialOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position>(entt::get<velocity>);

    std::cout << "Iterating over 1000000 entities, two components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateTwoComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, two components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity, comp<0>>);

    std::cout << "Iterating over 1000000 entities, three components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>>();

    std::cout << "Iterating over 1000000 entities, three components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsPartialOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity>(entt::get<comp<0>>);

    std::cout << "Iterating over 1000000 entities, three components, partial owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateThreeComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, three components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponents1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponents1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        timer timer;
        registry.view<position, velocity, comp<0>, comp<1>, comp<2>>().each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsNonOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<>(entt::get<position, velocity, comp<0>, comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, non owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsFullOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>, comp<1>, comp<2>>();

    std::cout << "Iterating over 1000000 entities, five components, full owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsPartialFourOfFiveOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>, comp<1>>(entt::get<comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (4 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsPartialThreeOfFiveOwningGroup1M) {
    entt::registry registry;
    const auto group = registry.group<position, velocity, comp<0>>(entt::get<comp<1>, comp<2>>);

    std::cout << "Iterating over 1000000 entities, five components, partial (3 of 5) owning group" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        timer timer;
        group.each(func);
        timer.elapsed();
    };

    test([](auto &... comp) {
        ((comp.x = {}), ...);
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1M) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity);
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MHalf) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, half of the entities have all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i % 2) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IterateFiveComponentsRuntime1MOne) {
    entt::registry registry;

    std::cout << "Iterating over 1000000 entities, five components, only one entity has all the components, runtime view" << std::endl;

    for(std::uint64_t i = 0; i < 1000000L; i++) {
        const auto entity = registry.create();
        registry.emplace<velocity>(entity);
        registry.emplace<comp<0>>(entity);
        registry.emplace<comp<1>>(entity);
        registry.emplace<comp<2>>(entity);

        if(i == 500000L) {
            registry.emplace<position>(entity);
        }
    }

    auto test = [&](auto func) {
        entt::id_type types[] = {
            entt::type_hash<position>::value(),
            entt::type_hash<velocity>::value(),
            entt::type_hash<comp<0>>::value(),
            entt::type_hash<comp<1>>::value(),
            entt::type_hash<comp<2>>::value()
        };

        timer timer;
        registry.runtime_view(std::begin(types), std::end(types)).each(func);
        timer.elapsed();
    };

    test([&registry](auto entity) {
        registry.get<position>(entity).x = {};
        registry.get<velocity>(entity).x = {};
        registry.get<comp<0>>(entity).x = {};
        registry.get<comp<1>>(entity).x = {};
        registry.get<comp<2>>(entity).x = {};
    });
}

TEST(Benchmark, IteratePathological) {
    std::cout << "Pathological case" << std::endl;

    pathological([](auto &registry, auto func) {
        timer timer;
        registry.template view<position, velocity, comp<0>>().each(func);
        timer.elapsed();
    });
}

TEST(Benchmark, IteratePathologicalNonOwningGroup) {
    std::cout << "Pathological case (non-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<>(entt::get<position, velocity, comp<0>>);

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(Benchmark, IteratePathologicalFullOwningGroup) {
    std::cout << "Pathological case (full-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<position, velocity, comp<0>>();

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(Benchmark, IteratePathologicalPartialOwningGroup) {
    std::cout << "Pathological case (partial-owning group)" << std::endl;

    pathological([](auto &registry, auto func) {
        auto group = registry.template group<position, velocity>(entt::get<comp<0>>);

        timer timer;
        group.each(func);
        timer.elapsed();
    });
}

TEST(Benchmark, SortSingle) {
    entt::registry registry;

    std::cout << "Sort 150000 entities, one component" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, SortMulti) {
    entt::registry registry;

    std::cout << "Sort 150000 entities, two components" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);
        registry.emplace<velocity>(entity, i, i);
    }

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    });

    timer timer;

    registry.sort<velocity, position>();

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedStdSort) {
    entt::registry registry;
    entt::entity entities[3]{};

    std::cout << "Sort 150000 entities, almost sorted, std::sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.emplace<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    });

    timer.elapsed();
}

TEST(Benchmark, AlmostSortedInsertionSort) {
    entt::registry registry;
    entt::entity entities[3]{};

    std::cout << "Sort 150000 entities, almost sorted, insertion sort" << std::endl;

    for(std::uint64_t i = 0; i < 150000L; i++) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i, i);

        if(!(i % 50000)) {
            entities[i / 50000] = entity;
        }
    }

    for(std::uint64_t i = 0; i < 3; ++i) {
        registry.destroy(entities[i]);
        const auto entity = registry.create();
        registry.emplace<position>(entity, 50000 * i, 50000 * i);
    }

    timer timer;

    registry.sort<position>([](const auto &lhs, const auto &rhs) {
        return lhs.x > rhs.x && lhs.y > rhs.y;
    }, entt::insertion_sort{});

    timer.elapsed();
}
