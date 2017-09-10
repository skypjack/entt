# EnTT - Entity-Component System in modern C++

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/uvw)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

# Introduction

`EnTT` is a header-only, tiny and easy to use Entity-Component System in modern C++.<br/>
_ECS_ is an architectural pattern used mostly in game development. For further details:

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
        // do whatever is needed with position
        (void)position;
    }

    // multi component view

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        // do whatever is needed with position and velocity
        (void)position;
        (void)velocity;
    }
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to beat another well known open source _ECS_ in terms of performance.<br>
I did it, of course, but it wasn't much satisfying. Actually it wasn't satisfying at all. The fastest and nothing more, fairly little indeed.<br/>
When I realized it, I tried hard to keep intact the great performance and to add all the features I want to see in my _ECS_ at the same time.

Today `EnTT` is finally what I was looking for: still faster than its rivals, a really good API and an amazing set of features.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if compared to my first choice (a well known open source _ECS_ that was already amazingly fast).<br/>
Here is a comparision (both of them compiled with GCC 7.2.0 on a Dell XPS 13 coming from the late 2014):

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.290214s | **0.143111s** |
| Destroying 10M entities | 0.104589s | **0.0917096s** |
| Iterating over 10M entities, unpacking one component | 0.0165628s | **1.37e-07s** |
| Iterating over 10M entities, unpacking two components | 0.0137463s | **0.0052857s** |
| Iterating over 10M entities, unpacking two components, half of the entities have all the components | 0.011949s | **0.00268133s** |
| Iterating over 10M entities, unpacking two components, one of the entities has all the components | 0.0115935s | **6.4e-07s** |
| Iterating over 10M entities, unpacking five components | 0.0143905s | **0.00527808s** |
| Iterating over 10M entities, unpacking ten components | 0.0165853s | **0.00528096s** |
| Iterating over 10M entities, unpacking ten components, half of the entities have all the components | 0.0169309s | **0.00327983s** |
| Iterating over 10M entities, unpacking ten components, one of the entities has all the components | 0.0110304s | **8.11e-07s** |
| Iterating over 50M entities, unpacking one component | 0.0827622s | **1.65e-07s** |
| Iterating over 50M entities, unpacking two components | 0.0830518s | **0.0265629s** |

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

There are two options to instantiate a registry:

* By using the default one:

    ```cpp
    auto registry = entt::DefaultRegistry<Components...>{args...};
    ```

  That is, users must provide the whole list of components to be registered with the default registry.

* By using directly the `Registry` class:

    ```cpp
    auto registry = entt::Registry<std::uint16_t, Components...>{args...};
    ```

  That is, users must provide the whole list of components to be registered with the registry **and** the desired type for the entities. Note that the default type is `std::uint32_t`, that is larger enough for almost all the games but also too big for the most of the games.

In both cases there are no requirements for the components but to be moveable, therefore POD types are just fine.

The `Registry` class offers a bunch of basic functionalities to query the internal structures. In almost all the cases those member functions can be used to query either the entity list or the component lists.<br/>
As an example, the member functions `empty` can be used to know if at least an entity exists and/or if at least one component of the given type has been assigned to an entity:

```cpp
bool b = registry.empty();
// ...
bool b = registry.empty<MyComponent>();
```

Similarly, `size` can be used to know the number of entities alive and/or the number of components of a given type still assigned to entities. `capacity` follows the same pattern and returns the storage capacity for the given element.

To know if an entity is valid, the `valid` member function is part of the registry interface:

```cpp
bool b = registry.valid(entity);
```

Let's go to something more tasty.

The `create` member function can be used to create a new entity and it comes in two flavors:

* The plain version just creates a naked entity with no components assigned to it:

    ```cpp
    auto entity = registry.create();
    ```

* The member function template creates an entity and assign to it the given default-initialized components before to return it:

    ```cpp
    auto entity = registry.create<Position, Velocity>();
    ```

  It's a helper function, mostly syncactic sugar and it's equivalent to the following:

    ```cpp
    auto entity = registry.create();
    registry.assign<Position>();
    registry.assign<Velocity>();
    ```

  See below to find more about the `assign` member function.

The `destroy` member function can be used instead to delete an entity and all its components (if any):

    ```cpp
    registry.destroy(entity);
    ```

On the other side, if the purpose is to remove a component, the `remove` member function template is the way to go:

    ```cpp
    registry.remove<Position>(entity);
    ```

The `reset` member function can be used to obtain the same result with a strictly defined behaviour (a performance penalty is the price to pay for that anyway). In particular it removes the component if and only if it exists, otherwise it safely returns to the caller:

    ```cpp
    registry.reset<Position>(entity);
    ```

There exist also two other _versions_ of the `reset` member function:

* If no entity is passed to it, `reset` will remove the given component from each entity that has it:

    ```cpp
    registry.reset<Position>();
    ```

