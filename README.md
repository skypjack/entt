![EnTT: Gaming meets modern C++](https://user-images.githubusercontent.com/1812216/103550016-90752280-4ea8-11eb-8667-12ed2219e137.png)

[![Build Status](https://github.com/skypjack/entt/workflows/build/badge.svg)](https://github.com/skypjack/entt/actions)
[![Coverage](https://codecov.io/gh/skypjack/entt/branch/master/graph/badge.svg)](https://codecov.io/gh/skypjack/entt)
[![Try online](https://img.shields.io/badge/try-online-brightgreen)](https://godbolt.org/z/zxW73f)
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://skypjack.github.io/entt/)
[![Vcpkg port](https://img.shields.io/vcpkg/v/entt)](https://vcpkg.link/ports/entt)
[![Conan Center](https://img.shields.io/conan/v/entt)](https://conan.io/center/recipes/entt)
[![Gitter chat](https://badges.gitter.im/skypjack/entt.png)](https://gitter.im/skypjack/entt)
[![Discord channel](https://img.shields.io/discord/707607951396962417?logo=discord)](https://discord.gg/5BjPWBd)

> `EnTT` has been a dream so far, we haven't found a single bug to date and it's
> super easy to work with
>
> -- Every EnTT User Ever

`EnTT` is a header-only, tiny and easy to use library for game programming and
much more written in **modern C++**.<br/>
[Among others](https://github.com/skypjack/entt/wiki/EnTT-in-Action), it's used
in [**Minecraft**](https://minecraft.net/en-us/attribution/) by Mojang, the
[**ArcGIS Runtime SDKs**](https://developers.arcgis.com/arcgis-runtime/) by Esri
and the amazing [**Ragdoll**](https://ragdolldynamics.com/).<br/>
If you don't see your project in the list, please open an issue, submit a PR or
add the [\#entt](https://github.com/topics/entt) tag to your _topics_! :+1:

---

Do you want to **keep up with changes** or do you have a **question** that
doesn't require you to open an issue?<br/>
Join the [gitter channel](https://gitter.im/skypjack/entt) and the
[discord server](https://discord.gg/5BjPWBd), meet other users like you. The
more we are, the better for everyone.<br/>
Don't forget to check the
[FAQs](https://github.com/skypjack/entt/wiki/Frequently-Asked-Questions) and the
[wiki](https://github.com/skypjack/entt/wiki) too. Your answers may already be
there.

Do you want to support `EnTT`? Consider becoming a
[**sponsor**](https://github.com/users/skypjack/sponsorship) or making a
donation via [**PayPal**](https://www.paypal.me/skypjack).<br/>
Many thanks to [these people](https://skypjack.github.io/sponsorship/) and
**special** thanks to:

[![mojang](https://user-images.githubusercontent.com/1812216/106253145-67ca1980-6217-11eb-9c0b-d93561b37098.png)](https://mojang.com)
[![imgly](https://user-images.githubusercontent.com/1812216/106253726-271ed000-6218-11eb-98e0-c9c681925770.png)](https://img.ly/)

# Table of Contents

* [Introduction](#introduction)
  * [Code Example](#code-example)
  * [Motivation](#motivation)
  * [Benchmark](#benchmark)
* [Integration](#integration)
  * [Requirements](#requirements)
  * [CMake](#cmake)
  * [Natvis support](#natvis-support)
  * [Packaging Tools](#packaging-tools)
  * [pkg-config](#pkg-config)
* [Documentation](#documentation)
* [Tests](#tests)
* [EnTT in Action](#entt-in-action)
* [Contributors](#contributors)
* [License](#license)

# Introduction

The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

This project started off as a pure entity-component system. Over time the
codebase has grown as more and more classes and functionalities were added.<br/>
Here is a brief, yet incomplete list of what it offers today:

* Built-in **RTTI system** mostly similar to the standard one.
* A `constexpr` utility for human-readable **resource names**.
* Minimal **configuration system** built using the monostate pattern.
* Incredibly fast **entity-component system** with its own _pay for what you
  use_ policy, unconstrained component types with optional pointer stability and
  hooks for storage customization.
* Views and groups to iterate entities and components and allow different access
  patterns, from **perfect SoA** to fully random.
* A lot of **facilities** built on top of the entity-component system to help
  the users and avoid reinventing the wheel.
* General purpose **execution graph builder** for optimal scheduling.
* The smallest and most basic implementation of a **service locator** ever seen.
* A built-in, non-intrusive and macro-free runtime **reflection system**.
* **Static polymorphism** made simple and within everyone's reach.
* A few homemade containers, like a sparse set based **hash map**.
* A **cooperative scheduler** for processes of any type.
* All that is needed for **resource management** (cache, loaders, handles).
* Delegates, **signal handlers** and a tiny event dispatcher.
* A general purpose **event emitter** as a CRTP idiom based class template.
* And **much more**! Check out the
  [**wiki**](https://github.com/skypjack/entt/wiki).

Consider this list a work in progress as well as the project. The whole API is
fully documented in-code for those who are brave enough to read it.<br/>
Please, do note that all tools are also DLL-friendly now and run smoothly across
boundaries.

One thing known to most is that `EnTT` is also used in **Minecraft**.<br/>
Given that the game is available literally everywhere, I can confidently say 
that the library has been sufficiently tested on every platform that can come to 
mind.

## Code Example

```cpp
#include <entt/entt.hpp>

struct position {
    float x;
    float y;
};

struct velocity {
    float dx;
    float dy;
};

void update(entt::registry &registry) {
    auto view = registry.view<const position, velocity>();

    // use a callback
    view.each([](const auto &pos, auto &vel) { /* ... */ });

    // use an extended callback
    view.each([](const auto entity, const auto &pos, auto &vel) { /* ... */ });

    // use a range-for
    for(auto [entity, pos, vel]: view.each()) {
        // ...
    }

    // use forward iterators and get only the components of interest
    for(auto entity: view) {
        auto &vel = view.get<velocity>(entity);
        // ...
    }
}

int main() {
    entt::registry registry;

    for(auto i = 0u; i < 10u; ++i) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
    }

    update(registry);
}
```

## Motivation

I started developing `EnTT` for the _wrong_ reason: my goal was to design an
entity-component system to beat another well known open source library both in
terms of performance and possibly memory usage.<br/>
In the end, I did it, but it wasn't very satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my own library* at the same time.

Nowadays, `EnTT` is finally what I was looking for: still faster than its
_competitors_, lower memory usage in the average case, a really good API and an
amazing set of features. And even more, of course.

## Benchmark

For what it's worth, you'll **never** see me trying to make other projects look
bad or offer dubious comparisons just to make this library seem cooler.<br/>
I leave this activity to others, if they enjoy it (and it seems that some people
actually like it). I prefer to make better use of my time.

If you are interested, you can compile the `benchmark` test in release mode (to
enable compiler optimizations, otherwise it would make little sense) by setting
the `ENTT_BUILD_BENCHMARK` option of `CMake` to `ON`, then evaluate yourself
whether you're satisfied with the results or not.

There are also a lot of projects out there that use `EnTT` as a basis for
comparison (this should already tell you a lot). Many of these benchmarks are
completely wrong, many others are simply incomplete, good at omitting some
information and using the wrong function to compare a given feature. Certainly
there are also good ones but they age quickly if nobody updates them, especially
when the library they are dealing with is actively developed.<br/>
Out of all of them, [this](https://github.com/abeimler/ecs_benchmark) seems like
the most up-to-date project and also covers a certain number of libraries. I
can't say exactly whether `EnTT` is used correctly or not. However, even if used
poorly, it should still give the reader an idea of where it's going to operate.

# Integration

`EnTT` is a header-only library. This means that including the `entt.hpp` header
is enough to include the library as a whole and use it. For those who are
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

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that
supports at least C++17.<br/>
The requirements below are mandatory to compile the tests and to extract the
documentation:

* `CMake` version 3.7 or later.
* `Doxygen` version 1.8 or later.

Alternatively, [Bazel](https://bazel.build) is also supported as a build system
(credits to [zaucy](https://github.com/zaucy) who offered to maintain it).<br/>
In the documentation below I'll still refer to `CMake`, this being the official
build system of the library.

## CMake

To use `EnTT` from a `CMake` project, just link an existing target to the
`EnTT::EnTT` alias.<br/>
The library offers everything you need for locating (as in `find_package`),
embedding (as in `add_subdirectory`), fetching (as in `FetchContent`) or using
it in many of the ways that you can think of and that involve `CMake`.<br/>
Covering all possible cases would require a treatise and not a simple README
file, but I'm confident that anyone reading this section also knows what it's
about and can use `EnTT` from a `CMake` project without problems.

## Natvis support

When using `CMake`, just enable the option `ENTT_INCLUDE_NATVIS` and enjoy
it.<br/>
Otherwise, most of the tools are covered via Natvis and all files can be found
in the `natvis` directory, divided by module.<br/>
If you spot errors or have suggestions, any contribution is welcome!

## Packaging Tools

`EnTT` is available for some of the most known packaging tools. In particular:

* [`Conan`](https://github.com/conan-io/conan-center-index), the C/C++ Package
  Manager for Developers.

* [`vcpkg`](https://github.com/Microsoft/vcpkg), Microsoft VC++ Packaging
  Tool.<br/>
  You can download and install `EnTT` in just a few simple steps:

  ```
  $ git clone https://github.com/Microsoft/vcpkg.git
  $ cd vcpkg
  $ ./bootstrap-vcpkg.sh
  $ ./vcpkg integrate install
  $ vcpkg install entt
  ```

  Or you can use the `experimental` feature to test the latest changes:

  ```
  vcpkg install entt[experimental] --head
  ```

  The `EnTT` port in `vcpkg` is kept up to date by Microsoft team members and
  community contributors.<br/>
  If the version is out of date, please
  [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the
  `vcpkg` repository.

* [`Homebrew`](https://github.com/skypjack/homebrew-entt), the missing package
  manager for macOS.<br/>
  Available as a homebrew formula. Just type the following to install it:

  ```
  brew install skypjack/entt/entt
  ```

* [`build2`](https://build2.org), build toolchain for developing and packaging C
  and C++ code.<br/>
  In order to use the [`entt`](https://cppget.org/entt) package in a `build2`
  project, add the following line or a similar one to the `manifest` file:

  ```
  depends: entt ^3.0.0
  ```

  Also check that the configuration refers to a valid repository, so that the
  package can be found by `build2`:

  * [`cppget.org`](https://cppget.org), the open-source community central
    repository, accessible as `https://pkg.cppget.org/1/stable`.

  * [Package source repository](https://github.com/build2-packaging/entt):
    accessible as either `https://github.com/build2-packaging/entt.git` or
    `ssh://git@github.com/build2-packaging/entt.git`.
    Feel free to [report issues](https://github.com/build2-packaging/entt) with
    this package.

  Both can be used with `bpkg add-repo` or added in a project
  `repositories.manifest`. See the official
  [documentation](https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml#guide-repositories)
  for more details.

* [`bzlmod`](https://bazel.build/external/overview#bzlmod), Bazel's external
  dependency management system.<br/>
  To use the [`entt`](https://registry.bazel.build/modules/entt) module in a
  `bazel` project, add the following to your `MODULE.bazel` file:

  ```starlark
  bazel_dep(name = "entt", version = "3.12.2")
  ```

  EnTT will now be available as `@entt` (short for `@entt//:entt`) to be used
  in your `cc_*` rule `deps`. 

Consider this list a work in progress and help me to make it longer if you like.

## pkg-config

`EnTT` also supports `pkg-config` (for some definition of _supports_ at least).
A suitable file called `entt.pc` is generated and installed in a proper
directory when running `CMake`.<br/>
This should also make it easier to use with tools such as `Meson` or similar.

# Documentation

The documentation is based on [doxygen](http://www.doxygen.nl/). To build it:

    $ cd build
    $ cmake .. -DENTT_BUILD_DOCS=ON
    $ make

The API reference is created in HTML format in the `build/docs/html` directory.
To navigate it with your favorite browser:

    $ cd build
    $ your_favorite_browser docs/html/index.html

The same version is also available [online](https://skypjack.github.io/entt/)
for the latest release, that is the last stable tag.<br/>
Moreover, there exists a [wiki](https://github.com/skypjack/entt/wiki) dedicated
to the project where users can find all related documentation pages.

# Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
`cmake` downloads and compiles the library before compiling anything else. In
order to build the tests, set the `CMake` option `ENTT_BUILD_TESTING` to `ON`.

To build the most basic set of tests:

* `$ cd build`
* `$ cmake -DENTT_BUILD_TESTING=ON ..`
* `$ make`
* `$ make test`

Note that benchmarks are not part of this set.

# EnTT in Action

`EnTT` is widely used in private and commercial applications. I cannot even
mention most of them because of some signatures I put on some documents time
ago. Fortunately, there are also people who took the time to implement open
source projects based on `EnTT` and did not hold back when it came to
documenting them.

[Here](https://github.com/skypjack/entt/wiki/EnTT-in-Action) you can find an
incomplete list of games, applications and articles that can be used as a
reference.

If you know of other resources out there that are about `EnTT`, feel free to
open an issue or a PR and I'll be glad to add them to the list.

# Contributors

Requests for features, PRs, suggestions and feedback are highly appreciated.

If you find you can help and want to contribute to the project with your
experience or you do want to get part of the project for some other reason, feel
free to contact me directly (you can find the mail in the
[profile](https://github.com/skypjack)).<br/>
I can't promise that each and every contribution will be accepted, but I can
assure that I'll do my best to take them all as soon as possible.

If you decide to participate, please see the guidelines for
[contributing](https://github.com/skypjack/entt/blob/master/CONTRIBUTING.md)
before to create issues or pull requests.<br/>
Take also a look at the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to
know who has participated so far.

# License

Code and documentation Copyright (c) 2017-2024 Michele Caini.<br/>
Colorful logo Copyright (c) 2018-2021 Richard Caseres.

Code released under
[the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).<br/>
Documentation released under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).<br/>
All logos released under
[CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
