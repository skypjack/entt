# EnTT - Entity-Component System in modern C++

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/uvw)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)

# Introduction

`EnTT` is a header-only, tiny and easy to use Entity-Component System in modern C++.<br/>
ECS is an architectural pattern used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

## Code Example

```cpp
#include <iostream>
#include <registry.hpp>

struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

using ECS = entt::DefaultRegistry<Position, Velocity>;

int main() {
    ECS ecs;

    for(auto i = 0; i < 10; ++i) {
        auto entity = ecs.create();
        ecs.assign<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { ecs.assign<Velocity>(entity, i * .1f, i * .1f); }
    }

    std::cout << "single component view" << std::endl;

    for(auto entity: ecs.view<Position>()) {
        auto &position = ecs.get<Position>(entity);
        std::cout << position.x << "," << position.y << std::endl;
    }

    std::cout << "multi component view" << std::endl;

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        std::cout << position.x << "," << position.y << " - " << velocity.dx << "," << velocity.dy << std::endl;
        if(entity % 4) { ecs.remove<Velocity>(entity); }
        else { ecs.destroy(entity); }
    }

    std::cout << "single component view" << std::endl;

    for(auto entity: ecs.view<Position>()) {
        auto &position = ecs.get<Position>(entity);
        std::cout << position.x << "," << position.y << std::endl;
    }

    std::cout << "multi component view" << std::endl;

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        std::cout << position.x << "," << position.y << " - " << velocity.dx << "," << velocity.dy << std::endl;
        if(entity % 4) { ecs.remove<Velocity>(entity); }
        else { ecs.destroy(entity); }
    }

    ecs.reset();
}
```

## Motivation

