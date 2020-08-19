![EnTT: Gaming meets modern C++](https://user-images.githubusercontent.com/1812216/42513718-ee6e98d0-8457-11e8-9baf-8d83f61a3097.png)

<!--
@cond TURN_OFF_DOXYGEN
-->
[![GitHub version](https://badge.fury.io/gh/skypjack%2Fentt.svg)](https://github.com/skypjack/entt/releases)
[![Build Status](https://github.com/skypjack/entt/workflows/build/badge.svg)](https://github.com/skypjack/entt/actions)
[![Coverage](https://codecov.io/gh/skypjack/entt/branch/master/graph/badge.svg)](https://codecov.io/gh/skypjack/entt)
[![Try online](https://img.shields.io/badge/try-online-brightgreen)](https://godbolt.org/z/cOUcm1)
[![Documentation](https://img.shields.io/badge/docs-docsforge-blue)](http://entt.docsforge.com/)
[![Gitter chat](https://badges.gitter.im/skypjack/entt.png)](https://gitter.im/skypjack/entt)
[![Discord channel](https://img.shields.io/discord/707607951396962417?logo=discord)](https://discord.gg/5BjPWBd)
[![Donate](https://img.shields.io/badge/donate-paypal-blue.svg)](https://www.paypal.me/skypjack)

`EnTT` is a header-only, tiny and easy to use library for game programming and
much more written in **modern C++**, mainly known for its innovative
**entity-component-system (ECS)** model.<br/>
[Among others](https://github.com/skypjack/entt/wiki/EnTT-in-Action), it's used
in [**Minecraft**](https://minecraft.net/en-us/attribution/) by Mojang and the
[**ArcGIS Runtime SDKs**](https://developers.arcgis.com/arcgis-runtime/) by
Esri.<br/>
If you don't see your project in the list, please open an issue, submit a PR or
add the [#entt](https://github.com/topics/entt) tag to your _topics_! :+1:

---

Do you want to **keep up with changes** or do you have a **question** that
doesn't require you to open an issue?<br/>
Join the [gitter channel](https://gitter.im/skypjack/entt) or the
[discord server](https://discord.gg/5BjPWBd) and meet other users like you. The
more we are, the better for everyone.

Wondering why your **debug build** is so slow on Windows or how to represent a
**hierarchy** with components?<br/>
Check out the
[FAQ](https://github.com/skypjack/entt/wiki/Frequently-Asked-Questions) and the
[wiki](https://github.com/skypjack/entt/wiki) if you have these or other doubts,
your answers may already be there.

If you use `EnTT` and you want to say thanks or support the project, please
**consider becoming a
[sponsor](https://github.com/users/skypjack/sponsorship)**.<br/>
You can help me make the difference.
[Many thanks](https://skypjack.github.io/sponsorship/) to those who supported me
and still support me today.

# Table of Contents

* [Introduction](#introduction)
  * [Code Example](#code-example)
  * [Motivation](#motivation)
  * [Performance](#performance)
* [Build Instructions](#build-instructions)
  * [Requirements](#requirements)
  * [Library](#library)
  * [Documentation](#documentation)
  * [Tests](#tests)
* [Packaging Tools](#packaging-tools)
* [EnTT in Action](#entt-in-action)
* [Contributors](#contributors)
* [License](#license)
* [Support](#support)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

This project started off as a pure entity-component system. Over time the
codebase has grown as more and more classes and functionalities were added.<br/>
Here is a brief, yet incomplete list of what it offers today:

* Statically generated integer **identifiers** for types (assigned either at
  compile-time or at runtime).
* A `constexpr` utility for human readable **resource names**.
* A minimal **configuration system** built using the monostate pattern.
* An incredibly fast **entity-component system** based on sparse sets, with its
  own _pay for what you use_ policy to adjust performance and memory usage
  according to the users' requirements.
* Views and groups to iterate entities and components and allow different access
  patterns, from **perfect SoA** to fully random.
* A lot of **facilities** built on top of the entity-component system to help
  the users and avoid reinventing the wheel (dependencies, snapshot, actor
  class, support for **reactive systems** and so on).
* The smallest and most basic implementation of a **service locator** ever seen.
* A built-in, non-intrusive and macro-free runtime **reflection system**.
* A **cooperative scheduler** for processes of any type.
* All that is needed for **resource management** (cache, loaders, handles).
* Delegates, **signal handlers** (with built-in support for collectors) and a
  tiny event dispatcher for immediate and delayed events to integrate in loops.
* A general purpose **event emitter** as a CRTP idiom based class template.
* And **much more**! Check out the
  [**wiki**](https://github.com/skypjack/entt/wiki).

Consider this list a work in progress as well as the project. The whole API is
fully documented in-code for those who are brave enough to read it.

It is also known that `EnTT` (version 3) is used in **Minecraft**.<br/>
Given that the game is available literally everywhere, I can confidently say 
that the library has been sufficiently tested on every platform that can come to 
mind.

## Code Example

```cpp
#include <entt/entt.hpp>
#include <cstdint>

struct position {
    float x;
    float y;
};

struct velocity {
    float dx;
    float dy;
};

void update(entt::registry &registry) {
    auto view = registry.view<position, velocity>();

    for(auto entity: view) {
        // gets only the components that are going to be used ...

        auto &vel = view.get<velocity>(entity);

        vel.dx = 0.;
        vel.dy = 0.;

        // ...
    }
}

void update(std::uint64_t dt, entt::registry &registry) {
    registry.view<position, velocity>().each([dt](auto &pos, auto &vel) {
        // gets all the components of the view at once ...

        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;

        // ...
    });
}

int main() {
    entt::registry registry;
    std::uint64_t dt = 16;

    for(auto i = 0; i < 10; ++i) {
        auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
    }

    update(dt, registry);
    update(registry);

    // ...
}
```

## Motivation

I started developing `EnTT` for the _wrong_ reason: my goal was to design an
entity-component system to beat another well known open source solution both in
terms of performance and possibly memory usage.<br/>
In the end, I did it, but it wasn't very satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my own library* at the same time.

Nowadays, `EnTT` is finally what I was looking for: still faster than its
_competitors_, lower memory usage in the average case, a really good API and an
amazing set of features. And even more, of course.

## Performance

The proposed entity-component system is incredibly fast to iterate entities and
components, this is a fact. Some compilers make a lot of optimizations because
of how `EnTT` works, some others aren't that good. In general, if we consider
real world cases, `EnTT` is somewhere between a bit and much faster than many of
the other solutions around, although I couldn't check them all for obvious
reasons.

If you are interested, you can compile the `benchmark` test in release mode (to
enable compiler optimizations, otherwise it would make little sense) by setting
the `BUILD_BENCHMARK` option of `CMake` to `ON`, then evaluate yourself whether
you're satisfied with the results or not.

Honestly I got tired of updating the README file whenever there is an
improvement.<br/>
There are already a lot of projects out there that use `EnTT` as a basis for
comparison (this should already tell you a lot). Many of these benchmarks are
completely wrong, many others are simply incomplete, good at omitting some
information and using the wrong function to compare a given feature. Certainly
there are also good ones but they age quickly if nobody updates them, especially
when the library they are dealing with is actively developed.

The choice to use `EnTT` should be based on its carefully designed API, its
set of features and the general performance, **not** because some single
benchmark shows it to be the fastest tool available.

In the future I'll likely try to get even better performance while still adding
new features, mainly for fun.<br/>
If you want to contribute and/or have suggestions, feel free to make a PR or
open an issue to discuss your idea.

# Build Instructions

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

If you are looking for a C++14 version of `EnTT`, check out the git tag `cpp14`.

## Library

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

## Documentation

The documentation is based on [doxygen](http://www.doxygen.nl/).
To build it:

    $ cd build
    $ cmake .. -DBUILD_DOCS=ON
    $ make

The API reference will be created in HTML format within the directory
`build/docs/html`. To navigate it with your favorite browser:

    $ cd build
    $ your_favorite_browser docs/html/index.html

<!--
@cond TURN_OFF_DOXYGEN
-->
The same version is also available [online](https://skypjack.github.io/entt/)
for the latest release, that is the last stable tag. If you are looking for
something more pleasing to the eye, consider reading the nice-looking version
available on [docsforge](https://entt.docsforge.com/): same documentation, much
more pleasant to read.<br/>
Moreover, there exists a [wiki](https://github.com/skypjack/entt/wiki) dedicated
to the project where users can find all related documentation pages.
<!--
@endcond TURN_OFF_DOXYGEN
-->

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
`cmake` will download and compile the library before compiling anything else.
In order to build the tests, set the CMake option `BUILD_TESTING` to `ON`.

To build the most basic set of tests:

* `$ cd build`
* `$ cmake -DBUILD_TESTING=ON ..`
* `$ make`
* `$ make test`

Note that benchmarks are not part of this set.

# Packaging Tools

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

Consider this list a work in progress and help me to make it longer.

<!--
@cond TURN_OFF_DOXYGEN
-->
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

`EnTT` was written initially as a faster alternative to other well known and
open source entity-component systems. Nowadays this library is moving its first
steps. Much more will come in the future and hopefully I'm going to work on it
for a long time.<br/>
Requests for features, PR, suggestions ad feedback are highly appreciated.

If you find you can help me and want to contribute to the project with your
experience or you do want to get part of the project for some other reasons,
feel free to contact me directly (you can find the mail in the
[profile](https://github.com/skypjack)).<br/>
I can't promise that each and every contribution will be accepted, but I can
assure that I'll do my best to take them all seriously.

If you decide to participate, please see the guidelines for
[contributing](CONTRIBUTING.md) before to create issues or pull
requests.<br/>
Take also a look at the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to
know who has participated so far.
<!--
@endcond TURN_OFF_DOXYGEN
-->

# License

Code and documentation Copyright (c) 2017-2020 Michele Caini.<br/>
Logo Copyright (c) 2018-2020 Richard Caseres.

Code released under
[the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).
Documentation released under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).<br/>
Logo released under
[CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

<!--
@cond TURN_OFF_DOXYGEN
-->
# Support

If you want to support this project, you can
[offer me](https://github.com/users/skypjack/sponsorship) an espresso.<br/>
If you find that it's not enough, feel free to
[help me](https://www.paypal.me/skypjack) the way you prefer.
<!--
@endcond TURN_OFF_DOXYGEN
-->
