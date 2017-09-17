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
        // ...
    }

    // multi component view

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        // ...
    }

    ecs.reset();
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to beat another well known open source _ECS_ in terms of performance.
I did it, of course, but it wasn't much satisfying. Actually it wasn't satisfying at all. The fastest and nothing more, fairly little indeed.
When I realized it, I tried hard to keep intact the great performance and to add all the features I want to see in my _ECS_ at the same time.

Today `EnTT` is finally what I was looking for: still faster than its _rivals_, a really good API and an amazing set of features.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if compared to my first choice (that was already amazingly fast indeed).<br/>
Here is a comparision between the two (both of them compiled with GCC 7.2.0 on a Dell XPS 13 out of the mid 2014):

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.177225s | **0.0881921s** |
| Destroying 10M entities | 0.066419s | **0.0552661s** |
| Iterating over 10M entities, unpacking one component | 0.0104935s | **8.8e-08s** |
| Iterating over 10M entities, unpacking two components | 0.00835546s | **0.00323798s** |
| Iterating over 10M entities, unpacking two components, half of the entities have all the components | 0.00772169s | **0.00162265s** |
| Iterating over 10M entities, unpacking two components, one of the entities has all the components | 0.00751099s | **5.17e-07s** |
| Iterating over 10M entities, unpacking five components | 0.00863762s | **0.00323384s** |
| Iterating over 10M entities, unpacking ten components | 0.0105657s | **0.00323742s** |
| Iterating over 10M entities, unpacking ten components, half of the entities have all the components | 0.00880251s | **0.00164593s** |
| Iterating over 10M entities, unpacking ten components, one of the entities has all the components | 0.0067667s | **5.38e-07s** |
| Iterating over 50M entities, unpacking one component | 0.0530271s | **7.7e-08s** |
| Iterating over 50M entities, unpacking two components | 0.056233s | **0.0161715s** |

