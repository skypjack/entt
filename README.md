# The EnTT Framework

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/entt)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

# Introduction

`EnTT` is a header-only, tiny and easy to use framework written in modern
C++.<br/>
It's entirely designed around an architectural pattern pattern called _ECS_ that
is used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

Originally, `EnTT` was written as a faster alternative to other well known and
open source entity-component systems.<br/>
After a while the codebase has grown and more features have become part of the
framework.

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

void update(entt::DefaultRegistry &registry) {
    auto view = ecs.view<Position, Velocity>();

    for(auto entity: view) {
        auto &position = view.get<Position>(entity);
        auto &velocity = view.get<Velocity>(entity);
        // ...
    }
}

int main() {
    entt::DefaultRegistry registry;

    for(auto i = 0; i < 10; ++i) {
        auto entity = registry.create();
        registry.assign<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { registry.assign<Velocity>(entity, i * .1f, i * .1f); }
    }

    update(registry);
    // ...
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to design
an entity-component system that beated another well known open source solution
in terms of performance.<br/>
I did it, of course, but it wasn't much satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my* entity-component system at the same
time.

Today `EnTT` is finally what I was looking for: still faster than its _rivals_,
a really good API and an amazing set of features. And even more, of course.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if
compared to my first choice (that was already amazingly fast indeed).<br/>
Here is a comparision between the two (both of them compiled with GCC 7.2.0 on a
Dell XPS 13 out of the mid 2014):

| Benchmark | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|
| Creating 10M entities | 0.128881s | **0.0408754s** |
| Destroying 10M entities | **0.0531374s** | 0.0545839s |
| Iterating over 10M entities, unpacking one component, standard view | 0.010661s | **1.58e-07s** |
| Iterating over 10M entities, unpacking two components, standard view | **0.0112664s** | 0.0840068s |
| Iterating over 10M entities, unpacking two components, standard view, half of the entities have all the components | **0.0077951s** | 0.042168s |
| Iterating over 10M entities, unpacking two components, standard view, one of the entities has all the components | 0.00713398s | **8.93e-07s** |
| Iterating over 10M entities, unpacking two components, persistent view | 0.0112664s | **5.68e-07s** |
| Iterating over 10M entities, unpacking five components, standard view | **0.00905084s** | 0.137757s |
| Iterating over 10M entities, unpacking five components, persistent view | 0.00905084s | **2.9e-07s** |
| Iterating over 10M entities, unpacking ten components, standard view | **0.0104708s** | 0.388602s |
| Iterating over 10M entities, unpacking ten components, standard view, half of the entities have all the components | **0.00899859s** | 0.200752s |
| Iterating over 10M entities, unpacking ten components, standard view, one of the entities has all the components | 0.00700349s | **2.565e-06s** |
| Iterating over 10M entities, unpacking ten components, persistent view | 0.0104708s | **6.23e-07s** |
| Iterating over 50M entities, unpacking one component, standard view | 0.055194s | **2.87e-07s** |
| Iterating over 50M entities, unpacking two components, standard view | **0.0533921s** | 0.243197s |
| Iterating over 50M entities, unpacking two components, persistent view | 0.055194s | **4.47e-07s** |
| Sort 150k entities, one component | - | **0.0080046s** |
| Sort 150k entities, match two components | - | **0.00608322s** |

`EnTT` includes its own tests and benchmarks. See
[benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp)
for further details.<br/>
On Github users can find also a
[benchmark suite](https://github.com/abeimler/ecs_benchmark) that compares a
bunch of different projects, one of which is `EnTT`.

Of course, probably I'll try to get out of `EnTT` more features and better
performance in the future, mainly for fun.<br/>
If you want to contribute and/or have any suggestion, feel free to make a PR or
open an issue to discuss your idea.

# Build Instructions

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that
supports at least C++14.<br/>
The requirements below are mandatory to compile the tests and to extract the
documentation:

* CMake version 3.2 or later.
* Doxygen version 1.8 or later.

## Library

`EnTT` is a header-only library. This means that including the `registry.hpp`
header is enough to use it.<br/>
It's a matter of adding the following line at the top of a file:

```cpp
#include <registry.hpp>
```

Then pass the proper `-I` argument to the compiler to add the `src` directory to
the include paths.

## Documentation

### API Reference

The documentation is based on [doxygen](http://www.stack.nl/~dimitri/doxygen/).
To build it:

$ cd build
$ cmake ..
$ make docs

The API reference will be created in HTML format within the directory
`build/docs/html`. To navigate it with your favorite browser:

$ cd build
$ your_favorite_browser docs/html/index.html

The API reference is also available [online](https://skypjack.github.io/entt/)
for the latest version.

### Crash Course

#### Vademecum

The `Registry` to store, the `View`s to iterate. That's all.

An entity (the _E_ of an _ECS_) is an opaque identifier that users should just
use as-is and store around if needed. Do not try to inspect an entity
identifier, its type can change in future and a registry offers all the
functionalities to query them out-of-the-box. The underlying type of an entity
(either `std::uint16_t`, `std::uint32_t` or `std::uint64_t`) can be specified
when defining a registry (actually the DefaultRegistry is nothing more than a
Registry where the type of the entities is `std::uint32_t`).<br/>
Components (the _C_ of an _ECS_) should be plain old data structures or more
complex and moveable data structures with a proper constructor. They are list
initialized by using the parameters provided to construct the component. No need
to register components or their types neither with the registry nor with the
entity-component system at all.<br/>
Systems (the _S_ of an _ECS_) are just plain functions, functors, lambdas or
whatever the users want. They can accept a Registry, a View or a PersistentView
and use them the way they prefer. No need to register systems or their types
neither with the registry nor with the entity-component system at all.

The following sections will explain in short how to use the entity-component
system, the core part of the `EnTT` framework.<br/>
In fact, the framework is composed of many other classes in addition to those
describe below. For more details, please refer to the
[online documentation](https://skypjack.github.io/entt/).

#### The Registry, the Entity and the Component

A registry is used to store and manage entities as well as to create views to
iterate the underlying data structures.<br/>
Registry is a class template that lets the users decide what's the preferred
type to represent an entity. Because `std::uint32_t` is large enough for almost
all the cases, there exists also an alias named DefaultRegistry for
`Registry<std::uint32_t>`.

Entities are represented by _entitiy identifiers_. An entity identifier is an
opaque type that users should not inspect or modify in any way. It carries
information about the entity itself and its version.

A registry can be used both to construct and to destroy entities:

```cpp
// constructs a naked entity with no components ad returns its identifier
auto entity = registry.create();

// constructs an entity and assigns it default-initialized components
auto another = registry.create<Position, Velocity>();

// destroys an entity and all its components
registry.destroy(entity);
```

Once an entity is deleted, the registry can freely reuse it internally with a
slightly different identifier. In particular, the version of an entity is
increased each and every time it's destroyed.<br/>
In case entity identifiers are stored around, the registry offers all the
functionalities required to test them and get out of the them all the
information they carry:

```cpp
// returns true if the entity is still valid, false otherwise
bool b = registry.valid(entity);

// gets the version contained in the entity identifier
auto version = registry.version(entity);

// gets the actual version for the given entity
auto curr = registry.current(entity);
```

Components can be assigned to or removed from entities at any time with a few
calls to member functions of the registry. As for the entities, the registry
offers also a set of functionalities users can use to work with the components.

The `assign` member function template creates, initializes and assigns to an
entity the given component. It accepts a variable number of arguments that are
used to construct the component itself if present:

```cpp
registry.assign<Position>(entity, 0., 0.);

// ...

auto &velocity = registry.assign<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

If the entity already has the given component, the `replace` member function
template can be used to replace it:

```cpp
registry.replace<Position>(entity, 0., 0.);

// ...

auto &velocity = registry.replace<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

In case users want to assign a component to an entity, but it's unknown whether
the entity already has it or not, `accomodate` does the work in a single call
(of course, there is a performance penalty to pay for that mainly due to the
fact that it must check if `entity` already has the given component or not):

```cpp
registry.accomodate<Position>(entity, 0., 0.);

// ...

auto &velocity = registry.accomodate<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

Note that `accomodate` is a sliglhty faster alternative for the following
`if`/`else` statement and nothing more:

```cpp
if(registry.has<Comp>(entity)) {
    registry.replace<Comp>(entity, arg1, argN);
} else {
    registry.assign<Comp>(entity, arg1, argN);
}
```

As already shown, if in doubt about whether or not an entity has one or more
components, the `has` member function template may be useful:

```cpp
bool b = registry.has<Position, Velocity>(entity);
```

On the other side, if the goal is to delete a single component, the `remove`
member function template is the way to go when it's certain that the entity owns
a copy of the component:

```cpp
registry.remove<Position>(entity);
```

Otherwise consider to use the `reset` member function. It behaves similarly to
`remove` but with a strictly defined behaviour (and a performance penalty is the
price to pay for that). In particular it removes the component if and only if it
exists, otherwise it returns safely to the caller:

```cpp
registry.reset<Position>(entity);
```

There exist also two other _versions_ of the `reset` member function:

* If no entity is passed to it, `reset` will remove the given component from
each entity that has it:

    ```cpp
    registry.reset<Position>();
    ```

* If neither the entity nor the component are specified, all the entities and
their components are destroyed:

    ```cpp
    registry.reset();
    ```

Finally, references to components can be retrieved by just doing this:

```cpp
// either a non-const reference ...
DefaultRegistry registry;
auto &position = registry.get<Position>(entity);

// ... or a const one
const auto &cregistry = registry;
const auto &position = cregistry.get<Position>(entity);
```

The `get` member function template gives direct access to the component of an
entity stored in the underlying data structures of the registry.

##### Sorting: is it possible?

Of course, sorting entities and components is an option with `EnTT`.<br/>
In fact, there are two functions that respond to slightly different needs:

* Components can be sorted directly:

    ```cpp
    registry.sort<Renderable>([](const auto &lhs, const auto &rhs) {
        return lhs.z < rhs.z;
    });
    ```

* Components can be sorted according to the order imposed by another component:

    ```cpp
    registry.sort<Movement, Physics>();
    ```

  In this case, instances of `Movement` are arranged in memory so that cache
  misses are minimized when the two components are iterated together.

#### View: to be or not to be (persistent)?

TODO

<!--
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
-->

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
`cmake` will download and compile the library before to compile anything else.

To build the tests:

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
Docs released under [Creative Commons](https://github.com/skypjack/entt/blob/master/docs/LICENSE).

# Donation

Developing and maintaining `EnTT` takes some time and lots of coffee. It still lacks a proper test suite, documentation is partially incomplete and not all functionalities have been fully implemented yet.<br/>
If you want to support this project, you can offer me an espresso. I'm from Italy, we're used to turning the best coffee ever in code.<br/>
Take a look at the donation button at the top of the page for more details or just click [here](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).
