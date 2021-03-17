# Crash Course: entity-component system

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Design decisions](#design-decisions)
  * [A bitset-free entity-component system](#a-bitset-free-entity-component-system)
  * [Pay per use](#pay-per-use)
  * [All or nothing](#all-or-nothing)
  * [Stateless systems](#stateless-systems)
* [Vademecum](#vademecum)
* [Pools](#pools)
* [The Registry, the Entity and the Component](#the-registry-the-entity-and-the-component)
  * [Observe changes](#observe-changes)
    * [They call me Reactive System](#they-call-me-reactive-system)
  * [Sorting: is it possible?](#sorting-is-it-possible)
  * [Helpers](#helpers)
    * [Null entity](#null-entity)
    * [To entity](#to-entity)
    * [Dependencies](#dependencies)
    * [Invoke](#invoke)
    * [Handle](#handle)
    * [Organizer](#organizer)
  * [Context variables](#context-variables)
    * [Aliased properties](#aliased-properties)
  * [Meet the runtime](#meet-the-runtime)
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

## A bitset-free entity-component system

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

`EnTT` is such that at every moment a pair `(T *, size)` is available to
directly access all the instances of a given component type `T`.<br/>
This was a guideline and a design decision that influenced many choices, for
better and for worse. I cannot say whether it will be useful or not to the
reader, but it's worth to mention it since it's one of the corner stones of
this library.

Many of the tools described below give the possibility to get this information
and have been designed around this need.<br/>
The rest is experimentation and the desire to invent something new, hoping to
have succeeded.

## Stateless systems

`EnTT` is designed so that it can work with _stateless systems_. In other words,
all systems can be free functions and there is no need to define them as classes
(although nothing prevents users from doing so).<br/>
This is possible because the main class with which the users will work provides
all what is needed to act as the sole _source of truth_ of an application.

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

Each pool contains all the instances of a single component, as well as all the
entities to which it's assigned. Sparse arrays are also _paged_ to avoid wasting
memory in some cases while packed arrays are not for obvious reasons.<br/>
Pools also make available at any time a pointer to the packed lists of entities
and components they contain, in addition to the number of elements in use. For
this reason, pools can rearrange their items in order to keep the internal
arrays tightly packed and maximize performance.

At the moment, it's possible to specialize pools within certain limits, although
a more flexible and user-friendly model is under development.

# The Registry, the Entity and the Component

A registry can store and manage entities, as well as create views and groups to
iterate the underlying data structures.<br/>
The class template `basic_registry` lets users decide what's the preferred type
to represent an entity. Because `std::uint32_t` is large enough for almost all
the cases, there exists also the enum class `entt::entity` that _wraps_ it and
the alias `entt::registry` for `entt::basic_registry<entt::entity>`.

Entities are represented by _entity identifiers_. An entity identifier carries
information about the entity itself and its version.<br/>
User defined identifiers can be introduced by means of enum classes and custom
types for which a specialization of `entt_traits` exists. For this purpose,
`entt_traits` is also defined as a _sfinae-friendly_ class template. In theory,
integral types can also be used as entity identifiers, even though this may
break in future and isn't recommended in general.

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

When an entity is destroyed, the registry can freely reuse it internally with a
slightly different identifier. In particular, the version of an entity is
increased after destruction (unless the overload that forces a version is used
instead of the default one).<br/>
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
if(registry.has<velocity>(entity)) {
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

If the goal is to delete a component from an entity that owns it, the `remove`
member function template is the way to go:

```cpp
registry.remove<position>(entity);
```

When in doubt whether the entity owns the component, use the `remove_if_exists`
member function instead. It behaves similarly to `remove` but it discards the
component if and only if it exists, otherwise it returns safely to the caller:

```cpp
registry.remove_if_exists<position>(entity);
```

The `clear` member function works similarly and can be used to either:

* Remove all instances of the given components from the entities that own them:

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

Be aware that `entt::null` and entity 0 aren't the same thing. Likewise, a zero
initialized entity isn't the same as `entt::null`. Therefore, although
`entt::entity{}` is in some sense an alias for entity 0, none of them can be
used to create a null entity.

### To entity

Sometimes it's useful to get the entity from a component instance.<br/>
This is what the `entt::to_entity` helper does. It accepts a registry and an
instance of a component and returns the entity associated with the latter:

```cpp
const auto entity = entt::to_entity(registry, position);
```

This utility doesn't perform any check on the validity of the component.
Therefore, trying to take the entity of an invalid element or of an instance
that isn't associated with the given registry can result in undefined behavior.

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

## Meet the runtime

`EnTT` takes full advantage of what the language offers at compile-time.<br/>
However, by combining these feature with a tool for static polymorphism, it's
also possible to have opaque proxies to work with _type-less_ pools at runtime.

These objects are returned by the `storage` member function, which accepts a
`type_info` object as an argument rather than a compile-time type (the same
returned by the `visit` member function):

```cpp
auto storage = registry.storage(info);
```

By default and to stay true with the philosophy of the library, the API of a
proxy is minimal and doesn't allow users to do much.<br/>
However, it's also completely customizable in a generic way and with the
possibility of defining specific behaviors for given types.

This section won't go into detail on how to define a poly storage to get all the
possible functionalities out of it. `EnTT` already contains enough snippets to
get inspiration from, both in the test suite and in the `example` folder.<br/>
In short, users will have to define their own _concepts_ (see the `entt::poly`
documentation for this) and register them via the `poly_storage_traits` class
template, which has been designed as sfinae-friendly for the purpose.

Once the concept that a poly storage must adhere to has been properly defined,
copying an entity will be as easy as:

```cpp
registry.visit(entity, [&](const auto info) {
    auto storage = registry.storage(info);
    storage->emplace(registry, other, storage->get(entity));
});
```

Where `other` is the entity to which the elements should be replicated.<br/>
Similarly, copying entire pools between different registries can look like this:

```cpp
registry.visit([&](const auto info) {
    auto storage = registry.storage(info);
    other.storage(info)->insert(other, storage->data(), storage->raw(), storage->size());
});
```

Where this time `other` represents the destination registry.

So, all in all, `EnTT` shifts the complexity to the one-time definition of a
_concept_ that reflects the user's needs, and then leaves room for ease of use
within the codebase.<br/>
The possibility of extreme customization is the icing on the cake in this sense,
allowing users to design this tool around their own requirements.

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
those still alive and those destroyed) along with their versions.<br/>
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

entt::snapshot{registry}.component<a_component, another_component>(output, view.cbegin(), view.cend());
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

The `orphans` member function literally destroys those entities that have no
components attached. It's usually useless if the snapshot is a full dump of the
source. However, in case all the entities are serialized but only few components
are saved, it could happen that some of the entities have no components once
restored. The best the users can do to deal with them is to destroy those
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

The `orphans` member function literally destroys those entities that have no
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

  The output archive can freely decide how to serialize the data. The register
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

Single component views are specialized in order to give a boost in terms of
performance in all the situations. This kind of views can access the underlying
data structures directly and avoid superfluous checks. There is nothing as fast
as a single component view. In fact, they walk through a packed array of
components and return them one at a time.<br/>
Single component views offer a bunch of functionalities to get the number of
entities they are going to return and a raw access to the entity list as well as
to the component list. It's also possible to ask a view if it contains a given
entity.<br/>
Refer to the inline documentation for all the details.

Multi component views iterate entities that have at least all the given
components in their bags. During construction, these views look at the number of
entities available for each component and pick up a reference to the smallest
set of candidates in order to speed up iterations.<br/>
They offer fewer functionalities than single component views. In particular,
a multi component view exposes utility functions to get the estimated number of
entities it is going to return and to know if it contains a given entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store views aside for they are extremely cheap to construct,
even though valid views can be copied without problems and reused freely.<br/>
Views also return newly created and correctly initialized iterators whenever
`begin` or `end` are invoked.

Views share the way they are created by means of a registry:

```cpp
// single component view
auto single = registry.view<position>();

// multi component view
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

As a side note, in the case of single component views, `get` accepts but doesn't
strictly require a template parameter, since the type is implicitly defined.
However, when the type isn't specified, for consistency with the multi component
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

Views can be combined with each other to create new and more specific
objects.<br/>
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
available for each component and pick up a reference to the smallest
set of candidates in order to speed up iterations.<br/>
They offer more or less the same functionalities of a multi component view.
However, they don't expose a `get` member function and users should refer to the
registry that generated the view to access components. In particular, a runtime
view exposes utility functions to get the estimated number of entities it is
going to return and to know whether it's empty or not. It's also possible to ask
a runtime view if it contains a given entity.<br/>
Refer to the inline documentation for all the details.

Runtime views are pretty cheap to construct and should not be stored aside in
any case. They should be used immediately after creation and then they should be
thrown away. The reasons for this go far beyond the scope of this document.<br/>
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
alternative to multi component views.<br/>
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

Groups offer a bunch of functionalities to get the number of entities they are
going to return and a raw access to the entity list as well as to the component
list for owned components. It's also possible to ask a group if it contains a
given entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store groups aside for they are extremely cheap to
construct, even though valid groups can be copied without problems and reused
freely.<br/>
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

It returns to the caller all the entities that are still in use.<br/>
As a rule of thumb, consider using a view or a group if the goal is to iterate
entities that have a determinate set of components. These tools are usually much
faster than combining this function with a bunch of custom tests.<br/>
In all the other cases, this is the way to go.

There exists also another member function to use to retrieve orphans. An orphan
is an entity that is still in use and has no assigned components.<br/>
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

## What is allowed and what is not

Most of the _ECS_ available out there don't allow to create and destroy entities
and components during iterations.<br/>
`EnTT` partially solves the problem with a few limitations:

* Creating entities and components is allowed during iterations in most cases.

* Deleting the current entity or removing its components is allowed during
  iterations. For all the other entities, destroying them or removing their
  components isn't allowed and can result in undefined behavior.

In these cases, iterators aren't invalidated. To be clear, it doesn't mean that
also references will continue to be valid.<br/>
Consider the following example:

```cpp
registry.view<position>([&](const auto entity, auto &pos) {
    registry.emplace<position>(registry.create(), 0., 0.);
    pos.x = 0.; // warning: dangling pointer
});
```

The `each` member function won't break (because iterators aren't invalidated)
but there are no guarantees on references. Use a common range-for loop and get
components directly from the view or move the creation of components at the end
of the function to avoid dangling pointers.

Iterators are invalidated instead and the behavior is undefined if an entity is
modified or destroyed and it's not the one currently returned by the iterator
nor a newly created one.<br/>
To work around it, possible approaches are:

* Store aside the entities and the components to be removed and perform the
  operations at the end of the iteration.

* Mark entities and components with a proper tag component that indicates they
  must be purged, then perform a second iteration to clean them up one by one.

A notable side effect of this feature is that the number of required allocations
is further reduced in most of the cases.

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

* Iterating a type of component that is part of a group with a single component
  view and adding to an entity all the components required to get it into the
  group may invalidate the iterators.

* Iterating a type of component that is part of a group with a multi component
  view and adding to an entity all the components required to get it into the
  group can invalidate the iterators, unless users specify another type of
  component to use to induce the order of iteration of the view (in this case,
  the former is treated as a free type and isn't affected by the limitation).

In other words, the limitation doesn't exist as long as a type is treated as a
free type (as an example with multi component views and partial- or non-owning
groups) or iterated with its own group, but it can occur if the type is used as
a main type to rule on an iteration.<br/>
This happens because groups own the pools of their components and organize the
data internally to maximize performance. Because of that, full consistency for
owned components is guaranteed only when they are iterated as part of their
groups or as free types with multi component views and groups in general.

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
won't be available. Finally, the `sort` functionality will onlyaccepts callbacks
that require to return entities rather than components:

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
This optimization can be disabled by defining the `ENTT_NO_ETO` macro. In this
case, empty types will be treated like all other types, no matter what.

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
they meet at least the requirements of bidirectional iterators.<br/>
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

Contrary to what the standard library containers offer, a const registry is
generally but not completely thread safe.<br/>
In particular, one (and only one) of its const member functions isn't fully
thread safe. That is the `view` method.

The reason for this is easy to explain. To avoid requiring types to be
_announced_ in advance, the registry lazily initializes the storage objects for
the different components.<br/>
In most cases, this isn't even necessary. The absence of a storage is itself the
required information. However, when building a view, all pools must necessarily
exist. This makes the `view` member function not thread safe even in its const
overload, unless all pools already exist.

Fortunately, there is also a way to instantiate storage classes early when in
doubt or when there are special requirements.<br/>
Calling the `prepare` method is equivalent to _announcing_ the existence of a
particular storage, to avoid running into problems. For those interested, there
are also alternative approaches, such as a single threaded tick for the registry
warm-up, but these are not always applicable.

# Beyond this document

There are many other features and functions not listed in this document.<br/>
`EnTT` and in particular its ECS part is in continuous development and some
things could be forgotten, others could have been omitted on purpose to reduce
the size of this file. Unfortunately, some parts may even be outdated and still
to be updated.

For further information, it's recommended to refer to the documentation included
in the code itself or join the official channels to ask a question.
