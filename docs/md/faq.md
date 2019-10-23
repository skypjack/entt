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

Moreover, the macro `ENTT_DISABLE_ASSERT` should be defined to disable internal
checks made by `EnTT` in debug. These are asserts introduced to help the users,
but require to access to the underlying containers and therefore risk ruining
the performance in some cases.

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

* If `std::uint32_t` isn't large enough as an underlying type.
* If you want to avoid conflicts when using multiple registries.

These identifiers are nothing more than enum classes with some salt.<br/>
To simplify the creation of new identifiers, `EnTT` provides the macro
`ENTT_OPAQUE_TYPE` that accepts two arguments:

* The name you want to give to the new identifier (watch out for namespaces).
* The underlying type to use (either `std::uint16_t`, `std::uint32_t`
  or `std::uint64_t`).

In fact, this is the definition of `entt::entity`:

```cpp
ENTT_OPAQUE_TYPE(entity, std::uint32_t)
```

The use of this macro is highly recommended, so as not to run into problems if
the requirements for the identifiers should change in the future.

## Warning C4307: integral constant overflow

According to [this](https://github.com/skypjack/entt/issues/121) issue, using a
hashed string under VS could generate a warning.<br/>
First of all, I want to reassure you: it's expected and harmless. However, it
can be annoying.

To suppress it and if you don't want to suppress all the other warnings as well,
here is a workaround in the form of a macro:

```cpp
#if defined(_MSC_VER)
    #define HS(str)\
        __pragma(warning(push))\
        __pragma(warning(disable:4307))\
        entt::hashed_string{str}\
        __pragma(warning(pop))
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

## Non copyable components

The registry uses the trait
[std::is_copy_constructible](https://en.cppreference.com/w/cpp/types/is_copy_constructible)
to check if a component is actually copyable. This trait does not check if the
object can actually be copied but only verifies if there is a copy constructor
and this may lead to [surprising result](https://stackoverflow.com/questions/18404108/false-positive-with-is-copy-constructible-on-vectorunique-ptr).

For example, the following component is actually not copyable but
`std::is_copy_constructible` will tell the opposite, this is because
[std::vector](https://en.cppreference.com/w/cpp/container/vector) defines a
copy constructor no matter what its underlying object is copyable or not.

```cpp
struct component {
    std::vector<std::unique_ptr<action>> actions;
};
```

When trying to assign this component to an entity, you'll get a compiler error,
instead you must explicitly mark the object as non copyable:

```cpp
struct component {
    std::vector<std::unique_ptr<action>> actions;

    component(const component&) = delete;
    component& operator=(const component&) = delete;
};
```

Unfortunately, this will also disable aggregate initialization.
