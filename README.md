![EnTT: Gaming meets modern C++](https://user-images.githubusercontent.com/1812216/42513718-ee6e98d0-8457-11e8-9baf-8d83f61a3097.png)

<!--
@cond TURN_OFF_DOXYGEN
-->
[![GitHub version](https://badge.fury.io/gh/skypjack%2Fentt.svg)](http://badge.fury.io/gh/skypjack%2Fentt)
[![LoC](https://tokei.rs/b1/github/skypjack/entt)](https://github.com/skypjack/entt)
[![Build Status](https://travis-ci.org/skypjack/entt.svg?branch=master)](https://travis-ci.org/skypjack/entt)
[![Build status](https://ci.appveyor.com/api/projects/status/rvhaabjmghg715ck?svg=true)](https://ci.appveyor.com/project/skypjack/entt)
[![Coverage Status](https://coveralls.io/repos/github/skypjack/entt/badge.svg?branch=master)](https://coveralls.io/github/skypjack/entt?branch=master)
[![Gitter chat](https://badges.gitter.im/skypjack/entt.png)](https://gitter.im/skypjack/entt)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://www.patreon.com/bePatron?u=11330786)

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
  * [Patreon](#patreon)
  * [Donation](#donation)
  * [Hire me](#hire-me)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` is a header-only, tiny and easy to use entity-component system (and much
more) written in modern C++ and even
[used by Mojang in Minecraft](https://minecraft.net/en-us/attribution/).<br/>
The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

A long time ago, the sole entity-component system was part of the project. After
a while the codebase has grown and more and more classes have become part of the
repository.<br/>
Here is a brief, yet incomplete list of what it offers today:

* Statically generated integer **identifiers for types** (assigned either at
  **compile-time** or at **runtime**).
* A **constexpr utility** for **human readable resource identifiers**.
* A minimal **configuration system** built on top of the **monostate pattern**.
* **An incredibly fast entity-component system** based on sparse sets, with its
  own views and a _pay for what you use_ policy to adjust performance and memory
  usage according to users' requirements.
* A lot of **facilities built on top of the entity-component system** to help
  developers and avoid reinventing the wheel (ie **dependencies**, **snapshot**,
  **actor class** for those who aren't confident with the architecture and so
  on).
* The smallest and most basic implementation of a **service locator** ever seen.
* A built-in, non-intrusive and macro-free **runtime reflection system**.
* A **cooperative scheduler** for processes of any type.
* All what is needed for **resource management** (cache, loaders, handles).
* **Delegates**, **signal handlers** (with built-in support for collectors) and
  a tiny **event dispatcher** for immediate and delayed events to integrate in
  loops.
* A general purpose **event emitter** as a CRTP idiom based class template.
* And **much more**! Check out the
  [**wiki**](https://github.com/skypjack/entt/wiki).

Consider it a work in progress. The whole API is also fully documented in-code
for those who are brave enough to read it.

Currently, `EnTT` is tested on Linux, Microsoft Windows and OS X. It has proven
to work also on both Android and iOS.<br/>
Most likely it will not be problematic on other systems as well, but has not
been sufficiently tested so far.

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

void update(entt::registry<> &registry) {
    auto view = registry.view<position, velocity>();

    for(auto entity: view) {
        // gets only the components that are going to be used ...

        auto &vel = view.get<velocity>(entity);

        vel.dx = 0.;
        vel.dy = 0.;

        // ...
    }
}

void update(std::uint64_t dt, entt::registry<> &registry) {
    registry.view<position, velocity>().each([dt](const auto, auto &pos, auto &vel) {
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
        registry.assign<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { registry.assign<velocity>(entity, i * .1f, i * .1f); }
    }

    update(dt, registry);
    update(registry);

    // ...
}
```

## Motivation

I started working on `EnTT` because of the wrong reason: my goal was to design
an entity-component system that beated another well known open source solution
in terms of performance and used (possibly) less memory in the average
case.<br/>
In the end, I did it, but it wasn't much satisfying. Actually it wasn't
satisfying at all. The fastest and nothing more, fairly little indeed. When I
realized it, I tried hard to keep intact the great performance of `EnTT` and to
add all the features I wanted to see in *my own library* at the same time.

Nowadays, `EnTT` is finally what I was looking for: still faster than its
_competitors_, lower memory usage in the average case, a really good API and an
amazing set of features. And even more, of course.

## Performance

As it stands right now, `EnTT` is just fast enough for my requirements if
compared to my first choice (it was already amazingly fast actually).<br/>
Below is a comparison between the two (both of them compiled with GCC 7.3.0 on a
Dell XPS 13 out of the mid 2014):

| Benchmark | EntityX (compile-time) | EnTT |
|-----------|-------------|-------------|
| Create 1M entities | 0.0147s | **0.0046s** |
| Destroy 1M entities | 0.0053s | **0.0045s** |
| 1M entities, one component | 0.0012s | **1.9e-07s** |
| 1M entities, two components | 0.0012s | **3.8e-07s** |
| 1M entities, two components<br/>Half of the entities have all the components | 0.0009s | **3.8e-07s** |
| 1M entities, two components<br/>One of the entities has all the components | 0.0008s | **1.0e-06s** |
| 1M entities, five components | 0.0010s | **7.0e-07s** |
| 1M entities, ten components | 0.0011s | **1.2e-06s** |
| 1M entities, ten components<br/>Half of the entities have all the components | 0.0010s | **1.2e-06s** |
| 1M entities, ten components<br/>One of the entities has all the components | 0.0008s | **1.2e-06s** |
| Sort 150k entities, one component<br/>Arrays are in reverse order | - | **0.0036s** |
| Sort 150k entities, enforce permutation<br/>Arrays are in reverse order | - | **0.0005s** |
| Sort 150k entities, one component<br/>Arrays are almost sorted, std::sort | - | **0.0035s** |
| Sort 150k entities, one component<br/>Arrays are almost sorted, insertion sort | - | **0.0007s** |

Note: The default version of `EntityX` (`master` branch) wasn't added to the
comparison because it's already much slower than its compile-time counterpart.

Pretty interesting, aren't them? In fact, these benchmarks are the same used by
`EntityX` to show _how fast it is_. To be honest, they aren't so good and these
results shouldn't be taken much seriously (they are completely unrealistic
indeed).<br/>
The proposed entity-component system is incredibly fast to iterate entities,
this is a fact. The compiler can make a lot of optimizations because of how
`EnTT` works, even more when components aren't used at all. This is exactly the
case for these benchmarks. On the other hand and if we consider real world
cases, `EnTT` is in the middle between a bit and much faster than the other
solutions around when users also access the components and not just the
entities, although it is not as fast as reported by these benchmarks.<br/>
This is why they are completely wrong and cannot be used to evaluate any of the
entity-component systems.

If you decide to use `EnTT`, choose it because of its API, features and
performance, not because there is a benchmark somewhere that makes it seem the
fastest.

Probably I'll try to get out of `EnTT` more features and even better performance
in the future, mainly for fun.<br/>
If you want to contribute and/or have any suggestion, feel free to make a PR or
open an issue to discuss your idea.

# Build Instructions

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that
supports at least C++17.<br/>
The requirements below are mandatory to compile the tests and to extract the
documentation:

* CMake version 3.2 or later.
* Doxygen version 1.8 or later.

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

The documentation is based on [doxygen](http://www.stack.nl/~dimitri/doxygen/).
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
It's also available [online](https://skypjack.github.io/entt/) for the latest
version.<br/>
Finally, there exists a [wiki](https://github.com/skypjack/entt/wiki) dedicated
to the project where users can find all related documentation pages.
<!--
@endcond TURN_OFF_DOXYGEN
-->

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
`cmake` will download and compile the library before compiling anything else.
In order to build without tests set CMake option `BUILD_TESTING=OFF`.

To build the most basic set of tests:

* `$ cd build`
* `$ cmake ..`
* `$ make`
* `$ make test`

Note that benchmarks are not part of this set.

# Packaging Tools

`EnTT` is available for some of the most known packaging tools. In particular:

* [`Conan`](https://bintray.com/skypjack/conan/entt%3Askypjack/_latestVersion),
  the C/C++ Package Manager for Developers.
* [`Homebrew`](https://github.com/skypjack/homebrew-entt), the missing package
  manager for macOS.<br/>
  Available as a homebrew formula. Just type the following to install it:
  ```
  brew install skypjack/entt/entt
  ```
* [`vcpkg`](https://github.com/Microsoft/vcpkg/tree/master/ports/entt),
  Microsoft VC++ Packaging Tool.

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
[contributing](docs/CONTRIBUTING.md) before to create issues or pull
requests.<br/>
Take also a look at the
[contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to
know who has participated so far.
<!--
@endcond TURN_OFF_DOXYGEN
-->

# License

Code and documentation Copyright (c) 2017-2018 Michele Caini.<br/>
Logo Copyright (c) 2018 Richard Caseres.

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

## Patreon

Become a [patron](https://www.patreon.com/bePatron?c=1772573) and get access to
extra content, help me spend more time on the projects you love and create new
ones for you. Your support will help me to continue the work done so far and
make it more professional and feature-rich every day.<br/>
It takes very little to
[become a patron](https://www.patreon.com/bePatron?c=1772573) and thus help the
software you use every day. Don't miss the chance.

## Donation

Developing and maintaining `EnTT` takes some time and lots of coffee. I'd like
to add more and more functionalities in future and turn it in a full-featured
solution.<br/>
If you want to support this project, you can offer me an espresso. I'm from
Italy, we're used to turning the best coffee ever in code. If you find that
it's not enough, feel free to support me the way you prefer.<br/>
Take a look at the donation button at the top of the page for more details or
just click [here](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=W2HF9FESD5LJY&lc=IT&item_name=Michele%20Caini&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).

## Hire me

If you start using `EnTT` and need help, if you want a new feature and want me
to give it the highest priority, if you have any other reason to contact me:
do not hesitate. I'm available for hiring.<br/>
Feel free to take a look at my [profile](https://github.com/skypjack) and
contact me by mail.
<!--
@endcond TURN_OFF_DOXYGEN
-->
