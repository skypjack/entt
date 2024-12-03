# Similar projects

# Table of Contents

* [Introduction](#introduction)
* [Similar projects](#similar-projects)

# Introduction

There are many projects similar to `EnTT`, both open source and not.<br/>
Some even borrowed some ideas from this library and expressed them in different
languages.<br/>
Others developed different architectures from scratch and therefore offer
alternative solutions with their pros and cons.

If you know of other similar projects out there, feel free to open an issue or a
PR and I'll be glad to add them to this page.<br/>
I hope the following lists can grow much more in the future.

# Similar projects

Below an incomplete list of similar projects that I've come across so far.<br/>
If some terms or designs aren't clear, I recommend referring to the
[_ECS Back and Forth_](https://skypjack.github.io/tags/#ecs) series for all the
details.

* C:
  * [destral_ecs](https://github.com/roig/destral_ecs): a single-file ECS based
    on sparse sets.
  * [Diana](https://github.com/discoloda/Diana): an ECS that uses sparse sets to
    keep track of entities in systems.
  * [Flecs](https://github.com/SanderMertens/flecs): a multithreaded archetype
    ECS based on semi-contiguous arrays rather than chunks.
  * [lent](https://github.com/nem0/lent): the Donald Trump of the ECS libraries.

* C++:
  * [decs](https://github.com/vblanco20-1/decs): a chunk based archetype ECS.
  * [ecst](https://github.com/SuperV1234/ecst): a multithreaded compile-time
    ECS that uses sparse sets to keep track of entities in systems.
  * [EntityX](https://github.com/alecthomas/entityx): a bitset based ECS that
    uses a single large matrix of components indexed with entities.
  * [Gaia-ECS](https://github.com/richardbiely/gaia-ecs): a chunk based
    archetype ECS.
  * [Polypropylene](https://github.com/pmbittner/Polypropylene): a hybrid
    solution between an ECS and dynamic mixins.

* C#
  * [Arch](https://github.com/genaray/Arch): a simple, fast and _unity entities_
    inspired archetype ECS with optional multithreading. 
  * [Entitas](https://github.com/sschmid/Entitas-CSharp): the ECS framework for
    C# and Unity, where _reactive systems_ were invented.
  * [Friflo ECS](https://github.com/friflo/Friflo.Engine.ECS): an archetype ECS
    with focus on performance and minimal GC allocations.
  * [LeoECS](https://github.com/Leopotam/ecs): simple lightweight C# Entity
    Component System framework.
  * [Massive ECS](https://github.com/nilpunch/massive): sparse set based ECS
    featuring rollbacks.
  * [Svelto.ECS](https://github.com/sebas77/Svelto.ECS): a very interesting
    platform agnostic and table based ECS framework.

* Go:
  * [gecs](https://github.com/tutumagi/gecs): a sparse sets based ECS inspired 
    by `EnTT`.

* Javascript:
  * [\@javelin/ecs](https://github.com/3mcd/javelin/tree/master/packages/ecs):
    an archetype ECS in TypeScript.
  * [ecsy](https://github.com/MozillaReality/ecsy): I haven't had the time to
    investigate the underlying design of `ecsy` but it looks cool anyway.

* Perl:
  * [Game::Entities](https://gitlab.com/jjatria/perl-game-entities): a simple
    entity registry for ECS designs inspired by `EnTT`.

* Raku:
  * [Game::Entities](https://gitlab.com/jjatria/raku-game-entities): a simple
    entity registry for ECS designs inspired by `EnTT`.

* Rust:
  * [Shipyard](https://github.com/leudz/shipyard): it borrows some ideas from
    `EnTT` and offers a sparse sets based ECS with grouping functionalities.
  * [Sparsey](https://github.com/LechintanTudor/sparsey): sparse set based ECS
    written in Rust.
  * [Specs](https://github.com/amethyst/specs): a parallel ECS based mainly on
    hierarchical bitsets that allows different types of storage as needed.

* Zig
  * [zig-ecs](https://github.com/prime31/zig-ecs): a _zig-ification_ of `EnTT`.
