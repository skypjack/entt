# Crash Course: entity-component system

# Table of Contents

* [Introduction](#introduction)
* [Design decisions](#design-decisions)
  * [Type-less and bitset-free](#type-less-and-bitset-free)
  * [Build your own](#build-your-own)
  * [Pay per use](#pay-per-use)
  * [All or nothing](#all-or-nothing)
* [Vademecum](#vademecum)
* [The Registry, the Entity and the Component](#the-registry-the-entity-and-the-component)
  * [Observe changes](#observe-changes)
    * [Auto-binding](#auto-binding)
    * [Entity lifecycle](#entity-lifecycle)
    * [Listeners disconnection](#listeners-disconnection)
  * [They call me reactive storage](#they-call-me-reactive-storage)
  * [Sorting: is it possible?](#sorting-is-it-possible)
  * [Helpers](#helpers)
    * [Null entity](#null-entity)
    * [Tombstone](#tombstone)
    * [To entity](#to-entity)
    * [Dependencies](#dependencies)
    * [Invoke](#invoke)
    * [Connection helper](#connection-helper)
    * [Handle](#handle)
    * [Organizer](#organizer)
  * [Context variables](#context-variables)
    * [Aliased properties](#aliased-properties)
  * [Snapshot: complete vs continuous](#snapshot-complete-vs-continuous)
    * [Snapshot loader](#snapshot-loader)
    * [Continuous loader](#continuous-loader)
    * [Archives](#archives)
    * [One example to rule them all](#one-example-to-rule-them-all)
* [Storage](#storage)
  * [Component traits](#component-traits)
  * [Empty type optimization](#empty-type-optimization)
  * [Void storage](#void-storage)
  * [Entity storage](#entity-storage)
    * [Reserved identifiers](#reserved-identifiers)
    * [One of a kind to the registry](#one-of-a-kind-to-the-registry)
  * [Pointer stability](#pointer-stability)
    * [In-place delete](#in-place-delete)
    * [Hierarchies and the like](#hierarchies-and-the-like)
* [Meet the runtime](#meet-the-runtime)
  * [A base class to rule them all](#a-base-class-to-rule-them-all)
  * [Beam me up, registry](#beam-me-up-registry)
* [Views and Groups](#views-and-groups)
  * [Views](#views)
    * [Create once, reuse many times](#create-once-reuse-many-times)
    * [Exclude-only](#exclude-only)
    * [View pack](#view-pack)
    * [Iteration order](#iteration-order)
    * [Runtime views](#runtime-views)
  * [Groups](#groups)
    * [Full-owning groups](#full-owning-groups)
    * [Partial-owning groups](#partial-owning-groups)
    * [Non-owning groups](#non-owning-groups)
  * [Types: const, non-const and all in between](#types-const-non-const-and-all-in-between)
  * [Give me everything](#give-me-everything)
  * [What is allowed and what is not](#what-is-allowed-and-what-is-not)
    * [More performance, more constraints](#more-performance-more-constraints)
* [Multithreading](#multithreading)
  * [Iterators](#iterators)
  * [Const registry](#const-registry)
* [Beyond this document](#beyond-this-document)

# Introduction

`EnTT` offers a header-only, tiny and easy to use entity-component system module
written in modern C++.<br/>
The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development.

# Design decisions

## Type-less and bitset-free

The library implements a sparse set based model that does not require users to
specify the set of components neither at compile-time nor at runtime.<br/>
This is why users can instantiate the core class simply like:

```cpp
entt::registry registry;
```

In place of its more annoying and error-prone counterpart:

```cpp
entt::registry<comp_0, comp_1, ..., comp_n> registry;
```

Furthermore, it is not necessary to announce the existence of a component type.
When the time comes, just use it and that is all.

## Build your own

The ECS module (as well as the rest of the library) is designed as a set of
containers that are used as needed, just like a vector or any other container.
It does not attempt in any way to take over on the user codebase, nor to control
its main loop or process scheduling.<br/>
Unlike other more or less well known models, it also makes use of independent
pools that are extended via _static mixins_. The built-in signal support is an
example of this flexible design: defined as a mixin, it is easily disabled if
not needed. Similarly, the storage class has a specialization that shows how
everything is customizable down to the smallest detail.

## Pay per use

Everything is designed around the principle that users only have to pay for what
they want.

When it comes to using an entity-component system, the tradeoff is usually
between performance and memory usage. The faster it is, the more memory it uses.
Even worse, some approaches tend to heavily affect other functionalities like
the construction and destruction of components to favor iterations, even when it
is not strictly required. In fact, slightly worse performance along non-critical
paths are the right price to pay to reduce memory usage and have overall better
performance.<br/>
`EnTT` follows a completely different approach. It gets the best out from the
basic data structures and gives users the possibility to pay more for higher
performance where needed.

## All or nothing

As a rule of thumb, a `T **` pointer (or whatever a custom pool returns) is
always available to directly access all the instances of a given component type
`T`.<br/>
This is one of the corner stones of the library. Many of the tools offered are
designed around this need and give the possibility to get this information.

# Vademecum

The `entt::entity` type implements the concept of _entity identifier_. An entity
(the _E_ of an _ECS_) is an opaque element to use as-is. Inspecting it is not
recommended since its format can change in future.<br/>
Components (the _C_ of an _ECS_) are of any type, without any constraints, not
even that of being movable. No need to register them nor their types.<br/>
Systems (the _S_ of an _ECS_) are plain functions, functors, lambdas and so on.
It is not required to announce them in any case and have no requirements.

The next sections go into detail on how to use the entity-component system part
of the `EnTT` library.<br/>
This module is likely larger than what is described below. For more details,
please refer to the inline documentation.

# The Registry, the Entity and the Component

A registry stores and manages entities (or _identifiers_) and components.<br/>
The class template `basic_registry` lets users decide what the preferred type to
represent an entity is. Because `std::uint32_t` is large enough for almost any
case, there also exists the enum class `entt::entity` that _wraps_ it and the
alias `entt::registry` for `entt::basic_registry<entt::entity>`.

Entities are represented by _entity identifiers_. An entity identifier contains
information about the entity itself and its version.<br/>
User defined identifiers are allowed as enum classes and class types that define
an `entity_type` member of type `std::uint32_t` or `std::uint64_t`.

A registry is used both to construct and to destroy entities:

```cpp
// constructs a naked entity with no components and returns its identifier
auto entity = registry.create();

// destroys an entity and all its components
registry.destroy(entity);
```

The `create` member function also accepts a hint. Moreover, it has an overload
that gets two iterators to use to generate many entities at once efficiently.
Similarly, the `destroy` member function also works with a range of entities:

```cpp
// destroys all the entities in a range
auto view = registry.view<a_component, another_component>();
registry.destroy(view.begin(), view.end());
```

In addition to offering an overload to force the version upon destruction.<br/>
This function removes all components from an entity before releasing it. There
also exists a _lighter_ alternative that does not query component pools, for use
with orphaned entities:

```cpp
// releases an orphaned identifier
registry.release(entity);
```

As with the `destroy` function, also in this case entity ranges are supported
and it is possible to force a _version_.

In both cases, when an identifier is released, the registry can freely reuse it
internally. In particular, the version of an entity is increased (unless the
overload that forces a version is used instead of the default one).<br/>
Users can then _test_ identifiers by means of a registry:

```cpp
// returns true if the entity is still valid, false otherwise
bool b = registry.valid(entity);

// gets the actual version for the given entity
auto curr = registry.current(entity);
```

Or _inspect_ them using some functions meant for parsing an identifier as-is,
such as:

```cpp
// gets the version contained in the entity identifier
auto version = entt::to_version(entity);
```

Components are assigned to or removed from entities at any time.<br/>
The `emplace` member function template creates, initializes and assigns to an
entity the given component. It accepts a variable number of arguments to use to
construct the component itself:

```cpp
registry.emplace<position>(entity, 0., 0.);

// ...

auto &vel = registry.emplace<velocity>(entity);
vel.dx = 0.;
vel.dy = 0.;
```

The default storage _detects_ aggregate types internally and exploits aggregate
initialization when possible.<br/>
Therefore, it is not strictly necessary to define a constructor for each type.

The `insert` member function works with _ranges_ and is used to:

* Assign the same component to all entities at once when a type is specified as
  a template parameter or an instance is passed as an argument:

  ```cpp
  // default initialized type assigned by copy to all entities
  registry.insert<position>(first, last);

  // user-defined instance assigned by copy to all entities
  registry.insert(from, to, position{0., 0.});
  ```

* Assign a set of components to the entities when a range is provided (the
  length of the range of components **must** be the same of that of entities):

  ```cpp
  // first and last specify the range of entities, instances points to the first element of the range of components
  registry.insert<position>(first, last, instances);
  ```

If an entity already has the given component, the `replace` and `patch` member
function templates are used to update it:

```cpp
// replaces the component in-place
registry.patch<position>(entity, [](auto &pos) { pos.x = pos.y = 0.; });

// constructs a new instance from a list of arguments and replaces the component
registry.replace<position>(entity, 0., 0.);
```

When it is unknown whether an entity already owns an instance of a component,
`emplace_or_replace` is the function to use instead:

```cpp
registry.emplace_or_replace<position>(entity, 0., 0.);
```

This is a slightly faster alternative to the following snippet:

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
function instead. It behaves similarly to `erase` but it drops the component if
and only if it exists, otherwise it returns safely to the caller:

```cpp
registry.remove<position>(entity);
```

The `clear` member function works similarly and is used to either:

* Erases all instances of the given components from the entities that own them:

  ```cpp
  registry.clear<position>();
  ```

* Or destroy all entities in a registry at once:

  ```cpp
  registry.clear();
  ```

Finally, references to components are obtained simply as:

```cpp
const auto &cregistry = registry;

// const and non-const reference
const auto &crenderable = cregistry.get<renderable>(entity);
auto &renderable = registry.get<renderable>(entity);

// const and non-const references
const auto [cpos, cvel] = cregistry.get<position, velocity>(entity);
auto [pos, vel] = registry.get<position, velocity>(entity);
```

If the existence of the component is not certain, `try_get` is the more suitable
function instead.

## Observe changes

By default, each storage comes with a mixin that adds signal support to it.<br/>
This allows for fancy things like dependencies and reactive systems.

The `on_construct` member function returns a _sink_ (which is an object for
connecting and disconnecting listeners) for those interested in notifications
when a new instance of a given component type is created:

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

Similarly, `on_destroy` and `on_update` are used to receive notifications about
the destruction and update of an instance, respectively.<br/>
Because of how C++ works, listeners attached to `on_update` are only invoked
following a call to `replace`, `emplace_or_replace` or `patch`.

Runtime pools are also supported by providing an identifier to the functions
above:

```cpp
registry.on_construct<position>("other"_hs).connect<&my_free_function>();
```

Refer to the following sections for more information about runtime pools.<br/>
In all cases, the function type of a listener is equivalent to the following:

```cpp
void(entt::registry &, entt::entity);
```

All listeners are provided with the registry that triggered the notification and
the involved entity. Note also that:

* Listeners for construction signals are invoked **after** components have been
  created.

* Listeners designed to observe changes are invoked **after** components have
  been updated.

* Listeners for destruction signals are invoked **before** components have been
  destroyed.

There are also some limitations on what a listener can and cannot do:

* Connecting and disconnecting other functions from within the body of a
  listener should be avoided. It can lead to undefined behavior in some cases.

* Removing the component from within the body of a listener that observes the
  construction or update of instances of a given type is not allowed.

* Assigning and removing components from within the body of a listener that
  observes the destruction of instances of a given type should be avoided. It
  can lead to undefined behavior in some cases. This type of listeners is
  intended to provide users with an easy way to perform cleanup and nothing
  more.

Please, refer to the documentation of the signal class to know about all the
features it offers.<br/>
There are many useful but less known functionalities that are not described
here, such as the connection objects or the possibility to attach listeners with
a list of parameters that is shorter than that of the signal itself.

### Auto-binding

Users don't need to create bindings manually each and every time. For managed
types, they can have `EnTT` setup listeners automatically.<br/>
The library searches the types for functions with specific names and signatures,
as in the following example:

```cpp
struct my_type {
    static void on_construct(entt::registry &registry, const entt::entity entt);
    static void on_update(entt::registry &registry, const entt::entity entt);
    static void on_destroy(entt::registry &registry, const entt::entity entt);

    // ...
};
```

As soon as a storage is created for such a defined type, these functions are
associated with the respective signals. The function name is self-explanatory of
the target signal.

### Entity lifecycle

Observing entities is also possible. In this case, the user must use the entity
type instead of the component type:

```cpp
registry.on_construct<entt::entity>().connect<&my_listener>();
```

Since entity storage is unique within a registry, if a _name_ is provided it is
ignored and therefore discarded.<br/>
As for the function signature, this is exactly the same as the components.

Entities support all types of signals: construct, destroy, and update. The
latter is perhaps ambiguous as an entity is not truly _updated_. Rather, its
identifier is created and finally released.<br/>
Indeed, the update signal is meant to send _general notifications_ regarding an
entity. It can be triggered via the `patch` function, as is the case with
components:

```cpp
registry.patch<entt::entity>(entity);
```

Destroying an entity and then updating the version of an identifier **does not**
give rise to these types of signals under any circumstances instead.<br/>
Finally, note that listeners that _observe_ the destruction of an entity are
invoked **after** all components have been removed, not **before**. This is
because the entity would be invalidated before deleting its elements otherwise,
making it difficult for the user to write component listeners.

### Listeners disconnection

The destruction order of the storage classes and therefore the disconnection of
the listeners is completely random.<br/>
There are no guarantees today, and while the logic is easily discerned, it is
not guaranteed that it will remain so in the future.

For example, a listener getting disconnected after a component is discarded as a
result of pool destruction is most likely a recipe for problems.<br/>
Rather, it is advisable to invoke the `clear` function of the registry before
destroying it. This forces the deletion of all components and entities without
ever discarding the pools.<br/>
As a result, a listener that wants to access components, entities, or pools can
safely do so against a still valid registry, while checking for the existence of
the various elements as appropriate.

## They call me reactive storage

Signals are the basic tools to construct reactive systems, even if they are not
enough on their own. `EnTT` tries to take another step in that direction with
its _reactive mixin_.<br/>
In order to explain what reactive systems are, this is a slightly revised quote
from the documentation of the library that first introduced this tool,
[Entitas](https://github.com/sschmid/Entitas-CSharp):

> Imagine you have 100 fighting units on the battlefield, but only 10 of them
> changed their positions. Instead of using a normal system and updating all 100
> entities depending on the position, you can use a reactive system which will
> only update the 10 changed units. So efficient.

In `EnTT`, this means iterating over a reduced set of entities and components
than what would otherwise be returned from a view or group.<br/>
On these words, however, the similarities with the proposal of `Entitas` also
end. The rules of the language and the design of the library obviously impose
and allow different things.

A reactive mixin can be used on a standalone storage with any value type
(perhaps using an alias to simplify its use):

```cpp
using reactive_storage = entt::reactive_mixin<entt::storage<void>>;

entt::registry registry{};
reactive_storage storage{};

storage.bind(registry);
```

In this case, it must be provided with a reference registry for subsequent
operations.<br/>
Alternatively, when using the value type provided by `EnTT`, it is also possible
to create a reactive storage directly inside a registry:

```cpp
entt::registry registry{};
auto &storage = registry.storage<entt::reactive>("observer"_hs);
```

In the latter case, there is the advantage that, in the event of destruction of
an entity, this storage is also automatically cleaned up.<br/>
Also note that, unlike all other storage, these classes do not support signals
by default (although they can be enabled if necessary).

Once it has been created and associated with a registry, the reactive mixin
needs to be informed about what it should _observe_.<br/>
Here the choice boils down to three main events affecting all elements (entities
or components), namely creation, update or destruction:

```cpp
storage
    // observe position component construction
    .on_construct<position>()
    // observe velocity component update
    .on_update<velocity>()
    // observe renderable component destruction
    .on_destroy<renderable>();
```

It goes without saying that it is possible to observe multiple events of the
same type or of different types with the same storage.<br/>
For example, to know which entities have been assigned or updated a component of
a certain type:

```cpp
storage
    .on_construct<my_type>()
    .on_update<my_type>();
```

Note that all configurations are in _or_ and never in _and_. Therefore, to track
entities that have been assigned two different components, there are a couple of
options:

* Create two reactive storage, then combine them in a view:

  ```cpp
  first_storage.on_construct<position>();
  second_storage.on_construct<velocity>();

  for(auto entity: entt::basic_view{first_storage, second_storage}) {
      // ...
  }
  ```

* Use a reactive storage with a non-`void` value type and a custom tracking
  function for the purpose:

  ```cpp
  using my_reactive_storage = entt::reactive_mixin<entt::storage<bool>>;

  void callback(my_reactive_storage &storage, const entt::registry &, const entt::entity entity) {
      storage.contains(entity) ? (storage.get(entity) = true) : storage.emplace(entity, false);
  }

  // ...

  my_reactive_storage storage{};
  storage
      .on_construct<position, &callback>()
      .on_construct<velocity, &callback>();

  // ...

  for(auto [entity, both_were_added]: storage.each()) {
      if(both_were_added) {
          // ...
      }
  }
  ```

As highlighted in the last example, the reactive mixin tracks the entities that
match the given conditions and saves them aside. However, this behavior can be
changed.<br/>
For example, it is possible to _capture_ all and only the entities for which a
certain component has been updated but only if a specific value is within a
given range:

```cpp
void callback(reactive_storage &storage, const entt::registry &registry, const entt::entity entity) {
    storage.remove(entity);

    if(const auto x = registry.get<position>(entity).x; x >= min_x && x <= max_x) {
        storage.emplace(entity);
    }
}

// ...

storage.on_update<position, &callback>();
```

This makes reactive storage extremely flexible and usable in a large number of
cases.<br/>
Finally, once the entities of interest have been collected, it is possible to
_visit_ the storage like any other:

```cpp
for(auto entity: storage) {
    // ...
}
```

Wrapping it in a view and combining it with other views is another option:

```cpp
for(auto [entity, pos]: (entt:.basic_view{storage} | registry.view<position>(entt::exclude<velocity>)).each()) {
    // ...
}
```

In order to simplify this last use case, the reactive mixin also provides a
specific function that returns a view of the storage already filtered according
to the provided requirements:

```cpp
for(auto [entity, pos]: storage.view<position>(entt::exclude<velocity>).each()) {
    // ...
}
```

The registry used in this case is the one associated with the storage and also
available via the `registry` function.

It should be noted that a reactive storage never deletes its entities (and
elements, if any). To process and then discard entities at regular intervals,
refer to the `clear` function available by default for each storage type.<br/>
Similarly, the reactive mixin does not disconnect itself from observed storages
upon destruction. Therefore, users have to do this themselves:

```cpp
entt::registry = storage.registry();

registry.on_construct<position>().disconnect(&storage);
registry.on_construct<velocity>().disconnect(&storage);
```

Destroying a reactive storage without disconnecting it from observed pools will
result in undefined behavior.

## Sorting: is it possible?

Sorting entities and components is possible using an in-place algorithm that
does not require memory allocations and is therefore quite convenient.<br/>
There are two functions that respond to slightly different needs:

* Components are sorted either directly:

  ```cpp
  registry.sort<renderable>([](const renderable &lhs, const renderable &rhs) {
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
  when the usage pattern is known.

* Components are sorted according to the order imposed by another component:

  ```cpp
  registry.sort<movement, physics>();
  ```

  In this case, instances of `movement` are arranged in memory so that cache
  misses are minimized when the two components are iterated together.

As a side note, the use of groups limits the possibility of sorting pools of
components. Refer to the specific documentation for more details.

## Helpers

The so-called _helpers_ are small classes and functions mainly designed to offer
built-in support for the most basic functionalities.

### Null entity

The `entt::null` variable models the concept of a _null entity_.<br/>
The library guarantees that the following expression always returns false:

```cpp
registry.valid(entt::null);
```

A registry rejects the null entity in all cases because it is not considered
valid. It also means that the null entity cannot own components.<br/>
The type of the null entity is internal and should not be used for any purpose
other than defining the null entity itself. However, there exist implicit
conversions from the null entity to identifiers of any allowed type:

```cpp
entt::entity null = entt::null;
```

Similarly, the null entity compares to any other identifier:

```cpp
const auto entity = registry.create();
const bool null = (entity == entt::null);
```

As for its integral form, the null entity only affects the entity part of an
identifier and is instead completely transparent to its version.

Be aware that `entt::null` and entity 0 are different. Likewise, a zero
initialized entity is not the same as `entt::null`. Therefore, although
`entt::entity{}` is in some sense an alias for entity 0, none of them are used
to create a null entity.

### Tombstone

Similar to the null entity, the `entt::tombstone` variable models the concept of
a _tombstone_.<br/>
Once created, the integral form of the two values is the same, although they
affect different parts of an identifier. In fact, the tombstone only uses the
version part of it and is completely transparent to the entity part.

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

Similarly, the tombstone compares to any other identifier:

```cpp
const auto entity = registry.create();
const bool tombstone = (entity == entt::tombstone);
```

Be aware that `entt::tombstone` and entity 0 are different. Likewise, a zero
initialized entity is not the same as `entt::tombstone`. Therefore, although
`entt::entity{}` is in some sense an alias for entity 0, none of them are used
to create tombstones.

### To entity

This function accepts a storage and an instance of a component of the storage
type, then it returns the entity associated with the latter:

```cpp
const auto entity = entt::to_entity(registry.storage<position>(), instance);
```

Where `instance` is a component of type `position`. A null entity is returned in
case the instance does not belong to the registry.

### Dependencies

The `registry` class is designed to create short circuits between its member
functions. This greatly simplifies the definition of a _dependency_.<br/>
For example, the following adds (or replaces) the component `a_type` whenever
`my_type` is assigned to an entity:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::emplace_or_replace<a_type>>();
```

Similarly, the code below removes `a_type` from an entity whenever `my_type` is
assigned to it:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::remove<a_type>>();
```

A dependency is easily _broken_ as follows:

```cpp
registry.on_construct<my_type>().disconnect<&entt::registry::emplace_or_replace<a_type>>();
```

There are many other types of _dependencies_. In general, most of the functions
that accept an entity as their first argument are good candidates for this
purpose.

### Invoke

The `invoke` helper allows to _propagate_ a signal to a member function of a
component without having to _extend_ it:

```cpp
registry.on_construct<clazz>().connect<entt::invoke<&clazz::func>>();
```

All it does is pick up the _right_ component for the received entity and invoke
the requested method, passing on the arguments if necessary.

### Connection helper

Connecting signals can quickly become cumbersome.<br/>
This utility aims to simplify the process by grouping the calls:

```cpp
entt::sigh_helper{registry}
    .with<position>()
        .on_construct<&a_listener>()
        .on_destroy<&another_listener>()
    .with<velocity>("other"_hs)
        .on_update<yet_another_listener>();
```

Runtime pools are also supported by providing an identifier when calling `with`,
as shown in the previous snippet. Refer to the following sections for more
information about runtime pools.<br/>
Obviously, this helper does not make the code disappear but it should at least
reduce the boilerplate in the most complex cases.

### Handle

A handle is a thin wrapper around an entity and a registry. It _replicates_ the
API of a registry by offering functions such as `get` or `emplace`. The
difference being that the entity is implicitly passed to the registry.<br/>
It is default constructible as an invalid handle that contains a null registry
and a null entity. When it contains a null registry, calling functions that
delegate execution to the registry causes undefined behavior. It is recommended
to test for validity with its implicit cast to `bool` if in doubt.<br/>
A handle is also non-owning, meaning that it is freely copied and moved around
without affecting its entity (in fact, handles happen to be trivially copyable).
An implication of this is that mutability becomes part of the type.

There are two aliases that use `entt::entity` as their default entity:
`entt::handle` and `entt::const_handle`.<br/>
Users can also easily create their own aliases for custom identifiers as:

```cpp
using my_handle = entt::basic_handle<entt::basic_registry<my_identifier>>;
using my_const_handle = entt::basic_handle<const entt::basic_registry<my_identifier>>;
```

Non-const handles are also implicitly convertible to const handles out of the
box but not the other way around.

This class is intended to simplify function signatures. In case of functions
that take a registry and an entity and do most of their work on that entity,
users might want to consider using handles, either const or non-const.

### Organizer

The `organizer` class template offers support for creating an execution graph
from a set of functions and their requirements on resources.<br/>
The resulting tasks are not executed in any case. This is not the goal of this
tool. Instead, they are returned to the user in the form of a graph that allows
for safe execution.

All functions are added in order of execution to the organizer:

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

These are the parameters that a free function or a member function can accept:

* A possibly constant reference to a registry.
* An `entt::basic_view` with any possible combination of storage classes.
* A possibly constant reference to any type `T` (that is, a context variable).

The function type for free functions and decayed lambdas passed as parameters to
`emplace` is `void(const void *, entt::registry &)` instead. The first parameter
is an optional pointer to user defined data to provide upon registration:

```cpp
clazz instance;
organizer.emplace(+[](const void *, entt::registry &) { /* ... */ }, &instance);
```

In all cases, it is also possible to associate a name with the task when
creating it. For example:

```cpp
organizer.emplace<&free_function>("func");
```

When a function is registered with the organizer, everything it accesses is
considered a _resource_ (views are _unpacked_ and their types are treated as
resources). The _constness_ of a type also dictates its access mode (RO/RW). In
turn, this affects the resulting graph, since it influences the possibility of
launching tasks in parallel.<br/>
As for the registry, if a function does not explicitly request it or requires a
constant reference to it, it is considered a read-only access. Otherwise, it is
considered as read-write access. All functions have the registry among their
resources.

When registering a function, users can also require resources that are not in
the list of parameters of the function itself. These are declared as template
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
as not constant, it is treated as constant as regards the generation of the task
graph.

To generate the task graph, the organizer offers the `graph` member function:

```cpp
std::vector<entt::organizer::vertex> graph = organizer.graph();
```

A graph is returned in the form of an adjacency list. Each vertex offers the
following features:

* `ro_count` and `rw_count`: the number of resources accessed in read-only or
  read-write mode.

* `ro_dependency` and `rw_dependency`: type info objects associated with the
  parameters of the underlying function.

* `top_level`: true if a node is a top level one (it has no entering edges),
  false otherwise.

* `info`: type info object associated with the underlying function.

* `name`: the name associated with the given vertex if any, a null pointer
  otherwise.

* `callback`: a pointer to the function to execute and whose function type is
  `void(const void *, entt::registry &)`.

* `data`: optional data to provide to the callback.

* `children`: the vertices reachable from the given node, in the form of indices
  within the adjacency list.

Since the creation of pools and resources within the registry is not necessarily
thread safe, each vertex also offers a `prepare` function which is used to setup
a registry for execution with the created graph:

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

Each registry has a _context_ associated with it, which is an `any` object map
accessible by both type and _name_ for convenience. The _name_ is not really a
name though. In fact, it is a numeric id of type `id_type` used as a key for the
variable. Any value is accepted, even runtime ones.<br/>
The context is returned via the `ctx` functions and offers a minimal set of
features including the following:

```cpp
// creates a new context variable by type and returns it
registry.ctx().emplace<my_type>(42, 'c');

// creates a new named context variable by type and returns it
registry.ctx().emplace_as<my_type>("my_variable"_hs, 42, 'c');

// inserts or assigns a context variable by (deduced) type and returns it
registry.ctx().insert_or_assign(my_type{42, 'c'});

// inserts or assigns a named context variable by (deduced) type and returns it
registry.ctx().insert_or_assign("my_variable"_hs, my_type{42, 'c'});

// gets the context variable by type as a non-const reference from a non-const registry
auto &var = registry.ctx().get<my_type>();

// gets the context variable by name as a const reference from either a const or a non-const registry
const auto &cvar = registry.ctx().get<const my_type>("my_variable"_hs);

// resets the context variable by type
registry.ctx().erase<my_type>();

// resets the context variable associated with the given name
registry.ctx().erase<my_type>("my_variable"_hs);
```

A context variable must be both default constructible and movable. If the
supplied type does not match that of the variable when using a _name_, the
operation fails.<br/>
For all users who want to use the context but do not want to create elements,
the `contains` and `find` functions are also available:

```cpp
const bool contains = registry.ctx().contains<my_type>();
const my_type *value = registry.ctx().find<const my_type>("my_variable"_hs);
```

Also in this case, both functions support constant types and accept a _name_ for
the variable to look up, as does `at`.

### Aliased properties

A context also supports creating _aliases_ for existing variables that are not
directly managed by the registry. Const and therefore read-only variables are
also accepted.<br/>
To do that, the type used upon construction must be a reference type and an
lvalue is necessarily provided as an argument:

```cpp
time clock;
registry.ctx().emplace<time &>(clock);
```

Read-only aliased properties are created using const types instead:

```cpp
registry.ctx().emplace<const time &>(clock);
```

Note that `insert_or_assign` does not support aliased properties and users must
necessarily use `emplace` or `emplace_as` for this purpose.<br/>
When `insert_or_assign` is used to update an aliased property, it _converts_
the property itself into a non-aliased one.

From the point of view of the user, there are no differences between a variable
that is managed by the registry and an aliased property. However, read-only
variables are not accessible as non-const references:

```cpp
// read-only variables only support const access
const my_type *ptr = registry.ctx().find<const my_type>();
const my_type &var = registry.ctx().get<const my_type>();
```

Aliased properties are erased as it happens with any other variable. Similarly,
it is also possible to assign them a _name_.

## Snapshot: complete vs continuous

This module comes with bare minimum support for serialization.<br/>
It does not convert components to bytes directly, there was not the need of
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
    .get<entt::entity>(output)
    .get<a_component>(output)
    .get<another_component>(output);
```

It is not necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal.

When _getting_ an entity type, the snapshot class serializes all entities along
with their versions.<br/>
In all other cases, entities and components from a given storage are passed to
the archive. Named pools are also supported:

```cpp
entt::snapshot{registry}.get<a_component>(output, "other"_hs);
```

There exists another version of the `get` member function that accepts a range
of entities to serialize. It can be used to _filter_ out those entities that
should not be serialized for some reasons:

```cpp
const auto view = registry.view<serialize>();
output_archive output;

entt::snapshot{registry}
    .get<a_component>(output, view.begin(), view.end())
    .get<another_component>(output, view.begin(), view.end());
```

Once a snapshot is created, there exist mainly two _ways_ to load it: as a whole
and in a kind of _continuous mode_.<br/>
The following sections describe both loaders and archives in details.

### Snapshot loader

A snapshot loader requires that the destination registry be empty. It loads all
the data at once while keeping intact the identifiers that the entities
originally had:

```cpp
input_archive input;

entt::snapshot_loader{registry}
    .get<entt::entity>(input)
    .get<a_component>(input)
    .get<another_component>(input)
    .orphans();
```

It is not necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal.<br/>
For obvious reasons, what is important is that the data are restored in exactly
the same order in which they were serialized.

When _getting_ an entity type, a snapshot loader restores all entities with the
versions that they originally had at the source.<br/>
In all other cases, entities and components are restored in a given storage. If
the registry does not contain the entity, it is also created accordingly. As for
the snapshot class, named pools are supported too:

```cpp
entt::snapshot_loader{registry}.get<a_component>(input, "other"_hs);
```

Finally, the `orphans` member function releases the entities that have no
components after a restore, if any.

### Continuous loader

A continuous loader is designed to load data from a source registry to a
(possibly) non-empty destination. The loader accommodates in a registry more
than one snapshot in a sort of _continuous loading_ that updates the destination
one step at a time.<br/>
Identifiers that entities originally had are not transferred to the target.
Instead, the loader maps remote identifiers to local ones while restoring a
snapshot. Wrapping the archive is a conveninent way of updating identifiers that
are part of components automatically (see the example below).<br/>
Another difference with the snapshot loader is that the continuous loader has an
internal state that must persist over time. Therefore, there is no reason to
limit its lifetime to that of a temporary object:

```cpp
entt::continuous_loader loader{registry};
input_archive input;

auto archive = [&loader, &input](auto &value) {
    input(value);

    if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>, dirty_component>) {
        value.parent = loader.map(value.parent);
        value.child = loader.map(value.child);
    }
};

loader
    .get<entt::entity>(input)
    .get<a_component>(input)
    .get<another_component>(input)
    .get<dirty_component>(input)
    .orphans();
```

It is not necessary to invoke all functions each and every time. What functions
to use in which case mostly depends on the goal.<br/>
For obvious reasons, what is important is that the data are restored in exactly
the same order in which they were serialized.

When _getting_ an entity type, a loader restores groups of entities and maps
each entity to a local counterpart when required. For each remote identifier not
yet registered by the loader, a local identifier is created so as to keep the
local entity in sync with the remote one.<br/>
In all other cases, entities and components are restored in a given storage. If
the registry does not contain the entity, it is also tracked accordingly. As for
the snapshot class, named pools are supported too:

```cpp
loader.get<a_component>(input, "other"_hs);
```

Finally, the `orphans` member function releases the entities that have no
components after a restore, if any.

### Archives

Archives must publicly expose a predefined set of member functions. The API is
straightforward and consists only of a group of function call operators that
are invoked by the snapshot class and the loaders.

In particular:

* An output archive (the one used when creating a snapshot) exposes a function
  call operator with the following signature to store entities:

  ```cpp
  void operator()(entt::entity);
  ```

  Where `entt::entity` is the type of the entities used by the registry.<br/>
  Note that all member functions of the snapshot class also make an initial call
  to store aside the _size_ of the set they are going to store. In this case,
  the expected function type for the function call operator is:

  ```cpp
  void operator()(std::underlying_type_t<entt::entity>);
  ```

  In addition, an archive accepts (const) references to the types of component
  to serialize. Therefore, given a type `T`, the archive offers a function call
  operator with the following signature:

  ```cpp
  void operator()(const T &);
  ```

  The output archive can freely decide how to serialize the data. The registry
  is not affected at all by the decision.

* An input archive (the one used when restoring a snapshot) exposes a function
  call operator with the following signature to load entities:

  ```cpp
  void operator()(entt::entity &);
  ```

  Where `entt::entity` is the type of the entities used by the registry. Each
  time the function is invoked, the archive reads the next element from the
  underlying storage and copies it in the given variable.<br/>
  All member functions of a loader class also make an initial call to read the
  _size_ of the set they are going to load. In this case, the expected function
  type for the function call operator is:

  ```cpp
  void operator()(std::underlying_type_t<entt::entity> &);
  ```

  In addition, an archive accepts references to the types of component to
  restore. Therefore, given a type `T`, the archive contains a function call
  operator with the following signature:

  ```cpp
  void operator()(T &);
  ```

  Every time this operator is invoked, the archive reads the next element from
  the underlying storage and copies it in the given variable.

### One example to rule them all

`EnTT` comes with some examples (actually some tests) that show how to integrate
a well known library for serialization as an archive. It uses
[`Cereal C++`](https://uscilab.github.io/cereal/) under the hood, mainly
because I wanted to learn how it works at the time I was writing the code.

The code **is not** production-ready and it is not neither the only nor
(probably) the best way to do it. However, feel free to use it at your own
risk.<br/>
The basic idea is to store everything in a group of queues in memory, then bring
everything back to the registry with different loaders.

# Storage

Pools of components are _specialized versions_ of the sparse set class. Each
pool contains all the instances of a single component type and all the entities
to which it is assigned.<br/>
Sparse arrays are _paged_ to avoid wasting memory. Packed arrays of components
are also paged to have pointer stability upon additions. Packed arrays of
entities are not instead.<br/>
All pools rearranges their items in order to keep the internal arrays tightly
packed and maximize performance, unless full pointer stability is enabled.

## Component traits

In `EnTT`, almost everything is customizable. Pools are no exception.<br/>
In this case, the _standardized_ way to access all component properties is the
`component_traits` class.

Various parts of the library access component properties through this class. It
makes it possible to use any type as a component, as long as its specialization
of `component_traits` implements all the required functionalities.<br/>
The non-specialized version of this class contains the following members:

* `in_place_delete`: `Type::in_place_delete` if present, true for non-movable
  types and false otherwise.

* `page_size`: `Type::page_size` if present, `ENTT_PACKED_PAGE` for non-empty
  types and 0 otherwise.

Where `Type` is any type of component. Properties are customized by specializing
the above class and defining its members, or by adding only those of interest to
a component definition:

```cpp
struct transform {
    static constexpr auto in_place_delete = true;
    // ... other data members ...
};
```

The `component_traits` class template takes care of _extracting_ the properties
from the supplied type.<br/>
Plus, it is _sfinae-friendly_ and also supports feature-based specializations.

## Empty type optimization

An empty type `T` is such that `std::is_empty_v<T>` returns true. They also are
the same types for which _empty base optimization_ (EBO) is possible.<br/>
`EnTT` handles these types in a special way, optimizing both in terms of
performance and memory usage. However, this also has consequences that are worth
mentioning.

When an empty type is detected, it is not instantiated by default. Therefore,
only the entities to which it is assigned are made available. There does not
exist a way to _get_ empty types from a storage or a registry. Views and groups
never return their instances too (for example, during a call to `each`).<br/>
On the other hand, iterations are faster because only the entities to which the
type is assigned are considered. Moreover, less memory is used, mainly because
there does not exist any instance of the component, no matter how many entities
it is assigned to.

More in general, none of the feature offered by the library is affected, but for
the ones that require to return actual instances.<br/>
This optimization is disabled by defining the `ENTT_NO_ETO` macro. In this case,
empty types are treated like all other types. Setting a page size at component
level via the `component_traits` class template is another way to disable this
optimization selectively rather than globally.

## Void storage

A void storage (or `entt::storage<void>` or `entt::basic_storage<Type, void>`),
is a fully functional storage type used to create pools not associated with a
particular component type.<br/>
From a technical point of view, it is in all respects similar to a storage for
empty types when their optimization is enabled. Pagination is disabled as well
as pointer stability (as not necessary).<br/>
However, this should be preferred to using a simple sparse set. In particular,
a void storage offers all those feature normally offered by other storage types.
Therefore, it is a perfectly valid pool for use with views and groups or within
a registry.

## Entity storage

This storage is such that the component type is the same as the entity type, for
example `entt::storage<entt::entity>` or `entt::basic_storage<Type, Type>`.<br/>
For this type of pools, there is a specific specialization within `EnTT`. In
fact, entities are subject to different rules with respect to components
(although still customizable by the user if needed). In particular:

* Entities are never truly _deleted_. They are moved out of the list of entities
  _in use_ and their versions are updated automatically.

* There are no `emplace` or `insert` functions in its interface. Instead, a
  range of `generate` functions are provided for creating or recycling entities.

* The `each` function returns an iterable object to visit the entities _in use_,
  that is, those not marked as _ready for reuse_. To iterate all the entities it
  is necessary to iterate the underlying sparse set instead.

This kind of storage is designed to be used where any other storage is fine and
can therefore be combined with views, groups and so on.

### Reserved identifiers

Since the entity storage is the one in charge of generating identifiers, it is
also possible to request that some of them be reserved and never returned.<br/>
By doing so, users can then generate and manage them autonomously, as needed.

To set a starting identifier, the `start_from` function is invoked as follows:

```cpp
storage.start_from(entt::entity{100});
```

Note that the version is irrelevant and is ignored in all cases. Identifiers are
always generated with default version.<br/>
By calling `start_from` as above, the first 100 elements are discarded and the
first identifier returned is the one with entity 100 and version 0.

### One of a kind to the registry

Within the registry, an entity storage is treated in all respects like any other
storage.<br/>
Therefore, it is possible to add mixins to it as well as retrieve it via the
`storage` function. It can also be used as storage in a view (for exclude-only
views for example):

```cpp
auto view = registry.view<entt::entity>(entt::exclude<my_type>);
```

However, it is also subject to a couple of exceptions, partly out of necessity
and partly for ease of use.

In particular, it is not possible to create multiple elements of this type.<br/>
This means that the _name_ used to retrieve this kind of storage is ignored and
the registry will only ever return the same element to the caller. For example:

```cpp
auto &other = registry.storage<entt::entity>("other"_hs);
```

In this case, the identifier is discarded as is. The call is in all respects
equivalent to the following:

```cpp
auto &storage = registry.storage<entt::entity>();
```

Because entity storage does not have a name, it cannot be retrieved via the
opaque `storage` function either.<br/>
It would make no sense to try anyway, given that the type of the registry and
therefore its entity type are known regardless.

Finally, when the user asks the registry for an iterable object to visit all the
storage elements inside it as follows:

```cpp
for(auto [id, storage]: registry.storage()) {
    // ...
}
```

Entity storage is never returned. This simplifies many tasks (such as copying an
entity) and fits perfectly with the fact that this type of storage does not have
an identifier inside the registry.

## Pointer stability

The ability to achieve pointer stability for one, several or all components is a
direct consequence of the design of `EnTT` and of its default storage.<br/>
In fact, although it contains what is commonly referred to as a _packed array_,
the default storage is paged and does not suffer from invalidation of references
when it runs out of space and has to reallocate.<br/>
However, this is not enough to ensure pointer stability in case of deletion. For
this reason, a _stable_ deletion method is also offered. This one is such that
the position of the elements is preserved by creating tombstones upon deletion
rather than trying to fill the holes that are created.

For performance reasons, `EnTT` favors storage compaction in all cases, although
often accessing a component occurs mostly randomly or traversing pools in a
non-linear order on the user side (as in the case of a hierarchy).<br/>
In other words, pointer stability is not automatic but is enabled on request.

### In-place delete

The library offers out of the box support for in-place deletion, thus offering
storage with completely stable pointers. This is achieved by specializing the
`component_traits` class or by adding the required properties to the component
definition when needed.<br/>
Views and groups adapt accordingly when they detect a storage with a different
deletion policy than the default. In particular:

* Groups are incompatible with stable storage and even refuse to compile.
* Multi type and runtime views are completely transparent to storage policies.
* Single type views for stable storage types offer the same interface of multi
  type views. For example, only `size_hint` is available.

In other words, the more generic version of a view is provided in case of stable
storage, even for a single type view.<br/>
In no case a tombstone is returned from the view itself. Likewise, non-existent
components are not returned, which could otherwise result in an UB.

### Hierarchies and the like

`EnTT` does not attempt in any way to offer built-in methods with hidden or
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
    static constexpr auto in_place_delete = true;

    transform *parent;
    // ... other data members ...
};
```

Furthermore, it is quite common for a group of elements to be created close in
time and therefore fallback into adjacent positions, thus favoring locality even
on random accesses. Locality that is not sacrificed over time given the
stability of storage positions, with undoubted performance advantages.

# Meet the runtime

`EnTT` takes advantage of what the language offers at compile-time. However,
this can have its downsides (well known to those familiar with type erasure
techniques).<br/>
To fill the gap, the library also provides a bunch of utilities and features
that are invaluable to handle types and pools at runtime.

## A base class to rule them all

Storage classes are fully self-contained types. They are _extended_ via mixins
to add more functionalities (generic or type specific). In addition, they offer
a basic set of functions that already allow users to go very far.<br/>
The aim is to limit the need for customizations as much as possible, offering
what is usually necessary for the vast majority of cases.

When a storage is used through its base class (for example, when its actual type
is not known), there is always the possibility of receiving a `type_info` object
for the type of elements associated with the entities (if any):

```cpp
if(entt::type_id<velocity>() == base.info()) {
    // ...
}
```

Furthermore, all features rely on internal functions that forward the calls to
the mixins. The latter can then make use of any information, which is set via
`bind`:

```cpp
base.bind(registry);
```

The `bind` function accepts any element by reference or by value and forwards it
to derived classes.<br/>
This is how a registry _passes_ itself to all pools that support signals and
also why a storage keeps sending events without requiring the registry to be
passed to it every time.

Alongside these more specific things, there are also a couple of functions
designed to address some common requirements such as copying an entity.<br/>
In particular, the base class behind a storage offers the possibility to _take_
the value associated with an entity through an opaque pointer:

```cpp
const void *instance = base.value(entity);
```

Similarly, the non-specialized `push` function accepts an optional opaque
pointer and behaves differently depending on the case:

* When the pointer is null, the function tries to default-construct an instance
  of the object to bind to the entity and returns true on success.

* When the pointer is not null, the function tries to copy-construct an instance
  of the object to bind to the entity and returns true on success.

This means that, starting from a reference to the base, it is possible to bind
components with entities without knowing their actual type and even initialize
them by copy if needed:

```cpp
// create a copy of an entity component by component
for(auto &&curr: registry.storage()) {
    if(auto &storage = curr.second; storage.contains(src)) {
        storage.push(dst, storage.value(src));
    }
}
```

This is particularly useful to clone entities in an opaque way. In addition, the
decoupling of features allows for filtering or use of different copying policies
depending on the type.

## Beam me up, registry

`EnTT` allows the user to assign a _name_ (or rather, a numeric identifier) to a
type and then create multiple pools of the same type:

```cpp
using namespace entt::literals;
auto &&storage = registry.storage<velocity>("second pool"_hs);
```

If a name is not provided, the default storage associated with the given type is
always returned.<br/>
Since the storage are also self-contained, the registry does not _duplicate_ its
own API for them. However, there is still no limit to the possibilities of use:

```cpp
auto &&other = registry.storage<velocity>("other"_hs);

registry.emplace<velocity>(entity);
other.push(entity);
```

Anything that can be done via the registry interface can also be done directly
on the reference storage.<br/>
On the other hand, those calls involving all storage are guaranteed to also
_reach_ manually created ones:

```cpp
// removes the entity from both storage
registry.destroy(entity);
```

Finally, a storage of this type works with any view (which also accepts multiple
storages of the same type, if necessary):

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
`EnTT` _at runtime_, which was previously quite limited.

# Views and Groups

Views are a non-intrusive tool for working with entities and components without
affecting other functionalities or increasing memory consumption.<br/>
Groups are an intrusive tool to use to improve performance along critical paths
but which also has a price to pay for that.

There are mainly two kinds of views: _compile-time_ (also known as `view`) and
runtime (also known as `runtime_view`).<br/>
The former requires a compile-time list of component (or storage) types and can
make several optimizations because of that. The latter is constructed at runtime
using numerical type identifiers instead and is a bit slower to iterate.<br/>
In both cases, creating and destroying views is not expensive at all since they
do not have any type of initialization.

Groups come in three different flavors: _full-owning groups_, _partial-owning
groups_ and _non-owning groups_. The main difference between them is in terms of
performance.<br/>
Groups can literally _own_ one or more component types. They are allowed to
rearrange pools so as to speed up iterations. Roughly speaking: the more
components a group owns, the faster it is to iterate them.

## Views

Single type views and multi type views behave differently and also have slightly
different APIs.

Single type views are specialized to give a performance boost in all cases.
There is nothing as fast as a single type view. They just walk through packed
(actually paged) arrays of elements and return them directly.<br/>
This kind of views also allows getting the exact number of elements they are
going to return.<br/>
Refer to the inline documentation for all the details.

Multi type views iterate entities that have at least all the given components.
During construction, they look at the number of elements available in each pool
and use the smallest set in order to speed up iterations.<br/>
This kind of views only allow to get the estimated number of elements they are
going to return.<br/>
Refer to the inline documentation for all the details.

Storing aside views is not required as they are extremely cheap to construct. In
fact, this is even discouraged when creating a view from a const registry. Since
all storage are lazily initialized, they may not exist when the view is created.
Thus, while perfectly usable, the view may contain pending references that are
never reinitialized with the actual storage.

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
Since they are not explicitly instantiated, empty components are not returned in
any case.

As a side note, in the case of single type views, `get` accepts but does not
strictly require a template parameter, since the type is implicitly defined.
However, when the type is not specified, the instance is returned using a tuple
for consistency with multi type views:

```cpp
auto view = registry.view<const renderable>();

for(auto entity: view) {
    auto [renderable] = view.get(entity);
    // ...
}
```

**Note**: prefer the `get` member function of a view instead of that of a
registry during iterations to get the types iterated by the view itself.

### Create once, reuse many times

Views support lazy initialization as well as _storage swapping_.<br/>
An empty (or partially initialized) view is such that it returns false when
converted to bool (to let the user know that it is not fully initialized) but it
also works as-is like any other view.

In order to initialize a view one piece at a time, it allows users to inject
storage classes when available:

```cpp
entt::storage_for_t<velocity> storage{};
entt::view<entt::get_t<position, velocity>> view{};

view.storage(storage);
```

If there are multiple storages of the same type, it is possible to disambiguate
using the _index_ of the element to be replaced:

```cpp
view.storage<1>(storage);
```

The ability to literally _replace_ a storage in a view also opens up its reuse
with different sets of entities.<br/>
For example, to _filter_ a view based on two groups of entities with different
characteristics, there will be no need to reinitialize anything:

```cpp
entt::view<entt::get<my_type, void>> view{registry.storage<my_type>>()};

entt::storage_for_t<void> the_good{};
entt::storage_for_t<void> the_bad{};

// initialize the sets above as needed

view.storage(the_good);

for(auto [entt, elem]: view) {
  // the good entities with their components here
}

view.storage(the_bad);

for(auto [entt, elem]: view) {
  // the bad entities with their components here
}
```

Finally, it should be noted that the lack of a storage is treated to all intents
and purposes as if it were an _empty_ element.<br/>
Thus, a _get_ storage (as in `entt::get_t`) makes the view empty automatically
while an _exclude_ storage (as in `entt::exclude_t`) is ignored as if that part
of the filter did not exist.

### Exclude-only

_Exclude-only_ views are not really a thing in `EnTT`.<br/>
However, the same result can be achieved by combining the right storage into a
simple view.

If one gets to the root of the problem, the purpose of an exclude-only view is
to return entities that do not meet certain requirements.<br/>
Since entity storage, unlike exclude-only views, **is** a thing in `EnTT`, users
can leverage it for these kinds of queries. It is also guaranteed to be unique
within a registry and is always accessible when creating a view:

```cpp
auto view = registry.view<entt::entity>(entt::exclude<my_type>);
```

The returned view is such that it will return only the entities that do not have
the `my_type` component, regardless of what other components they have.

### View pack

Views are combined with storage objects and with each other to create new, more
specific _queries_.<br/>
The type returned when combining multiple elements together is itself a view,
more in general a multi component one.

Combining different elements tries to mimic C++20 ranges:

```cpp
auto view = registry.view<position>();
auto other = registry.view<velocity>();
const auto &storage = registry.storage<renderable>();

auto pack = view | other | renderable;
```

The constness of the types is preserved and their order depends on the order in
which the views are combined. For example, the above _pack_ first returns an
instance of `position`, then one of `velocity`, and finally one of
`renderable`.<br/>
Since combining elements generates views, a chain can be of arbitrary length and
the above type order rules apply sequentially.

### Iteration order

By default, a view is iterated along the pool that contains the smallest number
of elements.<br/>
For example, if the registry contains fewer `velocity`s than it contains
`position`s, then the order of the elements returned by the following view
depends on how the `velocity` components are arranged in their pool:

```cpp
for(auto entity: registry.view<positon, velocity>()) { 
    // ...
}
```

Moreover, the order of types when constructing a view does not matter. Neither
does the order of views in a view pack.<br/>
However, it is possible to _enforce_ iteration of a view by given component
order by means of the `use` function:

```cpp
auto view = registry.view<position, velocity>();
view.use<position>();

for(auto entity: view) {
    // ...
}
```

On the other hand, if all a user wants is to iterate the elements in reverse
order, this is possible for a single type view using its reverse iterators:

```cpp
auto view = registry.view<position>();

for(auto it = view.rbegin(), last = view.rend(); it != last; ++iter) {
    // ...
}
```

Unfortunately, multi type views do not offer reverse iterators. Therefore, in
this case it is a must to implement this functionality manually or to use single
type views to lead the iteration.

### Runtime views

Multi type views iterate entities that have at least all the given components.
During construction, they look at the number of elements available in each pool
and use the smallest set in order to speed up iterations.<br/>
They offer more or less the same functionalities of a multi type view. However,
they do not expose a `get` member function and users should refer to the
registry that generated the view to access components.<br/>
Refer to the inline documentation for all the details.

Runtime views are pretty cheap to construct and should not be stored aside in
any case. They should be used immediately after creation and then they should be
thrown away.<br/>
To iterate a runtime view, either use it in a range-for loop:

```cpp
entt::runtime_view view{};
view.iterate(registry.storage<position>()).iterate(registry.storage<velocity>());

for(auto entity: view) {
    // ...
}
```

Or rely on the `each` member function to iterate entities:

```cpp
entt::runtime_view{}
    .iterate(registry.storage<position>())
    .iterate(registry.storage<velocity>())
    .each([](auto entity) {
        // ...
    });
```

Performance are exactly the same in both cases.<br/>
Filtering entities by components is also supported for this kind of views:

```cpp
entt::runtime_view view{};
view.iterate(registry.storage<position>()).exclude(registry.storage<velocity>());
```

Runtime views are meant for when users do not know at compile-time what types to
_use_ to iterate entities. The `storage` member function of a registry could be
useful in this regard.

## Groups

Groups are meant to iterate multiple components at once and to offer a faster
alternative to multi type views.<br/>
Groups overcome the performance of the other tools available but require to get
the ownership of components. This sets some constraints on their pools. On the
other hand, groups are not an automatism that increases memory consumption,
affects functionalities and tries to optimize iterations for all the possible
combinations of components. Users can decide when to pay for groups and to what
extent.<br/>
The most interesting aspect of groups is that they fit _usage patterns_. Other
solutions around usually try to optimize everything, because it is known that
somewhere within the _everything_ there are also our usage patterns. However
this has a cost that is not negligible, both in terms of performance and memory
usage. Ironically, users pay the price also for things they do not want and this
is not something I like much. Even worse, one cannot easily disable such a
behavior. Groups work differently instead and are designed to optimize only the
real use cases when users find they need to.<br/>
Another nice-to-have feature of groups is that they have no impact on memory
consumption, put aside full non-owning groups that are pretty rare and should be
avoided as long as possible.

All groups affect to an extent the creation and destruction of their components.
This is due to the fact that they must _observe_ changes in the pools of
interest and arrange data _correctly_ when needed for the types they own.<br/>
In all cases, a group allows to get the exact number of elements it is going to
return.<br/>
Refer to the inline documentation for all the details.

Storing aside groups is not required as they are extremely cheap to create, even
though valid groups can be copied without problems and reused freely.<br/>
A group performs an initialization step the very first time it is requested and
this could be quite costly. To avoid it, consider creating the group when no
components have been assigned yet. If the registry is empty, preparation is
extremely fast.

To iterate a group, either use it in a range-for loop:

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
Since they are not explicitly instantiated, empty components are not returned in
any case.

**Note**: prefer the `get` member function of a group instead of that of a
registry during iterations to get the types iterated by the group itself.

### Full-owning groups

A full-owning group is the fastest tool a user can expect to use to iterate
multiple components at once. It iterates all the components directly, no
indirection required.<br/>
This type of groups performs more or less as if users are accessing sequentially
a bunch of packed arrays of components all sorted identically, with no jumps nor
branches.

A full-owning group is created as:

```cpp
auto group = registry.group<position, velocity>();
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<position, velocity>({}, entt::exclude<renderable>);
```

Once created, the group gets the ownership of all the components specified in
the template parameter list and arranges their pools as needed.

Sorting owned components is no longer allowed once the group has been created.
However, full-owning groups are sorted using their `sort` member functions.
Sorting a full-owning group affects all its instances.

### Partial-owning groups

A partial-owning group works similarly to a full-owning group for the components
it owns, but relies on indirection to get components owned by other groups.<br/>
This is not as fast as a full-owning group, but it is already much faster than a
view when there are only one or two free components to retrieve (the most common
cases likely). In the worst case, it is not slower than views anyway.

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
the types provided via `entt::get` does not pass to the group instead.

Sorting owned components is no longer allowed once the group has been created.
However, partial-owning groups are sorted using their `sort` member functions.
Sorting a partial-owning group affects all its instances.

### Non-owning groups

Non-owning groups are usually fast enough, for sure faster than views and well
suited for most of the cases. However, they require custom data structures to
work properly and they increase memory consumption.<br/>
As a rule of thumb, users should avoid using non-owning groups, if possible.

A non-owning group is created as:

```cpp
auto group = registry.group<>(entt::get<position, velocity>);
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<>(entt::get<position, velocity>, entt::exclude<renderable>);
```

The group does not receive the ownership of any type of component in this
case. This type of groups is therefore the least performing in general, but also
the only one that can be used in any situation to slightly improve performance.

Non-owning groups are sorted using their `sort` member functions. Sorting a
non-owning group affects all its instances.

## Types: const, non-const and all in between

The `registry` class offers two overloads when it comes to constructing views
and groups: a const version and a non-const one. The former accepts only const
types as template parameters, the latter accepts both const and non-const types
instead.<br/>
It means that views and groups generated by a const registry also propagate the
constness to the types involved. As an example:

```cpp
entt::view<const position, const velocity> view = std::as_const(registry).view<const position, const velocity>();
```

Consider the following definition for a non-const view instead:

```cpp
entt::view<position, const velocity> view = registry.view<position, const velocity>();
```

In the example above, `view` is used to access either read-only or writable
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

It is not possible to get non-const references to `velocity` components from the
same view instead. Therefore, these result in compilation errors:

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
regardless of their components. This is done by accessing entity storage:

```cpp
for(auto entity: registry.view<entt::entity>()) {
    // ...
}
```

As a rule of thumb, consider using a view or a group if the goal is to iterate
entities that have a determinate set of components. These tools are usually much
faster than filtering entities with a bunch of custom tests.<br/>
In all the other cases, this is the way to go. For example, it is possible to
combine this view with the `orphan` member function to clean up orphan entities
(that is, entities that are still in use and have no assigned components):

```cpp
for(auto entity: registry.view<entt::entity>()) {
    if(registry.orphan(entity)) {
        registry.release(entity);
    }
}
```

In general, iterating all entities can result in poor performance. It should not
be done frequently to avoid the risk of a performance hit.<br/>
However, it is convenient when initializing an editor or to reclaim pending
identifiers.

## What is allowed and what is not

Most of the _ECS_ available out there do not allow to create and destroy
entities and components during iterations, nor to have pointer stability.<br/>
`EnTT` partially solves the problem with a few limitations:

* Creating entities and components is allowed during iterations in most cases
  and it never invalidates already existing references.

* Deleting the current entity or removing its components is allowed during
  iterations but it could invalidate references. For all the other entities,
  destroying them or removing their iterated components is not allowed and can
  result in undefined behavior.

* When pointer stability is enabled for the type leading the iteration, adding
  instances of the same type may or may not cause the entity involved to be
  returned. Destroying entities and components is always allowed instead, even
  if not currently iterated, without the risk of invalidating any references.

* In case of reverse iterations, adding or removing elements is not allowed
  under any circumstances. It could quickly lead to undefined behaviors.

In other terms, iterators are rarely invalidated. Also, component references
are not invalidated when a new element is added while they could be invalidated
upon destruction due to the _swap-and-pop_ policy, unless the type leading the
iteration undergoes in-place deletion.<br/>
As an example, consider the following snippet:

```cpp
registry.view<position>().each([&](const auto entity, auto &pos) {
    registry.emplace<position>(registry.create(), 0., 0.);
    // references remain stable after adding new instances
    pos.x = 0.;
});
```

The `each` member function will not break (because iterators remain valid) nor
will any reference be invalidated. Instead, more attention should be paid to the
destruction of entities or the removal of components.<br/>
Use a common range-for loop and get components directly from the view or move
the deletion of entities and components at the end of the function to avoid
dangling pointers.

For all types that do not offer stable pointers, iterators are also invalidated
and the behavior is undefined if an entity is modified or destroyed and it is
not the one currently returned by the iterator nor a newly created one.<br/>
To work around it, possible approaches are:

* Store aside the entities and the components to be removed and perform the
  operations at the end of the iteration.

* Mark entities and components with a proper tag component that indicates they
  must be purged, then perform a second iteration to clean them up one by one.

A notable side effect of this feature is that the number of required allocations
is further reduced in most cases.

### More performance, more constraints

Groups are a faster alternative to views. However, the higher the performance,
the greater the constraints on what is allowed and what is not.<br/>
In particular, groups add in some rare cases a limitation on the creation of
components during iterations. It happens in quite particular cases. Given the
nature and the scope of the groups, it is not something in which it will happen
to come across probably, but it is good to know it anyway.

First of all, it must be said that creating components while iterating a group
is not a problem at all and is done freely as it happens with the views. The
same applies to the destruction of components and entities, for which the rules
mentioned above apply.

The additional limitation arises instead when a given component that is owned by
a group is iterated outside of it. In this case, adding components that are part
of the group itself may invalidate the iterators. There are no further
limitations to the destruction of components and entities.<br/>
Fortunately, this is not always true. In fact, it almost never is and only
happens under certain conditions. In particular:

* Iterating a type of component that is part of a group with a single type view
  and adding to an entity all the components required to get it into the group
  may invalidate the iterators.

* Iterating a type of component that is part of a group with a multi type view
  and adding to an entity all the components required to get it into the group
  can invalidate the iterators, unless users specify another type of component
  to use to induce the order of iteration of the view (in this case, the former
  is treated as a free type and is not affected by the limitation).

In other words, the limitation does not exist as long as a type is treated as a
free type (as an example with multi type views and partial- or non-owning
groups) or iterated with its own group, but it can occur if the type is used as
a main type to rule on an iteration.<br/>
This happens because groups own the pools of their components and organize the
data internally to maximize performance. Because of that, full consistency for
owned components is guaranteed only when they are iterated as part of their
groups or as free types with multi type views and groups in general.

# Multithreading

In general, the entire registry is not thread safe as it is. Thread safety is
not something that users should want out of the box for several reasons. Just to
mention one of them: performance.<br/>
Views, groups and consequently the approach adopted by `EnTT` are the great
exception to the rule. It is true that views, groups and iterators in general
are not thread safe by themselves. Because of this users should not try to
iterate a set of components and modify the same set concurrently. However:

* As long as a thread iterates the entities that have the component `X` or
  assign and removes that component from a set of entities, another thread can
  safely do the same with components `Y` and `Z` and everything work like just
  fine. As a trivial example, users can freely execute the rendering system and
  iterate the renderable entities while updating a physic component concurrently
  on a separate thread.

* Similarly, a single set of components can be iterated by multiple threads as
  long as the components are neither assigned nor removed in the meantime. In
  other words, a hypothetical movement system can start multiple threads, each
  of which will access the components that carry information about velocity and
  position for its entities.

This kind of entity-component systems can be used in single-threaded
applications as well as along with async stuff or multiple threads. Moreover,
typical thread based models for _ECS_ do not require a fully thread-safe
registry to work. Actually, users can reach the goal with the registry as it is
while working with most of the common models.

Because of the few reasons mentioned above and many others not mentioned, users
are completely responsible for synchronization whether required. On the other
hand, they could get away with it without having to resort to particular
expedients.

Finally, `EnTT` is configured via a few compile-time definitions to make some of
its parts implicitly thread-safe, roughly speaking only the ones that really
make sense and cannot be turned around.<br/>
When using multiple threads with `EnTT`, you should define `ENTT_USE_ATOMIC`
unless you know exactly what you are doing. This is true even if each thread
only uses thread local data. For more information, see
[this section](config.md#entt_use_atomic).

## Iterators

A special mention is needed for the iterators returned by views and groups. Most
of the time they meet the requirements of random access iterators, in all cases
they meet at least the requirements of forward iterators.<br/>
In other terms, they are suitable for use with the parallel algorithms of the
standard library. If it is not clear, this is a great thing.

As an example, this kind of iterators are used in combination with
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
or later. Multi-pass guarantee will not break in any case and the performance
should even benefit from it further.

## Const registry

A const registry is also fully thread safe. This means that it is not able to
lazily initialize a missing storage when a view is generated.<br/>
The reason for this is easy to explain. To avoid requiring types to be
_announced_ in advance, a registry lazily creates the storage objects for the
different components. However, this is not possible for a thread safe const
registry.

Returned views are always valid and behave as expected in the context of the
caller. However, they may contain dangling references to non-existing storage
when created from a const registry.<br/>
As a result, such a view may misbehave over time if it is kept aside for a
second use.<br/>
Therefore, if the general advice is to create views when necessary and discard
them immediately afterwards, this becomes almost a rule when it comes to views
generated from a const registry.

Fortunately, there is also a way to instantiate storage classes early when in
doubt or when there are special requirements.<br/>
Calling the `storage` method is equivalent to _announcing_ a particular storage,
so as to avoid running into problems. For those interested, there are also
alternative approaches, such as a single threaded tick for the registry warm-up,
but these are not always applicable.<br/>
In this case, views never risk becoming _invalid_.

# Beyond this document

There are many other features and functions not listed in this document.<br/>
`EnTT` and in particular its ECS part is in continuous development and some
things could be forgotten, others could have been omitted on purpose to reduce
the size of this file. Unfortunately, some parts may even be outdated and still
to be updated.

For further information, it is recommended to refer to the documentation
included in the code itself or join the official channels to ask a question.
