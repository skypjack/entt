# EnTT Framework

[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/entt)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

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
reason, feel free to contact me directly (you can find the mail in the
[profile](https://github.com/skypjack)).<br/>
I can't promise that each and every contribution will be accepted, but I can
assure that I'll do my best to take them all seriously.

### State of the art

Here is a brief list of what it offers today:

* Statically generated integer identifiers for types (assigned either at
compile-time or at runtime).
* A constexpr utility for human readable resource identifiers.
* An incredibly fast entity-component system based on sparse sets, with its own
views and a _pay for what you use_ policy to adjust performance and memory
pressure according to the users' requirements.
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
refer to the [online documentation](https://skypjack.github.io/entt/).

### A note about the README

The README file stays true to the original project and it describes only the
entity-component system. However, the whole API is fully documented in-code and
the [online documentation](https://skypjack.github.io/entt/) contains much
more.<br/>
Continue reading to know how the core part of the project works or follow the
link above to take a look at the API reference for all other available classes.

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
in terms of performance.<br/>
In the end, I did it, but it wasn't much satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my* entity-component system at the same
time.

Today `EnTT` is finally what I was looking for: still faster than its
_competitors_, a really good API and an amazing set of features. And even more,
of course.

## Performance

As it stands right now, `EnTT` is just fast enough for my requirements if
compared to my first choice (it was already amazingly fast actually).<br/>
Here is a comparision between the two (both of them compiled with GCC 7.2.0 on a
Dell XPS 13 out of the mid 2014):

| Benchmark | EntityX (compile-time) | EnTT |
|-----------|-------------|-------------|
| Create 10M entities | 0.1289s | **0.0388s** |
| Destroy 10M entities | **0.0531s** | 0.0828s |
| Standard view, 10M entities, one component | 0.0107s | **7.8e-08s** |
| Standard view, 10M entities, two components | **0.0113s** | 0.0244s |
| Standard view, 10M entities, two components<br/>Half of the entities have all the components | **0.0078s** | 0.0129s |
| Standard view, 10M entities, two components<br/>One of the entities has all the components | 0.0071s | **5.5e-07s** |
| Persistent view, 10M entities, two components | 0.0113s | **1.1e-07s** |
| Standard view, 10M entities, five components | **0.0091s** | 0.0672s |
| Persistent view, 10M entities, five components | 0.0091s | **2.5e-07s** |
| Standard view, 10M entities, ten components | **0.0105s** | 0.1306s |
| Standard view, 10M entities, ten components<br/>Half of the entities have all the components | **0.0090s** | 0.0629s |
| Standard view, 10M entities, ten components<br/>One of the entities has all the components | 0.0070s | **1.3e-06s** |
| Persistent view, 10M entities, ten components | 0.0105s | **5.0e-07s** |
| Sort 150k entities, one component<br/>Arrays are in reverse order | - | **0.0043s** |
| Sort 150k entities, enforce permutation<br/>Arrays are in reverse order | - | **0.0006s** |

`EnTT` includes its own tests and benchmarks. See
[benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp)
for further details.<br/>
On Github users can find also a
[benchmark suite](https://github.com/abeimler/ecs_benchmark) that compares a
bunch of different projects, one of which is `EnTT`.

Probably I'll try to get out of `EnTT` more features and better performance in
the future, mainly for fun.<br/>
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

When it comes to using an entity-componet system, the tradeoff is usually
between performance and memory usage. The faster it is, the more memory it uses.
However, slightly worse performance along non-critical paths are the right price
to pay to reduce memory usage and I've always wondered why this kind of tools do
not leave me the choice.<br/>
`EnTT` follows a completely different approach. It squezees the best from the
basic data structures and gives users the possibility to pay more for higher
performance where needed.<br/>
The disadvantage of this approach is that users need to know the systems they
are working on and the tools they are using. Otherwise, the risk to ruin the
performance along critical paths is high.

So far, this choice has proven to be a good one and I really hope it can be for
many others besides me.

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
complex and moveable data structures with a proper constructor. Actually, the
sole requirement of a component type is that it must be both move constructible
and move assignable. They are list initialized by using the parameters provided
to construct the component itself. No need to register components or their types
neither with the registry nor with the entity-component system at all.<br/>
Systems (the _S_ of an _ECS_) are just plain functions, functors, lambdas or
whatever the users want. They can accept a Registry, a View or a PersistentView
and use them the way they prefer. No need to register systems or their types
neither with the registry nor with the entity-component system at all.

The following sections will explain in short how to use the entity-component
system, the core part of the whole framework.<br/>
In fact, the framework is composed of many other classes in addition to those
describe below. For more details, please refer to the
[online documentation](https://skypjack.github.io/entt/).

## The Registry, the Entity and the Component

A registry can store and manage entities, as well as create views to iterate the
underlying data structures.<br/>
Registry is a class template that lets the users decide what's the preferred
type to represent an entity. Because `std::uint32_t` is large enough for almost
all the cases, there exists also an alias named DefaultRegistry for
`Registry<std::uint32_t>`.

Entities are represented by _entitiy identifiers_. An entity identifier is an
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

If the entity already has the given component, the `replace` member function
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
has to check if `entity` already has the given component or not):

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

* If neither the entity nor the component are specified, all the entities and
their components are destroyed:

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
data structures or more complex and moveable data structures with a proper
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

As shown above, in almost all the cases the entity identifier isn't required,
since a single instance component can have only one associated entity and
therefore it doesn't make much sense to mention it explicitly.<br/>
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
uses [`duktape`](https://github.com/svaarala/duktape) under the hood, mainly
because I wanted to learn how it works at the time I was writing the code.

It's not production-ready and overall performance can be highly improved.
However, I sacrificed optimizations in favor of a more readable piece of
code. I hope I succeeded.<br/>
Note also that this isn't neither the only nor (probably) the best way to do it.
In fact, the right way depends on the scripting language and the problem one is
facing in general.

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

### Snapshot: complete vs progressive

The `Registry` class offers basic support to serialization.<br/>
It doesn't convert components and tags to bytes directly, there wasn't the need
of another tool for serialization out there. Instead, it accepts an opaque
object with a suitable interface (namely an _archive_) to serialize its internal
data structures and restore them later. The way types and instances are
converted to a bunch of bytes is completely in charge to the archive and thus to
the users.

The goal of the serialization part is to allow users to make both a dump of the
entire registry or a narrower snapshot, that is to select only the components
and the tags in which they are interested.<be/>
Intuitively, the use cases are different. As an example, the first approach is
suitable for local save/restore functionalities while the latter is suitable for
creating client-server applications and for transferring somehow parts of the
representation side to side.

To take a snapshot of the registry, use the `snapshot` member function. It
returns a temporary object properly initialized to _save_ the whole registry or
parts of it.

Example of use:

```cpp
OutputArchive output;

registry.snapshot()
    .entities(output)
    .destroyed(output)
    .component<AComponent, AnotherComponent>(output)
    .tag<MyTag>(output);
```

It isn't necessary to invoke all these functions each and every time. What
functions to use in which case mostly depends on the goal and there is not a
golden rule to do that.

The `entities` member function asks to the registry to serialize all the
entities that are still in use along with their versions. On the other side, the
`destroyed` member function tells to the registry to serialize the entities that
have been destroyed and are no longer in use.<br/>
These two functions can be used to save and restore the whole set of entities
with the versions they had during serialization.

The `component` member function is a function template the aim of which is to
store aside components. The presence of a template parameter list is a
consequence of a couple of design choices from the past and in the present:

* First of all, there is no reason to force an user to serialize all the
  components at once and most of the times it isn't desiderable. As an example,
  in case the stuff for the HUD in a game is put into the registry for some
  reasons, its components can be freely discarded during a serialization step
  because probably the software already knows how to reconstruct the HUD
  correctly from scratch.

* Furthermore, the registry makes heavy use of _type-erasure_ techniques
  internally and doesn't know at any time what types of components it contains.
  Therefore being explicit at the call point is mandatory.

The `tag` member function is similar to the previous one, apart from the fact
that it works with tags and not with components.<br/>
Note also that both `component` and `tag` store items along with entities. It
means that they work properly without a call to the `entities` member function.

Once a snapshot is created, there exist main two _ways_ to load it: as a whole
and in a kind of _progressive mode_.<br/>
The following sections describe both loaders and archives in details.

#### Snapshot loader

A snapshot loader requires that the destination registry be empty and loads all
the data at once while keeping intact the identifiers that the entities
originally had.<br/>
To do that, the registry offers a member function named `restore` that returns a
temporary object properly initialized to _restore_ a snapshot:

Example of use:

```cpp
InputArchive input;

registry.restore()
    .entities()
    .destroyed()
    .component<AComponent, AnotherComponent>(output)
    .tag<MyTag>(output)
    .orphans();
```

It isn't necessary to invoke all these functions each and every time. What
functions to use in which case mostly depends on the goal and there is not a
golden rule to do that. For obvious reasons, what is important is that the data
are restored in exactly the same order in which they were serialized.

The `entities` and `destroyed` member functions restore the sets of entities and
the versions that the entities originally had at the source.

The `component` member function restores all and only the components specified
and assigns them to the right entities. Note that the template parameter list
must be exactly the same used during the serialization. The same applies to the
`tag` member function.

The `orphans` member function literally destroys those entities that have
neither components nor tags. It's usually useless if the snapshot is a full dump
of the source. However, in case all the entities were serialized but only part
of the components and tags was saved, it could happen that some of the entities
have neither components nor tags once restored. The best one can do to deal with
them is to destroy those entities and thus update their versions.

#### Progressive loader

A progressive loader is designed to load data from a source registry to a
(possibly) non-empty destination. The loader can accomodate in a registry more
than one snapshot in a sort of _progressive loading_ that updates the
destination one step at a time.<br/>
Identifiers that entities originally had are not transferred to the target.
Instead, the loader maps remote identifiers to local ones while restoring a
snapshot. Because of that, this kind of loader offers a way to update
automatically identifiers that are part of components or tags (as an example, as
data members or gathered in a container).<br/>
Another difference with the snapshot loader is that the progressive loader does
not need to work with the private data structures of a registry. Furthermore, it
has an internal state that must persist over time. Therefore, there is no reason
to create it by means of a registry, or to limit its lifetime to that of a
temporary object.

Example of use:

```cpp
entt::ProgressiveLoader<entity_type> loader{registry};
InputArchive input;

loader.entities(input)
    .destroyed(input)
    .component<AComponent, AnotherComponent>(input)
    .component<DirtyComponent>(input, &DirtyComponent::parent, &DirtyComponent::child)
    .tag<MyTag>(input)
    .tag<DirtyTag>(input, &DirtyTag::container)
    .orphans()
    .shrink();
```

It isn't necessary to invoke all these functions each and every time. What
functions to use in which case mostly depends on the goal and there is not a
golden rule to do that. For obvious reasons, what is important is that the data
are restored in exactly the same order in which they were serialized.

The `entities` and `destroyed` member functions restore groups of entities and
map each entity to a local counterpart when required. In other terms, for each
remote entity identifier not yet registered by the loader, the latter creates a
local identifier so that it can keep the local entity in sync with the remote
one.

The `component` and `tag` member functions restore all and only the components
and the tags specified and assigns them to the right entities.<br/>
In case the component or tag contains entities itself (either as data members of
type `entity_type` or as containers of entities), the loader can update them
automatically. To do that, it's enough to specify the data members to update as
shown in the example. If the component or tag was in the middle of the template
parameter list during serialization, multiple commands are required during a
restore:

```cpp
registry.snapshot().component<ASimpleComponent, AnotherSimpleComponent, AMoreComplexComponent, TheLastComponent>();

// ...

loader
    .component<ASimpleComponent, AnotherSimpleComponent>(input)
    .component<AMoreComplexComponent>(input, &AMoreComplexComponent::entity);
    .component<TheLastComponent>(input);
```

The `orphans` member function literally destroys those entities that have
neither components nor tags after a restore. It has exactly the same purpose
described in the previous section and works the same way.

Finally, `shrink` helps to purge local entities that no longer have a remote
conterpart. Users should invoke this member function after restoring each
snapshot, unless they know exactly what they are doing.

#### Archives

Archives must publicly expose a predefined set of member functions. The API is
straightforward and consists only of a group of function call operators that
are invoked by the registry.

In particular:

* An output archive, the one used when creating a snapshot, must expose a
  function call operator with the following signature to store entities:

  ```cpp
  void operator()(Entity);
  ```

  Where `Entity` is the type of the entities used by the registry.<br/>
  In addition, it must accept the types of both the components and the tags to
  serialize. Therefore, given a type `T` (either a component or a tag), it must
  contain a function call operator with the following signature:

  ```cpp
  void operator()(const T &);
  ```

  The output archive can freely decide how to serialize the data. The register
  is not affected at all by the decision.

* An input archive, the one used when restoring a snapshot, must expose a
  function call operator with the following signature to load entities:

  ```cpp
  void operator()(Entity &);
  ```

  Where `Entity` is the type of the entities used by the registry. Each time the
  function is invoked, the archive must read the next element from the
  underlying storage and copy it in the given variable.<br/>
  In addition, it must accept the types of both the components and the tags to
  restore. Therefore, given a type `T` (either a component or a tag), it must
  contain a function call operator with the following signature:

  ```cpp
  void operator()(T &);
  ```

  Every time such an operator is invoked, the archive must read the next element
  from the underlying storage and copy it in the given variable.

This API is freely inspired by a well known
[library for serialization](https://uscilab.github.io/cereal/).

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
component, the most important of which can be used to reset the view and refresh
the reference to the set of candidate entities to iterate.<br/>
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
once: if the view has been already prepared before, the function returns
immediately and does nothing.

A persistent view offers a bunch of functionalities to get the number of
entities it's going to return, a raw access to the entity list and the
possibility to sort the underlying data structures according to the order of one
of the components for which it has been constructed.<br/>
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
In some cases there may be the need to iterate all the entities regardless of
their components. The registry offers a specific member function to do that:

```cpp
registry.each([](auto entity) {
    // ...
});
```

It returns to the caller all the entities that are still in use by means of the
given function.<br/>
The same method can be used also to iterate entities that have a specific set of
components without creating a view:

```cpp
const auto &cregistry = registry;

// const and non-const references
registry.each<Position, Velocity>([](auto entity, auto &position, auto &velocity) { /* ... */ });
cregistry.each<Position, Velocity>([](auto entity, const auto &position, const auto &velocity) { /* ... */ });
```

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

## Side notes

* Entity identifiers are numbers and nothing more. They are not classes and they
  have no member functions at all. As already mentioned, do no try to inspect or
  modify an entity descriptor in any way.

* As shown in the examples above, the preferred way to get references to the
  components while iterating a view is by using the view itself. It's a faster
  alternative to the `get` member function template that is part of the API of
  the Registry. This is because the registry must ensure that a pool for the
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
  As an example, users can freely execute the rendering system and iterate the
  renderable entities while updating a physic component concurrently on a
  separate thread if needed.

# Contributors

If you want to contribute, please send patches as pull requests against the
branch `master`.<br/>
Check the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to see
who has partecipated so far.

# License

Code and documentation Copyright (c) 2018 Michele Caini.<br/>
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
