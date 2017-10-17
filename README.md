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

## Performance

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

`EnTT` is a header-only library. This means that including the `entt.hpp`
header is enough to include the whole framework and use it. For those who are
interested only in the entity-component system, consider to include the sole
`entity/registry.hpp` header instead.<br/>
It's a matter of adding the following line to the top of a file:

```cpp
#include <entt/entt.hpp>
```

Use the line below to include only the entity-component system instead:

```cpp
#include <entt/entity/registry.hpp>
```

Then pass the proper `-I` argument to the compiler to add the `src` directory to
the include paths.

## Documentation

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

# Crash Course

## Vademecum

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

## The Registry, the Entity and the Component

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

### Sorting: is it possible?

Of course, sorting entities and components is possible with `EnTT`.<br/>
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

## View: to persist or not to persist?

There are mainly two kinds of views: standard (also known as View) and
persistent (alsa known as PersistentView).<br/>
Both of them have pros and cons to take in consideration. In particular:

* Standard views:

  Pros:
  * They work out-of-the-box and don't require any dedicated data
    structure.
  * Creating and destroying them isn't expensive at all because they don't
    have any type of initialization.
  * They are the best tool to iterate single components.
  * They are the best tool to iterate multiple components at once when
    tags are involved or one of the component is assigned to a
    significantly low number of entities.
  * They don't affect any other operations of the registry.

  Cons:
  * Their performance tend to degenerate when the number of components
    to iterate grows up and the most of the entities have all of them.

* Persistent views:

  Pros:
  * Once prepared, creating and destroying them isn't expensive at all
    because they don't have any type of initialization.
  * They are the best tool to iterate multiple components at once when
    the most of the entities have all of them.

  Cons:
    * They have dedicated data structures and thus affect the memory
      pressure to a minimal extent.
    * If not previously prepared, the first time they are used they go
      through an initialization step that could take a while.
    * They affect to a minimum the creation and destruction of entities and
      components. In other terms: the more persistent views there will be,
      the less performing will be creating and destroying entities and
      components.

To sum up and as a rule of thumb, use a standard view:
* To iterate entities for a single component.
* To iterate entities for multiple components when a significantly low
  number of entities have one of the components.
* In all those cases where a persistent view would give a boost to
  performance but the iteration isn't performed frequently.

Use a persistent view in all the other cases.

