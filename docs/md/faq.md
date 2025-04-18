# Frequently Asked Questions

# Table of Contents

* [Introduction](#introduction)
* [FAQ](#faq)
  * [Why is my debug build on Windows so slow?](#why-is-my-debug-build-on-windows-so-slow)
  * [How can I represent hierarchies with my components?](#how-can-i-represent-hierarchies-with-my-components)
  * [Custom entity identifiers: yay or nay?](#custom-entity-identifiers-yay-or-nay)
  * [Warning C4003: the min, the max and the macro](#warning-c4003-the-min-the-max-and-the-macro)
  * [The standard and the non-copyable types](#the-standard-and-the-non-copyable-types)
  * [Which functions trigger which signals](#which-functions-trigger-which-signals)
  * [Duplicate storage for the same component](#duplicate-storage-for-the-same-component)

# Introduction

This is a constantly updated section where I am trying to put the answers to the
most frequently asked questions.<br/>
If you do not find your answer here, there are two cases: nobody has done it
yet, or this section needs updating. In both cases, you can
[open a new issue](https://github.com/skypjack/entt/issues/new) or enter either
the [gitter channel](https://gitter.im/skypjack/entt) or the
[discord server](https://discord.gg/5BjPWBd) to ask for help.<br/>
Probably someone already has an answer for you and we can then integrate this
part of the documentation.

# FAQ

## Why is my debug build on Windows so slow?

`EnTT` is an experimental project that I also use to keep me up-to-date with the
latest revision of the language and the standard library. For this reason, it is
likely that some classes you are working with are using standard containers
under the hood.<br/>
Unfortunately, it is known that the standard containers are not particularly
performing in debugging (the reasons for this go beyond this document) and are
even less so on Windows, apparently. Fortunately, this can also be mitigated a
lot, achieving good results in many cases.

First of all, there are two things to do in a Windows project:

* Disable the [`/JMC`](https://docs.microsoft.com/cpp/build/reference/jmc)
  option (_Just My Code_ debugging), available starting with Visual Studio 2017
  version 15.8.

* Set the [`_ITERATOR_DEBUG_LEVEL`](https://docs.microsoft.com/cpp/standard-library/iterator-debug-level)
  macro to 0. This will disable checked iterators and iterator debugging.

Moreover, set the `ENTT_DISABLE_ASSERT` variable or redefine the `ENTT_ASSERT`
macro to disable internal debug checks in `EnTT`:

```cpp
#define ENTT_ASSERT(...) ((void)0)
```

These asserts are introduced to help the users, but they require access to the
underlying containers and therefore risk ruining the performance in some cases.

With these changes, debug performance should increase enough in most cases. If
you want something more, you can also switch to an optimization level `O0` or
preferably `O1`.

## How can I represent hierarchies with my components?

This is one of the first questions that anyone makes when starting to work with
the entity-component-system architectural pattern.<br/>
There are several approaches to the problem, and the best one depends mainly on
the real problem one is facing. In all cases, how to do it does not strictly
depend on the library in use, but the latter certainly allows or not different
techniques depending on how the data are laid out.

I tried to describe some of the approaches that fit well with the model of
`EnTT`. [This](https://skypjack.github.io/2019-06-25-ecs-baf-part-4/) is the
first post of a series that tries to _explore_ the problem. More will probably
come in the future.<br/>
In addition, `EnTT` also offers the possibility to create stable storage types
and therefore have pointer stability for one, all or some components. This is by
far the most convenient solution when it comes to creating hierarchies and
whatnot. See the documentation for the ECS part of the library and in particular
what concerns the `component_traits` class for further details.

## Custom entity identifiers: yay or nay?

Custom entity identifiers are definitely a good idea in two cases at least:

* If `std::uint32_t` is not large enough for your purposes, since this is the
  underlying type of `entt::entity`.

* If you want to avoid conflicts when using multiple registries.

Identifiers can be defined through enum classes and class types that define an
`entity_type` member of type `std::uint32_t` or `std::uint64_t`.<br/>
In fact, this is a definition equivalent to that of `entt::entity`:

```cpp
enum class entity: std::uint32_t {};
```

There is no limit to the number of identifiers that can be defined.

## Warning C4003: the min, the max and the macro

On Windows, a header file defines two macros `min` and `max` which may result in
conflicts with their counterparts in the standard library and therefore in
errors during compilation.

It is a pretty big problem. However, fortunately it is not a problem of `EnTT`
and there is a fairly simple solution to it.<br/>
It consists in defining the `NOMINMAX` macro before including any other header
so as to get rid of the extra definitions:

```cpp
#define NOMINMAX
```

Please refer to [this](https://github.com/skypjack/entt/issues/96) issue for
more details.

## The standard and the non-copyable types

`EnTT` uses internally the trait `std::is_copy_constructible_v` to check if a
component is actually copyable. However, this trait does not really check
whether a type is actually copyable. Instead, it just checks that a suitable
copy constructor and copy operator exist.<br/>
This can lead to surprising results due to some idiosyncrasies of the standard.

For example, `std::vector` defines a copy constructor that is conditionally
enabled depending on whether the value type is copyable or not. As a result,
`std::is_copy_constructible_v` returns true for the following specialization:

```cpp
struct type {
    std::vector<std::unique_ptr<action>> vec;
};
```

However, the copy constructor is effectively disabled upon specialization.
Therefore, trying to assign an instance of this type to an entity may trigger a
compilation error.<br/>
As a workaround, users can mark the type explicitly as non-copyable. This also
suppresses the implicit generation of the move constructor and operator, which
will therefore have to be defaulted accordingly:

```cpp
struct type {
    type(const type &) = delete;
    type(type &&) = default;

    type & operator=(const type &) = delete;
    type & operator=(type &&) = default;

    std::vector<std::unique_ptr<action>> vec;
};
```

Note that aggregate initialization is also disabled as a consequence.<br/>
Fortunately, this type of trick is quite rare. The bad news is that there is no
way to deal with it at the library level, this being due to the design of the
language. On the other hand, the fact that the language itself also offers a way
to mitigate the problem makes it manageable.

## Which functions trigger which signals

Storage classes offer three _signals_ that are emitted following specific
operations. Maybe not everyone knows what these operations are, though.<br/>
If this is not clear, below you can find a _vademecum_ for this purpose:

* `on_created` is invoked when a component is first added (neither modified nor 
  replaced) to an entity.

* `on_update` is called whenever an existing component is modified or replaced.

* `on_destroyed` is called when a component is explicitly or implicitly removed 
  from an entity.

Among the most controversial functions can be found `emplace_or_replace` and
`destroy`. However, following the above rules, it is quite simple to know what 
will happen.<br/>
In the first case, `on_created` is invoked if the entity has not the component,
otherwise the latter is replaced and therefore `on_update` is triggered. As for
the second case, components are removed from their entities and thus freed when
they are recycled. It means that `on_destroyed` is triggered for every component 
owned by the entity that is destroyed.

## Duplicate storage for the same component

It is rare, but you can see double sometimes, especially when it comes to
storage. This can be caused by a conflict in the hash assigned to the various
component types (one of a kind) or by bugs in your compiler
([more common](https://github.com/skypjack/entt/issues/1063) apparently).<br/>
Regardless of the cause, `EnTT` offers a customization point that also serves as
a solution in this case:

```cpp
template<>
struct entt::type_hash<Type> final {
    [[nodiscard]] static constexpr id_type value() noexcept {
        return hashed_string::value("Type");
    }

    [[nodiscard]] constexpr operator id_type() const noexcept {
        return value();
    }
};
```

Specializing `type_hash` directly bypasses the default implementation offered by
`EnTT`, thus avoiding any possible conflicts or compiler bugs.
