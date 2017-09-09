# EnTT - Entity-Component System in modern C++

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/uvw)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

# Introduction

`EnTT` is a header-only, tiny and easy to use Entity-Component System in modern C++.<br/>
ECS is an architectural pattern used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

## Code Example

```cpp
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

    // single component view

    for(auto entity: ecs.view<Position>()) {
        auto &position = ecs.get<Position>(entity);
        // do whatever you want with position
        (void)position;
    }

    // multi component view

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        // do whatever you want with position and velocity
        (void)position;
        (void)velocity;
    }
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to beat another well known open source ECS in terms of performance.<br>
I did it, of course, but it wasn't much satisfying. Actually it wasn't satisfying at all. The fastest and nothing more, fairly little indeed.<br/>
When I realized it, I tried hard to keep intact the great performance and to add all the features I want to see in my ECS at the same time.

Today `EnTT` is finally what I was looking for: still faster than its rivals, a really good API and an amazing set of features.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if compared to my first choice (a well known open source ECS that was already amazingly fast).<br/>
Here is a comparision (both of them compiled with GCC 7.2.0 on a Dell XPS 13 coming from the late 2014):

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.290214s | **0.143111s** |
| Destroying 10M entities | 0.104589s | **0.0917096s** |
| Iterating over 10M entities, unpacking one component | 0.0165628ss | **1.37e-07ss** |
| Iterating over 10M entities, unpacking two components | 0.0137463ss | **0.0052857ss** |
| Iterating over 10M entities, unpacking two components, half of the entities have all the components | 0.011949ss | **0.00268133ss** |
| Iterating over 10M entities, unpacking two components, one of the entities has all the components | 0.0115935ss | **6.4e-07ss** |
| Iterating over 10M entities, unpacking five components | 0.0143905ss | **0.00527808ss** |
| Iterating over 10M entities, unpacking ten components | 0.0165853ss | **0.00528096ss** |
| Iterating over 10M entities, unpacking ten components, half of the entities have all the components | 0.0169309ss | **0.00327983ss** |
| Iterating over 10M entities, unpacking ten components, one of the entities has all the components | 0.0110304ss | **8.11e-07ss** |
| Iterating over 50M entities, unpacking one component | 0.0827622ss | **1.65e-07ss** |
| Iterating over 50M entities, unpacking two components | 0.0830518ss | **0.0265629ss** |

See [benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp) for further details.<br/>
Even though `EnTT` includes its own tests and benchmarks, on Github there exists also a [benchmark suite](https://github.com/abeimler/ecs_benchmark) that compares a bunch of different projects, one of which is `EnTT`.

Of course, I'll try to get out of `EnTT` more features and better performance anyway in the future, mainly for fun.<br/>
If you want to contribute and/or have any suggestion, feel free to make a PR or open an issue to discuss your idea.

# Build Instructions

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that supports at least C++14.<br/>
CMake version 3.2 or later is mandatory to compile the tests, users don't have to install it otherwise.

## Library

`EnTT` is a header-only library. This means that including the `registry.hpp` header is enough to use it.<br/>
It's a matter of adding the following line at the top of a file:

```cpp
#include <registry.hpp>
```

Then pass the proper `-I` argument to the compiler to add the `src` directory to the include paths.

## Documentation

### API Reference

Unfortunately `EnTT` isn't documented yet and thus users cannot rely on in-code documentation.<br/>
Source code and names are self-documenting and I'm pretty sure that a glimpse to the API is enough for most of the users.<br/>
For all the others, below is a crash course that guides them through the project and tries to fill the gap.

### Crash Course

`EnTT` has two main actors: the *Registry* and the *View*.<br/>
The former can be used to manage components, entities and collections of components and entities. The latter allows users to iterate over the underlying collections.

#### The Registry

There are two options to instantiate your own registry:

* By using the default one:

    ```cpp
    auto registry = entt::DefaultRegistry<Components...>{args...};
    ```

  That is, you must provide the whole list of components to be registered with the default registry.

* By using directly the `Registry` class:

    ```cpp
    auto registry = entt::Registry<std::uint16_t, Components...>{args...};
    ```

  That is, you must provide the whole list of components to be registered with the registry **and** the desired type for the entities. Note that the default type is `std::uint32_t`, that is larger enough for almost all the games but also too big for the most of the games.

In both cases there are no requirements for the components but to be moveable, therefore POD types are just fine.

The `Registry` class offers a bunch of basic functionalities to query the internal structures. In almost all cases those methods can be used to query either the entity list or the component lists.<br/>
As an example, the member functions `empty` can be used to know if at least an entity exists and/or if at least one component of the given type has been assigned to an entity:

```cpp
bool b = registry.empty();
// ...
bool b = registry.empty<MyComponent>();
```

Similarly, `size` can be used to know the number of entities alive and/or the number of components of a given type still assigned to entities. `capacity` follows the same pattern and returns the storage capacity for the given element.

TODO

#### The View

TODO (ricorda di menzionare aggiunte/cancellazioni durante iterazioni)

#### The Pool

TODO

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
`cmake` will download and compile the library before to compile anything else.

Then, to build the tests:

* `$ cd build`
* `$ cmake ..`
* `$ make`
* `$ make test`

To build the benchmarks, use the following line instead:

* `$ cmake -DCMAKE_BUILD_TYPE=Release ..`

Benchmarks are compiled only in release mode currently.

# Contributors

If you want to contribute, please send patches as pull requests against the branch master.<br/>
Check the [contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to see who has partecipated so far.

# License

Code and documentation Copyright (c) 2017 Michele Caini.<br/>
Code released under [the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).

# Donation

Developing and maintaining `EnTT` takes some time and lots of coffee. If you want to support this project, you can offer me an espresso. I'm from Italy, we're used to turning the best coffee ever in code.<br/>
Take a look at the donation button at the top of the page for more details or just click [here](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).

<!--

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
-->