To easily iterate entities, all the views offer _C++-ish_ `begin` and `end`
member functions that allow users to use them in a typical range-for loop.<br/>
Continue reading for more details or refer to the
[official documentation](https://skypjack.github.io/entt/).

### Standard View

A standard view behaves differently if it's constructed for a single component
or if it has been requested to iterate multiple components. Even the API is
different in the two cases.<br/>
All that they share is the way they are created by means of a registry:

```cpp
// single component standard view
auto single = registry.view<Position>();

// multi component standard view
auto multi = registry.view<Position, Velocity>();
```

For all that remains, it's worth discussing them separately.<br/>

#### Single component standard view

Single component standard views are specialized in order to give a boost in
terms of performance in all the situation. This kind of views can access the
underlying data structures directly and avoid superflous checks.<br/>
They offer a bunch of functionalities to get the number of entities they are
going to return and a raw access to the entity list as well as to the component
list.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused
freely. In fact, they return newly created and correctly initialized iterators
whenever `begin` or `end` are invoked.<br/>
To iterate a single component standard view, just use it in range-for:

```cpp
auto view = registry.view<Renderable>();

for(auto entity: view) {
    auto &renderable = view.get(entity);

    // ...
}
```

**Note**: prefer the `get` member function of the view instead of the `get`
member function template of the registry during iterations.

#### Multi component standard view

Multi component standard views iterate entities that have at least all the given
components in their bags. During construction, these views look at the number
of entities available for each component and pick up a reference to the smallest
set of candidates in order to speed up iterations.<br/>
They offer fewer functionalities than their companion views for single
component, the most important of which can be used to reset the view and refresh
the reference to the set of candidate entities to iterate.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused
freely. In fact, they return newly created and correctly initialized iterators
whenever `begin` or `end` are invoked.<br/>
To iterate a multi component standard view, just use it in range-for:

```cpp
auto view = registry.view<Position, Velocity>();

for(auto entity: view) {
    auto &position = view.get<Position>(entity);
    auto &velocity = view.get<Velocity>(entity);

    // ...
}
```

**Note**: prefer the `get` member function template of the view instead of the
`get` member function template of the registry during iterations.

### Persistent View

A persistent view returns all the entities and only the entities that have at
least the given components. Moreover, it's guaranteed that the entity list is
thightly packed in memory for fast iterations.<br/>
In general, persistent views don't stay true to the order of any set of
components unless users explicitly sort them.

Persistent views can be used only to iterate multiple components. Create them
as it follows:

```cpp
auto view = registry.persistent<Position, Velocity>();
```

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused
freely. In fact, they return newly created and correctly initialized iterators
whenever `begin` or `end` are invoked.<br/>
That being said, persistent views perform an initialization step the very first
time they are constructed and this could be quite costly. To avoid it, consider
asking to the registry to _prepare_ them when no entities have been created yet:

```cpp
registry.prepare<Position, Velocity>();
```

If the registry is empty, preparation is extremely fast. Moreover the `prepare`
member function template is idempotent. Feel free to invoke it even more than
once: if the view has been alreadt prepared before, the function returns
immediately and does nothing.

A persistent view offers a bunch of functionalities to get the number of
entities it's going to return, a raw access to the entity list and the
possibility to sort the underlying data structures according to the order of one
of the components for which it has been constructed.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

To iterate a persistent view, just use it in range-for:

```cpp
auto view = registry.persistent<Position, Velocity>();

for(auto entity: view) {
    auto &position = view.get<Position>(entity);
    auto &velocity = view.get<Velocity>(entity);

    // ...
}
```

**Note**: prefer the `get` member function template of the view instead of the
`get` member function template of the registry during iterations.

## Side notes

* Entity identifiers are numbers and nothing more. They are not classes and they
  have no member functions at all. As already mentioned, do no try to inspect or
  modify an entity descriptor in any way.

* As shown in the examples above, the preferred way to get references to the
  components while iterating a view is by using the view itself. It's a faster
  alternative to the `get` member function template that is part of the API of
  the Registry. That's because the registry must ensure that a pool for the
  given component exists before to use it; on the other side, views force the
  construction of the pools for all their components and access them directly,
  thus avoiding all the checks.

* Most of the _ECS_ available out there have an annoying limitation (at least
  from my point of view): entities and components cannot be created and/or
  deleted during iterations.<br/>
  `EnTT` partially solves the problem with a few limitations:

  * Creating entities and components is allowed during iterations.
  * Deleting an entity or removing its components is allowed during
    iterations if it's the one currently returned by a view. For all the
    other entities, destroying them or removing their components isn't
    allowed and it can result in undefined behavior.

  Iterators are invalidated and the behaviour is undefined if an entity is
  modified or destroyed and it's not the one currently returned by the
  view.<br/>
  To work around it, possible approaches are:

    * Store aside the entities and the components to be removed and perform the
      operations at the end of the iteration.
    * Mark entities and components with a proper tag component that indicates
      they must be purged, then perform a second iteration to clean them up one
      by one.

* Views and thus their iterators aren't thread safe. Do no try to iterate a set
  of components and modify the same set concurrently.<br/>
  That being said, as long as a thread iterates the entities that have the
  component `X` or assign and removes that component from a set of entities,
  another thread can safely do the same with components `Y` and `Z` and
  everything will work like a charm.<br/>
  As an example, users can freely execute the rendering system and iterate the
  renderable entities while updating a physic component concurrently on a
  separate thread if needed.

## What else?

The `EnTT` framework is moving its first steps. More and more will come in the
future and hopefully I'm going to work on it for a long time.<br/>
Here is a brief list of what it offers today:

* Statically generated integer identifiers for types.
* An entity-component system based on sparse sets.
* Signal handlers and event emitters of any type.
* ...
* Any other business.

Consider it a work in progress. For more details and an updated list, please
refer to the [online documentation](https://skypjack.github.io/entt/).

# Contributors

If you want to contribute, please send patches as pull requests against the
branch `master`.<br/>
Check the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to see
who has partecipated so far.

# License

Code and documentation Copyright (c) 2017 Michele Caini.<br/>
Code released under
[the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).
Docs released under
[Creative Commons](https://github.com/skypjack/entt/blob/master/docs/LICENSE).

# Donation

Developing and maintaining `EnTT` takes some time and lots of coffee. I'd like
to add more and more functionalities in future and turn it in a full-featured
framework.<br/>
If you want to support this project, you can offer me an espresso. I'm from
Italy, we're used to turning the best coffee ever in code. If you find that
it's not enough, feel free to support me the way you prefer.<br/>
Take a look at the donation button at the top of the page for more details or
just click [here](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).
