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
* [The Registry, the Entity and the Component](#the-registry-the-entity-and-the-component)
  * [Observe changes](#observe-changes)
    * [They call me Reactive System](#they-call-me-reactive-system)
  * [Runtime components](#runtime-components)
    * [A journey through a plugin](#a-journey-through-a-plugin)
  * [Sorting: is it possible?](#sorting-is-it-possible)
  * [Helpers](#helpers)
    * [Null entity](#null-entity)
    * [Stomp and spawn](#stomp-and-spawn)
    * [Dependencies](#dependencies)
    * [Tags](#tags)
    * [Actor](#actor)
    * [Context variables](#context-variables)
  * [Snapshot: complete vs continuous](#snapshot-complete-vs-continuous)
    * [Snapshot loader](#snapshot-loader)
    * [Continuous loader](#continuous-loader)
    * [Archives](#archives)
    * [One example to rule them all](#one-example-to-rule-them-all)
* [Views and Groups](#views-and-groups)
  * [Views](#views)
  * [Runtime views](#runtime-views)
  * [Groups](#groups)
    * [Full-owning groups](#full-owning-groups)
    * [Partial-owning groups](#partial-owning-groups)
    * [Non-owning groups](#non-owning-groups)
  * [Types: const, non-const and all in between](#types-const-non-const-and-all-in-between)
  * [Give me everything](#give-me-everything)
  * [What is allowed and what is not](#what-is-allowed-and-what-is-not)
    * [More performance, more constraints](#more-performance-more-constraints)
* [Empty type optimization](#empty-type-optimization)
* [Multithreading](#multithreading)
  * [Iterators](#iterators)
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
to specify the set of components neither at compile-time nor at runtime before
being able to use the library itself.<br/>
This is why users can instantiate the core class simply like:

```cpp
entt::registry registry;
```

In place of its more annoying and error-prone counterpart:

```cpp
entt::registry<comp_0, comp_1, ..., comp_n> registry;
```

Furthermore, there is no need to indicate to the library in any way that a type
of component exists and will be used sooner or later. When the time comes, users
can just use it and that's all.

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
performance where needed.<br/>
The disadvantage of this approach is that users need to know the systems they
are working on and the tools they are using. Otherwise, the risk to ruin the
performance along critical paths is high.

So far, this choice has proven to be a good one and I really hope it can be for
many others besides me.

## All or nothing

`EnTT` is such that at every moment a pair `(T *, size)` is available to
directly access all the instances of a given component type `T`.<br/>
This was a guideline and a design decision that influenced many choices, for
better and for worse. I cannot say whether it will be useful or not to the
reader, but it's worth to mention it, because it's one of the corner stones of
this library.

Many of the tools described below, from the registry to the views and up to the
groups give the possibility to get this information and have been designed
around this need, which was and remains one of my main requirements during the
development.<br/>
The rest is experimentation and the desire to invent something new, hoping to
have succeeded.

## Stateless systems

`EnTT` is designed so that it can work with _stateless systems_. In other words,
all systems can be free functions and there is no need to define them as classes
(although nothing prevents users from doing so).<br/>
This is possible because the main class with which the users will work provides
all what is needed to act as the sole _source of truth_ of an application.

To be honest, this point became part of the design principles at a later date,
but has also become one of the cornerstones of the library to date, as stateless
systems are widely used and appreciated in general.

# Vademecum

The registry to store, the views and the groups to iterate. That's all.

An entity (the _E_ of an _ECS_) is an opaque identifier that users should just
use as-is and store around if needed. Do not try to inspect an entity
identifier, its format can change in future and a registry offers all the
functionalities to query them out-of-the-box. The underlying type of an entity
(either `std::uint16_t`, `std::uint32_t` or `std::uint64_t`) can be specified
when defining a registry. In fact, an `entt::registry` is nothing more than an
alias for `entt::basic_registry<entt::entity>` and `entt::entity` is a distinct
type that implements the concept of _entity identifier_.<br/>
Components (the _C_ of an _ECS_) should be plain old data structures or more
complex and movable data structures with a proper constructor. Actually, the
sole requirement of a component type is that it must be both move constructible
and move assignable. They are list initialized by using the parameters provided
to construct the component itself. No need to register components or their types
neither with the registry nor with the entity-component system at all.<br/>
Systems (the _S_ of an _ECS_) are just plain functions, functors, lambdas or
whatever users want. They can accept a registry, a view or a group of any type
and use them the way they prefer. No need to register systems or their types
neither with the registry nor with the entity-component system at all.

The following sections will explain in short how to use the entity-component
system, the core part of the whole library.<br/>
In fact, the project is composed of many other classes in addition to those
describe below. For more details, please refer to the inline documentation.

# The Registry, the Entity and the Component

A registry can store and manage entities, as well as create views and groups to
iterate the underlying data structures.<br/>
The class template `basic_registry` lets users decide what's the preferred type
to represent an entity. Because `std::uint32_t` is large enough for almost all
the cases, there exists also the type `entt::entity` for it, as well as the
alias `entt::registry` for `entt::basic_registry<entt::entity>`.

Entities are represented by _entity identifiers_. An entity identifier is an
opaque type that users should not inspect or modify in any way. It carries
information about the entity itself and its version.<br/>
User defined identifiers can be introduced by means of the `ENTT_OPAQUE_TYPE`
macro if needed.

A registry is used both to construct and to destroy entities:

```cpp
// constructs a naked entity with no components and returns its identifier
auto entity = registry.create();

// destroys an entity and all its components
registry.destroy(entity);
```

There exists also an overload of the `create` and `destroy` member functions
that accepts two iterators, that is a range to assign or to destroy. It can be
used to create or destroy multiple entities at once:

```cpp
// destroys all the entities in a range
auto view = registry.view<a_component, another_component>();
registry.destroy(view.begin(), view.end());
```

In both cases, the `create` member function accepts also a list of default
constructible types of components to assign to the entities before to return.
It's a faster alternative to the creation and subsequent assignment of
components in separate steps.

When an entity is destroyed, the registry can freely reuse it internally with a
slightly different identifier. In particular, the version of an entity is
increased each and every time it's discarded.<br/>
In case entity identifiers are stored around, the registry offers all the
functionalities required to test them and to get out of them the information
they carry:

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
registry.assign<position>(entity, 0., 0.);

// ...

auto &velocity = registry.assign<velocity>(entity);
vel.dx = 0.;
vel.dy = 0.;
```

If an entity already has the given component, the `replace` member function
template can be used to replace it:

```cpp
registry.replace<position>(entity, 0., 0.);

// ...

auto &velocity = registry.replace<velocity>(entity);
vel.dx = 0.;
vel.dy = 0.;
```

In case users want to assign a component to an entity, but it's unknown whether
the entity already has it or not, `assign_or_replace` does the work in a single
call (there is a performance penalty to pay for this mainly due to the fact that
it has to check if the entity already has the given component or not):

```cpp
registry.assign_or_replace<position>(entity, 0., 0.);

// ...

auto &velocity = registry.assign_or_replace<velocity>(entity);
vel.dx = 0.;
vel.dy = 0.;
```

Note that `assign_or_replace` is a slightly faster alternative for the following
`if/else` statement and nothing more:

```cpp
if(registry.has<comp>(entity)) {
    registry.replace<comp>(entity, arg1, argN);
} else {
    registry.assign<comp>(entity, arg1, argN);
}
```

As already shown, if in doubt about whether or not an entity has one or more
components, the `has` member function template may be useful:

```cpp
bool b = registry.has<position, velocity>(entity);
```

On the other side, if the goal is to delete a single component, the `remove`
member function template is the way to go when it's certain that the entity owns
a copy of the component:

```cpp
registry.remove<position>(entity);
```

Otherwise consider to use the `reset` member function. It behaves similarly to
`remove` but with a strictly defined behavior (and a performance penalty is the
price to pay for this). In particular it removes the component if and only if it
exists, otherwise it returns safely to the caller:

```cpp
registry.reset<position>(entity);
```

There exist also two other _versions_ of the `reset` member function:

* If no entity is passed to it, `reset` will remove the given component from
  each entity that has it:

  ```cpp
  registry.reset<position>();
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
const auto &crenderable = cregistry.get<renderable>(entity);
auto &renderable = registry.get<renderable>(entity);

// const and non-const references
const auto &[cpos, cvel] = cregistry.get<position, velocity>(entity);
auto &[pos, vel] = registry.get<position, velocity>(entity);
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

To be notified when components are destroyed, use the `on_destroy` member
function instead. Finally, the `on_replace` member function will return a sink
to which to connect listeners to observe changes on components.

The function type of a listener for the construction signal should be equivalent
to the following:

```cpp
void(entt::entity, entt::registry &, Component &);
```

Where `Component` is intuitively the type of component of interest. In other
words, a listener is provided with the registry that triggered the notification
and the entity affected by the change, in addition to the newly created
instance.<br/>
The sink returned by the `on_replace` member function accepts listeners the
signature of which is the same of that of the construction signal. The one of
the destruction signal is also similar, except for the `Component` parameter:

```cpp
void(entt::entity, entt::registry &);
```

This is mainly due to performance reasons. While the component is made available
after the construction, it is not when destroyed. Because of that, there are no
reasons to get it from the underlying storage unless the user requires so. In
this case, the registry is made available for the purpose.

Note also that:

* Listeners for the construction signal are invoked **after** components have
  been assigned to entities.
* Listeners designed to observe changes are invoked **before** components have
  been replaced and therefore before newly created instances have been assigned
  to entities.
* Listeners for the destruction signal are invoked **before** components have
  been removed from entities.
* The order of invocation of the listeners isn't guaranteed in any case.

There are also some limitations on what a listener can and cannot do. In
particular:

* Connecting and disconnecting other functions from within the body of a
  listener should be avoided. It can lead to undefined behavior in some cases.
* Assigning and removing components from within the body of a listener that
  observes the destruction of instances of a given type should be avoided. It
  can lead to undefined behavior in some cases. This type of listeners is
  intended to provide users with an easy way to perform cleanup and nothing
  more.

To a certain extent, these limitations don't apply. However, it's risky to try
to force them and users should respect the limitations unless they know exactly
what they are doing. Subtle bugs are the price to pay in case of errors
otherwise.

In general, events and therefore listeners must not be used as replacements for
systems. They should not contain much logic and interactions with a registry
should be kept to a minimum, if possible. Note also that the greater the number
of listeners, the greater the performance hit when components are created or
destroyed.

Please, refer to the documentation of the signal class to know all the features
it offers.<br/>
There are many useful but less known functionalities that aren't described here,
such as the connection objects or the possibility to attach listeners with a
list of parameters that is shorter than that of the signal itself.

### They call me Reactive System

As mentioned above, signals are the basic tools to construct reactive systems,
even if they are not enough on their own.<br/>
`EnTT` tries to take another step in that direction with the `observer` class
template.

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
end. The rules of language and the design of the library obviously impose and
allow different things.

An `observer` is initialized with an instance of a registry and a set of rules
that describe what are the entities to intercept. As an example:

```cpp
entt::observer observer{registry, entt::collector.replace<sprite>()};
```

The class is default constructible if required and it can be reconfigured at any
time by means of the `connect` member function. Moreover, instances can be
disconnected from the underlying registries through the `disconnect` member
function.<br/>
The `observer` offers also some member functions to query its internal state and
to know if it's empty or how many entities it contains. Moreover, it can return
a raw pointer to the list of entities it contains.

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

Note that the snippet above is equivalent to the following:

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
  for which one or more of the given components have been explicitly replaced
  and not yet destroyed.

  ```cpp
  entt::collector.replace<sprite>();
  ```

* Grouping matcher: an observer will return at least all the living entities
  that would have entered the given group if it existed and that would have
  not yet left it.

  ```cpp
  entt::collector.group<position, velocity>(entt::exclude<destroyed>);
  ```

  A grouping matcher supports also exclusion lists as well as single components.

Roughly speaking, an observing matcher intercepts the entities for which the
given components are replaced (as in `registry::replace`) while a grouping
matcher tracks the entities that have assigned the given components since the
last time one asked.<br/>
Note that, for a grouping matcher, if an entity already has all the components
except one and the missing type is assigned to it, it is intercepted.

In addition, a matcher can be filtered with a `where` clause:

```cpp
entt::collector.replace<sprite>().where<position>(entt::exclude<velocity>);
```

This clause introduces a way to intercept entities if and only if they are
already part of a hypothetical group. If they are not, they aren't returned by
the observer, no matter if they matched the given rule.<br/>
In the example above, whenever the component `sprite` of an entity is replaced,
the observer probes the entity itself to verify that it has at least `position`
and has not `velocity` before to store it aside. If one of the two conditions of
the filter isn't respected, the entity is discared, no matter what.

A `where` clause accepts a theoretically unlimited number of types as well as
multiple elements in the exclusion list. Moreover, every matcher can have it's
own clause and multiple clauses for the same matcher are combined in a single
one.

## Runtime components

Defining components at runtime is useful to support plugin systems and mods in
general. However, it seems impossible with a tool designed around a bunch of
templates. Indeed it's not that difficult.<br/>
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
entt::registry registry;

// component identifier
auto type = registry.type<position>();
```

Once the identifiers are made available, almost everything becomes pretty
simple.

### A journey through a plugin

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

## Sorting: is it possible?

It goes without saying that sorting entities and components is possible with
`EnTT`.<br/>
In fact, there are two functions that respond to slightly different needs:

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

  There exists also the possibility to use a custom sort function object, as
  long as it adheres to the requirements described in the inline
  documentation.<br/>
  This is possible mainly because users can get much more with a custom sort
  function object if the usage pattern is known. As an example, in case of an
  almost sorted pool, quick sort could be much, much slower than insertion sort.

* Components can be sorted according to the order imposed by another component:

  ```cpp
  registry.sort<movement, physics>();
  ```

  In this case, instances of `movement` are arranged in memory so that cache
  misses are minimized when the two components are iterated together.

As a side note, when groups are involved, the sorting functions are applied
separately to the elements that are part of the group and to those that are not,
effectively generating two partitions, both of which can be ordered
independently of each other.

## Helpers

The so called _helpers_ are small classes and functions mainly designed to offer
built-in support for the most basic functionalities.<br/>
The list of helpers will grow longer as time passes and new ideas come out.

### Null entity

In `EnTT`, there exists a sort of _null entity_ made available to users that is
accessible via the `entt::null` variable.<br/>
The library guarantees that the following expression always returns false:

```cpp
registry.valid(entt::null);
```

In other terms, a registry will reject the null entity in all cases because it
isn't considered valid. It means that the null entity cannot own components for
obvious reasons.<br/>
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

### Stomp and spawn

The use of multiple registries is quite common. Examples of use are the
separation of the UI from the simulation or the loading of different scenes in
the background, possibly on a separate thread, without having to keep track of
which entity belongs to which scene.<br/>
In fact, with `EnTT` this is even a recommended practice, as the registry is
nothing more than a container and different optimizations and strategies can be
applied to different containers.

Once there are multiple registries available, however, one or more methods are
needed to transfer information from one container to another and this results in
the `stomp` member function and a couple of overloads of the `create` member
function for the `registry` class .<br/>
The `stomp` function allows to take one entity from a registry and use it to
_stomp_ one or more entities in another registry (or even the same, actually
making local copies). On the other hand, the overloads of the `create` member
function can be used to spawn new entities from a prototype.

These features open definitely the doors to a lot of interesting features like
migrating entities between registries, prototypes, shadow registry, prefabs,
shared components without an explicit owner and copy-on-write policies among the
other things.

### Dependencies

The `registry` class is designed to create short circuits between its functions
within certain limits. This allows to easily define dependencies between
different operations.<br/>
For example, the following adds (or replaces) the component `a_type` whenever
`my_type` is assigned to an entity:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::assign_or_replace<a_type>>(registry);
```

Similarly, the code shown below removes `a_type` from an entity whenever
`my_type` is assigned to it:

```cpp
registry.on_construct<my_type>().connect<&entt::registry::reset<a_type>>(registry);
```

A dependency can also be easily broken as follows:

```cpp
registry.on_construct<my_type>().disconnect<&entt::registry::assign_or_replace<a_type>>(registry);
```

There are many other types of dependencies besides those shown above. In
general, all functions that accept an entity as the first argument are good
candidates for this purpose.

### Tags

There's nothing magical about the way tags can be assigned to entities while
avoiding a performance hit at runtime. Nonetheless, the syntax can be annoying
and that's why a more user-friendly shortcut is provided to do it.<br/>
This shortcut is the alias template `entt::tag`.

If used in combination with hashed strings, it helps to use tags where types
would be required otherwise. As an example:

```cpp
registry.assign<entt::tag<"enemy"_hs>>(entity);
```

### Actor

The `actor` class is designed for those who don't feel immediately comfortable
working with components or for those who are migrating a project and want to
approach it one step at a time.

This class acts as a thin wrapper for an entity and for all its components. It's
constructed with a registry to be used behind the scenes and is in charge of the
destruction of the entity when it goes out of the scope.<br/>
An actor offers all the functionalities required to work with components, such
as the `assign` and` remove` member functions, but also `has`,` get`, `try_get`
and so on.

My advice isn't to use the `actor` class to hide entities and components behind
a more object-oriented interface. Instead, users should rely on it only where
strictly necessary. In all other cases, it's highly advisable to become familiar
with the model of `EnTT` and work directly with the registry, the views and the
groups, rather than with a tool that could introduce a performance degradation.

### Context variables

It is often convenient to assign context variables to a registry, so as to make
it the only _source of truth_ of an application.<br/>
This is possible by means of a member function named `set` to use to create a
context variable from a given type. Later on, either `ctx` or `try_ctx` can be
used to retrieve the newly created instance and `unset` is there to literally
reset it if needed.

Example of use:

```cpp
// creates a new context variable initialized with the given values
registry.set<my_type>(42, 'c');

// gets the context variable
const auto &var = registry.ctx<my_type>();

// if in doubts, probe the registry to avoid assertions in case of errors
if(auto *ptr = registry.try_ctx<my_type>(); ptr) {
    // uses the context variable associated with the registry, if any
}

// unsets the context variable
registry.unset<my_type>();
```

The type of a context variable must be such that it's default constructible and
can be moved. The `set` member function either creates a new instance of the
context variable or overwrites an already existing one if any. The `try_ctx`
member function returns a pointer to the context variable if it exists,
otherwise it returns a null pointer. This fits well with the `if` statement with
initializer.

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

To take a snapshot of the registry, use the `snapshot` member function. It
returns a temporary object properly initialized to _save_ the whole registry or
parts of it.

Example of use:

```cpp
output_archive output;

registry.snapshot()
    .entities(output)
    .destroyed(output)
    .component<a_component, another_component>(output);
```

It isn't necessary to invoke all these functions each and every time. What
functions to use in which case mostly depends on the goal and there is not a
golden rule to do that.

The `entities` member function asks the registry to serialize all the entities
that are still in use along with their versions. On the other side, the
`destroyed` member function tells to the registry to serialize the entities that
have been destroyed and are no longer in use.<br/>
These two functions can be used to save and restore the whole set of entities
with the versions they had during serialization.

The `component` member function is a function template the aim of which is to
store aside components. The presence of a template parameter list is a
consequence of a couple of design choices from the past and in the present:

* First of all, there is no reason to force a user to serialize all the
  components at once and most of the times it isn't desiderable. As an example,
  in case the stuff for the HUD in a game is put into the registry for some
  reasons, its components can be freely discarded during a serialization step
  because probably the software already knows how to reconstruct the HUD
  correctly from scratch.

* Furthermore, the registry makes heavy use of _type-erasure_ techniques
  internally and doesn't know at any time what types of components it contains.
  Therefore being explicit at the call point is mandatory.

There exists also another version of the `component` member function that
accepts a range of entities to serialize. This version is a bit slower than the
other one, mainly because it iterates the range of entities more than once for
internal purposes. However, it can be used to filter out those entities that
shouldn't be serialized for some reasons.<br/>
As an example:

```cpp
const auto view = registry.view<serialize>();
output_archive output;

registry.snapshot().component<a_component, another_component>(output, view.cbegin(), view.cend());
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
To do that, the registry offers a member function named `loader` that returns a
temporary object properly initialized to _restore_ a snapshot.

Example of use:

```cpp
input_archive input;

registry.loader()
    .entities(input)
    .destroyed(input)
    .component<a_component, another_component>(input)
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
must be exactly the same used during the serialization.

The `orphans` member function literally destroys those entities that have no
components attached. It's usually useless if the snapshot is a full dump of the
source. However, in case all the entities are serialized but only few components
are saved, it could happen that some of the entities have no components once
restored. The best users can do to deal with them is to destroy those entities
and thus update their versions.

### Continuous loader

A continuous loader is designed to load data from a source registry to a
(possibly) non-empty destination. The loader can accommodate in a registry more
than one snapshot in a sort of _continuous loading_ that updates the
destination one step at a time.<br/>
Identifiers that entities originally had are not transferred to the target.
Instead, the loader maps remote identifiers to local ones while restoring a
snapshot. Because of that, this kind of loader offers a way to update
automatically identifiers that are part of components (as an example, as data
members or gathered in a container).<br/>
Another difference with the snapshot loader is that the continuous loader does
not need to work with the private data structures of a registry. Furthermore, it
has an internal state that must persist over time. Therefore, there is no reason
to create it by means of a registry, or to limit its lifetime to that of a
temporary object.

Example of use:

```cpp
entt::continuous_loader<entt::entity> loader{registry};
input_archive input;

loader.entities(input)
    .destroyed(input)
    .component<a_component, another_component, dirty_component>(input, &dirty_component::parent, &dirty_component::child)
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

  Where `entt::entity` is the type of the entities used by the registry. Note
  that all the member functions of the snapshot class make also an initial call
  to this endpoint to save the _size_ of the set they are going to store.<br/>
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
  underlying storage and copy it in the given variable. Note that all the member
  functions of a loader class make also an initial call to this endpoint to read
  the _size_ of the set they are going to load.<br/>
  In addition, the archive must accept a pair of entity and component for each
  type to be restored. Therefore, given a type `T`, the archive must contain a
  function call operator with the following signature:

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

First of all, it is worth answering an obvious question: why views and
groups?<br/>
Briefly, they are a good tool to enforce single responsibility. A system that
has access to a registry can create and destroy entities, as well as assign and
remove components. On the other side, a system that has access to a view or a
group can only iterate entities and their components, then read or update the
data members of the latter.<br/>
It is a subtle difference that can help designing a better software sometimes.

More in details, views are a non-intrusive tool to access entities and
components without affecting other functionalities or increasing the memory
consumption. On the other side, groups are an intrusive tool that allows to
reach higher performance along critical paths but has also a price to pay for
that.

There are mainly two kinds of views: _compile-time_ (also known as `view`) and
runtime (also known as `runtime_view`).<br/>
The former require that users indicate at compile-time what are the components
involved and can make several optimizations because of that. The latter can be
constructed at runtime instead and are a bit slower to iterate entities and
components.<br/>
In both cases, creating and destroying a view isn't expensive at all because
views don't have any type of initialization. Moreover, views don't affect any
other functionality of the registry and keep memory usage at a minimum.

Groups come in three different flavors: _full-owning groups_, _partial-owning
groups_ and _non-owning groups_. The main difference between them is in terms of
performance.<br/>
Groups can literally _own_ one or more types of components. It means that they
will be allowed to rearrange pools so as to speed up iterations. Roughly
speaking: the more components a group owns, the faster it is to iterate them.
On the other side, a given component can belong only to one group, so users have
to define groups carefully to get the best out of them.

Continue reading for more details or refer to the inline documentation.

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
to the component list. It's also possible to ask a view if it contains a
given entity.<br/>
Refer to the inline documentation for all the details.

Multi component views iterate entities that have at least all the given
components in their bags. During construction, these views look at the number of
entities available for each component and pick up a reference to the smallest
set of candidates in order to speed up iterations.<br/>
They offer fewer functionalities than their companion views for single
component. In particular, a multi component view exposes utility functions to
get the estimated number of entities it is going to return and to know whether
it's empty or not. It's also possible to ask a view if it contains a given
entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store views around for they are extremely cheap to
construct, even though they can be copied without problems and reused freely.
Views also return newly created and correctly initialized iterators whenever
`begin` or `end` are invoked.

Views share the way they are created by means of a registry:

```cpp
// single component view
auto single = registry.view<position>();

// multi component view
auto multi = registry.view<position, velocity>();
```

To iterate a view, either use it in a range-for loop:

```cpp
auto view = registry.view<position, velocity>();

for(auto entity: view) {
    // a component at a time ...
    auto &position = view.get<position>(entity);
    auto &velocity = view.get<velocity>(entity);

    // ... or multiple components at once
    auto &[pos, vel] = view.get<position, velocity>(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities and get all their
components at once:

```cpp
registry.view<position, velocity>().each([](auto entity, auto &pos, auto &vel) {
    // ...
});
```

The `each` member function is highly optimized. Unless users want to iterate
only entities or get only some of the components, this should be the preferred
approach. Note that the entity can also be excluded from the parameter list if
not required, but this won't improve performance for multi component views.<br/>
There exists also an alternative version of `each` named `less` that works
exactly as its counterpart but for the fact that it doesn't return empty
components to the caller.

As a side note, when using a single component view, the most common error is to
invoke `get` with the type of the component as a template parameter. This is
probably due to the fact that it's required for multi component views:

```cpp
auto view = registry.view<position, const velocity>();

for(auto entity: view) {
    const auto &vel = view.get<const velocity>(entity);
    // ...
}
```

However, in case of a single component view, `get` doesn't accept a template
parameter, since it's implicitly defined by the view itself:

```cpp
auto view = registry.view<const renderable>();

for(auto entity: view) {
    const auto &renderable = view.get(entity);
    // ...
}
```

**Note**: prefer the `get` member function of a view instead of the `get` member
function template of a registry during iterations, if possible. However, keep in
mind that it works only with the components of the view itself.

## Runtime views

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

Runtime view are extremely cheap to construct and should not be stored around in
any case. They should be used immediately after creation and then they should be
thrown away. The reasons for this go far beyond the scope of this document.<br/>
To iterate a runtime view, either use it in a range-for loop:

```cpp
entt::component types[] = { registry.type<position>(), registry.type<velocity>() };
auto view = registry.runtime_view(std::cbegin(types), std::cend(types));

for(auto entity: view) {
    // a component at a time ...
    auto &position = registry.get<position>(entity);
    auto &velocity = registry.get<velocity>(entity);

    // ... or multiple components at once
    auto &[pos, vel] = registry.get<position, velocity>(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities:

```cpp
entt::component types[] = { registry.type<position>(), registry.type<velocity>() };

registry.runtime_view(std::cbegin(types), std::cend(types)).each([](auto entity) {
    // ...
});
```

Performance are exactly the same in both cases.

**Note**: runtime views are meant for all those cases where users don't know at
compile-time what components to use to iterate entities. This is particularly
well suited to plugin systems and mods in general. Where possible, don't use
runtime views, as their performance are slightly inferior to those of the other
views.

## Groups

Groups are meant to iterate multiple components at once and offer a faster
alternative to views. Roughly speaking, they just play in another league when
compared to views.<br/>
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
groups on other functionalities of the registry.

Groups offer a bunch of functionalities to get the number of entities they are
going to return and a raw access to the entity list as well as to the component
list for owned components. It's also possible to ask a group if it contains a
given entity.<br/>
Refer to the inline documentation for all the details.

There is no need to store groups around for they are extremely cheap to
construct, even though they can be copied without problems and reused freely.
A group performs an initialization step the very first time it's requested and
this could be quite costly. To avoid it, consider creating the group when no
components have been assigned yet. If the registry is empty, preparation is
extremely fast. Groups also return newly created and correctly initialized
iterators whenever `begin` or `end` are invoked.

To iterate groups, either use them in a range-for loop:

```cpp
auto group = registry.group<position>(entt::get<velocity>);

for(auto entity: group) {
    // a component at a time ...
    auto &position = group.get<position>(entity);
    auto &velocity = group.get<velocity>(entity);

    // ... or multiple components at once
    auto &[pos, vel] = group.get<position, velocity>(entity);

    // ...
}
```

Or rely on the `each` member function to iterate entities and get all their
components at once:

```cpp
registry.group<position>(entt::get<velocity>).each([](auto entity, auto &pos, auto &vel) {
    // ...
});
```

The `each` member function is highly optimized. Unless users want to iterate
only entities, this should be the preferred approach. Note that the entity can
also be excluded from the parameter list if not required and it can improve even
further the performance during iterations.

**Note**: prefer the `get` member function of a group instead of the `get`
member function template of a registry during iterations, if possible. However,
keep in mind that it works only with the components of the group itself.

Let's go a bit deeper into the different types of groups made available by this
library to know how they are constructed and what are the differences between
them.

### Full-owning groups

A full-owning group is the fastest tool an user can expect to use to iterate
multiple components at once. It iterates all the components directly, no
indirection required. This type of groups performs more or less as if users are
accessing sequentially a bunch of packed arrays of components all sorted
identically.

A full-owning group is created as:

```cpp
auto group = registry.group<position, velocity>();
```

Filtering entities by components is also supported:

```cpp
auto group = registry.group<position, velocity>(entt::exclude<renderable>);
```

Once created, the group gets the ownership of all the components specified in
the template parameter list and arranges their pools so as to iterate all of
them as fast as possible.

Sorting owned components is no longer allowed once the group has been created.
However, full-owning groups can be sorted by means of their `sort` member
functions, if required. Sorting a full-owning group affects all the instances of
the same group (it means that users don't have to call `sort` on each instance
to sort all of them because they share the underlying data structure).<br/>
The elements that aren't part of the group can still be sorted separately for
each pool using the `sort` member function of the registry.

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
the template parameter list and arranges their pools so as to iterate all of
them as fast as possible. The ownership of the types provided via `entt::get`
doesn't pass to the group instead.

Sorting owned components is no longer allowed once the group has been created.
However, partial-owning groups can be sorted by means of their `sort` member
functions, if required. Sorting a partial-owning group affects all the instances
of the same group (it means that users don't have to call `sort` on each
instance to sort all of them because they share the underlying data
structure).<br/>
Regarding the owned types, the elements that aren't part of the group can still
be sorted separately for each pool using the `sort` member function of the
registry.

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
the only one that can be used in any situation to improve a performance where
necessary.

Non-owning groups can be sorted by means of their `sort` member functions, if
required. Sorting a non-owning group affects all the instance of the same group
(it means that users don't have to call `sort` on each instance to sort all of
them because they share the set of entities).

# Types: const, non-const and all in between

The `registry` class offers two overloads when it comes to constructing views
and groups: a const version and a non-const one. The former accepts both const
and non-const types as template parameters, the latter accepts only const types
instead.<br/>
It means that views and groups can be constructed also from a const registry and
they propagate the constness of the registry to the types involved. As an
example:

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
In other terms, these statements are all valid:

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

Similarly, the `each` member functions will propagate constness to the type of
the components returned during iterations:

```cpp
view.each([](auto entity, position &pos, const velocity &vel) {
    // ...
});
```

Obviously, a caller can still refer to the `position` components through a const
reference because of the rules of the language that fortunately already allow
it.

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

It returns to the caller all the entities that are still in use by means of the
given function.<br/>
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

* Creating entities and components is allowed during iterations in almost all
  cases.
* Deleting the current entity or removing its components is allowed during
  iterations. For all the other entities, destroying them or removing their
  components isn't allowed and can result in undefined behavior.

In these cases, iterators aren't invalidated. To be clear, it doesn't mean that
also references will continue to be valid.<br/>
Consider the following example:

```cpp
registry.view<position>([&](const auto entity, auto &pos) {
    registry.assign<position>(registry.create(), 0., 0.);
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
only the entities to which it's assigned are made available. All the iterators
as well as the `get` member functions of the registry, the views and the groups
will return temporary objects. Similarly, some functions such as `try_get` or
the raw access to the list of components aren't available for this kind of
types.<br/>
On the other hand, iterations are faster because only the entities to which the
type is assigned are considered. Moreover, less memory is used, since there
doesn't exist any instance of the component, no matter how many entities it is
assigned to.

For similar reasons, wherever a function type of a listener accepts a component,
it cannot be caught by a non-const reference. Capture it by copy or by const
reference instead.

More in general, none of the features offered by the library is affected, but
for the ones that require to return actual instances.

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

## Iterators

A special mention is needed for the iterators returned by the views and the
groups. Most of the time they meet the requirements of random access iterators,
in all cases they meet at least the requirements of forward iterators.<br/>
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
means that the iterators provided by the library cannot return proxy objects as
references and **must** return actual reference types instead.<br/>
This may change in the future and the iterators will almost certainly return
both the entities and a list of references to their components sooner or later.
Multi-pass guarantee won't break in any case and the performance should even
benefit from it further.

# Beyond this document

There are many other features and functions not listed in this document.<br/>
`EnTT` and in particular its ECS part is in continuous development and some
things could be forgotten, others could have been omitted on purpose to reduce
the size of this file. Unfortunately, some parts may even be outdated and still
to be updated.

For further information, it's recommended to refer to the documentation included
in the code itself or join the official channels to ask a question.
