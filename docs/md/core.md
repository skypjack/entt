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
* [Any as in any type](#any-as-in-any-type)
  * [Small buffer optimization](#small-buffer-optimization)
  * [Alignment requirement](#alignment-requirement)
* [Type support](#type-support)
  * [Type info](#type-info)
    * [Almost unique identifiers](#almost-unique-identifiers)
  * [Type traits](#type-traits)
    * [Size of](#size-of)
    * [Is applicable](#is-applicable)
    * [Constness as](#constness-as)
    * [Member class type](#member-class-type)
    * [Integral constant](#integral-constant)
    * [Tag](#tag)
    * [Type list and value list](#type-list-and-value-list)
* [Utilities](#utilities)
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
original string through the `data` member function or converting the instance
into a number.<br/>
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
using namespace entt::literals;
constexpr auto str = "text"_hs;
```

To use it, remember that all user defined literals in `EnTT` are enclosed in the
`entt::literals` namespace. Therefore, the entire namespace or selectively the
literal of interest must be explicitly included before each use, a bit like
`std::literals`.<br/>
Finally, in case users need to create hashed strings at runtime, this class also
offers the necessary functionalities:

```cpp
std::string orig{"text"};

// create a full-featured hashed string...
entt::hashed_string str{orig.c_str()};

// ... or compute only the unique identifier
const auto hash = entt::hashed_string::value(orig.c_str());
```

This possibility shouldn't be exploited in tight loops, since the computation
takes place at runtime and no longer at compile-time and could therefore impact
performance to some degrees.

## Wide characters

The hashed string has a design that is close to that of an `std::basic_string`.
It means that `hashed_string` is nothing more than an alias for
`basic_hashed_string<char>`. For those who want to use the C++ type for wide
character representation, there exists also the alias `hashed_wstring` for
`basic_hashed_string<wchar_t>`.<br/>
In this case, the user defined literal to use to create hashed strings on the
fly is `_hws`:

```cpp
constexpr auto str = L"text"_hws;
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

# Any as in any type

`EnTT` comes with its own `any` type. It may seem redundant considering that
C++17 introduced `std::any`, but it is not (hopefully).<br/>
In fact, the _type_ returned by an `std::any` is a const reference to an
`std::type_info`, an implementation defined class that's not something everyone
wants to see in a software. Furthermore, there is no way to connect it with the
type system of the library and therefore with its integrated RTTI support.<br/>
Note that this class is largely used internally by the library itself.

The API is very similar to that of its most famous counterpart, mainly because
this class serves the same purpose of being an opaque container for any type of
value.<br/>
Instances of `any` also minimize the number of allocations by relying on a well
known technique called _small buffer optimization_ and a fake vtable.

Creating an object of the `any` type, whether empty or not, is trivial:

```cpp
// an empty container
entt::any empty{};

// a container for an int
entt::any any{0};

// in place construction
entt::any in_place{std::in_place_type<int>, 42};
```

The `any` class takes the burden of destroying the contained element when
required, regardless of the storage strategy used for the specific object.<br/>
Furthermore, an instance of `any` is not tied to an actual type. Therefore, the
wrapper will be reconfigured by assigning it an object of a different type than
the one contained, so as to be able to handle the new instance.<br/>
When in doubt about the type of object contained, the `type` member function of
`any` returns an instance of `type_info` associated with its element, or an
invalid `type_info` object if the container is empty. The type is also used
internally when comparing two `any` objects:

```cpp
if(any == empty) { /* ... */ }
```

In this case, before proceeding with a comparison, it's verified that the _type_
of the two objects is actually the same.<br/>
Refer to the `EnTT` type system documentation for more details on how
`type_info` works and on possible risks of a comparison.

A particularly interesting feature of this class is that it can also be used as
an opaque container for const and non-const references:

```cpp
int value = 42;

// reference construction
entt::any any{std::ref(value)};
entt::any cany{std::cref(value)};

// alias construction
int value = 42;
entt::any in_place{std::in_place_type<int &>, &value};
```

In other words, whenever `any` intercepts a `reference_wrapper` or is explicitly
told that users want to construct an alias, it acts as a pointer to the original
instance rather than making a copy of it or moving it internally. The contained
object is never destroyed and users must ensure that its lifetime exceeds that
of the container.<br/>
Similarly, it's possible to create non-owning copies of `any` from an existing
object:

```cpp
// aliasing constructor
entt::any ref = other.as_ref();
```

In this case, it doesn't matter if the original container actually holds an
object or acts already as a reference for unmanaged elements, the new instance
thus created won't create copies and will only serve as a reference for the
original item.<br/>
This means that, starting from the example above, both `ref` and` other` will
point to the same object, whether it's initially contained in `other` or already
an unmanaged element.

As a side note, it's worth mentioning that, while everything works transparently
when it comes to non-const references, there are some exceptions when it comes
to const references.<br/>
In particular, the `data` member function invoked on a non-const instance of
`any` that wraps a const reference will return a null pointer in all cases.

To cast an instance of `any` to a type, the library offers a set of `any_cast`
functions in all respects similar to their most famous counterparts.<br/>
The only difference is that, in the case of `EnTT`, these won't raise exceptions
but will only trigger an assert in debug mode, otherwise resulting in undefined
behavior in case of misuse in release mode.

## Small buffer optimization

The `any` class uses a technique called _small buffer optimization_ to reduce
the number of allocations where possible.<br/>
The default reserved size for an instance of `any` is `sizeof(double[2])`.
However, this is also configurable if needed. In fact, `any` is defined as an
alias for `basic_any<Len>`, where `Len` is the size above.<br/>
Users can easily set a custom size or define their own aliases:

```cpp
using my_any = entt::basic_any<sizeof(double[4])>;
```

This feature, in addition to allowing the choice of a size that best suits the
needs of an application, also offers the possibility of forcing dynamic creation
of objects during construction.<br/>
In other terms, if the size is 0, `any` avoids the use of any optimization and
always dynamically allocates objects (except for aliasing cases).

Note that the size of the internal storage as well as the alignment requirements
are directly part of the type and therefore contribute to define different types
that won't be able to interoperate with each other.

## Alignment requirement

The alignment requirement is optional and by default the most stringent (the
largest) for any object whose size is at most equal to the one provided.<br/>
The `basic_any` class template inspects the alignment requirements in each case,
even when not provided and may decide not to use the small buffer optimization
in order to meet them.

The alignment requirement is provided as an optional second parameter following
the desired size for the internal storage:

```cpp
using my_any = entt::basic_any<sizeof(double[4]), alignof(double[4])>;
```

Note that the alignment requirements as well as the size of the internal storage
are directly part of the type and therefore contribute to define different types
that won't be able to interoperate with each other.

# Type support

`EnTT` provides some basic information about types of all kinds.<br/>
It also offers additional features that are not yet available in the standard
library or that will never be.

## Type info

The `type_info` class isn't a drop-in replacement for `std::type_info` but can
provide similar information which are not implementation defined and don't
require to enable RTTI.<br/>
Therefore, they can sometimes be even more reliable than those obtained
otherwise.

A type info object is an opaque class that is also copy and move constructible.
This class is returned by the `type_id` function template:

```cpp
auto info = entt::type_id<a_type>();
```

These are the information made available by this object:

* The unique, sequential identifier associated with a given type:

  ```cpp
  auto index = entt::type_id<a_type>().seq();
  ```

  This is also an alias for the following:

  ```cpp
  auto index = entt::type_seq<a_type>::value();
  ```

  The returned value isn't guaranteed to be stable across different runs.
  However, it can be very useful as index in associative and unordered
  associative containers or for positional accesses in a vector or an array.
  
  So as not to conflict with the other tools available, the `family` class isn't
  used to generate these indexes. Therefore, the numeric identifiers returned by
  the two tools may differ.<br/>
  On the other hand, this leaves users with full powers over the `family` class
  and therefore the generation of custom runtime sequences of indices for their
  own purposes, if necessary.
  
  An external generator can also be used if needed. In fact, `type_seq` can be
  specialized by type and is also _sfinae-friendly_ in order to allow more
  refined specializations such as:
  
  ```cpp
  template<typename Type>
  struct entt::type_seq<Type, std::void_d<decltype(Type::index())>> {
      static entt::id_type value() ENTT_NOEXCEPT {
          return Type::index();
      }
  };
  ```
  
  Note that indexes **must** still be generated sequentially in this case.<br/>
  The tool is widely used within `EnTT`. Generating indices not sequentially
  would break an assumption and would likely lead to undesired behaviors.

* The hash value associated with a given type:

  ```cpp
  auto hash = entt::type_id<a_type>().hash();
  ```

  This is also an alias for the following:

  ```cpp
  auto hash = entt::type_hash<a_type>::value();
  ```

  In general, the `value` function exposed by `type_hash` is also `constexpr`
  but this isn't guaranteed for all compilers and platforms (although it's valid
  with the most well-known and popular ones).<br/>
  The `hash` function offered by the type info object isn't `constexpr` in any
  case instead.

  This function **can** use non-standard features of the language for its own
  purposes. This makes it possible to provide compile-time identifiers that
  remain stable across different runs.<br/>
  In all cases, users can prevent the library from using these features by means
  of the `ENTT_STANDARD_CPP` definition. In this case, there is no guarantee
  that identifiers remain stable across executions. Moreover, they are generated
  at runtime and are no longer a compile-time thing.

  As for `type_seq`, also `type_hash` is a _sfinae-friendly_ class that can be
  specialized in order to customize its behavior globally or on a per-type or
  per-traits basis.

* The name associated with a given type:

  ```cpp
  auto name = entt::type_id<my_type>().name();
  ```

  This is also an alias for the following:

  ```cpp
  auto name = entt::type_name<a_type>::value();
  ```

  The name associated with a type is extracted from some information generally
  made available by the compiler in use. Therefore, it may differ depending on
  the compiler and may be empty in the event that this information isn't
  available.<br/>
  For example, given the following class:

  ```cpp
  struct my_type { /* ... */ };
  ```

  The name is `my_type` when compiled with GCC or CLang and `struct my_type`
  when MSVC is in use.<br/>
  Most of the time the name is also retrieved at compile-time and is therefore
  always returned through an `std::string_view`. Users can easily access it and
  modify it as needed, for example by removing the word `struct` to standardize
  the result. `EnTT` won't do this for obvious reasons, since it requires
  copying and creating a new string potentially at runtime.

  This function **can** use non-standard features of the language for its own
  purposes. Users can prevent the library from using non-standard features by
  means of the `ENTT_STANDARD_CPP` definition. In this case, the name will be
  empty by default.

  As for `type_seq`, also `type_name` is a _sfinae-friendly_ class that can be
  specialized in order to customize its behavior globally or on a per-type or
  per-traits basis.

### Almost unique identifiers

Since the default non-standard, compile-time implementation of `type_hash` makes
use of hashed strings, it may happen that two types are assigned the same hash
value.<br/>
In fact, although this is quite rare, it's not entirely excluded.

Another case where two types are assigned the same identifier is when classes
from different contexts (for example two or more libraries loaded at runtime)
have the same fully qualified name. In this case, also `type_name` will return
the same value for the two types.<br/>
Fortunately, there are several easy ways to deal with this:

* The most trivial one is to define the `ENTT_STANDARD_CPP` macro. Runtime
  identifiers don't suffer from the same problem in fact. However, this solution
  doesn't work well with a plugin system, where the libraries aren't linked.

* Another possibility is to specialize the `type_name` class for one of the
  conflicting types, in order to assign it a custom identifier. This is probably
  the easiest solution that also preserves the feature of the tool.

* A fully customized identifier generation policy (based for example on enum
  classes or preprocessing steps) may represent yet another option.

These are just some examples of possible approaches to the problem but there are
many others. As already mentioned above, since users have full control over
their types, this problem is in any case easy to solve and should not worry too
much.<br/>
In all likelihood, it will never happen to run into a conflict anyway.

## Type traits

A handful of utilities and traits not present in the standard template library
but which can be useful in everyday life.<br/>
This list **is not** exhaustive and contains only some of the most useful
classes. Refer to the inline documentation for more information on the features
offered by this module.

### Size of

The standard operator `sizeof` complains when users provide it for example with
function or incomplete types. On the other hand, it's guaranteed that its result
is always nonzero, even if applied to an empty class type.<br/>
This small class combines the two and offers an alternative to `sizeof` that
works under all circumstances, returning zero if the type isn't supported:

```cpp
const auto size = entt::size_of_v<void>;
```

### Is applicable

The standard library offers the great `std::is_invocable` trait in several
forms. This takes a function type and a series of arguments and returns true if
the condition is satisfied.<br/>
Moreover, users are also provided with `std::apply`, a tool for combining
invocable elements and tuples of arguments.

It would therefore be a good idea to have a variant of `std::is_invocable` that
also accepts its arguments in the form of a tuple-like type, so as to complete
the offer:

```cpp
constexpr bool result = entt::is_applicable<Func, std::tuple<a_type, another_type>>;
```

This trait is built on top of `std::is_invocable` and does nothing but unpack a
tuple-like type and simplify the code at the call site.

### Constness as

An utility to easily transfer the constness of a type to another type:

```cpp
// type is const dst_type because of the constness of src_type
using type = entt::constness_as_t<dst_type, const src_type>;
```

The trait is subject to the rules of the language. Therefore, for example,
transferring constness between references won't give the desired effect.

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

### Tag

Since `id_type` is very important and widely used in `EnTT`, there is a more
user-friendly shortcut for the creation of integral constants based on it.<br/>
This shortcut is the alias template `entt::tag`.

If used in combination with hashed strings, it helps to use human-readable names
where types would be required otherwise. As an example:

```cpp
registry.emplace<entt::tag<"enemy"_hs>>(entity);
```

However, this isn't the only permitted use. Literally any value convertible to
`id_type` is a good candidate, such as the named constants of an unscoped enum.

### Type list and value list

There is no respectable library where the much desired _type list_ can be
missing.<br/>
`EnTT` is no exception and provides (making extensive use of it internally) the
`type_list` type, in addition to its `value_list` counterpart dedicated to
non-type template parameters.

Here is a (possibly incomplete) list of the functionalities that come with a
type list:

* `type_list_element[_t]` to get the N-th element of a type list.
* `type_list_cast[_t]` and a handy `operator+` to concatenate type lists.
* `type_list_unique[_t]` to remove duplicate types from a type list.
* `type_list_contains[_v]` to know if a type list contains a given type.
* `type_list_diff[_t]` to remove types from type lists.

I'm also pretty sure that more and more utilities will be added over time as
needs become apparent.<br/>
Many of these functionalities also exist in their version dedicated to value
lists. We therefore have `value_list_element[_v]` as well as
`value_list_cat[_t]`and so on.

# Utilities

It's not possible to escape the temptation to add utilities of some kind to a
library. In fact, `EnTT` also provides a handful of tools to simplify the
life of developers:

* `entt::identity`: the identity function object that will be available with
  C++20. It returns its argument unchanged and nothing more. It's useful as a
  sort of _do nothing_ function in template programming.

* `entt::overload`: a tool to disambiguate different overloads from their
  function type. It works with both free and member functions.<br/>
  Consider the following definition:

  ```cpp
  struct clazz {
      void bar(int) {}
      void bar() {}
  };
  ```

  This utility can be used to get the _right_ overload as:

  ```cpp
  auto *member = entt::overload<void(int)>(&clazz::bar);
  ```

  The line above is literally equivalent to:

  ```cpp
  auto *member = static_cast<void(clazz:: *)(int)>(&clazz::bar);
  ```

  Just easier to read and shorter to type.

* `entt::overloaded`: a small class template used to create a new type with an
  overloaded `operator()` from a bunch of lambdas or functors.<br/>
  As an example:

  ```cpp
  entt::overloaded func{
      [](int value) { /* ... */ },
      [](char value) { /* ... */ }
  };

  func(42);
  func('c');
  ```

  Rather useful when doing metaprogramming and having to pass to a function a
  callable object that supports multiple types at once.

* `entt::y_combinator`: this is a C++ implementation of **the** _y-combinator_.
  If it's not clear what it is, there is probably no need for this utility.<br/>
  Below is a small example to show its use:

  ```cpp
  entt::y_combinator gauss([](const auto &self, auto value) -> unsigned int {
      return value ? (value + self(value-1u)) : 0;
  });

  const auto result = gauss(3u);
  ```

  Maybe convoluted at a first glance but certainly effective. Unfortunately,
  the language doesn't make it possible to do much better.

This is a rundown of the (actually few) utilities made available by `EnTT`. The
list will probably grow over time but the size of each will remain rather small,
as has been the case so far.
