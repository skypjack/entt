# EnTT Framework

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/entt)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

# Table of Contents

   * [Introduction](#introduction)
   * [Build Instructions](#build-instructions)
   * [Crash Course: entity-component system](#crash-course-entity-component-system)
      * [Design choices](#design-choices)
         * [A bitset-free entity-component system](#a-bitset-free-entity-component-system)
         * [Pay per use](#pay-per-use)
      * [Vademecum](#vademecum)
      * [The Registry, the Entity and the Component](#the-registry-the-entity-and-the-component)
         * [Single instance components](#single-instance-components)
         * [Runtime components](#runtime-components)
            * [A journey through a plugin](#a-journey-through-a-plugin)
         * [Sorting: is it possible?](#sorting-is-it-possible)
      * [View: to persist or not to persist?](#view-to-persist-or-not-to-persist)
         * [Standard View](#standard-view)
            * [Single component standard view](#single-component-standard-view)
            * [Multi component standard view](#multi-component-standard-view)
         * [Persistent View](#persistent-view)
         * [Give me everything](#give-me-everything)
      * [Spaces](#spaces)
      * [Side notes](#side-notes)
   * [Crash Course: core functionalities](#crash-course-core-functionalities)
      * [Compile-time identifiers](#compile-time-identifiers)
      * [Runtime identifiers](#runtime-identifiers)
      * [Hashed strings](#hashed-strings)
   * [Crash Course: service locator](#crash-course-service-locator)
   * [Crash Course: cooperative scheduler](#crash-course-cooperative-scheduler)
      * [The process](#the-process)
      * [The scheduler](#the-scheduler)
   * [Crash Course: resource management](#crash-course-resource-management)
      * [The resource, the loader and the cache](#the-resource-the-loader-and-the-cache)
   * [Crash Course: events, signals and everything in between](#crash-course-events-signals-and-everything-in-between)
      * [Signals](#signals)
      * [Compile-time event bus](#compile-time-event-bus)
      * [Delegate](#delegate)
      * [Event dispatcher](#event-dispatcher)
      * [Event emitter](#event-emitter)
   * [License](#license)
   * [Support](#support)

# Introduction

`EnTT` is a header-only, tiny and easy to use framework written in modern
C++.<br/>
It was originally designed entirely around an architectural pattern called _ECS_
that is used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

A long time ago, the sole entity-component system was part of the project. After
a while the codebase has grown and more and more classes have become part
of the repository.<br/>
That's why today it's called _the EnTT Framework_.

Currently, `EnTT` is tested on Linux, Microsoft Windows and OS X. It has proven
to work also on both Android and iOS.<br/>
Most likely it will not be problematic on other systems as well, but has not
been sufficiently tested so far.

## The framework

`EnTT` was written initially as a faster alternative to other well known and
open source entity-component systems. Nowadays the `EnTT` framework is moving
its first steps. Much more will come in the future and hopefully I'm going to
work on it for a long time.<br/>
Requests for feature, PR, suggestions ad feedback are highly appreciated.

If you find you can help me and want to contribute to the `EnTT` framework with
your experience or you do want to get part of the project for some other
reasons, feel free to contact me directly (you can find the mail in the
[profile](https://github.com/skypjack)).<br/>
I can't promise that each and every contribution will be accepted, but I can
assure that I'll do my best to take them all seriously.

### State of the art

Here is a brief list of what it offers today:

* Statically generated integer identifiers for types (assigned either at
compile-time or at runtime).
* A constexpr utility for human readable resource identifiers.
* An incredibly fast entity-component system based on sparse sets, with its own
views and a _pay for what you use_ policy to adjust performance and memory usage
according to the users' requirements.
* Spaces, a nice and easy way to create partitions between entities.
* Actor class for those who aren't confident with entity-component systems.
* The smallest and most basic implementation of a service locator ever seen.
* A cooperative scheduler for processes of any type.
* All what is needed for resource management (cache, loaders, handles).
* Signal handlers of any type, delegates and an event bus.
* A general purpose event emitter, that is a CRTP idiom based class template.
* An event dispatcher for immediate and delayed events to integrate in loops.
* ...
* Any other business.

Consider it a work in progress. For more details and an updated list, please
refer to the [online documentation](https://skypjack.github.io/entt/). It
probably contains much more. Moreover, the whole API is fully documented
in-code for those who are brave enough to read it.<br/>
Continue reading to know how the different parts of the project work or follow
the link above to take a look at the API reference.

## Code Example

```cpp
#include <entt/entt.hpp>
#include <cstdint>

struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

void update(entt::DefaultRegistry &registry) {
    auto view = registry.view<Position, Velocity>();

    for(auto entity: view) {
        // gets only the components that are going to be used ...

        auto &velocity = view.get<Velocity>(entity);

        velocity.dx = 0.;
        velocity.dy = 0.;

        // ...
    }
}

void update(std::uint64_t dt, entt::DefaultRegistry &registry) {
    registry.view<Position, Velocity>().each([dt](auto entity, auto &position, auto &velocity) {
        // gets all the components of the view at once ...

        position.x += velocity.dx * dt;
        position.y += velocity.dy * dt;

        // ...
    });
}

int main() {
    entt::DefaultRegistry registry;
    std::uint64_t dt = 16;

    for(auto i = 0; i < 10; ++i) {
        auto entity = registry.create(Position{i * 1.f, i * 1.f});
        if(i % 2 == 0) { registry.assign<Velocity>(entity, i * .1f, i * .1f); }
    }

    update(dt, registry);
    update(registry);

    // ...
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to design
an entity-component system that beated another well known open source solution
in terms of performance and used (possibly) less memory in the average
case.<br/>
In the end, I did it, but it wasn't much satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my own library* at the same time.

Today `EnTT` is finally what I was looking for: still faster than its
_competitors_, lower memory usage in the average case, a really good API and an
amazing set of features. And even more, of course.

## Performance

As it stands right now, `EnTT` is just fast enough for my requirements if
compared to my first choice (it was already amazingly fast actually).<br/>
Below is a comparison between the two (both of them compiled with GCC 7.3.0 on a
Dell XPS 13 out of the mid 2014):

| Benchmark | EntityX (compile-time) | EnTT |
|-----------|-------------|-------------|
| Create 1M entities | 0.0167s | **0.0046s** |
| Destroy 1M entities | 0.0053s | **2.8e-06s** |
| Standard view, 1M entities, one component | 0.0012s | **1.9e-07s** |
| Standard view, 1M entities, two components | 0.0012s | **3.8e-07s** |
| Standard view, 1M entities, two components<br/>Half of the entities have all the components | 0.0009s | **3.8e-07s** |
| Standard view, 1M entities, two components<br/>One of the entities has all the components | 0.0008s | **1.0e-06s** |
| Persistent view, 1M entities, two components | 0.0012s | **2.8e-07s** |
| Standard view, 1M entities, five components | 0.0010s | **7.0e-07s** |
| Persistent view, 1M entities, five components | 0.0010s | **2.8e-07s** |
| Standard view, 1M entities, ten components | 0.0011s | **1.2e-06s** |
| Standard view, 1M entities, ten components<br/>Half of the entities have all the components | 0.0010s | **1.2e-06s** |
| Standard view, 1M entities, ten components<br/>One of the entities has all the components | 0.0008s | **1.2e-06s** |
| Persistent view, 1M entities, ten components | 0.0011s | **3.0e-07s** |
| Sort 150k entities, one component<br/>Arrays are in reverse order | - | **0.0036s** |
| Sort 150k entities, enforce permutation<br/>Arrays are in reverse order | - | **0.0005s** |

Note: The default version of `EntityX` (`master` branch) wasn't added to the
comparison because it's already much slower than its compile-time
counterpart.

Pretty interesting, aren't them? In fact, these benchmarks are the same used by
`EntityX` to show _how fast it is_. To be honest, they aren't so good and these
results shouldn't be taken much seriously (they are completely unrealistic
indeed).<br/>
The proposed entity-component system is incredibly fast to iterate entities,
this is a fact. The compiler can make a lot of optimizations because of how
`EnTT` works, even more when components aren't used at all. This is exactly the
case for these benchmarks.<br/>
This is why they are completely wrong and cannot be used to evaluate any of the
entity-component systems.

If you decide to use `EnTT`, choose it because of its API and its performance,
not because there is a benchmark somewhere that makes it seem the fastest.

Probably I'll try to get out of `EnTT` more features and even better performance
in the future, mainly for fun.<br/>
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

To build the most basic set of tests:

* `$ cd build`
* `$ cmake ..`
* `$ make`
* `$ make test`

Note that benchmarks are not part of this set.

# Crash Course: entity-component system

## Design choices

### A bitset-free entity-component system

`EnTT` is a _bitset-free_ entity-component system that doesn't require users to
specify the component set at compile-time.<br/>
This is why users can instantiate the core class simply like:

```cpp
entt::DefaultRegistry registry;
```

In place of its more annoying and error-prone counterpart:

```cpp
entt::DefaultRegistry<Comp0, Comp1, ..., CompN> registry;
```

### Pay per use

`EnTT` is entirely designed around the principle that users have to pay only for
what they want.

When it comes to using an entity-component system, the tradeoff is usually
between performance and memory usage. The faster it is, the more memory it uses.
However, slightly worse performance along non-critical paths are the right price
to pay to reduce memory usage and I've always wondered why this kind of tools do
not leave me the choice.<br/>
`EnTT` follows a completely different approach. It squeezes the best from the
basic data structures and gives users the possibility to pay more for higher
performance where needed.<br/>
The disadvantage of this approach is that users need to know the systems they
are working on and the tools they are using. Otherwise, the risk to ruin the
performance along critical paths is high.

So far, this choice has proven to be a good one and I really hope it can be for
many others besides me.

## Vademecum

The `Registry` to store, the `View` to iterate. That's all.

An entity (the _E_ of an _ECS_) is an opaque identifier that users should just
use as-is and store around if needed. Do not try to inspect an entity
identifier, its format can change in future and a registry offers all the
functionalities to query them out-of-the-box. The underlying type of an entity
(either `std::uint16_t`, `std::uint32_t` or `std::uint64_t`) can be specified
when defining a registry (actually the `DefaultRegistry` is nothing more than a
`Registry` where the type of the entities is `std::uint32_t`).<br/>
Components (the _C_ of an _ECS_) should be plain old data structures or more
complex and movable data structures with a proper constructor. Actually, the
sole requirement of a component type is that it must be both move constructible
and move assignable. They are list initialized by using the parameters provided
to construct the component itself. No need to register components or their types
neither with the registry nor with the entity-component system at all.<br/>
Systems (the _S_ of an _ECS_) are just plain functions, functors, lambdas or
whatever the users want. They can accept a `Registry`, a `View` or a
`PersistentView` and use them the way they prefer. No need to register systems
or their types neither with the registry nor with the entity-component system at
all.

The following sections will explain in short how to use the entity-component
system, the core part of the whole framework.<br/>
In fact, the framework is composed of many other classes in addition to those
describe below. For more details, please refer to the
[online documentation](https://skypjack.github.io/entt/).

## The Registry, the Entity and the Component

A registry can store and manage entities, as well as create views to iterate the
underlying data structures.<br/>
`Registry` is a class template that lets the users decide what's the preferred
type to represent an entity. Because `std::uint32_t` is large enough for almost
all the cases, there exists also an alias named `DefaultRegistry` for
`Registry<std::uint32_t>`.

Entities are represented by _entity identifiers_. An entity identifier is an
opaque type that users should not inspect or modify in any way. It carries
information about the entity itself and its version.

A registry can be used both to construct and to destroy entities:

```cpp
// constructs a naked entity with no components and returns its identifier
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
entity the given component. It accepts a variable number of arguments to
construct the component itself if present:

```cpp
registry.assign<Position>(entity, 0., 0.);

// ...

Velocity &velocity = registry.assign<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

If an entity already has the given component, the `replace` member function
template can be used to replace it:

```cpp
registry.replace<Position>(entity, 0., 0.);

// ...

Velocity &velocity = registry.replace<Velocity>(entity);
velocity.dx = 0.;
velocity.dy = 0.;
```

In case users want to assign a component to an entity, but it's unknown whether
the entity already has it or not, `accomodate` does the work in a single call
(there is a performance penalty to pay for this mainly due to the fact that it
has to check if the entity already has the given component or not):

```cpp
registry.accomodate<Position>(entity, 0., 0.);

// ...

Velocity &velocity = registry.accomodate<Velocity>(entity);
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
`remove` but with a strictly defined behavior (and a performance penalty is the
price to pay for this). In particular it removes the component if and only if it
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

* If neither the entity nor the component are specified, all the entities still
in use and their components are destroyed:

  ```cpp
  registry.reset();
  ```

Finally, references to components can be retrieved simply by doing this:

```cpp
const auto &cregistry = registry;

// const and non-const reference
const Position &position = cregistry.get<Position>(entity);
Position &position = registry.get<Position>(entity);

// const and non-const references
std::tuple<const Position &, const Velocity &> tup = cregistry.get<Position, Velocity>(entity);
std::tuple<Position &, Velocity &> tup = registry.get<Position, Velocity>(entity);
```

The `get` member function template gives direct access to the component of an
entity stored in the underlying data structures of the registry.

### Single instance components

In those cases where all what is needed is a single instance component, tags are
the right tool to achieve the purpose.<br/>
Tags undergo the same requirements of components. They can be either plain old
data structures or more complex and movable data structures with a proper
constructor.<br/>
Actually, the same type can be used both as a tag and as a component and the
registry will not complain about it. It is up to the users to properly manage
their own types.

Attaching tags to entities and removing them is trivial:

```cpp
auto player = registry.create();
auto camera = registry.create();

// attaches a default-initialized tag to an entity
registry.attach<PlayingCharacter>(player);

// attaches a tag to an entity and initializes it
registry.attach<Camera>(camera, player);

// removes tags from their owners
registry.remove<PlayingCharacter>();
registry.remove<Camera>();
```

If in doubt about whether or not a tag has already an owner, the `has` member
function template may be useful:

```cpp
bool b = registry.has<PlayingCharacter>();
```

References to tags can be retrieved simply by doing this:

```cpp
const auto &cregistry = registry;

// either a non-const reference ...
PlayingCharacter &player = registry.get<PlayingCharacter>();

// ... or a const one
const Camera &camera = cregistry.get<Camera>();
```

The `get` member function template gives direct access to the tag as stored in
the underlying data structures of the registry.

As shown above, in almost all the cases the entity identifier isn't required.
Since a single instance component can have only one associated entity, it
doesn't make much sense to mention it explicitly.<br/>
To find out who the owner is, just do the following:

```cpp
auto player = registry.attachee<PlayingCharacter>();
```

Note that iterating tags isn't possible for obvious reasons. Tags give direct
access to single entities and nothing more.

### Runtime components

Defining components at runtime is useful to support plugins and mods in general.
However, it seems impossible with a tool designed around a bunch of templates.
Indeed it's not that difficult.<br/>
Of course, some features cannot be easily exported into a runtime
environment. As an example, sorting a group of components defined at runtime
isn't for free if compared to most of the other operations. However, the basic
functionalities of an entity-component system such as `EnTT` fit the problem
perfectly and can also be used to manage runtime components if required.<br/>
All that is necessary to do it is to know the identifiers of the components. An
identifier is nothing more than a number or similar that can be used at runtime
to work with the type system.

In `EnTT`, identifiers are easily accessible:

```cpp
entt::DefaultRegistry registry;

// standard component identifier
auto ctype = registry.component<Position>();

// single instance component identifier
auto ttype = registry.tag<PlayingCharacter>();
```

Once the identifiers are made available, almost everything becomes pretty
simple.

#### A journey through a plugin

`EnTT` comes with an example (actually a test) that shows how to integrate
compile-time and runtime components in a stack based JavaScript environment. It
uses [`Duktape`](https://github.com/svaarala/duktape) under the hood, mainly
because I wanted to learn how it works at the time I was writing the code.

The code is not production-ready and overall performance can be highly improved.
However, I sacrificed optimizations in favor of a more readable piece of code. I
hope I succeeded.<br/>
Note also that this isn't neither the only nor (probably) the best way to do it.
In fact, the right way depends on the scripting language and the problem one is
facing in general.<br/>
That being said, feel free to use it at your own risk.

The basic idea is that of creating a compile-time component aimed to map all the
runtime components assigned to an entity.<br/>
Identifiers come in use to address the right function from a map when invoked
from the runtime environment and to filter entities when iterating.<br/>
With a bit of gymnastic, one can narrow views and improve the performance to
some extent but it was not the goal of the example.

### Sorting: is it possible?

It goes without saying that sorting entities and components is possible with
`EnTT`.<br/>
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

First of all, it is worth answering an obvious question: why views?<br/>
Roughly speaking, they are a good tool to enforce single responsibility. A
system that has access to a registry can create and destroy entities, as well as
assign and remove components. On the other side, a system that has access to a
view can only iterate entities and their components as well as modify their data
members.<br/>
It is a subtle difference that can help designing a better software sometimes.

There are mainly two kinds of views: standard (also known as `View`) and
persistent (also known as `PersistentView`).<br/>
Both of them have pros and cons to take in consideration. In particular:

* Standard views:

  Pros:
  * They work out-of-the-box and don't require any dedicated data
    structure.
  * Creating and destroying them isn't expensive at all because they don't
    have any type of initialization.
  * They are the best tool to iterate single components.
  * They are the best tool to iterate multiple components at once when one of
    the components is assigned to a significantly low number of entities.
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
    * They have dedicated data structures and thus affect the memory usage to a
      minimal extent.
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

To easily iterate entities, all the views offer the common `begin` and `end`
member functions that allow users to use a view in a typical range-for
loop.<br/>
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
underlying data structures directly and avoid superfluous checks.<br/>
They offer a bunch of functionalities to get the number of entities they are
going to return and a raw access to the entity list as well as to the component
list. It's also possible to ask a view if it contains a given entity.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused
freely. In fact, they return newly created and correctly initialized iterators
whenever `begin` or `end` are invoked.<br/>
To iterate a single component standard view, either use it in range-for loop:

```cpp
auto view = registry.view<Renderable>();

for(auto entity: view) {
    Renderable &renderable = view.get(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities and get all their
components at once:

```cpp
registry.view<Renderable>().each([](auto entity, auto &renderable) {
    // ...
});
```

Performance are more or less the same. The best approach depends mainly on
whether all the components have to be accessed or not.

**Note**: prefer the `get` member function of a view instead of the `get` member
function template of a registry during iterations, if possible. However, keep in
mind that it works only with the components of the view itself.

#### Multi component standard view

Multi component standard views iterate entities that have at least all the given
components in their bags. During construction, these views look at the number
of entities available for each component and pick up a reference to the smallest
set of candidates in order to speed up iterations.<br/>
They offer fewer functionalities than their companion views for single
component. In particular, a multi component standard view exposes utility
functions to reset its internal state (optimization purposes) and to get the
estimated number of entities it is going to return. It's also possible to ask a
view if it contains a given entity.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused
freely. In fact, they return newly created and correctly initialized iterators
whenever `begin` or `end` are invoked.<br/>
To iterate a multi component standard view, either use it in range-for loop:

```cpp
auto view = registry.view<Position, Velocity>();

for(auto entity: view) {
    // a component at a time ...
    Position &position = view.get<Position>(entity);
    Velocity &velocity = view.get<Velocity>(entity);

    // ... or multiple components at once
    std::tuple<Position &, Velocity &> tup = view.get<Position, Velocity>(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities and get all their
components at once:

```cpp
registry.view<Position, Velocity>().each([](auto entity, auto &position, auto &velocity) {
    // ...
});
```

Performance are more or less the same. The best approach depends mainly on
whether all the components have to be accessed or not.

**Note**: prefer the `get` member function of a view instead of the `get` member
function template of a registry during iterations, if possible. However, keep in
mind that it works only with the components of the view itself.

### Persistent View

A persistent view returns all the entities and only the entities that have at
least the given components. Moreover, it's guaranteed that the entity list is
tightly packed in memory for fast iterations.<br/>
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
once: if the view has been already prepared before, the function returns
immediately and does nothing.

A persistent view offers a bunch of functionalities to get the number of
entities it's going to return, a raw access to the entity list and the
possibility to sort the underlying data structures according to the order of one
of the components for which it has been constructed. It's also possible to ask a
view if it contains a given entity.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

To iterate a persistent view, either use it in range-for loop:

```cpp
auto view = registry.persistent<Position, Velocity>();

for(auto entity: view) {
    // a component at a time ...
    Position &position = view.get<Position>(entity);
    Velocity &velocity = view.get<Velocity>(entity);

    // ... or multiple components at once
    std::tuple<Position &, Velocity &> tup = view.get<Position, Velocity>(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities and get all their
components at once:

```cpp
registry.persistent<Position, Velocity>().each([](auto entity, auto &position, auto &velocity) {
    // ...
});
```

Performance are more or less the same. The best approach depends mainly on
whether all the components have to be accessed or not.

**Note**: prefer the `get` member function of a view instead of the `get` member
function template of a registry during iterations, if possible. However, keep in
mind that it works only with the components of the view itself.

### Give me everything

Views are narrow windows on the entire list of entities. They work by filtering
entities according to their components.<br/>
In some cases there may be the need to iterate all the entities still in use
regardless of their components. The registry offers a specific member function
to do that:

```cpp
registry.each([](auto entity) {
    // ...
});
```

It returns to the caller all the entities that are still in use by means of the
given function.<br/>
As a rule of thumb, consider using a view if the goal is to iterate entities
that have a determinate set of components. A view is usually much faster than
combining this function with a bunch of custom tests.<br/>
In all the other cases, this is the way to go.

There exists also another member function to use to retrieve orphans. An orphan
is an entity that is still in use and has neither assigned components nor
tags.<br/>
The signature of the function is the same of `each`:

```cpp
registry.orphans([](auto entity) {
    // ...
});
```

To test the _orphanity_ of a single entity, use the member function `orphan`
instead. It accepts a valid entity identifer as an argument and returns true in
case the entity is an orphan, false otherwise.

In general, all these functions can result in poor performance.<br/>
`each` is fairly slow because of some checks it performs on each and every
entity. For similar reasons, `orphans` can be even slower. Both functions should
not be used frequently to avoid the risk of a performance hit.

## Spaces

Spaces are sort of partitions of a registry. They can be used to easily get a
subset of the entities of a view or a registry without recurring to multiple
registries to separate them explicitly.<br/>
To learn more about their intended use,
[here](https://gamedevelopment.tutsplus.com/tutorials/spaces-useful-game-object-containers--gamedev-14091)
is an interesting article that goes deep into the topic.

Spaces aren't for free. In most of the cases, the cost isn't relevant. However,
keep in mind that they add an extra check during iterations and it could slow
down a bit the whole thing.<br/>
Alternatives to spaces exist, but they have their own problems:

* Multiple registries: memory usage tends to grow up and some tasks are just
  more difficult to accomplish (as an example, putting an entity logically in
  more than one registry requires syncing them and it can quickly become a
  problem).

* Dedicated components: memory usage tends to grow up and the number of spaces
  is fixed and defined at compile-time (at least, it ought to be for performance
  reasons), moreover the solution is much more error-prone.

Another benefit of spaces defined as an external class is that users of a space
do not have access to the whole registry, thus separation of responsibility is
automatically enforced. In both the alternatives described above, systems have
access to the whole set of entities instead and can easily break the contract
with the callers.

The `EnTT` framework offers support to spaces out of the box. Spaces are
constructed using a registry to which they refer:

```cpp
entt::DefaultRegistry registry;
entt::Space<typename entt::DefaultRegistry::entity_type> space{registry};
```

They offer the classical set of member functions to know the estimated number of
entities and to check if a space has a given entity.<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

In addition, they expose two member functions to create an entity through a
space or to assign to a space an already existent entity, other than member
functions to remove entities from a space:

```cpp
// creates an entity using a space
auto entity = space.create();

// assigns an already existent entity to a space
space.assign(registry.create());

// removes an entity from the given space
space.remove(entity);

// removes all the entities from a space
space.reset();
```

Entities returned through the `create` member function are created directly into
the underlying registry and assigned immediately to the space.<br/>
Removing an entity from a space doesn't mean that it's destroyed within the
underlying registry in any case.

Spaces and thus the entities they contain can be easily iterated in a range-for
loop:

```cpp
for(auto entity: space) {
    // ...
}
```

However, this isn't the best way to iterate entities in a space, mainly because
this member function returns all the entities it contains, no matter what are
their components. To iterate entities that have specific components, spaces
expose two dedicated member functions that differ for the view they use under
the hood:

```cpp
// uses a standard view internally
space.view<AComponent, AnotherComponent>([](auto entity, auto &aComponent, auto &anotherComponent) {
    // ...
});

// uses a persistent view internally
space.persisten<AComponent, AnotherComponent>([](auto entity, auto &aComponent, auto &anotherComponent) {
    // ...
});
```

Spaces get rid of entities that are no longer in use during iterations. They
aren't kept in sync with a registry each and every time an entity is destroyed
so as to avoid penalties in terms of performance. Instead, spaces remove invalid
entities as soon as they are detected during iterations.

Because of the _lazy clean_ policy, the size of a space could grow up if
destroyed entities are never detected for some reasons. To avoid it, spaces has
a member function named `shrink` that forces a clean up and reduce the size to a
minimum:

```cpp
// gets rid of all the invalid entities still tracked by a space
space.shrink();
```

Note that the size of a space isn't a problem in terms of performance. Views
rule during iterations, mainly because of the order which may have been imposed
by the user for some reasons and must be respected. Therefore unused entities
are never visited and thus they don't affect iterations. However, memory usage
can be reduced by shrinking spaces every so often.

## Side notes

* Entity identifiers are numbers and nothing more. They are not classes and they
  have no member functions at all. As already mentioned, do no try to inspect or
  modify an entity descriptor in any way.

* As shown in the examples above, the preferred way to get references to the
  components while iterating a view is by using the view itself. It's a faster
  alternative to the `get` member function template that is part of the API of
  the `Registry`. This is because the registry must ensure that a pool for the
  given component exists before to use it; on the other side, views force the
  construction of the pools for all their components and access them directly,
  thus avoiding all the checks.

* Most of the _ECS_ available out there have an annoying limitation (at least
  from my point of view): entities and components cannot be created and/or
  destroyed during iterations.<br/>
  `EnTT` partially solves the problem with a few limitations:

  * Creating entities and components is allowed during iterations.
  * Deleting an entity or removing its components is allowed during
    iterations if it's the one currently returned by a view. For all the
    other entities, destroying them or removing their components isn't
    allowed and it can result in undefined behavior.

  Iterators are invalidated and the behavior is undefined if an entity is
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
  As a trivial example, users can freely execute the rendering system and
  iterate the renderable entities while updating a physic component concurrently
  on a separate thread.

# Crash Course: core functionalities

The `EnTT` framework comes with a bunch of core functionalities mostly used by
the other parts of the library itself.<br/>
Hardly users of the framework will include these features in their code, but
it's worth describing what `EnTT` offers so as not to reinvent the wheel in case
of need.

## Compile-time identifiers

Sometimes it's useful to be able to give unique identifiers to types at
compile-time.<br/>
There are plenty of different solutions out there and I could have used one of
them. However, I decided to spend my time to define a compact and versatile tool
that fully embraces what the modern C++ has to offer.

The _result of my efforts_ is the `ident` `constexpr` variable:

```cpp
#include <ident.hpp>

// defines the identifiers for the given types
constexpr auto identifiers = entt::ident<AType, AnotherType>;

// ...

switch(aTypeIdentifier) {
case identifers.get<AType>():
    // ...
    break;
case identifers.get<AnotherType>():
    // ...
    break;
default:
    // ...
}
```

This is all what the variable has to offer: a `get` member function that returns
a numerical identifier for the given type. It can be used in any context where
constant expressions are required.

As long as the list remains unchanged, identifiers are also guaranteed to be the
same for every run. In case they have been used in a production environment and
a type has to be removed, one can just use a placeholder to left the other
identifiers unchanged:

```cpp
template<typename> struct IgnoreType {};

constexpr auto identifiers = entt::ident<
    ATypeStillValid,
    IgnoreType<ATypeNoLongerValid>,
    AnotherTypeStillValid
>;
```

A bit ugly to see, but it works at least.

## Runtime identifiers

Sometimes it's useful to be able to give unique identifiers to types at
runtime.<br/>
There are plenty of different solutions out there and I could have used one of
them. In fact, I adapted the most common one to my requirements and used it
extensively within the entire framework.

It's the `Family` class. Here is an example of use directly from the
entity-component system:

```cpp
using component_family = entt::Family<struct InternalRegistryComponentFamily>;

// ...

template<typename Component>
component_type component() const noexcept {
    return component_family::type<Component>();
}
```

This is all what a _family_ has to offer: a `type` member function that returns
a numerical identifier for the given type.

Please, note that identifiers aren't guaranteed to be the same for every run.
Indeed it mostly depends on the flow of execution.

## Hashed strings

A hashed string is a zero overhead resource identifier. Users can use
human-readable identifiers in the codebase while using their numeric
counterparts at runtime, thus without affecting performance.<br/>
The class has an implicit `constexpr` constructor that chews a bunch of
characters. Once created, all what one can do with it is getting back the
original string or converting it into a number.<br/>
The good part is that a hashed string can be used wherever a constant expression
is required and no _string-to-number_ conversion will take place at runtime if
used carefully.

Example of use:

```cpp
auto load(entt::HashedString::hash_type resource) {
    // uses the numeric representation of the resource to load and return it
}

auto resource = load(entt::HashedString{"gui/background"});
```

### Conflicts

The hashed string class uses internally FNV-1a to compute the numeric
counterpart of a string. Because of the _pigeonhole principle_, conflicts are
possible. This is a fact.<br/>
There is no silver bullet to solve the problem of conflicts when dealing with
hashing functions. In this case, the best solution seemed to be to give up.
That's all.<br/>
After all, human-readable resource identifiers aren't something strictly defined
and over which users have not the control. Choosing a slightly different
identifier is probably the best solution to make the conflict disappear in this
case.

# Crash Course: service locator

Usually service locators are tightly bound to the services they expose and it's
hard to define a general purpose solution. This template based implementation
tries to fill the gap and to get rid of the burden of defining a different
specific locator for each application.<br/>
This class is tiny, partially unsafe and thus risky to use. Moreover it doesn't
fit probably most of the scenarios in which a service locator is required. Look
at it as a small tool that can sometimes be useful if the user knows how to
handle it.

The API is straightforward. The basic idea is that services are implemented by
means of interfaces and rely on polymorphism.<br/>
The locator is instantiated with the base type of the service if any and a
concrete implementation is provided along with all the parameters required to
initialize it. As an example:

```cpp
// the service has no base type, a locator is used to treat it as a kind of singleton
entt::ServiceLocator<MyService>::set(params...);

// sets up an opaque service
entt::ServiceLocator<AudioInterface>::set<AudioImplementation>(params...);

// resets (destroys) the service
entt::ServiceLocator<AudioInterface>::reset();
```

The locator can also be queried to know if an active service is currently set
and to retrieve it if necessary (either as a pointer or as a reference):

```cpp
// no service currently set
auto empty = entt::ServiceLocator<AudioInterface>::empty();

// gets a (possibly empty) shared pointer to the service ...
std::shared_ptr<AudioInterface> ptr = entt::ServiceLocator<AudioInterface>::get();

// ... or a reference, but it's undefined behaviour if the service isn't set yet
AudioInterface &ref = entt::ServiceLocator<AudioInterface>::ref();
```

A common use is to wrap the different locators in a container class, creating
aliases for the various services:

```cpp
struct Locator {
    using Camera = entt::ServiceLocator<CameraInterface>;
    using Audio = entt::ServiceLocator<AudioInterface>;
    // ...
};

// ...

void init() {
    Locator::Camera::set<CameraNull>();
    Locator::Audio::set<AudioImplementation>(params...);
    // ...
}
```

# Crash Course: cooperative scheduler

Sometimes processes are a useful tool to work around the strict definition of a
system and introduce logic in a different way, usually without resorting to the
introduction of other components.

The `EnTT` framework offers a minimal support to this paradigm by introducing a
few classes that users can use to define and execute cooperative processes.

## The process

A typical process must inherit from the `Process` class template that stays true
to the CRTP idiom. Moreover, derived classes must specify what's the intended
type for elapsed times.

A process should expose publicly the following member functions whether
required (note that it isn't required to define a function unless the derived
class wants to _override_ the default behavior):

* `void update(Delta, void *);`

  It's invoked once per tick until a process is explicitly aborted or it
  terminates either with or without errors. Even though it's not mandatory to
  declare this member function, as a rule of thumb each process should at
  least define it to work properly. The `void *` parameter is an opaque pointer
  to user data (if any) forwarded directly to the process during an update.

* `void init(void *);`

  It's invoked at the first tick, immediately before an update. The `void *`
  parameter is an opaque pointer to user data (if any) forwarded directly to the
  process during an update.

* `void succeeded();`

  It's invoked in case of success, immediately after an update and during the
  same tick.

* `void failed();`

  It's invoked in case of errors, immediately after an update and during the
  same tick.

* `void aborted();`

  It's invoked only if a process is explicitly aborted. There is no guarantee
  that it executes in the same tick, this depends solely on whether the
  process is aborted immediately or not.

Derived classes can also change the internal state of a process by invoking
`succeed` and `fail`, as well as `pause` and `unpause` the process itself. All
these are protected member functions made available to be able to manage the
life cycle of a process from a derived class.

Here is a minimal example for the sake of curiosity:

```cpp
struct MyProcess: entt::Process<MyProcess, std::uint32_t> {
    using delta_type = std::uint32_t;

    void update(delta_type delta, void *) {
        remaining = delta > remaining ? delta_type{] : (remaining - delta);

        // ...

        if(!remaining) {
            succeed();
        }
    }

    void init(void *data) {
        remaining = *static_cast<delta_type *>(data);
    }

private:
    delta_type remaining;
};
```

### Adaptor

Lambdas and functors can't be used directly with a scheduler for they are not
properly defined processes with managed life cycles.<br/>
This class helps in filling the gap and turning lambdas and functors into
full featured processes usable by a scheduler.

The function call operator has a signature similar to the one of the `update`
function of a process but for the fact that it receives two extra arguments to
call whenever a process is terminated with success or with an error:

```cpp
void(Delta delta, void *data, auto succeed, auto fail);
```

Parameters have the following meaning:

* `delta` is the elapsed time.
* `data` is an opaque pointer to user data if any, `nullptr` otherwise.
* `succeed` is a function to call when a process terminates with success.
* `fail` is a function to call when a process terminates with errors.

Both `succeed` and `fail` accept no parameters at all.

Note that usually users shouldn't worry about creating adaptors at all. A
scheduler creates them internally each and every time a lambda or a functor is
used as a process.

## The scheduler

A cooperative scheduler runs different processes and helps managing their life
cycles.

Each process is invoked once per tick. If it terminates, it's removed
automatically from the scheduler and it's never invoked again. Otherwise it's
a good candidate to run once more the next tick.<br/>
A process can also have a child. In this case, the process is replaced with
its child when it terminates if it returns with success. In case of errors,
both the process and its child are discarded. This way, it's easy to create
chain of processes to run sequentially.

Using a scheduler is straightforward. To create it, users must provide only the
type for the elapsed times and no arguments at all:

```cpp
Scheduler<std::uint32_t> scheduler;
```

It has member functions to query its internal data structures, like `empty` or
`size`, as well as a `clear` utility to reset it to a clean state:

```cpp
// checks if there are processes still running
bool empty = scheduler.empty();

// gets the number of processes still running
Scheduler<std::uint32_t>::size_type size = scheduler.size();

// resets the scheduler to its initial state and discards all the processes
scheduler.clear();
```

To attach a process to a scheduler there are mainly two ways:

* If the process inherits from the `Process` class template, it's enough to
  indicate its type and submit all the parameters required to construct it to
  the `attach` member function:

  ```cpp
  scheduler.attach<MyProcess>("foobar");
  ```

* Otherwise, in case of a lambda or a functor, it's enough to provide an
  instance of the class to the `attach` member function:

  ```cpp
  scheduler.attach([](auto...){ /* ... */ });
  ```

In both cases, the return value is an opaque object that offers a `then` member
function to use to create chains of processes to run sequentially.<br/>
As a minimal example of use:

```cpp
// schedules a task in the form of a lambda function
scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
    // ...
})
// appends a child in the form of another lambda function
.then([](auto delta, void *, auto succeed, auto fail) {
    // ...
})
// appends a child in the form of a process class
.then<MyProcess>();
```

To update a scheduler and thus all its processes, the `update` member function
is the way to go:

```cpp
// updates all the processes, no user data are provided
scheduler.update(delta);

// updates all the processes and provides them with custom data
scheduler.update(delta, &data);
```

In addition to these functions, the scheduler offers an `abort` member function
that can be used to discard all the running processes at once:

```cpp
// aborts all the processes abruptly ...
scheduler.abort(true);

// ... or gracefully during the next tick
scheduler.abort();
```

# Crash Course: resource management

Resource management is usually one of the most critical part of a software like
a game. Solutions are often tuned to the particular application. There exist
several approaches and all of them are perfectly fine as long as they fit the
requirements of the piece of software in which they are used.<br/>
Examples are loading everything on start, loading on request, predictive
loading, and so on.

The `EnTT` framework doesn't pretend to offer a _one-fits-all_ solution for the
different cases. Instead, it offers a minimal and perhaps trivial cache that can
be useful most of the time during prototyping and sometimes even in a production
environment.<br/>
For those interested in the subject, the plan is to improve it considerably over
time in terms of performance, memory usage and functionalities. Hoping to make
it, of course, one step at a time.

## The resource, the loader and the cache

There are three main actors in the model: the resource, the loader and the
cache.

The _resource_ is whatever the user wants it to be. An image, a video, an audio,
whatever. There are no limits.<br/>
As a minimal example:

```cpp
struct MyResource { const int value; };
```

A _loader_ is a class the aim of which is to load a specific resource. It has to
inherit directly from the dedicated base class as in the following example:

```cpp
struct MyLoader final: entt::ResourceLoader<MyLoader, MyResource> {
    // ...
};
```

Where `MyResource` is the type of resources it creates.<br/>
A resource loader must also expose a public const member function named `load`
that accepts a variable number of arguments and returns a shared pointer to a
resource.<br/>
As an example:

```cpp
struct MyLoader: entt::ResourceLoader<MyLoader, MyResource> {
    std::shared_ptr<MyResource> load(int value) const {
        // ...
        return std::shared_ptr<MyResource>(new MyResource{ value });
    }
};
```

In general, resource loaders should not have a state or retain data of any type.
They should let the cache manage their resources instead.<br/>
As a side note, base class and CRTP idiom aren't strictly required with the
current implementation. One could argue that a cache can easily work with
loaders of any type. However, future changes won't be breaking ones by forcing
the use of a base class today and that's why the model is already in its place.

Finally, a cache is a specialization of a class template tailored to a specific
resource:

```cpp
using MyResourceCache = entt::ResourceCache<MyResource>;

// ...

MyResourceCache cache{};
```

The idea is to create different caches for different types of resources and to
manage each one independently and in the most appropriate way.<br/>
As a (very) trivial example, audio tracks can survive in most of the scenes of
an application while meshes can be associated with a single scene and then
discarded when the user leaves it.

A cache offers a set of basic functionalities to query its internal state and to
_organize_ it:

```cpp
// gets the number of resources managed by a cache
auto size = cache.size();

// checks if a cache contains at least a valid resource
auto empty = cache.empty();

// clears a cache and discards its content
cache.clear();
```

Besides these member functions, it contains what is needed to load, use and
discard resources of the given type.<br/>
Before to explore this part of the interface, it makes sense to mention how
resources are identified. The type of the identifiers to use is defined as:

```cpp
entt::ResourceCache<Resource>::resource_type
```

Where `resource_type` is an alias for `entt::HashedString`. Therefore, resource
identifiers are created explicitly as in the following example:

```cpp
constexpr auto identifier = entt::ResourceCache<Resource>::resource_type{"my/resource/identifier"};
// this is equivalent to the following
constexpr auto hs = entt::HashedString{"my/resource/identifier"};
```

The class `HashedString` is described in a dedicated section, so I won't do in
details here.

Resources are loaded and thus stored in a cache through the `load` member
function. It accepts the loader to use as a template parameter, the resource
identifier and the parameters used to construct the resource as arguments:

```cpp
// uses the identifier declared above
cache.load<MyLoader>(identifier, 0);

// uses a const char * directly as an identifier
cache.load<MyLoader>("another/identifier", 42);
```

The return value can be used to know if the resource has been loaded correctly.
In case the loader returns an invalid pointer or the resource already exists in
the cache, a false value is returned:

```cpp
if(!cache.load<MyLoader>("another/identifier", 42)) {
    // ...
}
```

Unfortunately, in this case there is no way to know what was the problem
exactly. However, before trying to load a resource or after an error, one can
use the `contains` member function to know if a cache already contains a
specific resource:

```cpp
auto exists = cache.contains("my/identifier");
```

There exists also a member function to use to force a reload of an already
existing resource if needed:

```cpp
auto result = cache.reload<MyLoader>("another/identifier", 42);
```

As above, the function returns true in case of success, false otherwise. The
sole difference in this case is that an error necessarily means that the loader
has failed for some reasons to load the resource.<br/>
Note that the `reload` member function is a kind of alias of the following
snippet:

```cpp
cache.discard(identifier);
cache.load<MyLoader>(identifier, 42);
```

Where the `discard` member function is used to get rid of a resource if loaded.
In case the cache doesn't contain a resource for the given identifier, the
function does nothing and returns immediately.

So far, so good. Resources are finally loaded and stored within the cache.<br/>
They are returned to the users in the form of handles. To get one of them:

```cpp
auto handle = cache.handle("my/identifier");
```

The idea behind a handle is the same of the flyweight pattern. In other terms,
resources aren't copied around. Instead, instances are shared between handles.
Users of a resource owns a handle and it guarantees that a resource isn't
destroyed until all the handles are destroyed, even if the resource itself is
removed from the cache.<br/>
Handles are tiny objects both movable and copyable. They returns the contained
resource as a const reference on request:

* By means of the `get` member function:

  ```cpp
  const auto &resource = handle.get();
  ```

* Using the proper cast operator:

  ```cpp
  const auto &resource = handle;
  ```

* Through the dereference operator:

  ```cpp
  const auto &resource = *handle;
  ```

The resource can also be accessed directly using the arrow operator if required:

```cpp
auto value = handle->value;
```

To test if a handle is still valid, the cast operator to `bool` allows the users
to use it in a guard:

```cpp
if(handle) {
    // ...
}
```

Finally, in case there is the need to load a resource and thus to get a handle
without storing the resource itself in the cache, users can rely on the `temp`
member function template.<br/>
The declaration is similar to the one of `load` but for the fact that it doesn't
return a boolean value. Instead, it returns a (possibly invalid) handle for the
resource:

```cpp
auto handle = cache.temp<MyLoader>("another/identifier", 42);
```

Do not forget to test the handle for validity. Otherwise, getting the reference
to the resource it points may result in undefined behavior.

# Crash Course: events, signals and everything in between

Signals are usually a core part of games and software architectures in
general.<br/>
Roughly speaking, they help to decouple the various parts of a system while
allowing them to communicate with each other somehow.

The so called _modern C++_ comes with a tool that can be useful in these terms,
the `std::function`. As an example, it can be used to create delegates.<br/>
However, there is no guarantee that an `std::function` does not perform
allocations under the hood and this could be problematic sometimes. Furthermore,
it solves a problem but may not adapt well to other requirements that may arise
from time to time.

In case that the flexibility and potential of an `std::function` are not
required or where you are looking for something different, the `EnTT` framework
offers a full set of classes to solve completely different problems.

## Signals

There are two types of signal handlers in `EnTT`, internally called _managed_
and _unmanaged_.<br/>
They differ in the way they work around the tradeoff between performance, memory
usage and safety. Managed listeners must be wrapped in an `std::shared_ptr` and
the sink will take care of disconneting them whenever they die. Unmanaged
listeners can be any kind of objects and the client is in charge of connecting
and disconnecting them from a sink to avoid crashes due to different lifetimes.

### Managed signal handler

A managed signal handler works with weak pointers to classes and pointers to
member functions as well as pointers to free functions. References are
automatically removed when the instances to which they point are freed.<br/>
In other terms, users can simply connect a listener and forget about it, thus
getting rid of the burden of controlling its lifetime. The drawback is that
listeners must be allocated on the dynamic storage and wrapped into an
`std::shared_ptr`. Performance and memory management can suffer from this in
real world softwares.

To create an instance of this type of handler, the function type is all what is
needed:

```cpp
entt::Signal<void(int, char)> signal;
```

From now on, free functions and member functions that respect the given
signature can be easily connected to and disconnected from the signal:

```cpp
void foo(int, char) { /* ... */ }

struct S {
    void bar(int, char) { /* ... */ }
};

// ...

auto instance = std::make_shared<S>();

signal.connect<&foo>();
signal.connect<S, &S::bar>(instance);

// ...

signal.disconnect<&foo>();

// disconnect a specific member function of an instance ...
signal.disconnect<S, &S::bar>(instance);

// ... or an instance as a whole
signal.disconnect(instance);
```

Once listeners are attached (or even if there are no listeners at all), events
and data in general can be published through a signal by means of the `publish`
member function:

```cpp
signal.publish(42, 'c');
```

This is more or less all what a managed signal handler has to offer.<br/>
A bunch of other member functions are exposed actually. As an example, there is
a method to use to know how many listeners a managed signal handler contains
(`size`) or if it contains at least a listener (`empty`), to reset it to its
initial state (`clear`) and even to swap two handlers (`swap`).<br/>
Refer to the [official documentation](https://skypjack.github.io/entt/) for all
the details.

### Unmanaged signal handler

An unmanaged signal handler works with naked pointers to classes and pointers to
member functions as well as pointers to free functions. Removing references when
the instances to which they point are freed is in charge to the users.<br/>
In other terms, users must explicitly disconnect a listener before to delete the
class to which it belongs, thus taking care of the lifetime of each instance. On
the other side, performance shouldn't be affected that much by the presence of
such a signal handler.

The API of an unmanaged signal handler is similar to the one of a managed signal
handler.<br/>
The most important difference is that it comes in two forms: with and without a
collector. In case it is associated with a collector, all the values returned by
the listeners can be literally _collected_ and used later by the caller.<br/>

**Note**: collectors are allowed only in case of function types whose the return
type isn't `void` for obvious reasons.

To create instances of this type of handler there exist mainly two ways:

```cpp
// no collector type
entt::SigH<void(int, char)> signal;

// explicit collector type
entt::SigH<void(int, char), MyCollector<bool>> collector;
```

As expected, an unmanaged signal handler offers all the basic functionalities
required to know how many listeners it contains (`size`) or if it contains at
least a listener (`empty`), to reset it to its initial state (`clear`) and even
to swap two handlers (`swap`).

Besides them, there are member functions to use both to connect and disconnect
listeners in all their forms:

```cpp
void foo(int, char) { /* ... */ }

struct S {
    void bar(int, char) { /* ... */ }
};

// ...

S instance;

signal.connect<&foo>();
signal.connect<S, &S::bar>(&instance);

// ...

signal.disconnect<&foo>();

// disconnect a specific member function of an instance ...
signal.disconnect<S, &S::bar>(&instance);

// ... or an instance as a whole
signal.disconnect(&instance);
```

Once listeners are attached (or even if there are no listeners at all), events
and data in general can be published through a signal by means of the `publish`
member function:

```cpp
signal.publish(42, 'c');
```

To collect data, the `collect` member function should be used instead. Below is
a minimal example to show how to use it:

```cpp
struct MyCollector {
    std::vector<int> vec{};

    bool operator()(int v) noexcept {
        vec.push_back(v);
        return true;
    }
};

int f() { return 0; }
int g() { return 1; }

// ...

entt::SigH<int(), MyCollector<int>> signal;

signal.connect<&f>();
signal.connect<&g>();

MyCollector collector = signal.collect();

assert(collector.vec[0] == 0);
assert(collector.vec[1] == 1);
```

As shown above, a collector must expose a function operator that accepts as an
argument a type to which the return type of the listeners can be converted.
Moreover, it has to return a boolean value that is false to stop collecting
data, true otherwise. This way one can avoid calling all the listeners in case
it isn't necessary.

## Compile-time event bus

A bus can be used to create a compile-time backbone for event management.<br/>
The intended use is as a base class, which is the opposite of what the signals
are meant for. Internally it uses either managed or unmanaged signal handlers,
that is why there exist both a managed and an unmanaged event bus.

The API of a bus is a kind of subset of the one of a signal. First of all, it
requires that all the types of events are specified when the bus is declared:

```cpp
struct AnEvent { int value; };
struct AnotherEvent {};

// define a managed bus that works with std::shared_ptr/std::weak_ptr
entt::ManagedBus<AnEvent, AnotherEvent> managed;

// define an unmanaged bus that works with naked pointers
entt::UnmanagedBus<AnEvent, AnotherEvent> unmanaged;
```

For the sake of brevity, below is described the interface of the sole unmanaged
bus. The interface of the managed bus is almost the same but for the fact that
it accepts smart pointers instead of naked pointers.

In order to register an instance of a class to a bus, its type must expose one
or more member functions named `receive` of which the return types are `void`
and the argument lists are `const E &`, for each type of event `E`.<br/>
The `reg` member function is the way to go to register such an instance:

```cpp
struct Listener
{
    void receive(const AnEvent &) { /* ... */ }
    void receive(const AnotherEvent &) { /* ... */ }
};

// ...

Listener listener;
bus.reg(&listener);
```

To disconnect an instance of a class from a bus, use the `unreg` member
function instead:

```cpp
bus.unreg(&listener);
```

Each function that respects the accepted signature is automatically registered
and/or unregistered. Note that invoking `unreg` with an instance of a class that
hasn't been previously registered is a perfectly valid operation.

Free functions can be registered and unregistered as well by means of the
dedicated member functions, namely `connect` and `disconnect`:

```cpp
void foo(const AnEvent &) { /* ... */ }
void bar(const AnotherEvent &) { /* ... */ }

// ...

bus.connect<AnEvent, &foo>();
bus.connect<AnotherEvent, &bar>();

// ...

bus.disconnect<AnEvent, &foo>();
bus.disconnect<AnotherEvent, &bar>();
```

Whenever the need to send an event arises, it can be done through the `publish`
member function:

```cpp
bus.publish<AnEvent>(42);
bus.publish<AnotherEvent>();
```

Finally, there are another few functions to use to query the internal state of a
bus like `empty` and `size` whose meaning is quite intuitive.

## Delegate

A delegate can be used as general purpose invoker with no memory overhead for
free functions and member functions provided along with an instance on which
to invoke them.<br/>
It does not claim to be a drop-in replacement for an `std::function`, so do not
expect to use it whenever an `std::function` fits well. However, it can be used
to send opaque delegates around to be used to invoke functions as needed.

The interface is trivial. It offers a default constructor to create empty
delegates:

```cpp
entt::Delegate<int(int)> delegate{};
```

All what is needed to create an instance is to specify the type of the function
the delegate will _contain_, that is the signature of the free function or the
member function one wants to assign to it.

Attempting to use an empty delegate by invoking its function call operator
results in undefined behavior, most likely a crash actually. Before to use a
delegate, it must be initialized.<br/>
There exist two functions to do that, both named `connect`:

```cpp
int f(int i) { return i; }

struct MyStruct {
    int f(int i) { return i }
};

// bind a free function to the delegate
delegate.connect<&f>();

// bind a member function to the delegate
MyStruct instance;
delegate.connect<MyStruct, &MyStruct::f>(&instance);
```

It hasn't a `disconnect` counterpart. Instead, there exists a `reset` member
function to clear it.<br/>
Finally, to invoke a delegate, the function call operator is the way to go as
usual:

```cpp
auto ret = delegate(42);
```

Probably too much small and pretty poor of functionalities, but the delegate
class can help in a lot of cases and it has shown that it is worth keeping it
within the framework.

## Event dispatcher

The event dispatcher class is designed so as to be used in a loop. It allows
users both to trigger immediate events or to queue events to be published all
together once per tick.<br/>
Internally it uses either managed or unmanaged signal handlers, that is why
there exist both a managed and an unmanaged event dispatcher.

This class shares part of its API with the one of the signals, but it doesn't
require that all the types of events are specified when declared:

```cpp
// define a managed dispatcher that works with std::shared_ptr/std::weak_ptr
entt::Dispatcher<entt::Signal> managed{};

// define an unmanaged dispatcher that works with naked pointers
entt::Dispatcher<entt::SigH> unmanaged{};
```

Actually there exist two aliases for the classes shown in the previous example:
`entt::ManagedDispatcher` and `entt::UnmanagedDispatcher`.

For the sake of brevity, below is described the interface of the sole unmanaged
dispatcher. The interface of the managed dispatcher is almost the same but for
the fact that it accepts smart pointers instead of naked pointers.

In order to register an instance of a class to a dispatcher, its type must
expose one or more member functions of which the return types are `void` and the
argument lists are `const E &`, for each type of event `E`.<br/>
To ease the development, member functions that are named `receive` are
automatically detected and have not to be explicitly specified when registered.
In all the other cases, the name of the member function aimed to receive the
event must be provided to the `connect` member function:

```cpp
struct AnEvent { int value; };
struct AnotherEvent {};

struct Listener
{
    void receive(const AnEvent &) { /* ... */ }
    void method(const AnotherEvent &) { /* ... */ }
};

// ...

Listener listener;
dispatcher.connect<AnEvent>(&listener);
dispatcher.connect<AnotherEvent, Listener, &Listener::method>(&listener);
```

The `disconnect` member function follows the same pattern and can be used to
selectively remove listeners:

```cpp
dispatcher.disconnect<AnEvent>(&listener);
dispatcher.disconnect<AnotherEvent, Listener, &Listener::method>(&listener);
```

The `trigger` member function serves the purpose of sending an immediate event
to all the listeners registered so far. It offers a convenient approach that
relieves the user from having to create the event itself. Instead, it's enough
to specify the type of event and provide all the parameters required to
construct it.<br/>
As an example:

```cpp
dispatcher.trigger<AnEvent>(42);
dispatcher.trigget<AnotherEvent>();
```

Listeners are invoked immediately, order of execution isn't guaranteed. This
method can be used to push around urgent messages like an _is terminating_
notification on a mobile app.

On the other hand, the `enqueue` member function queues messages together and
allows to maintain control over the moment they are sent to listeners. The
signature of this method is more or less the same of `trigger`:

```cpp
dispatcher.enqueue<AnEvent>(42);
dispatcher.enqueue<AnotherEvent>();
```

Events are stored aside until the `update` member function is invoked, then all
the messages that are still pending are sent to the listeners at once:

```cpp
dispatcher.update();
```

This way users can embed the dispatcher in a loop and literally dispatch events
once per tick to their systems.

## Event emitter

A general purpose event emitter thought mainly for those cases where it comes to
working with asynchronous stuff.<br/>
Originally designed to fit the requirements of
[`uvw`](https://github.com/skypjack/uvw) (a wrapper for `libuv` written in
modern C++), it was adapted later to be included in this library.

To create a custom emitter type, derived classes must inherit directly from the
base class as:

```cpp
struct MyEmitter: Emitter<MyEmitter> {
    // ...
}
```

The full list of accepted types of events isn't required. Handlers are created
internally on the fly and thus each type of event is accepted by default.

Whenever an event is published, an emitter provides the listeners with a
reference to itself along with a const reference to the event. Therefore
listeners have an handy way to work with it without incurring in the need of
capturing a reference to the emitter itself.<br/>
In addition, an opaque object is returned each time a connection is established
between an emitter and a listener, allowing the caller to disconnect them at a
later time.<br/>
The opaque object used to handle connections is both movable and copyable. On
the other side, an event emitter is movable but not copyable by default.

To create new instances of an emitter, no arguments are required:

```cpp
MyEmitter emitter{};
```

Listeners must be movable and callable objects (free functions, lambdas,
functors, `std::function`s, whatever) whose function type is:

```cpp
void(const Event &, MyEmitter &)
```

Where `Event` is the type of event they want to listen.<br/>
There are two ways to attach a listener to an event emitter that differ
slightly from each other:

* To register a long-lived listener, use the `on` member function. It is meant
  to register a listener designed to be invoked more than once for the given
  event type.<br/>
  As an example:

  ```cpp
  auto conn = emitter.on<MyEvent>([](const MyEvent &event, MyEmitter &emitter) {
      // ...
  });
  ```

  The connection object can be freely discarded. Otherwise, it can be used later
  to disconnect the listener if required.

* To register a short-lived listener, use the `once` member function. It is
  meant to register a listener designed to be invoked only once for the given
  event type. The listener is automatically disconnected after the first
  invocation.<br/>
  As an example:

  ```cpp
  auto conn = emitter.once<MyEvent>([](const MyEvent &event, MyEmitter &emitter) {
      // ...
  });
  ```

  The connection object can be freely discarded. Otherwise, it can be used later
  to disconnect the listener if required.

In both cases, the connection object can be used with the `erase` member
function:

```cpp
emitter.erase(conn);
```

There are also two member functions to use either to disconnect all the
listeners for a given type of event or to clear the emitter:

```cpp
// removes all the listener for the specific event
emitter.clear<MyEvent>();

// removes all the listeners registered so far
emitter.clear();
```

To send an event to all the listeners that are interested in it, the `publish`
member function offers a convenient approach that relieves the user from having
to create the event:

```cpp
struct MyEvent { int i; };

// ...

emitter.publish<MyEvent>(42);
```

Finally, the `empty` member function tests if there exists at least either a
listener registered with the event emitter or to a given type of event:

```cpp
bool empty;

// checks if there is any listener registered for the specific event
empty = emitter.empty<MyEvent>();

// checks it there are listeners registered with the event emitter
empty = emitter.empty();
```

In general, the event emitter is a handy tool when the derived classes _wrap_
asynchronous operations, because it introduces a _nice-to-have_ model based on
events and listeners that kindly hides the complexity behind the scenes. However
it is not limited to such uses.

# Contributors

If you want to contribute, please send patches as pull requests against the
branch `master`.<br/>
See the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to
know who has participated so far.

# License

Code and documentation Copyright (c) 2018 Michele Caini.<br/>
Code released under
[the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).
Docs released under
[Creative Commons](https://github.com/skypjack/entt/blob/master/docs/LICENSE).

# Support

## Donation

Developing and maintaining `EnTT` takes some time and lots of coffee. I'd like
to add more and more functionalities in future and turn it in a full-featured
framework.<br/>
If you want to support this project, you can offer me an espresso. I'm from
Italy, we're used to turning the best coffee ever in code. If you find that
it's not enough, feel free to support me the way you prefer.<br/>
Take a look at the donation button at the top of the page for more details or
just click [here](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).

## Hire me

If you start using `EnTT` and need help, if you want a new feature and want me
to give it the highest priority, or for any other reason, I'm available for
hiring.<br/>
Feel free to take a look at my [profile](https://github.com/skypjack) and
contact me by mail.