`EnTT` includes its own tests and benchmarks. See [benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp) for further details.<br/>
On Github users can find also a [benchmark suite](https://github.com/abeimler/ecs_benchmark) that compares a bunch of different projects, one of which is `EnTT`.

Of course, probably I'll try to get out of `EnTT` more features and better performance in the future, mainly for fun.<br/>
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

`EnTT` has two main actors: the **Registry** and the **View**.<br/>
The former can be used to manage components, entities and collections of components and entities. The latter allows users to iterate the underlying collections.

#### The Registry

There are two options to instantiate a registry:

* Use the `DefaultRegistry` alias:

    ```cpp
    auto registry = entt::DefaultRegistry<Components...>{args...};
    ```

  Users must provide the whole list of components to be registered with the default registry and that's all.

* Use directly the `Registry` class template:

    ```cpp
    auto registry = entt::Registry<std::uint16_t, Components...>{args...};
    ```

  Users must provide the whole list of components to be registered with the registry **and** the desired type for the entities.
  Note that the default type (the one used by the default registry) is `std::uint32_t`, that is larger enough for almost all the games but also too big for the most of the games.

In both cases there are no requirements for the components but to be moveable, therefore POD types are just fine.

The `Registry` class offers a bunch of basic functionalities to query the internal data structures.
In almost all the cases those member functions can be used to query either the entity list or the components lists.<br/>
As an example, the member functions `empty` can be used to know if at least an entity exists and/or if at least one component of the given type has been assigned to an entity.<br/>

```cpp
bool b = registry.empty();
// ...
bool b = registry.empty<MyComponent>();
```

Similarly, `size` can be used to know the number of entities alive and/or the number of components of a given type still assigned to entities. `capacity` follows the same pattern and returns the storage capacity for the given element.

The `valid` member function returns true if `entity` is still in use, false otherwise:

```cpp
bool b = registry.valid(entity);
```

Boring, I agree. Let's go to something more tasty.
The following functionalities are meant to give users the chance to play with entities and components within a registry.

The `create` member function can be used to construct a new entity and it comes in two flavors:

* The plain version just creates a _naked_ entity with no components assigned to it:

    ```cpp
    auto entity = registry.create();
    ```

* The member function template creates an entity and assigns to it the given _default-initialized_ components:

    ```cpp
    auto entity = registry.create<Position, Velocity>();
    ```

  It's a helper function, mostly syncactic sugar and it's equivalent to the following snippet:

    ```cpp
    auto entity = registry.create();
    registry.assign<Position>();
    registry.assign<Velocity>();
    ```

  See below to find more about the `assign` member function.

On the other side, the `destroy` member function can be used to delete an entity and all its components (if any):

```cpp
registry.destroy(entity);
```

It requires that `entity` is valid. In case it is not, an assertion will fail in debug mode and the behaviour is undefined in release mode.

If the purpose is to remove a single component instead, the `remove` member function template is the way to go:

```cpp
registry.remove<Position>(entity);
```

Again, it requires that `entity` is valid. Moreover, an instance of the component must have been previously assigned to the entity.
If one of the requirements isn't satisfied, an assertion will fail in debug mode and the behaviour is undefined in release mode.

The `reset` member function behaves similarly but with a strictly defined behaviour (and a performance penalty is the price to pay for that). In particular it removes the component if and only if it exists, otherwise it returns safely to the caller:

```cpp
registry.reset<Position>(entity);
```

It requires only that `entity` is valid. In case it is not, an assertion will fail in debug mode and the behaviour is undefined in release mode.

There exist also two more _versions_ of the `reset` member function:

* If no entity is passed to it, `reset` will remove the given component from each entity that has it:

    ```cpp
    registry.reset<Position>();
    ```

* If neither the entity nor the component are specified, all the entities and their components are destroyed:

    ```cpp
    registry.reset();
    ```

  **Note**: the registry has an assert in debug mode that verifies that entities are no longer valid when it's destructed. This function can be used to reset the registry to its initial state and thus to satisfy the requirement.

To assign a component to an entity, users can rely on the `assign` member function template. It accepts a variable number of arguments that are used to construct the component itself if present:

```cpp
registry.assign<Position>(entity, 0., 0.);
// ...
auto &velocity = registry.assign<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

It requires that `entity` is valid. Moreover, the entity shouldn't have another instance of the component assigned to it.
If one of the requirements isn't satisfied, an assertion will fail in debug mode and the behaviour is undefined in release mode.

If the entity already has the given component and the user wants to replace it, the `replace` member function template is the way to go:

```cpp
registry.replace<Position>(entity, 0., 0.);
// ...
auto &velocity = registry.replace<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

It requires that `entity` is valid. Moreover, an instance of the component must have been previously assigned to the entity.
If one of the requirements isn't satisfied, an assertion will fail in debug mode and the behaviour is undefined in release mode.

In case users want to assign a component to an entity, but it's unknown whether the entity already has it or not, `accomodate` does the work in a single call
(of course, there is a performance penalty to pay for that mainly due to the fact that it must check if `entity` already has the given component or not):

```cpp
registry.accomodate<Position>(entity, 0., 0.);
// ...
auto &velocity = registry.accomodate<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

It requires only that `entity` is valid. In case it is not, an assertion will fail in debug mode and the behaviour is undefined in release mode.<br/>
Note that `accomodate` is a sliglhty faster alternative for the following if/else statement and nothing more:

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

It requires only that `entity` is valid. In case it is not, an assertion will fail in debug mode and the behaviour is undefined in release mode.

Entities can also be cloned and either partially or fully copied:

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

All the functions above mentioned require that entities provided as arguments are valid and components exist wherever they have to be accessed.
In case they are not, an assertion will fail in debug mode and the behaviour is undefined in release mode.

There exists also an utility member function that can be used to `swap` components between entities:

```cpp
registry.swap<Position>(e1, e2);
```

As usual, it requires that the two entities are valid and that two instances of the component have been previously assigned to them.
In case they are not, an assertion will fail in debug mode and the behaviour is undefined in release mode.

The `get` member function template (either the non-const or the const version) gives direct access to the component of an entity instead:

```cpp
auto &position = registry.get<Position>(entity);
```

It requires that `entity` is valid. Moreover, an instance of the component must have been previously assigned to the entity.
If one of the requirements isn't satisfied, an assertion will fail in debug mode and the behaviour is undefined in release mode.

Components can also be sorted in memory by means of the `sort` member function templates. In particular:

* Components can be sorted according to a component:

    ```cpp
    registry.sort<Renderable>([](const auto &lhs, const auto &rhs) { return lhs.z < rhs.z; });
    ```

* Components can be sorted according to the order imposed by another component:

    ```cpp
    registry.sort<Movement, Physics>();
    ```

  In this case, instances of `Movement` are arranged in memory so that cache misses are minimized when the two components are iterated together.

Finally, the `view` member function template returns an iterable portion of entities and components:

```cpp
auto view = registry.view<Position, Velocity>();
```

Views are the other core component of `EnTT` and are usually extensively used by softwares that include it. See below for more details about the types of views.

#### The View

There are two types of views:

* **Single component view**.

  A single component view gives direct access to both the components and the entities to which the components are assigned.<br/>
  This kind of views are created from the `Registry` class by means of the `view` member function template as it follows:

    ```cpp
    // Actual type is Registry<Components...>::view_type<Comp>, where Comp is the component for which the view should be created ...
    // ... well, auto is far easier to use in this case, isn't it?
    auto view = registry.view<Sprite>();
    ```

  Components and entities are stored in tightly packed arrays and single component views are the fastest solution to iterate them.<br/>
  They have the _C++11-ish_ `begin` and `end` member function that allow users to use them in a typical range-for loop:

    ```cpp
    auto view = registry.view<Sprite>();

    for(auto entity: view) {
        auto &sprite = registry.get<Sprite>(entity);
        // ...
    }
    ```

  Iterating a view this way returns entities that can be further used to get components or perform other activities.<br/>
  There is also another method one can use to iterate the array of entities, that is by using the `size` and `data` member functions:

    ```cpp
    auto view = registry.view<Sprite>();
    const auto *data = view.data();

    for(auto i = 0, end = view.size(); i < end; ++i) {
        auto entity = *(data + i);
        // ...
    }
    ```

  Entites are good when the sole component isn't enough to perform a task.
  Anyway they come with a cost: accessing components by entities has an extra level of indirection. It's pretty fast, but not that fast in some cases.<br/>
  Direct access to the packed array of components is the other option around of a single component view. Member functions `size` and `raw` are there for that:

    ```cpp
    auto view = registry.view<Sprite>();
    const auto *raw = view.raw();

    for(auto i = 0, end = view.size(); i < end; ++i) {
        auto &sprite = *(raw + i);
        // ...
    }
    ```

  This is the fastest solution to iterate over the components: they are packed together by construction and visit them in order will reduce to a minimum the number of cache misses.

* **Multi component view**.

  A multi component view gives access only to the entities to which the components are assigned.<br/>
  This kind of views are created from the `Registry` class by means of the `view` member function template as it follows:

    ```cpp
    // Actual type is Registry<Components...>::view_type<Comp...>, where Comp... are the components for which the view should be created ...
    // ... well, auto is far easier to use in this case, isn't it?
    auto view = registry.view<Position, Velocity>();
    ```

  Multi component views can be iterated by means of the `begin` and `end` member functions in a typical range-for loop:

    ```cpp
    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = registry.get<Position>(entity);
        auto &velocity = registry.get<Velocity>(entity);
        // ...
    }
    ```

  Note that there exists a packed array of entities to which the component is assigned for each component.
  Iterators of a multi component view pick the shortest array up and use it to visit the smallest set of potential entities.<br/>
  The choice is performed when the view is constructed. It's good enough as long as views are discarded once they have been used.
  For all the other cases, the `reset` member function can be used whenever the data within the registry are known to be changed and forcing the choice again could speed up the execution.

  **Note**: one could argue that an iterator should return the set of references to components for each entity instead of the entity itself.
  Well, who wants to spend CPU cycles to get a reference to an useless tag component? This drove the design choice indeed.

All the views can be used more than once. They return newly created and correctly initialized iterators whenever `begin` or `end` is invoked.
The same is valid for `data` and `raw` too. Anyway views and iterators are tiny objects and the time spent to construct them can be safely ignored.<br/>
I'd suggest not to store them anywhere and to invoke the `Registry::view` member function template at each iteration to get a properly initialized view through which to iterate.

#### Side notes

* Entities are numbers and nothing more. They are not classes and they have no member functions at all.

* Most of the _ECS_ available out there have an annoying limitation (at least from my point of view): entities and components cannot be created, assigned or deleted while users are iterating on them.<br/>
  `EnTT` partially solves the problem with a few limitations:

    * Entities can be created at any time while iterating one or more components.
    * Components can be assigned to any entity at any time while iterating one or more components.
    * During an iteration, the current entity (that is the one returned by the iterator) can be deleted and all its components can be removed safely.

  Entities that are not the current one (that is the one returned by the iterator) cannot be deleted from within a loop.<br/>
  Components assigned to entities that are not the current one (that is the one returned by the iterator) cannot be removed from within a loop.<br/>
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
