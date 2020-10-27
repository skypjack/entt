# Frequently Asked Questions

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [FAQ](#faq)
  * [Why is my debug build on Windows so slow?](#why-is-my-debug-build-on-windows-so-slow)
  * [How can I represent hierarchies with my components?](#how-can-i-represent-hierarchies-with-my-components)
  * [Custom entity identifiers: yay or nay?](#custom-entity-identifiers-yay-or-nay)
  * [Warning C4307: integral constant overflow](#warning-C4307-integral-constant-overflow)
  * [Warning C4003: the min, the max and the macro](#warning-C4003-the-min-the-max-and-the-macro)
  * [The standard and the non-copyable types](#the-standard-and-the-non-copyable-types)
  * [Which functions trigger which signals](#which-functions-trigger-which-signals)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

This is a constantly updated section where I'll try to put the answers to the
most frequently asked questions.<br/>
If you don't find your answer here, there are two cases: nobody has done it yet
or this section needs updating. In both cases, try to
[open a new issue](https://github.com/skypjack/entt/issues/new) or enter the
[gitter channel](https://gitter.im/skypjack/entt) and ask your question.
Probably someone already has an answer for you and we can then integrate this
part of the documentation.

# FAQ

## Why is my debug build on Windows so slow?

`EnTT` is an experimental project that I also use to keep me up-to-date with the
latest revision of the language and the standard library. For this reason, it's
likely that some classes you're working with are using standard containers under
the hood.<br/>
Unfortunately, it's known that the standard containers aren't particularly
performing in debugging (the reasons for this go beyond this document) and are
even less so on Windows apparently. Fortunately this can also be mitigated a
lot, achieving good results in many cases.

First of all, there are two things to do in a Windows project:

* Disable the [`/JMC`](https://docs.microsoft.com/cpp/build/reference/jmc)
  option (_Just My Code_ debugging), available starting in Visual Studio 2017
  version 15.8.

* Set the [`_ITERATOR_DEBUG_LEVEL`](https://docs.microsoft.com/cpp/standard-library/iterator-debug-level)
  macro to 0. This will disable checked iterators and iterator debugging.

Moreover, the macro `ENTT_ASSERT` should be redefined to disable internal checks
made by `EnTT` in debug:

```cpp
#define ENTT_ASSERT(...) ((void)0)
```

These asserts are introduced to help the users but they require to access to the
underlying containers and therefore risk ruining the performance in some cases.

With these changes, debug performance should increase enough for most cases. If
you want something more, you can can also switch to an optimization level `O0`
or preferably `O1`.

## How can I represent hierarchies with my components?

This is one of the first questions that anyone makes when starting to work with
the entity-component-system architectural pattern.<br/>
There are several approaches to the problem and what’s the best one depends
mainly on the real problem one is facing. In all cases, how to do it doesn't
strictly depend on the library in use, but the latter can certainly allow or
not different techniques depending on how the data are laid out.

I tried to describe some of the techniques that fit well with the model of
`EnTT`. [Here](https://skypjack.github.io/2019-06-25-ecs-baf-part-4/) is the
first post of a series that tries to explore the problem. More will probably
come in future.

Long story short, you can always define a tree where the nodes expose implicit
lists of children by means of the following type:

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

The sort functionalities of `EnTT`, the groups and all the other features of the
library can help then to get the best in terms of data locality and therefore
performance from this component.

## Custom entity identifiers: yay or nay?

Custom entity identifiers are definitely a good idea in two cases at least:

* If `std::uint32_t` is too large or isn't large enough for your purposes, since
  this is the underlying type of `entt::entity`.
* If you want to avoid conflicts when using multiple registries.

Identifiers can be defined through enum classes and custom types for which a
specialization of `entt_traits` exists. For this purpose, `entt_traits` is also
defined as a _sfinae-friendly_ class template.<br/>
In fact, this is a definition equivalent to that of `entt::entity`:

```cpp
enum class entity: std::uint32_t {};
```

In theory, integral types can also be used as entity identifiers, even though
this may break in future and isn't recommended in general.

## Warning C4307: integral constant overflow

According to [this](https://github.com/skypjack/entt/issues/121) issue, using a
hashed string under VS could generate a warning.<br/>
First of all, I want to reassure you: it's expected and harmless. However, it
can be annoying.

To suppress it and if you don't want to suppress all the other warnings as well,
here is a workaround in the form of a macro:

```cpp
#if defined(_MSC_VER)
#define HS(str) __pragma(warning(suppress:4307)) entt::hashed_string{str}
#else
#define HS(str) entt::hashed_string{str}
#endif
```

With an example of use included:

```cpp
constexpr auto identifier = HS("my/resource/identifier");
```

Thanks to [huwpascoe](https://github.com/huwpascoe) for the courtesy.

## Warning C4003: the min, the max and the macro

On Windows, a header file defines two macros `min` and `max` which may result in
conflicts with their counterparts in the standard library and therefore in
errors during compilation.

It's a pretty big problem but fortunately it's not a problem of `EnTT` and there
is a fairly simple solution to it.<br/>
It consists in defining the `NOMINMAX` macro before to include any other header
so as to get rid of the extra definitions:

```cpp
#define NOMINMAX
```

Please refer to [this](https://github.com/skypjack/entt/issues/96) issue for
more details.

## The standard and the non-copyable types

`EnTT` uses internally the trait `std::is_copy_constructible_v` to check if a
component is actually copyable. This trait doesn't check if an object can
actually be copied but only verifies if there is a copy constructor
available.<br/>
This can lead to surprising results due to some idiosyncrasies of the standard
mainly related to the need to guarantee backward compatibility.

For example, `std::vector` defines a copy constructor no matter if its value
type is copyable or not. As a result, `std::is_copy_constructible_v` is true
for the following specialization:

```cpp
struct type {
    std::vector<std::unique_ptr<action>> vec;
};
```

When trying to assign an instance of this type to an entity in the ECS part,
this may trigger a compilation error because we cannot really make a copy of
it.<br/>
As a workaround, users can mark the type explicitly as non-copyable:

```cpp
struct type {
    type(const type &) = delete;
    type & operator=(const type &) = delete;

    std::vector<std::unique_ptr<action>> vec;
};
```

Unfortunately, this will also disable aggregate initialization.

## Which functions trigger which signals

The `registry` class offers three signals that are emitted following specific
operations. Maybe not everyone knows what these operations are, though.<br/>
If this isn't clear, below you can find a _vademecum_ for this purpose:

* `on_created` is invoked when a component is first added (neither modified nor 
  replaced) to an entity.
* `on_update` is called whenever an existing component is modified or replaced.
* `on_destroyed` is called when a component is explicitly or implicitly removed 
  from an entity.

Among the most controversial functions can be found `emplace_or_replace` and
`destroy`. However, following the above rules, it's quite simple to know what 
will happen.<br/>
In the first case, `on_created` is invoked if the entity has not the component,
otherwise the latter is replaced and therefore `on_update` is triggered. As for
the second case, components are removed from their entities and thus freed when
they are recycled. It means that `on_destroyed` is triggered for every component 
owned by the entity that is destroyed.