* If neither the entity nor the component are specified, all the entities and their components are destroyed:

    ```cpp
    registry.reset();
    ```

  *Note*: the registry has an assert in debug mode that verifies that entities are no longer valid when it's destructed. This function can be used to reset the registry to its initial state and thus satisfy the requirement.

To assign a component to an entity, users can use the `assign` member function template. It accepts a variable number of arguments that, if present, are used to construct the component itself:

    ```cpp
    registry.assign<Position>(entity, 0., 0.);

    // ...

    auto &velocity = registr.assign<Velocity>(entity);

    velocity.dx = 0.;
    velocity.dy = 0.;
    ```

If the entity already has the given component and the user wants to replace it, the 'replace` member function template is the way to go. It works exactly as `assign`:

    ```cpp
    registry.replace<Position>(entity, 0., 0.);

    // ...

    auto &velocity = registr.replace<Velocity>(entity);

    velocity.dx = 0.;
    velocity.dy = 0.;
    ```

In case users want to assign a component to an entity, but it's unknown whether the entity already has it or not, `accomodote` does the work in a single call:

    ```cpp
    registry.accomodate<Position>(entity, 0., 0.);

    // ...

    auto &velocity = registr.accomodate<Velocity>(entity);

    velocity.dx = 0.;
    velocity.dy = 0.;
    ```

Note that `accomodate` is a sliglhty faster alternative for the following if/else statement:

    ```cpp
    if(registry.has<Comp>(entity)) {
        registry.replace<Comp>(entity, arg1, argN);
    } else {
        registry.assign<Comp>(entity, arg1, argN);
    }
    ```

As already shown, if in doubt about whether or not an entity has one or more components, the `has` member function template may be useful:

    ```cpp
    bool b = registry.has<Position, Velocity>(entity);
    ```

Entities can also be cloned and partially or fully copied:

    ```cpp
    auto entity = registry.clone(other);

    // ...

    auto &velocity = registry.copy<Velocity>(to, from);

    // ...

    registry.copy(dst, src);
    ```

In particular:

* The `clone` member function creates a new entity and copy all the components from the given one.
* The `copy` member function template copies one component from an entity to another one.
* The `copy` member function copies all the components from an entity to another one.

There exists also an utility member function that can be used to `swap` components between entities:

    ```cpp
    registry.swap<Position>(e1, e2);
    ```

The `get` member function template (either the non-const or the const version) gives direct access to the component of an entity instead:

    ```cpp
    auto &position = registry.get<Position>(entity);
    ```

Components can also be sorted in memory by means of the `sort` member function templates. In particular:

* Components can be sorted according to a component:

    ```cpp
    registry.sort<Renderable>([](const auto &lhs, const auto &rhs) { return lhs.z < rhs.z; });
    ```

* Components can be sorted according to the order imposed by another component:

    ```cpp
    registry.sort<Movement, Physics>();
    ```

  In this case, `Movement` components are arranged in memory so that cache misses are minimized when the two components are iterated together.

*Note*. Several functions require that entities received as arguments are valid. In case they are not, an assertion will fail in debug mode and the behaviour is undefined in release mode.<br/>
Here is the full list of functions for which the requirement applies:

* `destroy`
* `assign`
* `remove`
* `has`
* `get`
* `replace`
* `accomodate`
* `clone`
* `copy`
* `swap`
* `reset`

Finally, the `view` member function template returns an iterable portion of entities and components (see below for more details):

    ```cpp
    auto view = registry.view<Position, Velocity>();
    ```

Views are the other core component of `EnTT` and are usually extensively used by softwares that include it.

#### The View

TODO (ricorda di menzionare aggiunte/cancellazioni durante iterazioni)

#### Side notes

* Entities are numbers and nothing more. They are not classes and they have no member functions at all.

* Most of the _ECS_ available out there have an annoying limitation (at least from my point of view): entities and components cannot be created, assigned or deleted while users are iterating on them.<br/>
  `EnTT` partially solves the problem with a few limitations:

    * Entities can be created at any time while iterating on one or more components.
    * Components can be assigned to any entity at any time while iterating on one or more components.
    * During an iteration, the current entity (that is the one returned by the iterator) can be deleted and all its components can be removed safely.

  Entities that are not the current one (that is the one returned by the iterator) cannot be deleted. Components assigned to entities that are not the current one (that is the one returned by the iterator) cannot be removed.<br/>
  In this case, iterators are invalidated and the behaviour is undefined if one continues to use those iterators. Possible approaches are:

    * Store aside the entities and components to be removed and perform the operations at the end of the iteration.
    * Mark entities and components with a proper tag component that indicates that they must be purged, then perform a second iteration to clean them up one by one.

* Iterators aren't thread safe. Do no try to iterate over a set of components and modify them concurrently.<br/>
  That being said, as long as a thread iterates over the entities that have the component `X` or assign and removes that component from a set of entities and another thread does something similar with components `Y` and `Z`, it shouldn't be a problem at all.<br/>
  As an example, that means that users can freely run the rendering system over the renderable entities and update the physics concurrently on a separate thread if needed.

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
-->
