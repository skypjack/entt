# Crash Course: User provided STL

# Table of Contents

* [Introduction](#introduction)
* [The required tools](#the-required-tools)
* [STL injection](#stl-injection)
  * [Discoverability](#discoverability)
  * [Importability](#importability)
* [Final note](#final-note)

# Introduction

`EnTT` internally _decouples_ the standard template library from the `std`
namespace. It does this by introducing the internal `stl` namespace into which
the necessary tools are _imported_ from `std`.<br/>
This import process is also configurable, and the end user can replace all or
some of the standard template library includes by providing their own
implementations.

# The required tools

The list of _what_ is imported from `std` is not fixed and stable over time, but
changes as the library evolves. At any time, users can easily know what is
required and what is not.<br/>
The easiest way to avoid errors is to provide everything a standard header
provides. However, it's also a good idea to look at its counterpart in the `stl`
folder and implement only the tools actually used by `EnTT`.<br/>
There's no guarantee that these won't change over time. Therefore, the first
approach is the safest way to avoid surprises.

# STL injection

Includes from the standard template library can be _injected_ into `EnTT` in
bulk or individually. As long as the API and guarantees remain the same, there
will be no problem using different sources or mixing different
implementations.<br/>
However, there are two fundamental requirements for the library to be able to
_find_ the user files and use them correctly.

## Discoverability

To begin with, the files to inject must be discoverable via `__has_include` in
the `entt/ext/stl/` path, and have the extension `.hpp`.<br/>
For example, to replace `<bit>`, users need to provide a resolvable file such as
`<entt/ext/stl/bit.hpp>`.

This is fairly simple to do on the command line and can be easily automated with
tools like `CMake`.<br/>
The `EnTT` test suite provides an example of how to proceed, also designed to
avoid future regressions.

## Importability 

Having all the required files is not enough, since `EnTT` does not know the
namespace in which the user defined their tools. Therefore, anyone providing the
new headers must also import all the tools into the `entt::stl` namespace.<br/>
Importing from outside into this namespace is accepted and, in a sense, expected
by the library itself, unlike what happens with the standard C++ library.

The easiest solution is to include a custom implementation and _import_ it into
the required namespace via the files indicated.<br/>
An `using namespace my_stl;` directive should get the job done, but users can
also import the tools one by one if they prefer to.

# Final note

`EnTT` is designed to work with the API, guarantees, and pros and cons of the
standard template library.<br/>
The flexibility afforded by the ability to _import_ a custom implementation does
not compromise this requirement. Therefore, in the event of an incompatible API
(not _different_, but _incompatible_) or broken requirements, there is no
guarantee that `EnTT` will:

* Compile correctly
* Perform correctly

Obviously, any errors due to incorrect implementations or broken guarantees are
not attributable to the library, and no follow-up will be given to any issues
arising from these types of problems.
