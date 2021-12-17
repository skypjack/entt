# Crash Course: entity-component system

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Design decisions](#design-decisions)
  * [Type-less and bitset-free](#type-less-and-bitset-free)
  * [Build your own](#build-your-own)
  * [Pay per use](#pay-per-use)
  * [All or nothing](#all-or-nothing)
* [Vademecum](#vademecum)
* [Pools](#pools)
* [The Registry, the Entity and the Component](#the-registry-the-entity-and-the-component)
  * [Observe changes](#observe-changes)
    * [They call me Reactive System](#they-call-me-reactive-system)
  * [Sorting: is it possible?](#sorting-is-it-possible)
  * [Helpers](#helpers)
    * [Null entity](#null-entity)
    * [Tombstone](#tombstone)
    * [To entity](#to-entity)
    * [Dependencies](#dependencies)
    * [Invoke](#invoke)
    * [Handle](#handle)
    * [Organizer](#organizer)
  * [Context variables](#context-variables)
    * [Aliased properties](#aliased-properties)
  * [Pointer stability](#pointer-stability)
    * [In-place delete](#in-place-delete)
    * [Hierarchies and the like](#hierarchies-and-the-like)
  * [Making the most of range-destroy](#making-the-most-of-range-destroy)
  * [Meet the runtime](#meet-the-runtime)
    * [A base class to rule them all](#a-base-class-to-rule-them-all)
    * [Beam me up, registry](#beam-me-up-registry)
  * [Snapshot: complete vs continuous](#snapshot-complete-vs-continuous)
    * [Snapshot loader](#snapshot-loader)
    * [Continuous loader](#continuous-loader)
    * [Archives](#archives)
    * [One example to rule them all](#one-example-to-rule-them-all)
* [Views and Groups](#views-and-groups)
  * [Views](#views)
    * [View pack](#view-pack)
    * [Runtime views](#runtime-views)
  * [Groups](#groups)
    * [Full-owning groups](#full-owning-groups)
    * [Partial-owning groups](#partial-owning-groups)
    * [Non-owning groups](#non-owning-groups)
    * [Nested groups](#nested-groups)
  * [Types: const, non-const and all in between](#types-const-non-const-and-all-in-between)
  * [Give me everything](#give-me-everything)
  * [What is allowed and what is not](#what-is-allowed-and-what-is-not)
    * [More performance, more constraints](#more-performance-more-constraints)
* [Empty type optimization](#empty-type-optimization)
* [Multithreading](#multithreading)
  * [Iterators](#iterators)
  * [Const registry](#const-registry)
* [Beyond this document](#beyond-this-document)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` is a header-only, tiny and easy to use entity-component system (and much
more) written in modern C++.<br/>
The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development.

# Design decisions

## Type-less and bitset-free

`EnTT` offers a _bitset-free_ entity-component system that doesn't require users
to specify the set of components neither at compile-time nor at runtime.<br/>
This is why users can instantiate the core class simply like:

```cpp
entt::registry registry;
```

In place of its more annoying and error-prone counterpart:

```cpp
entt::registry<comp_0, comp_1, ..., comp_n> registry;
```

Furthermore, it isn't necessary to announce the existence of a component type.
When the time comes, just use it and that's all.

## Build your own

`EnTT` is designed as a container that can be used at any time just as a vector
or any other tool would be used. It doesn't attempt in any way to take over on
the user code base, nor to control its main loop or process scheduling.<br/>
Unlike other more or less known models, it makes use of independent pools. This
has some advantages and disadvantages. The main purpose is to provide a fully
customizable tool, where users have the freedom to define pools and opaque
proxies for types with specific requirements.

The library provides a default implementation for many things and a mixin model
that allows users to completely replace or even just enrich the pool dedicated
to one or more components.<br/>
The built-in signal support is an example of that: defined as a mixin, it's
easily disabled if not needed. Similarly, the storage class has a specialization
that shows how everything is customizable down to the smallest detail.

## Pay per use

`EnTT` is entirely designed around the principle that users have to pay only for
what they want.

When it comes to using an entity-component system, the tradeoff is usually
between performance and memory usage. The faster it is, the more memory it uses.
Even worse, some approaches tend to heavily affect other functionalities like
the construction and destruction of components to favor iterations, even when it
isn't strictly required. In fact, slightly worse performance along non-critical
paths are the right price to pay to reduce memory usage and have overall better
perfomance sometimes and I've always wondered why this kind of tools do not
leave me the choice.<br/>
`EnTT` follows a completely different approach. It gets the best out from the
basic data structures and gives users the possibility to pay more for higher
performance where needed.

So far, this choice has proven to be a good one and I really hope it can be for
many others besides me.

## All or nothing

`EnTT` is such that a `T**` pointer (or whatever a custom pool returns) is
always available to directly access all the instances of a given component type
`T`.<br/>
I cannot say whether it will be useful or not to the reader, but it's worth to
mention it since it's one of the corner stones of this library.

Many of the tools described below give the possibility to get this information
and have been designed around this need.<br/>
The rest is experimentation and the desire to invent something new, hoping to
have succeeded.

# Vademecum

The registry to store, the views and the groups to iterate. That's all.

An entity (the _E_ of an _ECS_) is an opaque identifier that users should use
as-is. Inspecting an identifier isn't recommended since its format can change in
future and a registry has all the functionalities to query them out-of-the-box.
The type `entt::entity` implements the concept of _entity identifier_.<br/>
Components (the _C_ of an _ECS_) must be both move constructible and move
assignable. They are list initialized by using the parameters provided to
construct the component itself. No need to register components or their types
neither with the registry nor with the entity-component system at all.<br/>
Systems (the _S_ of an _ECS_) can be plain functions, functors, lambdas and so
on. It's not required to announce them in any case and have no requirements.

The following sections explain in short how to use the entity-component system,
the core part of the whole library.<br/>
The project is composed of many other classes in addition to those describe
below. For more details, please refer to the inline documentation.

# Pools

In `EnTT`, pools of components are made available through a specialized version
of a sparse set.

Each pool contains all the instances of a single component and all the entities
to which it's assigned. Sparse arrays are _paged_ to avoid wasting memory in
some cases. Packed arrays of components are also paged to have pointer stability
upon additions. Packed arrays of entities are not instead.<br/>
All pools can rearrange their items in order to keep the internal arrays tightly
packed and maximize performance.

# The Registry, the Entity and the Component

A registry can store and manage entities, as well as create views and groups to
iterate the underlying data structures.<br/>
The class template `basic_registry` lets users decide what's the preferred type
to represent an entity. Because `std::uint32_t` is large enough for almost all
the cases, there exists also the enum class `entt::entity` that _wraps_ it and
the alias `entt::registry` for `entt::basic_registry<entt::entity>`.

Entities are represented by _entity identifiers_. An entity identifier contains
information about the entity itself and its version.<br/>
User defined identifiers can be introduced through enum classes and class types
that define an `entity_type` member of type `std::uint32_t` or `std::uint64_t`.

A registry is used both to construct and to destroy entities:

```cpp
// constructs a naked entity with no components and returns its identifier
auto entity = registry.create();

// destroys an entity and all its components
registry.destroy(entity);
```

The `create` member function accepts also a hint and has an overload that gets
two iterators and can be used to generate multiple entities at once efficiently.
Similarly, the `destroy` member function works also with a range of entities:

```cpp
// destroys all the entities in a range
auto view = registry.view<a_component, another_component>();
registry.destroy(view.begin(), view.end());
```

In addition to offering an overload to force the version upon destruction. Note
that this function removes all components from an entity before releasing its
identifier. There exists also a _lighter_ alternative that only releases the
elements without poking in any pool, for use with orphaned entities:

```cpp
// releases an orphaned identifier
registry.release(entity);
```

As with the `destroy` function, also in this case entity ranges are supported
and it's possible to force the version during release.

In both cases, when an identifier is released, the registry can freely reuse it
internally. In particular, the version of an entity is increased (unless the
overload that forces a version is used instead of the default one).<br/>
Users can probe an identifier to know the information it carries:

```cpp
// returns true if the entity is still valid, false otherwise
bool b = registry.valid(entity);

// gets the version contained in the entity identifier
auto version = registry.version(entity);

// gets the actual version for the given entity
auto curr = registry.current(entity);
```

Components can be assigned to or removed from entities at any time. As for the
entities, the registry offers a set of functions to use to work with components.

The `emplace` member function template creates, initializes and assigns to an
entity the given component. It accepts a variable number of arguments to use to
construct the component itself if present:

```cpp
registry.emplace<position>(entity, 0., 0.);

// ...

auto &vel = registry.emplace<velocity>(entity);
vel.dx = 0.;
vel.dy = 0.;
```

The default storage _detects_ aggregate types internally and exploits aggregate
initialization when possible.<br/>
Therefore, it's not strictly necessary to define a constructor for each type, in
accordance with the rules of the language.

On the other hand, `insert` works with _ranges_ and can be used to:

* Assign the same component to all entities at once when a type is specified as
  a template parameter or an instance is passed as an argument:

  ```cpp
  // default initialized type assigned by copy to all entities
  registry.insert<position>(first, last);

  // user-defined instance assigned by copy to all entities
  registry.insert(from, to, position{0., 0.});
  ```

* Assign a set of components to the entities when a range is provided (the
  length of the range of components must be the same of that of entities):

  ```cpp
  // first and last specify the range of entities, instances points to the first element of the range of components
  registry.insert<position>(first, last, instances);
  ```

If an entity already has the given component, the `replace` and `patch` member
function templates can be used to update it:

```cpp
// replaces the component in-place
registry.patch<position>(entity, [](auto &pos) { pos.x = pos.y = 0.; });

// constructs a new instance from a list of arguments and replaces the component
registry.replace<position>(entity, 0., 0.);
```

When it's unknown whether an entity already owns an instance of a component,
`emplace_or_replace` is the function to use instead:

```cpp
registry.emplace_or_replace<position>(entity, 0., 0.);
```

This is a slightly faster alternative for the following snippet:

```cpp
if(registry.all_of<velocity>(entity)) {
    registry.replace<velocity>(entity, 0., 0.);
} else {
    registry.emplace<velocity>(entity, 0., 0.);
}
```

The `all_of` and `any_of` member functions may also be useful if in doubt about
whether or not an entity has all the components in a set or any of them:

```cpp
// true if entity has all the given components
bool all = registry.all_of<position, velocity>(entity);

// true if entity has at least one of the given components
bool any = registry.any_of<position, velocity>(entity);
```

If the goal is to delete a component from an entity that owns it, the `erase`
member function template is the way to go:

```cpp
registry.erase<position>(entity);
```

When in doubt whether the entity owns the component, use the `remove` member
function instead. It behaves similarly to `erase` but it erases the component
if and only if it exists, otherwise it returns safely to the caller:

```cpp
registry.remove<position>(entity);
```

The `clear` member function works similarly and can be used to either:

* Erases all instances of the given components from the entities that own them:

  ```cpp
  registry.clear<position>();
  ```

* Or destroy all entities in a registry at once:

  ```cpp
  registry.clear();
  ```

Finally, references to components can be retrieved simply as:

```cpp
const auto &cregistry = registry;

// const and non-const reference
const auto &crenderable = cregistry.get<renderable>(entity);
auto &renderable = registry.get<renderable>(entity);

// const and non-const references
const auto [cpos, cvel] = cregistry.get<position, velocity>(entity);
auto [pos, vel] = registry.get<position, velocity>(entity);
```

The `get` member function template gives direct access to the component of an
entity stored in the underlying data structures of the registry. There exists
also an alternative member function named `try_get` that returns a pointer to
the component owned by an entity if any, a null pointer otherwise.

## Observe changes

Because of how the registry works internally, it stores a bunch of signal
handlers for each pool in order to notify some of its data structures on the
construction and destruction of components or when an instance of a component is
explicitly replaced by the user.<br/>
These signal handlers are also exposed and made available to users. These are
the basic bricks to build fancy things like dependencies and reactive systems.

To get a sink to be used to connect and disconnect listeners so as to be
notified on the creation of a component, use the `on_construct` member function:

```cpp
// connects a free function
registry.on_construct<position>().connect<&my_free_function>();

// connects a member function
registry.on_construct<position>().connect<&my_class::member>(instance);

// disconnects a free function
registry.on_construct<position>().disconnect<&my_free_function>();

// disconnects a member function
registry.on_construct<position>().disconnect<&my_class::member>(instance);
```

To be notified when a component is destroyed, use the `on_destroy` member
function instead. Finally, the `on_update` member function will return a sink
to which to connect listeners to observe changes.<br/>
In the last case, given the way C++ works, it's also necessary to use specific
member functions to allow the signal to be triggered. In particular, listeners
attached to `on_update` will only be invoked following a call to `replace` or
`patch`.

The function type of a listener should be equivalent to the following:

```cpp
void(entt::registry &, entt::entity);
```

In all cases, listeners are provided with the registry that triggered the
notification and the involved entity.

Note also that:

* Listeners for the construction signals are invoked **after** components have
  been assigned to entities.

* Listeners designed to observe changes are invoked **after** components have
  been updated.

* Listeners for the destruction signals are invoked **before** components have
  been removed from entities.

There are also some limitations on what a listener can and cannot do:

* Connecting and disconnecting other functions from within the body of a
  listener should be avoided. It can lead to undefined behavior in some cases.

* Removing the component from within the body of a listener that observes the
  construction or update of instances of a given type isn't allowed.

* Assigning and removing components from within the body of a listener that
  observes the destruction of instances of a given type should be avoided. It
  can lead to undefined behavior in some cases. This type of listeners is
  intended to provide users with an easy way to perform cleanup and nothing
  more.

To a certain extent, these limitations don't apply. However, it's risky to try
to force them and users should respect the limitations unless they know exactly
what they are doing.

Events and therefore listeners must not be used as replacements for systems.
They shouldn't contain much logic and interactions with a registry should be
kept to a minimum. Moreover, the greater the number of listeners, the greater
the performance hit when components are created or destroyed.

Please, refer to the documentation of the signal class to know all the features
it offers.<br/>
There are many useful but less known functionalities that aren't described here,
such as the connection objects or the possibility to attach listeners with a
list of parameters that is shorter than that of the signal itself.

### They call me Reactive System

Signals are the basic tools to construct reactive systems, even if they aren't
enough on their own. `EnTT` tries to take another step in that direction with
the `observer` class template.<br/>
In order to explain what reactive systems are, this is a slightly revised quote
from the documentation of the library that first introduced this tool,
[Entitas](https://github.com/sschmid/Entitas-CSharp):

>Imagine you have 100 fighting units on the battlefield but only 10 of them
>changed their positions. Instead of using a normal system and updating all 100
>entities depending on the position, you can use a reactive system which will
>only update the 10 changed units. So efficient.

In `EnTT`, this means to iterating over a reduced set of entities and components
with respect to what would otherwise be returned from a view or a group.<br/>
On these words, however, the similarities with the proposal of `Entitas` also
end. The rules of the language and the design of the library obviously impose
and allow different things.

An `observer` is initialized with an instance of a registry and a set of rules
that describes what are the entities to intercept. As an example:

```cpp
entt::observer observer{registry, entt::collector.update<sprite>()};
```

The class is default constructible and can be reconfigured at any time by means
of the `connect` member function. Moreover, instances can be disconnected from
the underlying registries through the `disconnect` member function.<br/>
The `observer` offers also what is needed to query the internal state and to
know if it's empty or how many entities it contains. Moreover, it can return a
raw pointer to the list of entities it contains.

However, the most important features of this class are that:

* It's iterable and therefore users can easily walk through the list of entities
  by means of a range-for loop or the `each` member function.

* It's clearable and therefore users can consume the entities and literally
  reset the observer after each iteration.

These aspects make the observer an incredibly powerful tool to know at any time
what are the entities that matched the given rules since the last time one
asked:

```cpp
for(const auto entity: observer) {
    // ...
}

observer.clear();
```

The snippet above is equivalent to the following:

```cpp
observer.each([](const auto entity) {
    // ...
});
```

At least as long as the `observer` isn't const. This means that the non-const
overload of `each` does also reset the underlying data structure before to
return to the caller, while the const overload does not for obvious reasons.

The `collector` is an utility aimed to generate a list of `matcher`s (the actual
rules) to use with an `observer` instead.<br/>
There are two types of `matcher`s:

* Observing matcher: an observer will return at least all the living entities
  for which one or more of the given components have been updated and not yet
  destroyed.

  ```cpp
  entt::collector.update<sprite>();
  ```

  _Updated_ in this case means that all listeners attached to `on_update` are
  invoked. In order for this to happen, specific functions such as `patch` must
  be used. Refer to the specific documentation for more details.

* Grouping matcher: an observer will return at least all the living entities
  that would have entered the given group if it existed and that would have
  not yet left it.

  ```cpp
  entt::collector.group<position, velocity>(entt::exclude<destroyed>);
  ```

  A grouping matcher supports also exclusion lists as well as single components.

Roughly speaking, an observing matcher intercepts the entities for which the
given components are updated while a grouping matcher tracks the entities that
have assigned the given components since the last time one asked.<br/>
If an entity already has all the components except one and the missing type is
assigned to it, the entity is intercepted by a grouping matcher.

In addition, a matcher can be filtered with a `where` clause:

```cpp
entt::collector.update<sprite>().where<position>(entt::exclude<velocity>);
```

This clause introduces a way to intercept entities if and only if they are
already part of a hypothetical group. If they are not, they aren't returned by
the observer, no matter if they matched the given rule.<br/>
In the example above, whenever the component `sprite` of an entity is updated,
the observer probes the entity itself to verify that it has at least `position`
and has not `velocity` before to store it aside. If one of the two conditions of
the filter isn't respected, the entity is discared, no matter what.

A `where` clause accepts a theoretically unlimited number of types as well as
multiple elements in the exclusion list. Moreover, every matcher can have its
own clause and multiple clauses for the same matcher are combined in a single
one.

## Sorting: is it possible?

Sorting entities and components is possible with `EnTT`. In particular, it's
feasible with an in-place algorithm that doesn't require memory allocations nor
anything else and is therefore particularly convenient.<br/>
With this in mind, there are two functions that respond to slightly different
needs:

* Components can be sorted either directly:

  ```cpp
  registry.sort<renderable>([](const auto &lhs, const auto &rhs) {
      return lhs.z < rhs.z;
  });
  ```

  Or by accessing their entities:

  ```cpp
  registry.sort<renderable>([](const entt::entity lhs, const entt::entity rhs) {
      return entt::registry::entity(lhs) < entt::registry::entity(rhs);
  });
  ```

  There exists also the possibility to use a custom sort function object for
  when the usage pattern is known. As an example, in case of an almost sorted
  pool, quick sort could be much slower than insertion sort.

* Components can be sorted according to the order imposed by another component:

  ```cpp
  registry.sort<movement, physics>();
  ```

  In this case, instances of `movement` are arranged in memory so that cache
  misses are minimized when the two components are iterated together.

As a side note, the use of groups limits the possibility of sorting pools of
components. Refer to the specific documentation for more details.

## Helpers

The so called _helpers_ are small classes and functions mainly designed to offer
built-in support for the most basic functionalities.<br/>
The list of helpers will grow longer as time passes and new ideas come out.

### Null entity

In `EnTT`, the `entt::null` variable models the concept of _null entity_.<br/>
The library guarantees that the following expression always returns false:

```cpp
registry.valid(entt::null);
```

A registry rejects the null entity in all cases because it isn't considered
valid. It also means that the null entity cannot own components.<br/>
The type of the null entity is internal and should not be used for any purpose
other than defining the null entity itself. However, there exist implicit
conversions from the null entity to identifiers of any allowed type:

```cpp
entt::entity null = entt::null;
```

Similarly, the null entity can be compared to any other identifier:

```cpp
const auto entity = registry.create();
const bool null = (entity == entt::null);
```

As for its integral form, the null entity only affects the entity part of an
identifier and is instead completely transparent to its version.

Be aware that `entt::null` and entity 0 aren't the same thing. Likewise, a zero
initialized entity isn't the same as `entt::null`. Therefore, although
`entt::entity{}` is in some sense an alias for entity 0, none of them can be
used to create a null entity.

### Tombstone

In addition to the null entity, `EnTT` also models the concept of _tombstone_
with the `entt::tombstone` variable.<br/>
Once created, the integral form of the two values is the same, although they
affect different parts of an identifier. In fact, the tombstone uses only the
version part and is completely transparent to the entity part.

Also in this case, the following expression always returns false:

```cpp
registry.valid(entt::tombstone);
```

Moreover, users cannot set the tombstone version when releasing an entity:

```cpp
registry.destroy(entity, entt::tombstone);
```

In this case, a different version number is implicitly generated.<br/>
The type of a tombstone is internal and can change at any time. However, there
exist implicit conversions from a tombstone to identifiers of any allowed type:

```cpp
entt::entity null = entt::tombstone;
```

Similarly, the tombstone can be compared to any other identifier:

```cpp
const auto entity = registry.create();
const bool tombstone = (entity == entt::tombstone);
```

Be aware that `entt::tombstone` and entity 0 aren't the same thing. Likewise, a
zero initialized entity isn't the same as `entt::tombstone`. Therefore, although
`entt::entity{}` is in some sense an alias for entity 0, none of them can be
used to create tombstones.

### To entity

Sometimes it's useful to get the entity from a component instance.<br/>
This is what the `entt::to_entity` helper does. It accepts a registry and an
instance of a component and returns the entity associated with the latter:

```cpp
const auto entity = entt::to_entity(registry, position);
```

A null entity is returned in case the component doesn't belong to the registry.

### Dependencies

The `registry` class is designed to be able to create short circuits between its
functions. This simplifies the definition of _dependencies_ between different
operations.<br/>
For example, the following adds (or replaces) the component `a_type` whenever
`my_type` is assigned to an entity:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::emplace_or_replace<a_type>>();
```

Similarly, the code shown below removes `a_type` from an entity whenever
`my_type` is assigned to it:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::remove<a_type>>();
```

A dependency can also be easily broken as follows:

```cpp
registry.on_construct<my_type>().disconnect<&entt::registry::emplace_or_replace<a_type>>();
```

There are many other types of dependencies. In general, most of the functions
that accept an entity as the first argument are good candidates for this
purpose.

### Invoke

Sometimes it's useful to be able to directly invoke a member function of a
component as a callback. It's already possible in practice but requires users to
_extend_ their classes and this may not always be possible.<br/>
The `invoke` helper allows to _propagate_ the signal in these cases:

```cpp
registry.on_construct<clazz>().connect<entt::invoke<&clazz::func>>();
```

All it does is pick up the _right_ component for the received entity and invoke
the requested method, passing on the arguments if necessary.

### Handle

A handle is a thin wrapper around an entity and a registry. It provides the same
functions that the registry offers for working with components, such as
`emplace`, `get`, `patch`, `remove` and so on. The difference being that the
entity is implicitly passed to the registry.<br/>
It's default constructible as an invalid handle that contains a null registry
and a null entity. When it contains a null registry, calling functions that
delegate execution to the registry will cause an undefined behavior, so it's
recommended to check the validity of the handle with implicit cast to `bool`
when in doubt.<br/>
A handle is also non-owning, meaning that it can be freely copied and moved
around without affecting its entity (in fact, handles happen to be trivially
copyable). An implication of this is that mutability becomes part of the
type.

There are two aliases that use `entt::entity` as their default entity:
`entt::handle` and `entt::const_handle`.<br/>
Users can also easily create their own aliases for custom identifiers as:

```cpp
using my_handle = entt::basic_handle<my_identifier>;
using my_const_handle = entt::basic_handle<const my_identifier>;
```

Handles are also implicitly convertible to const handles out of the box but not
the other way around.<br/>
A handle stores a non-const pointer to a registry and therefore it can do all
the things that can be done with a non-const registry. On the other hand, a
const handles store const pointers to registries and offer a restricted set of
functionalities.

This class is intended to simplify function signatures. In case of functions
that take a registry and an entity and do most of their work on that entity,
users might want to consider using handles, either const or non-const.

### Organizer

The `organizer` class template offers minimal support (but sufficient in many
cases) for creating an execution graph from functions and their requirements on
resources.<br/>
The resulting tasks aren't executed in any case. This isn't the goal of this
tool. Instead, they are returned to the user in the form of a graph that allows
for safe execution.

The functions are added in order of execution to the organizer. Free functions
and member functions are supported as template parameters, however there is also
the possibility to pass pointers to free functions or decayed lambdas as
parameters to the `emplace` member function:

```cpp
entt::organizer organizer;

// adds a free function to the organizer
organizer.emplace<&free_function>();

// adds a member function and an instance on which to invoke it to the organizer
clazz instance;
organizer.emplace<&clazz::member_function>(&instance);

// adds a decayed lambda directly
organizer.emplace(+[](const void *, entt::registry &) { /* ... */ });
```

As for free functions and member functions, these are the parameters that can be
presented by their function types and that will be correctly handled:

* A possibly constant reference to a registry. The one passed to the task when
  it's run will also be passed to the function as-is.

* An `entt::view` with any possible combination of types. It will be created
  from the registry passed to the task and supplied directly to the function.

* A possibly constant reference to any type `T`. It will be interpreted as
  context variable, which will be created within the registry and passed to the
  function.

The function type for free functions and decayed lambdas passed as parameters to
`emplace` is `void(const void *, entt::registry &)` instead. The registry is the
same as provided to the task. The first parameter is an optional pointer to user
defined data to provide upon registration:

```cpp
clazz instance;
organizer.emplace(+[](const void *, entt::registry &) { /* ... */ }, &instance);
```

In all cases, it's also possible to associate a name with the task when creating
it. For example:

```cpp
organizer.emplace<&free_function>("func");
```

When a function of any type is registered with the organizer, everything it
accesses is considered a _resource_ (views are _unpacked_ and their types are
treated as resources). The _constness_ of the type also dictates its access mode
(RO/RW). In turn, this affects the resulting graph, since it influences the
possibility of launching tasks in parallel.<br/>
As for the registry, if a function doesn't explicitly request it or requires a
constant reference to it, it's considered a read-only access. Otherwise, it's
considered as read-write access. All functions will still have the registry
among their resources.

When registering a function, users can also require resources that aren't in the
list of parameters of the function itself. These are declared as template
parameters:

```cpp
organizer.emplace<&free_function, position, velocity>("func");
```

Similarly, users can override the access mode of a type again via template
parameters:

```cpp
organizer.emplace<&free_function, const renderable>("func");
```

In this case, even if `renderable` appears among the parameters of the function
as not constant, it will be treated as constant as regards the generation of the
task graph.

To generate the task graph, the organizer offers the `graph` member function:

```cpp
std::vector<entt::organizer::vertex> graph = organizer.graph();
```

The graph is returned in the form of an adjacency list. Each vertex offers the
following features:

* `ro_count` and `rw_count`: they return the number of resources accessed in
  read-only or read-write mode.

* `ro_dependency` and `rw_dependency`: useful for retrieving the type info
  objects associated with the parameters of the underlying function.

* `top_level`: indicates whether a node is a top level one, that is, it has no
  entering edges.

* `info`: returns the type info object associated with the underlying function.

* `name`: returns the name associated with the given vertex if any, a null
  pointer otherwise.

* `callback`: a pointer to the function to execute and whose function type is
  `void(const void *, entt::registry &)`.

* `data`: optional data to provide to the callback.

* `children`: the vertices reachable from the given node, in the form of indices
  within the adjacency list.

Since the creation of pools and resources within the registry isn't necessarily
thread safe, each vertex also offers a `prepare` function which can be called to
setup a registry for execution with the created graph:

```cpp
auto graph = organizer.graph();
entt::registry registry;

for(auto &&node: graph) {
    node.prepare(registry);
}
```

The actual scheduling of the tasks is the responsibility of the user, who can
use the preferred tool.

## Context variables

It is often convenient to assign context variables to a registry, so as to make
it the only _source of truth_ of an application.<br/>
This is possible by means of a member function named `set` to use to create a
context variable from a given type. Either `ctx` or `try_ctx` can be used to
retrieve the newly created instance, while `unset` is meant to clear the
variable if needed:

```cpp
// creates a new context variable initialized with the given values
registry.set<my_type>(42, 'c');

// gets the context variable as a non-const reference from a non-const registry
auto &var = registry.ctx<my_type>();

// gets the context variable as a const reference from either a const or a non-const registry
const auto &cvar = registry.ctx<const my_type>();

// unsets the context variable
registry.unset<my_type>();
```

The type of a context variable must be such that it's default constructible and
can be moved. The `set` member function either creates a new instance of the
context variable or overwrites an already existing one if any.<br/>
The `try_ctx` member function returns a pointer to the context variable if it
exists, otherwise it returns a null pointer. As `ctx`, it supports both const
and non-const types and requires a const one when used on a const registry:

```cpp
if(auto *cptr = registry.try_ctx<const my_type>(); cptr) {
    // uses the context variable associated with the registry, if any
}
```

### Aliased properties

Context variables can also be used to create aliases for existing variables that
aren't directly managed by the registry. In this case, it's also possible to
make them read-only.<br/>
To do that, the type used upon construction must be a reference type and an
lvalue is necessarily provided as an argument:

```cpp
time clock;
registry.set<my_type &>(clock);
```

Read-only aliased properties are created using const types instead:

```cpp
registry.set<const my_type &>(clock);
```

From the point of view of the user, there are no differences between a variable
that is managed by the registry and an aliased property. However, read-only
variables aren't accesible as non-const references:

```cpp
// read-only variables only support const access
const my_type *ptr = registry.try_ctx<const my_type>();
const my_type &var = registry.ctx<const my_type>();
```

Aliased properties can be unset and are overwritten when `set` is invoked, as it
happens with standard variables.

## Pointer stability

The ability to achieve pointer stability for one, several or all components is a
direct consequence of the design of `EnTT` and of its default storage.<br/>
In fact, although it contains what is commonly referred to as a _packed array_,
the default storage is paged and doesn't suffer from invalidation of references
when it runs out of space and has to reallocate.<br/>
However, this isn't enough to ensure pointer stability in case of deletion. For
this reason, a _stable_ deletion method is also offered. This one is such that
the position of the elements is preserved by creating tombstones upon deletion
rather than trying to fill the holes that are created.

For performance reasons, `EnTT` will also favor storage compaction in all cases,
although often accessing a component occurs mostly randomly or traversing pools
in a non-linear order on the user side (as in the case of a hierarchy).<br/>
In other words, pointer stability is not automatic but is enabled on request.

### In-place delete

By default, `EnTT` keeps all pools compact when a component is removed. This is
done through a swap-and-pop between the removed item and the one occupying the
last position in the storage.<br/>
Unfortunately, this also inevitably leads the components to change position
within the storage, making direct access almost impossible (be it via pointer or
index).

However, the underlying model with its independent pools helps introduce storage
with different deletion policies, so that users can best choose type by
type.<br/>
In particular, the library offers out of the box support for in-place deletion,
thus offering storage with completely stable pointers. To do so, it's required
to specialize the `component_traits` class.<br/>
The definition common to all components is the following:

```cpp
struct basic_component_traits {
    static constexpr auto in_place_delete = false;
    static constexpr auto ignore_if_empty = ENTT_IGNORE_IF_EMPTY;
    static constexpr auto page_size = ENTT_PACKED_PAGE;
};
```

Where `in_place_delete` instructs the library on the deletion policy for a given
type while `ignore_if_empty` selectively disables empty type optimization and
`page_size` dictates the storage behavior for non-empty types.<br/>
The `component_traits` class template is _sfinae-friendly_, it supports single
and multi type specializations as well as feature-based ones:

```cpp
template<>
struct entt::component_traits<position>: basic_component_traits {
    using in_place_delete = std::true_type;
};
```

This will ensure in-place deletion for the `position` component without further
user intervention.<br/>
Views and groups adapt accordingly when they detect a storage with a different
deletion policy than the default. No specific action is required from the user
once in-place deletion is enabled. In particular:

* Groups are incompatible with stable storage and will trigger a compile-time
  error if detected.

* Multi type views are completely transparent to storage policies.

* Single type views for stable storage types offer the same interface of multi
  type views. For example, only `size_hint` is available.

In other words, the more generic version of a view will be provided in case of
stable storage, even for single components, always supported by an appropriate
iteration policy if required.<br/>
The latter will be such that in no case will a tombstone be returned from the
view itself, regardless of the iteration method. Similarly, no non-existent
components will be accessed, which could result in an UB otherwise.

### Hierarchies and the like

`EnTT` doesn't attempt in any way to offer built-in methods with hidden or
unclear costs to facilitate the creation of hierarchies.<br/>
There are various solutions to the problem, such as using the following class:

```cpp
struct relationship {
    std::size_t children{};
    entt::entity first{entt::null};
    entt::entity prev{entt::null};
    entt::entity next{entt::null};
    entt::entity parent{entt::null};
    // ... other data members ...
};
```

However, it should be pointed out that the possibility of having stable pointers
for one, many or all types solves the problem of hierarchies at the root in many
cases.<br/>
In fact, if a certain type of component is visited mainly in random order or
according to hierarchical relationships, using direct pointers has many
advantages:

```cpp
struct transform {
    transform *parent;
    // ... other data members ...
};

template<>
struct entt::component_traits<transform>: basic_component_traits {
    using in_place_delete = std::true_type;
};
```

Furthermore, it's quite common for a group of elements to be created close in
time and therefore fallback into adjacent positions, thus favoring locality even
on random accesses. Locality that won't be sacrificed over time given the
stability of storage positions, with undoubted performance advantages.<br/>
Of course, the cost moves to linear iterations, where views and groups will have
to identify (and discard) all tombstones. However, once considered the benefits,
from performance to ease of use, and given the many optimizations that make this
cost negligible, this is configured as one of the most convenient solutions and
certainly something to take into consideration.

## Making the most of range-destroy

The range-destroy functionality offers an improved path under the hood. To
understand it, let's try to describe what problem it tries to solve.<br/>
This function accepts two iterators that point to the beginning and end of a
range of entities. If the iterators are those returned from a view, this pair
cannot be passed to the first storage asking to remove all entities and then to
all other storage. This is because the range may be empty when passed to the
second pool, as not all of those entities still own all the components iterated
from the view itself.<br/>
As a result, only one component is removed and no entities are destroyed.

To avoid this, in many cases the registry doesn't pass the range to all pools.
Instead, it iterates the range and passes an entity at a time to all pools.<br/>
It goes without saying that the latter is slightly slower than the former.

On the other side, the `destroy` function also uses `is_iterator_type` under the
hood to detect _dangerous_ iterators. Whenever possible, it still chooses the
fastest path.<br/>
This means that performance will improve if, for example, two iterators returned
from an `std::vector` are used or, more in general, with all iterators that are
not part of `EnTT`.

Unfortunately, this risks falling into the error described above in some corner
cases. In particular, where an iterator is used that is not defined by `EnTT`
but which uses one of the latter _within_ it.<br/>
It's quite unlikely to happen even in large software. However, the library
offers a solution also in this case, so as to allow for custom iterators and
better performance at the same time.<br/>
In particular, it's necessary to either expose the member type `iterator_type`
and declare that an iterator from `EnTT` is used internally or specialize the
`is_iterator_type` class to drive the choice of the `destroy` function.<br/>
In both cases, the aim is to not choose the optimized route if it can cause
problems.

With a good chance, the last note can be ignored and there will never be a need
to do the above even after writing millions of lines of code.<br/>
However, it's good to know how to exploit the `destroy` function to get the best
out of it.

## Meet the runtime

`EnTT` takes full advantage of what the language offers at compile-time.<br/>
However, this can have its downsides (well known to those familiar with type
erasure techniques).

To bridge this gap, the library provides a bunch of utilities and features that
can be very useful when needed.<br/>
On the one hand, storage classes are standalone objects that allow users to work
with components within certain limits and without knowing the actual types. On
the other hand, it's possible to create component storage dynamically and link
them to a name rather than a type.

### A base class to rule them all

Storage classes are fully self-contained types. These can be extended via mixins
to add more functionalities (generic or type specific). In addition, they offer
a basic set of functions that already allow users to go very far.<br/>
The aim is to limit the need for customizations as much as possible, offering
what is usually necessary for the majority of cases.

When a storage is used through its base class (i.e. when its actual type isn't
known), there is always the possibility of receiving a `type_info` describing
the type of the objects associated with the entities (if any):

```cpp
if(entt::type_id<velocity>() == base.type()) {
    // ...
}
```

Furthermore, all features rely on internal functions that forward the calls to
the mixins. The latter can then make use of any context variables, which can be
set via `bind`:

```cpp
base.bind(entt::forward_as_any(registry));
```

The `bind` function accepts an `entt::any` object, that is a _typed type-erased_
value.<br/>
This is how a registry _passes_ itself to all pools that support signals and
also why a storage keeps sending events without requiring the registry to be
passed to it every time.

Alongside these more specific things, there are also a couple of functions
designed to address some common requirements such as copying an entity.<br/>
In particular, the base class behind a storage offers the possibility to _take_
the object associated with an entity through an opaque pointer:

```cpp
const void *instance = base.get(entity);
```

Similarly, the non-specialized `emplace` function accepts an optional opaque
pointer and behaves differently depending on the case:

* When the pointer is null, the function tries to default-construct an instance
  of the object to bind to the entity and returns true on success.

* When the pointer is non-null, the function tries to copy-construct an instance
  of the object to bind to the entity and returns true on success.

This means that, starting from a reference to the base, it's possible to bind
components with entities without knowing their actual type and even initialize
them by copy if needed:

```cpp
// create a copy of an entity component by component
for(auto &&curr: registry.storage()) {
    if(auto &storage = curr.second; storage.contains(entity)) {
        storage.emplace(other, storage.get(entity));
    }
}
```

This is particularly useful to clone entities in an opaque way. In addition, the
decoupling of features allows for filtering or use of different copying policies
depending on the type.

### Beam me up, registry

`EnTT` is strongly based on types and has always allowed to create only one
storage of a certain type within a registry.<br/>
However, this doesn't work well for users who want to create multiple storage of
the same type associated with different _names_, such as for interacting with a
scripting system.

Nowadays, the library has _solved_ this limitation and offers the possibility of
associating a _type_ with a name (or rather, a numeric identifier):

```cpp
using namespace entt::literals;
auto &&storage = registry.storage<velocity>("second pool"_hs);
```

If a name isn't provided, the default storage associated with the given type is
always returned.<br/>
Since the storage are also self-contained, the registry doesn't try in any way
to _duplicate_ its API and offer parallel functionalities for storage discovered
by name.<br/>
However, there is still no limit to the possibilities of use. For example:

```cpp
auto &&other = registry.storage<velocity>("other"_hs);

registry.emplace<velocity>(entity);
storage.emplace(entity);
```

In other words, anything that can be done via the registry interface can also be
done directly on the reference storage.<br/>
On the other hand, those calls involving all storage are guaranteed to also
_reach_ manually created ones:

```cpp
// will remove the entity from both storage
registry.destroy(entity);
```

Finally, a storage of this type can be used with any view (which also accept
multiple storages of the same type, if necessary):

```cpp
// direct initialization
entt::basic_view direct{
    registry.storage<velocity>(),
    registry.storage<velocity>("other"_hs)
};

// concatenation
auto join = registry.view<velocity>() | entt::basic_view{registry.storage<velocity>("other"_hs)};
```

The possibility of direct use of storage combined with the freedom of being able
to create and use more than one of the same type opens the door to the use of
`EnTT` _at runtime_, which was previously quite limited.<br/>
Sure the basic design remains very type-bound, but finally it's no longer bound
to this one option alone.

## Snapshot: complete vs continuous

The `registry` class offers basic support to serialization.<br/>
It doesn't convert components to bytes directly, there wasn't the need of
another tool for serialization out there. Instead, it accepts an opaque object
with a suitable interface (namely an _archive_) to serialize its internal data
structures and restore them later. The way types and instances are converted to
a bunch of bytes is completely in charge to the archive and thus to final users.

The goal of the serialization part is to allow users to make both a dump of the
entire registry or a narrower snapshot, that is to select only the components in
which they are interested.<br/>
Intuitively, the use cases are different. As an example, the first approach is
suitable for local save/restore functionalities while the latter is suitable for
creating client-server applications and for transferring somehow parts of the
representation side to side.

To take a snapshot of a registry, use the `snapshot` class:

```cpp
output_archive output;

entt::snapshot{registry}
    .entities(output)
    .component<a_component, another_component>(output);
```

It isn't necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal and there is not a golden rule
for that.

The `entities` member function makes the snapshot serialize all entities (both
those still alive and those released) along with their versions.<br/>
On the other hand, the `component` member function is a function template the
aim of which is to store aside components. The presence of a template parameter
list is a consequence of a couple of design choices from the past and in the
present:

* First of all, there is no reason to force a user to serialize all the
  components at once and most of the times it isn't desiderable. As an example,
  in case the stuff for the HUD in a game is put into the registry for some
  reasons, its components can be freely discarded during a serialization step
  because probably the software already knows how to reconstruct them correctly.

* Furthermore, the registry makes heavy use of _type-erasure_ techniques
  internally and doesn't know at any time what component types it contains.
  Therefore being explicit at the call site is mandatory.

There exists also another version of the `component` member function that
accepts a range of entities to serialize. This version is a bit slower than the
other one, mainly because it iterates the range of entities more than once for
internal purposes. However, it can be used to filter out those entities that
shouldn't be serialized for some reasons.<br/>
As an example:

```cpp
const auto view = registry.view<serialize>();
output_archive output;

entt::snapshot{registry}.component<a_component, another_component>(output, view.begin(), view.end());
```

Note that `component` stores items along with entities. It means that it works
properly without a call to the `entities` member function.

Once a snapshot is created, there exist mainly two _ways_ to load it: as a whole
and in a kind of _continuous mode_.<br/>
The following sections describe both loaders and archives in details.

### Snapshot loader

A snapshot loader requires that the destination registry be empty and loads all
the data at once while keeping intact the identifiers that the entities
originally had.<br/>
To use it, just pass to the constructor a valid registry:

```cpp
input_archive input;

entt::snapshot_loader{registry}
    .entities(input)
    .component<a_component, another_component>(input)
    .orphans();
```

It isn't necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal and there is not a golden rule
for that. For obvious reasons, what is important is that the data are restored
in exactly the same order in which they were serialized.

The `entities` member function restores the sets of entities and the versions
that they originally had at the source.

The `component` member function restores all and only the components specified
and assigns them to the right entities. Note that the template parameter list
must be exactly the same used during the serialization.

The `orphans` member function literally releases those entities that have no
components attached. It's usually useless if the snapshot is a full dump of the
source. However, in case all the entities are serialized but only few components
are saved, it could happen that some of the entities have no components once
restored. The best the users can do to deal with them is to release those
entities and thus update their versions.

### Continuous loader

A continuous loader is designed to load data from a source registry to a
(possibly) non-empty destination. The loader can accommodate in a registry more
than one snapshot in a sort of _continuous loading_ that updates the destination
one step at a time.<br/>
Identifiers that entities originally had are not transferred to the target.
Instead, the loader maps remote identifiers to local ones while restoring a
snapshot. Because of that, this kind of loader offers a way to update
automatically identifiers that are part of components (as an example, as data
members or gathered in a container).<br/>
Another difference with the snapshot loader is that the continuous loader has an
internal state that must persist over time. Therefore, there is no reason to
limit its lifetime to that of a temporary object.

Example of use:

```cpp
entt::continuous_loader loader{registry};
input_archive input;

loader.entities(input)
    .component<a_component, another_component, dirty_component>(input, &dirty_component::parent, &dirty_component::child)
    .orphans()
    .shrink();
```

It isn't necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal and there is not a golden rule
for that. For obvious reasons, what is important is that the data are restored
in exactly the same order in which they were serialized.

The `entities` member function restores groups of entities and maps each entity
to a local counterpart when required. In other terms, for each remote entity
identifier not yet registered by the loader, it creates a local identifier so
that it can keep the local entity in sync with the remote one.

The `component` member function restores all and only the components specified
and assigns them to the right entities.<br/>
In case the component contains entities itself (either as data members of type
`entt::entity` or as containers of entities), the loader can update them
automatically. To do that, it's enough to specify the data members to update as
shown in the example.

The `orphans` member function literally releases those entities that have no
components after a restore. It has exactly the same purpose described in the
previous section and works the same way.

Finally, `shrink` helps to purge local entities that no longer have a remote
conterpart. Users should invoke this member function after restoring each
snapshot, unless they know exactly what they are doing.

### Archives

Archives must publicly expose a predefined set of member functions. The API is
straightforward and consists only of a group of function call operators that
are invoked by the snapshot class and the loaders.

In particular:

* An output archive, the one used when creating a snapshot, must expose a
  function call operator with the following signature to store entities:

  ```cpp
  void operator()(entt::entity);
  ```

  Where `entt::entity` is the type of the entities used by the registry.<br/>
  Note that all member functions of the snapshot class make also an initial call
  to store aside the _size_ of the set they are going to store. In this case,
  the expected function type for the function call operator is:

  ```cpp
  void operator()(std::underlying_type_t<entt::entity>);
  ```

  In addition, an archive must accept a pair of entity and component for each
  type to be serialized. Therefore, given a type `T`, the archive must contain a
  function call operator with the following signature:

  ```cpp
  void operator()(entt::entity, const T &);
  ```

  The output archive can freely decide how to serialize the data. The registry
  is not affected at all by the decision.

* An input archive, the one used when restoring a snapshot, must expose a
  function call operator with the following signature to load entities:

  ```cpp
  void operator()(entt::entity &);
  ```

  Where `entt::entity` is the type of the entities used by the registry. Each
  time the function is invoked, the archive must read the next element from the
  underlying storage and copy it in the given variable.<br/>
  Note that all member functions of a loader class make also an initial call to
  read the _size_ of the set they are going to load. In this case, the expected
  function type for the function call operator is:

  ```cpp
  void operator()(std::underlying_type_t<entt::entity> &);
  ```

  In addition, the archive must accept a pair of references to an entity and its
  component for each type to be restored. Therefore, given a type `T`, the
  archive must contain a function call operator with the following signature:

  ```cpp
  void operator()(entt::entity &, T &);
  ```

  Every time such an operator is invoked, the archive must read the next
  elements from the underlying storage and copy them in the given variables.

### One example to rule them all

`EnTT` comes with some examples (actually some tests) that show how to integrate
a well known library for serialization as an archive. It uses
[`Cereal C++`](https://uscilab.github.io/cereal/) under the hood, mainly
because I wanted to learn how it works at the time I was writing the code.

The code is not production-ready and it isn't neither the only nor (probably)
the best way to do it. However, feel free to use it at your own risk.

The basic idea is to store everything in a group of queues in memory, then bring
everything back to the registry with different loaders.

# Views and Groups

First of all, it's worth answering a question: why views and groups?<br/>
Briefly, they're a good tool to enforce single responsibility. A system that has
access to a registry can create and destroy entities, as well as assign and
remove components. On the other side, a system that has access to a view or a
group can only iterate, read and update entities and components.<br/>
It is a subtle difference that can help designing a better software sometimes.

More in details:

* Views are a non-intrusive tool to access entities and components without
  affecting other functionalities or increasing the memory consumption.

* Groups are an intrusive tool that allows to reach higher performance along
  critical paths but has also a price to pay for that.

There are mainly two kinds of views: _compile-time_ (also known as `view`) and
runtime (also known as `runtime_view`).<br/>
The former requires a compile-time list of component types and can make several
optimizations because of that. The latter can be constructed at runtime instead
using numerical type identifiers and are a bit slower to iterate.<br/>
In both cases, creating and destroying a view isn't expensive at all since they
don't have any type of initialization.

Groups come in three different flavors: _full-owning groups_, _partial-owning
groups_ and _non-owning groups_. The main difference between them is in terms of
performance.<br/>
Groups can literally _own_ one or more component types. They are allowed to
rearrange pools so as to speed up iterations. Roughly speaking: the more
components a group owns, the faster it is to iterate them.<br/>
A given component can belong to multiple groups only if they are _nested_, so
users have to define groups carefully to get the best out of them.

## Views

A view behaves differently if it's constructed for a single component or if it
has been created to iterate multiple components. Even the API is slightly
different in the two cases.

Single type views are specialized to give a boost in terms of performance in all
the situations. This kind of views can access the underlying data structures
directly and avoid superfluous checks. There is nothing as fast as a single type
view. In fact, they walk through a packed (actually paged) array of components
and return them one at a time.<br/>
Views also offer a bunch of functionalities to get the number of entities and
components they are going to return. It's also possible to ask a view if it
contains a given entity.<br/>
Refer to the inline documentation for all the details.

Multi type views iterate entities that have at least all the given components in
their bags. During construction, these views look at the number of entities
available for each component and pick up a reference to the smallest set of
candidates in order to speed up iterations.<br/>
They offer fewer functionalities than single type views. In particular, a multi
type view exposes utility functions to get the estimated number of entities it
is going to return and to know if it contains a given entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store views aside as they are extremely cheap to construct.
In fact, this is even discouraged when creating a view from a const registry.
Since all storage are lazily initialized, they may not exist when the view is
built. Therefore, the view itself will refer to an empty _placeholder_ and will
never be re-assigned the actual storage.<br/>
In all cases, views return newly created and correctly initialized iterators for
the storage they refer to when `begin` or `end` are invoked.

Views share the way they are created by means of a registry:

```cpp
// single type view
auto single = registry.view<position>();

// multi type view
auto multi = registry.view<position, velocity>();
```

Filtering entities by components is also supported:

```cpp
auto view = registry.view<position, velocity>(entt::exclude<renderable>);
```

To iterate a view, either use it in a range-for loop:

```cpp
auto view = registry.view<position, velocity, renderable>();

for(auto entity: view) {
    // a component at a time ...
    auto &position = view.get<position>(entity);
    auto &velocity = view.get<velocity>(entity);

    // ... multiple components ...
    auto [pos, vel] = view.get<position, velocity>(entity);

    // ... all components at once
    auto [pos, vel, rend] = view.get(entity);

    // ...
}
```

Or rely on the `each` member functions to iterate both entities and components
at once:

```cpp
// through a callback
registry.view<position, velocity>().each([](auto entity, auto &pos, auto &vel) {
    // ...
});

// using an input iterator
for(auto &&[entity, pos, vel]: registry.view<position, velocity>().each()) {
    // ...
}
```

Note that entities can also be excluded from the parameter list when received
through a callback and this can improve even further the performance during
iterations.<br/>
Since they aren't explicitly instantiated, empty components aren't returned in
any case.

As a side note, in the case of single type views, `get` accepts but doesn't
strictly require a template parameter, since the type is implicitly defined.
However, when the type isn't specified, for consistency with the multi type
view, the instance will be returned using a tuple:

```cpp
auto view = registry.view<const renderable>();

for(auto entity: view) {
    auto [renderable] = view.get(entity);
    // ...
}
```

**Note**: prefer the `get` member function of a view instead of that of a
registry during iterations to get the types iterated by the view itself.

### View pack

Views are combined with each other to create new and more specific types.<br/>
The type returned when combining multiple views together is itself a view, more
in general a multi component one.

Combining different views tries to mimic C++20 ranges:

```cpp
auto view = registry.view<position>();
auto other = registry.view<velocity>();

auto pack = view | other;
```

The constness of the types is preserved and their order depends on the order in
which the views are combined. Therefore, the pack in the example above will
return an instance of `position` first and then one of `velocity`.<br/>
Since combining views generates views, a chain can be of arbitrary length and
the above type order rules apply sequentially.

### Runtime views

Runtime views iterate entities that have at least all the given components in
their bags. During construction, these views look at the number of entities
available for each component and pick up a reference to the smallest set of
candidates in order to speed up iterations.<br/>
They offer more or less the same functionalities of a multi type view. However,
they don't expose a `get` member function and users should refer to the registry
that generated the view to access components. In particular, a runtime view
exposes utility functions to get the estimated number of entities it is going to
return and to know whether it's empty or not. It's also possible to ask a
runtime view if it contains a given entity.<br/>
Refer to the inline documentation for all the details.

Runtime views are pretty cheap to construct and should not be stored aside in
any case. They should be used immediately after creation and then they should be
thrown away.<br/>
To iterate a runtime view, either use it in a range-for loop:

```cpp
entt::id_type types[] = { entt::type_hash<position>::value(), entt::type_hash<velocity>::value() };
auto view = registry.runtime_view(std::cbegin(types), std::cend(types));

for(auto entity: view) {
    // ...
}
```

Or rely on the `each` member function to iterate entities:

```cpp
entt::id_type types[] = { entt::type_hash<position>::value(), entt::type_hash<velocity>::value() };

registry.runtime_view(std::cbegin(types), std::cend(types)).each([](auto entity) {
    // ...
});
```

Performance are exactly the same in both cases.<br/>
Filtering entities by components is also supported for this kind of views:

```cpp
entt::id_type components[] = { entt::type_hash<position>::value() };
entt::id_type filter[] = { entt::type_hash<velocity>::value() };
auto view = registry.runtime_view(std::cbegin(components), std::cend(components), std::cbegin(filter), std::cend(filter));
```

**Note**: runtime views are meant for all those cases where users don't know at
compile-time what components to _use_ to iterate entities. If possible, don't
use runtime views as their performance are inferior to those of the other views.

## Groups

Groups are meant to iterate multiple components at once and to offer a faster
alternative to multi type views.<br/>
Groups overcome the performance of the other tools available but require to get
the ownership of components and this sets some constraints on pools. On the
other side, groups aren't an automatism that increases memory consumption,
affects functionalities and tries to optimize iterations for all the possible
combinations of components. Users can decide when to pay for groups and to what
extent.<br/>
The most interesting aspect of groups is that they fit _usage patterns_. Other
solutions around usually try to optimize everything, because it is known that
somewhere within the _everything_ there are also our usage patterns. However
this has a cost that isn't negligible, both in terms of performance and memory
usage. Ironically, users pay the price also for things they don't want and this
isn't something I like much. Even worse, one cannot easily disable such a
behavior. Groups work differently instead and are designed to optimize only the
real use cases when users find they need to.<br/>
Another nice-to-have feature of groups is that they have no impact on memory
consumption, put aside full non-owning groups that are pretty rare and should be
avoided as long as possible.

All groups affect to an extent the creation and destruction of their components.
This is due to the fact that they must _observe_ changes in the pools of
interest and arrange data _correctly_ when needed for the types they own.<br/>
That being said, the way groups operate is beyond the scope of this document.
However, it's unlikely that users will be able to appreciate the impact of
groups on the other functionalities of a registry.

Groups offer a bunch of functionalities to get the number of entities and
components they are going to return. It's also possible to ask a group if it
contains a given entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store groups aside for they are extremely cheap to create,
even though valid groups can be copied without problems and reused freely.<br/>
A group performs an initialization step the very first time it's requested and
this could be quite costly. To avoid it, consider creating the group when no
components have been assigned yet. If the registry is empty, preparation is
extremely fast. Groups also return newly created and correctly initialized
iterators whenever `begin` or `end` are invoked.

To iterate groups, either use them in a range-for loop:

```cpp
auto group = registry.group<position>(entt::get<velocity, renderable>);

for(auto entity: group) {
    // a component at a time ...
    auto &position = group.get<position>(entity);
    auto &velocity = group.get<velocity>(entity);

    // ... multiple components ...
    auto [pos, vel] = group.get<position, velocity>(entity);

    // ... all components at once
    auto [pos, vel, rend] = group.get(entity);

    // ...
}
```

Or rely on the `each` member functions to iterate both entities and components
at once:

```cpp
// through a callback
registry.group<position>(entt::get<velocity>).each([](auto entity, auto &pos, auto &vel) {
    // ...
});

// using an input iterator
for(auto &&[entity, pos, vel]: registry.group<position>(entt::get<velocity>).each()) {
    // ...
}
```

Note that entities can also be excluded from the parameter list when received
through a callback and this can improve even further the performance during
iterations.<br/>
Since they aren't explicitly instantiated, empty components aren't returned in
any case.

**Note**: prefer the `get` member function of a group instead of that of a
registry during iterations to get the types iterated by the group itself.

### Full-owning groups

A full-owning group is the fastest tool an user can expect to use to iterate
multiple components at once. It iterates all the components directly, no
indirection required. This type of groups performs more or less as if users are
accessing sequentially a bunch of packed arrays of components all sorted
identically, with no jumps nor branches.

A full-owning group is created as:

```cpp
auto group = registry.group<position, velocity>();
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<position, velocity>(entt::exclude<renderable>);
```

Once created, the group gets the ownership of all the components specified in
the template parameter list and arranges their pools as needed.

Sorting owned components is no longer allowed once the group has been created.
However, full-owning groups can be sorted by means of their `sort` member
functions. Sorting a full-owning group affects all its instances.

### Partial-owning groups

A partial-owning group works similarly to a full-owning group for the components
it owns, but relies on indirection to get components owned by other groups. This
isn't as fast as a full-owning group, but it's already much faster than views
when there are only one or two free components to retrieve (the most common
cases likely). In the worst case, it's not slower than views anyway.

A partial-owning group is created as:

```cpp
auto group = registry.group<position>(entt::get<velocity>);
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<position>(entt::get<velocity>, entt::exclude<renderable>);
```

Once created, the group gets the ownership of all the components specified in
the template parameter list and arranges their pools as needed. The ownership of
the types provided via `entt::get` doesn't pass to the group instead.

Sorting owned components is no longer allowed once the group has been created.
However, partial-owning groups can be sorted by means of their `sort` member
functions. Sorting a partial-owning group affects all its instances.

### Non-owning groups

Non-owning groups are usually fast enough, for sure faster than views and well
suited for most of the cases. However, they require custom data structures to
work properly and they increase memory consumption. As a rule of thumb, users
should avoid using non-owning groups, if possible.

A non-owning group is created as:

```cpp
auto group = registry.group<>(entt::get<position, velocity>);
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<>(entt::get<position, velocity>, entt::exclude<renderable>);
```

The group doesn't receive the ownership of any type of component in this
case. This type of groups is therefore the least performing in general, but also
the only one that can be used in any situation to slightly improve performance.

Non-owning groups can be sorted by means of their `sort` member functions.
Sorting a non-owning group affects all its instances.

### Nested groups

A type of component cannot be owned by two or more conflicting groups such as:

* `registry.group<transform, sprite>()`.
* `registry.group<transform, rotation>()`.

However, the same type can be owned by groups belonging to the same _family_,
also called _nested groups_, such as:

* `registry.group<sprite, transform>()`.
* `registry.group<sprite, transform, rotation>()`.

Fortunately, these are also very common cases if not the most common ones.<br/>
It allows to increase performance on a greater number of component combinations.

Two nested groups are such that they own at least one componet type and the list
of component types involved by one of them is contained entirely in that of the
other. More specifically, this applies independently to all component lists used
to define a group.<br/>
Therefore, the rules for defining whether two or more groups are nested can be
summarized as:

* One of the groups involves one or more additional component types with respect
  to the other, whether they are owned, observed or excluded.

* The list of component types owned by the most restrictive group is the same or
  contains entirely that of the others. This also applies to the list of
  observed and excluded components.

It means that nested groups _extend_ their parents by adding more conditions in
the form of new components.

As mentioned, the components don't necessarily have to be all _owned_ so that
two groups can be considered nested. The following definitions are fully valid:

* `registry.group<sprite>(entt::get<renderable>)`.
* `registry.group<sprite, transform>(entt::get<renderable>)`.
* `registry.group<sprite, transform>(entt::get<renderable, rotation>)`.

Exclusion lists also play their part in this respect. When it comes to defining
nested groups, an excluded component type `T` is treated as being an observed
type `not_T`. Therefore, consider these two definitions:

* `registry.group<sprite, transform>()`.
* `registry.group<sprite, transform>(entt::exclude<rotation>)`.

They are treated as if users were defining the following groups:

* `group<sprite, transform>()`.
* `group<sprite, transform>(entt::get<not_rotation>)`.

Where `not_rotation` is an empty tag present only when `rotation` is not.

Because of this, to define a new group that is more restrictive than an existing
one, it's enough to take the list of component types of the latter and extend it
by adding new component types either owned, observed or excluded, without any
precautions depending on the case.<br/>
The opposite is also true. To define a _larger_ group, it will be enough to take
an existing one and remove _constraints_ from it, in whatever form they are
expressed.<br/>
Note that the greater the number of component types involved by a group, the
more restrictive it is.

Despite the extreme flexibility of nested groups which allow to independently
use component types either owned, observed or excluded, the real strength of
this tool lies in the possibility of defining a greater number of groups that
**own** the same components, thus offering the best performance in more
cases.<br/>
In fact, given a list of component types involved by a group, the greater the
number of those owned, the greater the performance of the group itself.

As a side note, it's no longer possible to sort all groups when defining nested
ones. This is because the most restrictive group shares its elements with the
less restrictive ones and ordering the latter would invalidate the former.<br/>
However, given a family of nested groups, it's still possible to sort the most
restrictive of them. To prevent users from having to remember which of their
groups is the most restrictive, the registry class offers the `sortable` member
function to know if a group can be sorted or not.

## Types: const, non-const and all in between

The `registry` class offers two overloads when it comes to constructing views
and groups: a const version and a non-const one. The former accepts only const
types as template parameters, the latter accepts both const and non-const types
instead.<br/>
It means that views and groups can be constructed from a const registry and they
propagate the constness of the registry to the types involved. As an example:

```cpp
entt::view<const position, const velocity> view = std::as_const(registry).view<const position, const velocity>();
```

Consider the following definition for a non-const view instead:

```cpp
entt::view<position, const velocity> view = registry.view<position, const velocity>();
```

In the example above, `view` can be used to access either read-only or writable
`position` components while `velocity` components are read-only in all
cases.<br/>
Similarly, these statements are all valid:

```cpp
position &pos = view.get<position>(entity);
const position &cpos = view.get<const position>(entity);
const velocity &cpos = view.get<const velocity>(entity);
std::tuple<position &, const velocity &> tup = view.get<position, const velocity>(entity);
std::tuple<const position &, const velocity &> ctup = view.get<const position, const velocity>(entity);
```

It's not possible to get non-const references to `velocity` components from the
same view instead and these will result in compilation errors:

```cpp
velocity &cpos = view.get<velocity>(entity);
std::tuple<position &, velocity &> tup = view.get<position, velocity>(entity);
std::tuple<const position &, velocity &> ctup = view.get<const position, velocity>(entity);
```

The `each` member functions also propagates constness to its _return values_:

```cpp
view.each([](auto entity, position &pos, const velocity &vel) {
    // ...
});
```

A caller can still refer to the `position` components through a const reference
because of the rules of the language that fortunately already allow it.

The same concepts apply to groups as well.

## Give me everything

Views and groups are narrow windows on the entire list of entities. They work by
filtering entities according to their components.<br/>
In some cases there may be the need to iterate all the entities still in use
regardless of their components. The registry offers a specific member function
to do that:

```cpp
registry.each([](auto entity) {
    // ...
});
```

As a rule of thumb, consider using a view or a group if the goal is to iterate
entities that have a determinate set of components. These tools are usually much
faster than combining the `each` function with a bunch of custom tests.<br/>
In all the other cases, this is the way to go. For example, it's possible to
combine `each` with the `orphan` member function to clean up orphan entities
(that is, entities that are still in use and have no assigned components):

```cpp
registry.each([&registry](auto entity) {
    if(registry.orphan(entity)) {
        registry.release(entity);
    }
});
```

In general, iterating all entities can result in poor performance. It should not
be done frequently to avoid the risk of a performance hit.<br/>
However, it can be convenient when initializing an editor or to reclaim pending
identifiers.

## What is allowed and what is not

Most of the _ECS_ available out there don't allow to create and destroy entities
and components during iterations, nor to have pointer stability.<br/>
`EnTT` partially solves the problem with a few limitations:

* Creating entities and components is allowed during iterations in most cases
  and it never invalidates already existing references.

* Deleting the current entity or removing its components is allowed during
  iterations but it could invalidate references. For all the other entities,
  destroying them or removing their iterated components isn't allowed and can
  result in undefined behavior.

* When pointer stability is enabled for the type leading the iteration, adding
  instances of the same type may or may not cause the entity involved to be
  returned. Destroying entities and components is always allowed instead, even
  if not currently iterated, without the risk of invalidating any references.

In other terms, iterators are rarely invalidated. Also, component references
aren't invalidated when a new element is added while they could be invalidated
upon destruction due to the _swap-and-pop_ policy, unless the type leading the
iteration undergoes in-place deletion.<br/>
As an example, consider the following snippet:

```cpp
registry.view<position>([&](const auto entity, auto &pos) {
    registry.emplace<position>(registry.create(), 0., 0.);
    // references remain stable after adding new instances
    pos.x = 0.;
});
```

The `each` member function won't break (because iterators remain valid) nor will
any reference be invalidated. Instead, more attention should be paid to the
destruction of entities or the removal of components.<br/>
Use a common range-for loop and get components directly from the view or move
the deletion of entities and components at the end of the function to avoid
dangling pointers.

For all types that don't offer stable pointers, iterators are also invalidated
and the behavior is undefined if an entity is modified or destroyed and it's not
the one currently returned by the iterator nor a newly created one.<br/>
To work around it, possible approaches are:

* Store aside the entities and the components to be removed and perform the
  operations at the end of the iteration.

* Mark entities and components with a proper tag component that indicates they
  must be purged, then perform a second iteration to clean them up one by one.

A notable side effect of this feature is that the number of required allocations
is further reduced in most cases.

### More performance, more constraints

Groups are a (much) faster alternative to views. However, the higher the
performance, the greater the constraints on what is allowed and what is
not.<br/>
In particular, groups add in some rare cases a limitation on the creation of
components during iterations. It happens in quite particular cases. Given the
nature and the scope of the groups, it isn't something in which it will happen
to come across probably, but it's good to know it anyway.

First of all, it must be said that creating components while iterating a group
isn't a problem at all and can be done freely as it happens with the views. The
same applies to the destruction of components and entities, for which the rules
mentioned above apply.

The additional limitation pops out instead when a given component that is owned
by a group is iterated outside of it. In this case, adding components that are
part of the group itself may invalidate the iterators. There are no further
limitations to the destruction of components and entities.<br/>
Fortunately, this isn't always true. In fact, it almost never is and this
happens only under certain conditions. In particular:

* Iterating a type of component that is part of a group with a single type view
  and adding to an entity all the components required to get it into the group
  may invalidate the iterators.

* Iterating a type of component that is part of a group with a multi type view
  and adding to an entity all the components required to get it into the group
  can invalidate the iterators, unless users specify another type of component
  to use to induce the order of iteration of the view (in this case, the former
  is treated as a free type and isn't affected by the limitation).

In other words, the limitation doesn't exist as long as a type is treated as a
free type (as an example with multi type views and partial- or non-owning
groups) or iterated with its own group, but it can occur if the type is used as
a main type to rule on an iteration.<br/>
This happens because groups own the pools of their components and organize the
data internally to maximize performance. Because of that, full consistency for
owned components is guaranteed only when they are iterated as part of their
groups or as free types with multi type views and groups in general.

# Empty type optimization

An empty type `T` is such that `std::is_empty_v<T>` returns true. They are also
the same types for which _empty base optimization_ (EBO) is possibile.<br/>
`EnTT` handles these types in a special way, optimizing both in terms of
performance and memory usage. However, this also has consequences that are worth
mentioning.

When an empty type is detected, it's not instantiated in any case. Therefore,
only the entities to which it's assigned are made available.<br/>
There doesn't exist a way to _get_ empty types from a registry, views and groups
will never return instances for them (for example, during a call to `each`) and
some functions such as `try_get` or the raw access to the list of components
aren't available for empty types. Finally, the `sort` functionality will only
accepts callbacks that require to return entities rather than components:

```cpp
registry.sort<empty_type>([](const entt::entity lhs, const entt::entity rhs) {
    return entt::registry::entity(lhs) < entt::registry::entity(rhs);
});
```

On the other hand, iterations are faster because only the entities to which the
type is assigned are considered. Moreover, less memory is used, mainly because
there doesn't exist any instance of the component, no matter how many entities
it is assigned to.

More in general, none of the features offered by the library is affected, but
for the ones that require to return actual instances.<br/>
This optimization can be disabled for the whole application by defining the
`ENTT_NO_ETO` macro. In this case, empty types will be treated like all other
types, no matter what.<br/>
Otherwise, users can specialize the `component_traits` template class and in
particular the `ignore_if_empty` alias, disabling this optimization for some
types only.

# Multithreading

In general, the entire registry isn't thread safe as it is. Thread safety isn't
something that users should want out of the box for several reasons. Just to
mention one of them: performance.<br/>
Views, groups and consequently the approach adopted by `EnTT` are the great
exception to the rule. It's true that views, groups and iterators in general
aren't thread safe by themselves. Because of this users shouldn't try to iterate
a set of components and modify the same set concurrently. However:

* As long as a thread iterates the entities that have the component `X` or
  assign and removes that component from a set of entities, another thread can
  safely do the same with components `Y` and `Z` and everything will work like a
  charm. As a trivial example, users can freely execute the rendering system and
  iterate the renderable entities while updating a physic component concurrently
  on a separate thread.

* Similarly, a single set of components can be iterated by multiple threads as
  long as the components are neither assigned nor removed in the meantime. In
  other words, a hypothetical movement system can start multiple threads, each
  of which will access the components that carry information about velocity and
  position for its entities.

This kind of entity-component systems can be used in single threaded
applications as well as along with async stuff or multiple threads. Moreover,
typical thread based models for _ECS_ don't require a fully thread safe registry
to work. Actually, users can reach the goal with the registry as it is while
working with most of the common models.

Because of the few reasons mentioned above and many others not mentioned, users
are completely responsible for synchronization whether required. On the other
hand, they could get away with it without having to resort to particular
expedients.

Finally, `EnTT` can be configured via a few compile-time definitions to make
some of its parts implicitly thread-safe, roughly speaking only the ones that
really make sense and can't be turned around.<br/>
In particular, when multiple instances of objects referencing the type index
generator (such as the `registry` class) are used in different threads, then it
might be useful to define `ENTT_USE_ATOMIC`.<br/>
See the relevant documentation for more information.

## Iterators

A special mention is needed for the iterators returned by views and groups. Most
of the times they meet the requirements of random access iterators, in all cases
they meet at least the requirements of forward iterators.<br/>
In other terms, they are suitable for use with the parallel algorithms of the
standard library. If it's not clear, this is a great thing.

As an example, this kind of iterators can be used in combination with
`std::for_each` and `std::execution::par` to parallelize the visit and therefore
the update of the components returned by a view or a group, as long as the
constraints previously discussed are respected:

```cpp
auto view = registry.view<position, const velocity>();

std::for_each(std::execution::par_unseq, view.begin(), view.end(), [&view](auto entity) {
    // ...
});
```

This can increase the throughput considerably, even without resorting to who
knows what artifacts that are difficult to maintain over time.

Unfortunately, because of the limitations of the current revision of the
standard, the parallel `std::for_each` accepts only forward iterators. This
means that the default iterators provided by the library cannot return proxy
objects as references and **must** return actual reference types instead.<br/>
This may change in the future and the iterators will almost certainly return
both the entities and a list of references to their components by default sooner
or later. Multi-pass guarantee won't break in any case and the performance
should even benefit from it further.

## Const registry

A const registry is also fully thread safe. This means that it won't be able to
lazily initialize a missing storage when a view is generated.<br/>
The reason for this is easy to explain. To avoid requiring types to be
_announced_ in advance, a registry lazily creates the storage objects for the
different components. However, this isn't possible for a thread safe const
registry.<br/>
On the other side, all pools must necessarily _exist_ when creating a view.
Therefore, static _placeholders_ for missing storage are used to fill the gap.

Note that returned views are always valid and behave as expected in the context
of the caller. The only difference is that static _placeholders_ (if any) are
never renewed.<br/>
As a result, a view created from a const registry may behave incorrectly over
time if it's kept for a second use.<br/>
Therefore, if the general advice is to create views when necessary and discard
them immediately afterwards, this becomes almost a rule when it comes to views
generated from a const registry.

Fortunately, there is also a way to instantiate storage classes early when in
doubt or when there are special requirements.<br/>
Calling the `prepare` method is equivalent to _announcing_ the existence of a
particular storage, to avoid running into problems. For those interested, there
are also alternative approaches, such as a single threaded tick for the registry
warm-up, but these are not always applicable.<br/>
In this case, no placeholders will be used since all storage exist. In other
words, views never risk becoming _invalid_.

# Beyond this document

There are many other features and functions not listed in this document.<br/>
`EnTT` and in particular its ECS part is in continuous development and some
things could be forgotten, others could have been omitted on purpose to reduce
the size of this file. Unfortunately, some parts may even be outdated and still
to be updated.

For further information, it's recommended to refer to the documentation included
in the code itself or join the official channels to ask a question.
