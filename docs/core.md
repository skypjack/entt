# Crash Course: core functionalities

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Compile-time identifiers](#compile-time-identifiers)
* [Runtime identifiers](#runtime-identifiers)
* [Hashed strings](#hashed-strings)
  * [Conflicts](#conflicts)
* [Monostate](#monostate)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` comes with a bunch of core functionalities mostly used by the other parts
of the library itself.<br/>
Hardly users will include these features in their code, but it's worth
describing what `EnTT` offers so as not to reinvent the wheel in case of need.

# Compile-time identifiers

Sometimes it's useful to be able to give unique identifiers to types at
compile-time.<br/>
There are plenty of different solutions out there and I could have used one of
them. However, I decided to spend my time to define a compact and versatile tool
that fully embraces what the modern C++ has to offer.

The _result of my efforts_ is the `identifier` class template:

```cpp
#include <ident.hpp>

// defines the identifiers for the given types
using id = entt::identifier<a_type, another_type>;

// ...

switch(a_type_identifier) {
case id::type<a_type>:
    // ...
    break;
case id::type<another_type>:
    // ...
    break;
default:
    // ...
}
```

This is all what the class template has to offer: a `type` inline variable that
contains a numerical identifier for the given type. It can be used in any
context where constant expressions are required.

As long as the list remains unchanged, identifiers are also guaranteed to be the
same for every run. In case they have been used in a production environment and
a type has to be removed, one can just use a placeholder to left the other
identifiers unchanged:

```cpp
template<typename> struct ignore_type {};

using id = entt::identifier<
    a_type_still_valid,
    ignore_type<a_type_no_longer_valid>,
    another_type_still_valid
>;
```

A bit ugly to see, but it works at least.

# Runtime identifiers

Sometimes it's useful to be able to give unique identifiers to types at
runtime.<br/>
There are plenty of different solutions out there and I could have used one of
them. In fact, I adapted the most common one to my requirements and used it
extensively within the entire library.

It's the `family` class. Here is an example of use directly from the
entity-component system:

```cpp
using component_family = entt::family<struct internal_registry_component_family>;

// ...

template<typename Component>
component_type component() const noexcept {
    return component_family::type<Component>;
}
```

This is all what a _family_ has to offer: a `type` inline variable that contains
a numerical identifier for the given type.

Please, note that identifiers aren't guaranteed to be the same for every run.
Indeed it mostly depends on the flow of execution.

# Hashed strings

A hashed string is a zero overhead unique identifier. Users can use
human-readable identifiers in the codebase while using their numeric
counterparts at runtime, thus without affecting performance.<br/>
The class has an implicit `constexpr` constructor that chews a bunch of
characters. Once created, all what one can do with it is getting back the
original string or converting it into a number.<br/>
The good part is that a hashed string can be used wherever a constant expression
is required and no _string-to-number_ conversion will take place at runtime if
used carefully.

Example of use:

```cpp
auto load(entt::hashed_string::hash_type resource) {
    // uses the numeric representation of the resource to load and return it
}

auto resource = load(entt::hashed_string{"gui/background"});
```

There is also a _user defined literal_ dedicated to hashed strings to make them
more user-friendly:

```cpp
constexpr auto str = "text"_hs;
```

## Conflicts

The hashed string class uses internally FNV-1a to compute the numeric
counterpart of a string. Because of the _pigeonhole principle_, conflicts are
possible. This is a fact.<br/>
There is no silver bullet to solve the problem of conflicts when dealing with
hashing functions. In this case, the best solution seemed to be to give up.
That's all.<br/>
After all, human-readable unique identifiers aren't something strictly defined
and over which users have not the control. Choosing a slightly different
identifier is probably the best solution to make the conflict disappear in this
case.

# Monostate

The monostate pattern is often presented as an alternative to a singleton based
configuration system. This is exactly its purpose in `EnTT`. Moreover, this
implementation is thread safe by design (hopefully).<br/>
Keys are represented by hashed strings, values are basic types like `int`s or
`bool`s. Values of different types can be associated to each key, even more than
one at a time. Because of this, users must pay attention to use the same type
both during an assignment and when they try to read back their data. Otherwise,
they will probably incur in unexpected results.

Example of use:

```cpp
entt::monostate<entt::hashed_string{"mykey"}>{} = true;
entt::monostate<"mykey"_hs>{} = 42;

// ...

const bool b = entt::monostate<"mykey"_hs>{};
const int i = entt::monostate<entt::hashed_string{"mykey"}>{};
```