I started using another well known Entity-Component System named [`EntityX`](https://github.com/alecthomas/entityx).<br/>
While I was playing with it, I found that I didn't like that much the way it manages its memory.
Moreover, I was pretty sure that one could achieve better performance with a slightly modified pool under the hood.<br/>
That's also the reason for which the interface is quite similar to the one of `EntityX`, so that `EnTT` can be used as a
drop-in replacement for it with a minimal effort.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if compared to my first choice (that was already
amazingly fast).
These are the results of the twos when compiled with GCC 6.3:

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.187042s | **0.0928331s** |
| Destroying 10M entities | 0.0735151s | **0.060166s** |
| Iterating over 10M entities, unpacking one component | 0.00784801s | **1.02e-07s** |
| Iterating over 10M entities, unpacking two components | 0.00865273s | **0.00326714s** |
| Iterating over 10M entities, unpacking five components | 0.0122006s | **0.00323354s** |
| Iterating over 10M entities, unpacking ten components | 0.0100089s | **0.00323615s** |
| Iterating over 50M entities, unpacking one component | 0.0394404s | **1.14e-07s** |
| Iterating over 50M entities, unpacking two components | 0.0400407s | **0.0179783s** |

These are the results of the twos when compiled with Clang 3.8.1:

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.268049s | **0.0899998s** |
| Destroying 10M entities | **0.0713912s** | 0.078663s |
| Iterating over 10M entities, unpacking one component | 0.00863192s | **3.05e-07s** |
| Iterating over 10M entities, unpacking two components | 0.00780158s | **2.5434e-05s** |
| Iterating over 10M entities, unpacking five components | 0.00829669s | **2.5497e-05s** |
| Iterating over 10M entities, unpacking ten components | 0.00789789s | **2.5563e-05s** |
| Iterating over 50M entities, unpacking one component | 0.0423244s | **1.94e-07s** |
| Iterating over 50M entities, unpacking two components | 0.0435464s | **0.00012661s** |

I don't know what Clang does to squeeze out of `EnTT` the performance above, but I'd say that it does it incredibly well.

See [benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp) for further details.<br/>
Of course, I'll try to get out of it more features and better performance anyway in the future, mainly for fun.
If you want to contribute and have any suggestion, feel free to make a PR or open an issue to discuss them.

### Benchmarks / Comparisons

`EnTT` includes its own benchmarks, mostly similar to the ones of `EntityX` so as to compare them.<br/>
On Github you can find also a [benchmark suite](https://github.com/abeimler/ecs_benchmark) testing `EntityX` (both the official version and the compile-time one), `Anax` and `Artemis C++` with up to 10M entities.

# Build Instructions

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that supports at least C++14.<br/>
CMake version 3.4 or later is mandatory to compile the tests, you don't have to install it otherwise.

## Library

`EnTT` is a header-only library. This means that including the `registry.hpp` header is enough to use it.<br/>
It's a matter of adding the following line at the top of a file:

```cpp
#include <registry.hpp>
```

Then pass the proper `-I` argument to the compiler to add the `src` directory to the include paths.<br/>

## Documentation

### API Reference

`EnTT` contains three main actors: the *registry*, the *view* and the *pool*.<br/>
Unless you have specific requirements of memory management, the default registry (that used the pool provided with
`EnTT`) should be good enough for any use. Customization is an option anyway, so that you can use your own pool as
long as it offers the expected interface.

#### The Registry

There are three options to instantiate your own registry:

* By using the default one:

    ```cpp
    auto registry = entt::DefaultRegistry<Components...>{args...};
    ```

  That is, you must provide the whole list of components to be registered with the default registry.

* By using the standard one:

    ```cpp
    auto registry = entt::StandardRegistry<std::uint16_t, Components...>{args...};
    ```

  That is, you must provide the whole list of components to be registered with the default registry **and** the desired type for the entities. Note that the default type is `std::uint32_t`, that is larger enough for almost all the games but also too big for the most of the games.

* By using your own pool:

    ```cpp
    auto registry = entt::Registry<DesiredEntityType, YourOwnPool<Components...>>{args...};
    ```

  Note that the registry expects a class template where the template parameters are the components to be managed.

In both cases, `args...` parameters are forwarded to the underlying pool during the construction.<br/>
There are no requirements for the components but to be moveable, therefore POD types are just fine.

Once you have created a registry, the followings are the exposed member functions:

* `size`: returns the number of entities still alive.
* `capacity`: returns the maximum number of entities created till now.
* `valid`: returns true if the entity is still in use, false otherwise.
* `empty<Component>`: returns `true` if at least an instance of `Component` exists, `false` otherwise.
* `empty`: returns `true` if all the entities have been destroyed, `false` otherwise.
* `create<Components...>`: creates a new entity and assigns it the given components, then returns the entity.
* `create`: creates a new entity and returns it, no components assigned.
* `destroy`: destroys the entity and all its components.
* `assign<Component>(entity, args...)`: assigns the given component to the entity and uses `args...` to initialize it.
* `remove<Component>(entity)`: removes the given component from the entity.
* `has<Components...>(entity)`: returns `true` if the entity has the given components, `false` otherwise.
* `get<Component>(entity)`: returns a reference to the given component for the entity (undefined behaviour if the entity has not the component).
* `replace<Component>(entity, args...)`: replaces the given component for the entity, using `args...` to create the new component.
* `accomodate<Component>(entity, args...)`: replaces the given component for the entity if it exists, otherwise assigns it to the entity and uses `args...` to initialize it.
* `clone(entity)`: clones an entity and all its components, then returns the new entity identifier.
* `copy<Component>(to, from)`: copies a component from an entity to another one (both the entities must already have been assigned the component, undefined behaviour otherwise).
* `copy(to, from)`: copies all the components and their contents from an entity to another one (comoonents are created or destroyed if needed).
* `reset<Component>(entity)`: removes the given component from the entity if assigned.
* `reset<Component>()`: destroys all the instances of `Component`.
* `reset()`: resets the pool and destroys all the entities and their components.
* `view<Components...>()`: gets a view of the entities that have the given components (see below for further details).

Note that entities are numbers and nothing more. They are not classes and they have no member functions at all.

#### The View

There are three different kinds of view, each one with a slighlty different interface:

* The _single component view_.
* The _multi component view_.

All of them are iterable. In other terms they have `begin` and `end` member functions that are suitable for a range-based for loop:

```cpp
auto view = registry.view<Position, Velocity>();

for(auto entity: view) {
    // do whatever you want with your entities
}
```

Iterators are extremely poor, they are meant exclusively to be used to iterate over a set of entities.<br/>
Guaranteed exposed member functions are:

* `operator++()`
* `operator++(int)`
* `operator==()`
* `operator!=()`
* `operator*()`

The single component view has an additional member function:

* `size()`: returns the exact number of expected entities.

The multi component view has an additional member function:

* `reset()`: reorganizes internal data so as to further create optimized iterators (use it whenever the data within the registry are known to be changed).

All the views can be used more than once. They return newly created and correctly initialized iterators whenever
`begin` or `end` is invoked. Anyway views and iterators are tiny objects and the time to construct them can be safely ignored.
I'd suggest not to store them anywhere and to invoke the `Registry::view` member function at each iteration to get a properly
initialized view over which to iterate.

**Note**: If underlying sets are modified, iterators are invalidated and using them is undefined behaviour.<br/>
Do not try to assign or remove components on which you are iterating to entities. There are no guarantees.

**Note**: Iterators aren't thread safe. Do no try to iterate over a set of components and modify them concurrently.
That being said, as long as a thread iterates over the entities that have the component `X` or assign and removes
that component from a set of entities and another thread does something similar with components `Y` and `Z`, it shouldn't be a
problem at all.<br/>
As an example, that means that users can freely run the rendering system over the renderable entities and update the physics
concurrently on a separate thread if needed.

#### The Pool

Custom pools for a given component can be defined as a specialization of the class template `ComponentPool`.<br/>
In particular:

```cpp
template<>
struct ComponentPool<Entity, MyComponent> final {
    // ...
};
```

Where `Entity` is the desired type for the entities, `MyComponent` the type of the component to be stored.

A custom pool should expose at least the following member functions:

* `bool empty() const noexcept;`
* `size_type capacity() const noexcept;`
* `size_type size() const noexcept;`
* `iterator_type begin() noexcept;`
* `const_iterator_type begin() const noexcept;`
* `iterator_type end() noexcept;`
* `const_iterator_type end() const noexcept;`
* `bool has(entity_type entity) const noexcept;`
* `const component_type & get(entity_type entity) const noexcept;`
* `component_type & get(entity_type entity) noexcept;`
* `template<typename... Args> component_type & construct(entity_type entity, Args... args);`
* `void destroy(entity_type entity);`
* `void reset();`

This is a fast and easy way to define a custom pool specialization for a given component (as an example, if the
component `X` requires to be ordered internally somehow during construction or destruction operations) and to use the
default pool for all the other components.<br/>
It's a mattrer of including the given specialization along with the registry, so that it can find it during the instantiation.<br/>
In this case, users are not required to use the more explicit `Registry` class. Instead, they can still use `entt::DefaultRegistry`.

In cases when the per-component pools are not good enough, the registry can be initialized with a custom pool.<br/>
In other terms, `entt::Registry` has a template template parameter that can be used to provide both the pool and the list of
components:

```cpp
auto registry = entt::Registry<Entity, MyCustomPool<Component1, Component2>>{};
```

Even though the underlying pool doesn't store the components separately, the registry must know them to be able to do
specific actions (like `destroy` or `copy`). That's why they must be explicitly specified.<br/>
A generic pool should expose at least the following memeber functions:

* `template<typename Component> bool empty() const noexcept;`
* `template<typename Component> size_type capacity() const noexcept;`
* `template<typename Component> size_type size() const noexcept;`
* `template<typename Component> iterator_type begin() noexcept;`
* `template<typename Component> const_iterator_type begin() const noexcept;`
* `template<typename Component> iterator_type end() noexcept;`
* `template<typename Component> const_iterator_type end() const noexcept;`
* `template<typename Component> bool has(entity_type entity) const noexcept;`
* `template<typename Component> const Comp & get(entity_type entity) const noexcept;`
* `template<typename Component> Comp & get(entity_type entity) noexcept;`
* `template<typename Component, typename... Args> Comp & construct(entity_type entity, Args... args);`
* `template<typename Component> void destroy(entity_type entity);`
* `template<typename Component> void reset();`
* `void reset();`

Good luck. If you come out with a more performant components pool, do not forget to make a PR so that I can add it to
the list of available ones. I would be glad to have such a contribution to the project!!

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
Run the script `deps.sh` to download it. It is good practice to do it every time one pull the project.

Then, to build the tests:

* `$ cd build`
* `$ cmake ..`
* `$ make`
* `$ make test`

# Contributors

If you want to contribute, please send patches as pull requests against the branch master.<br/>
Check the [contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to see who has partecipated so far.

# License

Code and documentation Copyright (c) 2017 Michele Caini.<br/>
Code released under [the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).
