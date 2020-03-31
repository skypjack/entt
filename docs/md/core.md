# Crash Course: core functionalities

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Unique sequential identifiers](#unique-sequential-identifiers)
  * [Compile-time generator](#compile-time-generator)
  * [Runtime generator](#runtime-generator)
* [Hashed strings](#hashed-strings)
  * [Wide characters](wide-characters)
  * [Conflicts](#conflicts)
* [Monostate](#monostate)
* [Type support](#type-support)
  * [Type info](#type-info)
    * [Almost unique identifiers](#almost-unique-identifiers)
  * [Type index](#type-index)
  * [Type traits](#type-traits)
    * [Member class type](#member-class-type)
    * [Integral constant](#integral-constant)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` comes with a bunch of core functionalities mostly used by the other parts
of the library itself.<br/>
Hardly users will include these features in their code, but it's worth
describing what `EnTT` offers so as not to reinvent the wheel in case of need.

# Unique sequential identifiers

Sometimes it's useful to be able to give unique, sequential numeric identifiers
to types either at compile-time or runtime.<br/>
There are plenty of different solutions for this out there and I could have used
one of them. However, I decided to spend my time to define a couple of tools
that fully embraces what the modern C++ has to offer.

## Compile-time generator

To generate sequential numeric identifiers at compile-time, `EnTT` offers the
`identifier` class template:

```cpp
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

This is all what this class template has to offer: a `type` inline variable that
contains a numeric identifier for the given type. It can be used in any context
where constant expressions are required.

As long as the list remains unchanged, identifiers are also guaranteed to be
stable across different runs. In case they have been used in a production
environment and a type has to be removed, one can just use a placeholder to left
the other identifiers unchanged:

```cpp
template<typename> struct ignore_type {};

using id = entt::identifier<
    a_type_still_valid,
    ignore_type<a_type_no_longer_valid>,
    another_type_still_valid
>;
```

Perhaps a bit ugly to see in a codebase but it gets the job done at least.

## Runtime generator

To generate sequential numeric identifiers at runtime, `EnTT` offers the
`family` class template:

```cpp
// defines a custom generator
using id = entt::family<struct my_tag>;

// ...

const auto a_type_id = id::type<a_type>;
const auto another_type_id = id::type<another_type>;
```

This is all what a _family_ has to offer: a `type` inline variable that contains
a numeric identifier for the given type.<br/>
The generator is customizable, so as to get different _sequences_ for different
purposes if needed.

Please, note that identifiers aren't guaranteed to be stable across different
runs. Indeed it mostly depends on the flow of execution.

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

## Wide characters

The hashed string has a design that is close to that of an `std::basic_string`.
It means that `hashed_string` is nothing more than an alias for
`basic_hashed_string<char>`. For those who want to use the C++ type for wide
character representation, there exists also the alias `hashed_wstring` for
`basic_hashed_string<wchar_t>`.<br/>
In this case, the user defined literal to use to create hashed strings on the
fly is `_hws`:

```cpp
constexpr auto str = "text"_hws;
```

Note that the hash type of the `hashed_wstring` is the same of its counterpart.

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

# Type support

`EnTT` provides some basic information about types of all kinds.<br/>
It also offers additional features that are not yet available in the standard
library or that will never be.

## Type info

This class template isn't a drop-in replacement for `std::type_info` but can
provide similar information which are not implementation defined at least.
Therefore, they can sometimes be even more reliable than those obtained otherwise.

Currently, the only information available is the numeric identifier associated
with a given type:

```cpp
auto id = entt::type_info<my_type>::id();
```

In general, the `id` function is also `constexpr` but this isn't guaranteed for
all compilers and platforms (although it's valid with the most well-known and
popular ones).<br/>
This function **can** use non-standard features of the language for its own
purposes. This allows it to provide compile-time identifiers that remain stable
across different runs. However, it's possible to force the use of standard
features only by defining the macro `ENTT_STANDARD_CPP`. In this case, there is
no guarantee that the identifiers are stable across executions though. Moreover,
identifiers are generated at runtime and are no longer a compile-time thing.

An external type system can also be used if needed. In fact, `type_info` can be
specialized by type and is also _sfinae-friendly_ in order to allow more refined
specializations such as:

```cpp
template<typename Type>
struct entt::type_info<Type, std::void_d<decltype(Type::custom_id())>> {
    static constexpr ENTT_ID_TYPE id() ENTT_NOEXCEPT {
        return Type::custom_id();
    }
};
```

Note that this class template and its specializations are widely used within
`EnTT`. It also plays a very important role in making `EnTT` work transparently
across boundaries in many cases.<br/>
Please refer to the dedicated section for more details.

### Almost unique identifiers

Since the default non-standard, compile-time implementation makes use of hashed
strings, it may happen that two types are assigned the same numeric
identifier.<br/>
In fact, although this is quite rare, it's not entirely excluded.

Another case where two types are assigned the same identifier is when classes
from different contexts (for example two or more libraries loaded at runtime)
have the same fully qualified name.<br/>
If the types have the same name and belong to the same namespace then their
identifiers _could_ be identical (they won't necessarily be the same though).

Fortunately, there are several easy ways to deal with this:

* The most trivial one is to define the `ENTT_STANDARD_CPP` macro. Runtime
  identifiers don't suffer from the same problem in fact. However, this solution
  doesn't work well with a plugin system, where the libraries aren't linked.

* Another possibility is to specialize the `type_info` class for one of the
  conflicting types, in order to assign it a custom identifier. This is probably
  the easiest solution that also preserves the feature of the tool.

* A fully customized identifier generation policy (based for example on enum
  classes or preprocessing steps) may represent yet another option.

These are just some examples of possible approaches to the problem but there are
many others. As already mentioned above, since users have full control over
their types, this problem is in any case easy to solve and should not worry too
much.<br/>
In all likelihood, it will never happen to run into a conflict anyway.

## Type index

Types in `EnTT` are assigned also unique, sequential _indexes_ generated at
runtime:

```cpp
auto index = entt::type_index<my_type>::value();
```

This value may differ from the numeric identifier of a type and isn't guaranteed
to be stable across different runs. However, it can be very useful as index in
associative and unordered associative containers or for positional accesses in a
vector or an array.

So as not to conflict with the other tools available, the `family` class isn't
used to generate these indexes. Therefore, the numeric identifiers returned by
the two tools may differ.<br/>
On the other hand, this leaves users with full powers over the `family` class
and therefore the generation of custom runtime sequences of indices for their
own purposes, if necessary.

An external generator can also be used if needed. In fact, `type_index` can be
specialized by type and is also _sfinae-friendly_ in order to allow more refined
specializations such as:

```cpp
template<typename Type>
struct entt::type_index<Type, std::void_d<decltype(Type::index())>> {
    static id_type value() ENTT_NOEXCEPT {
        return Type::index();
    }
};
```

Note that indexes **must** still be generated sequentially in this case.<br/>
The tool is widely used within `EnTT`. It also plays a very important role in
making `EnTT` work nicely across boundaries in many cases. Generating indices
not sequentially would break an assumption and would likely lead to undesired
behaviors.

## Type traits

A handful of utilities and traits not present in the standard template library
but which can be useful in everyday life.

### Member class type

The `auto` template parameter introduced with C++17 made it possible to simplify
many class templates and template functions but also made the class type opaque
when members are passed as template arguments.<br/>
The purpose of this utility is to extract the class type in a few lines of code:

```cpp
template<typename Member>
using clazz = entt::member_class_t<Member>;
```

### Integral constant

Since `std::integral_constant` may be annoying because of its form that requires
to specify both a type and a value of that type, there is a more user-friendly
shortcut for the creation of integral constants.<br/>
This shortcut is the alias template `entt::integral_constant`:

```cpp
constexpr auto constant = entt::integral_constant<42>;
```

Among the other uses, when combined with a hashed string it helps to define tags
as human-readable _names_ where actual types would be required otherwise:

```cpp
constexpr auto enemy_tag = entt::integral_constant<"enemy"_hs>;
registry.emplace<enemy_tag>(entity);
```
