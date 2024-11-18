# Crash Course: core functionalities

# Table of Contents

* [Introduction](#introduction)
* [Any as in any type](#any-as-in-any-type)
  * [Small buffer optimization](#small-buffer-optimization)
  * [Alignment requirement](#alignment-requirement)
* [Bit](#bit)
* [Compressed pair](#compressed-pair)
* [Enum as bitmask](#enum-as-bitmask)
* [Hashed strings](#hashed-strings)
  * [Wide characters](wide-characters)
  * [Conflicts](#conflicts)
* [Iterators](#iterators)
  * [Input iterator pointer](#input-iterator-pointer)
  * [Iota iterator](#iota-iterator)
  * [Iterable adaptor](#iterable-adaptor)
* [Memory](#memory)
  * [Allocator aware unique pointers](#allocator-aware-unique-pointers)
* [Monostate](#monostate)
* [Type support](#type-support)
  * [Built-in RTTI support](#built-in-rtti-support)
    * [Type info](#type-info)
    * [Almost unique identifiers](#almost-unique-identifiers)
  * [Type traits](#type-traits)
    * [Size of](#size-of)
    * [Is applicable](#is-applicable)
    * [Constness as](#constness-as)
    * [Member class type](#member-class-type)
    * [N-th argument](#n-th-argument)
    * [Integral constant](#integral-constant)
    * [Tag](#tag)
    * [Type list and value list](#type-list-and-value-list)
* [Unique sequential identifiers](#unique-sequential-identifiers)
  * [Compile-time generator](#compile-time-generator)
  * [Runtime generator](#runtime-generator)
* [Utilities](#utilities)

# Introduction

`EnTT` comes with a bunch of core functionalities mostly used by the other parts
of the library.<br/>
Many of these tools are also useful in everyday work. Therefore, it's worth
describing them so as not to reinvent the wheel in case of need.

# Any as in any type

`EnTT` offers its own `any` type. It may seem redundant considering that C++17
introduced `std::any`, but it is not (hopefully).<br/>
First of all, the _type_ returned by an `std::any` is a const reference to an
`std::type_info`, an implementation defined class that's not something everyone
wants to see in a software. Furthermore, there is no way to bind it to the type
system of the library and therefore with its integrated RTTI support.

The `any` API is very similar to that of its most famous counterpart, mainly
because this class serves the same purpose of being an opaque container for any
type of value.<br/>
Instances also minimize the number of allocations by relying on a well known
technique called _small buffer optimization_ and a fake vtable.

Creating an object of the `any` type, whether empty or not, is trivial:

```cpp
// an empty container
entt::any empty{};

// a container for an int
entt::any any{0};

// in place type construction
entt::any in_place_type{std::in_place_type<int>, 42};

// take ownership of already existing, dynamically allocated objects
entt::any in_place{std::in_place, std::make_unique<int>(42).release()};
```

Alternatively, the `make_any` function serves the same purpose. It requires to
always be explicit about the type and doesn't support taking ownership:

```cpp
entt::any any = entt::make_any<int>(42);
```

In all cases, the `any` class takes the burden of destroying the contained
element when required, regardless of the storage strategy used for the specific
object.<br/>
Furthermore, an instance of `any` isn't tied to an actual type. Therefore, the
wrapper is reconfigured when it's assigned a new object of a type other than
the one it contains.

There is also a way to directly assign a value to the variable contained by an
`entt::any`, without necessarily replacing it. This is especially useful when
the object is used in _aliasing mode_, as described below:

```cpp
entt::any any{42};
entt::any value{3};

// assigns by copy
any.assign(value);

// assigns by move
any.assign(std::move(value));
```

The `any` class performs a check on the type information and whether or not the
original type was copy or move assignable, as appropriate.<br/>
In all cases, the `assign` function returns a boolean value that is true in case
of success and false otherwise.

When in doubt about the type of object contained, the `type` member function
returns a const reference to the `type_info` associated with its element, or
`type_id<void>()` if the container is empty.<br/>
The type is also used internally when comparing two `any` objects:

```cpp
if(any == empty) { /* ... */ }
```

In this case, before proceeding with a comparison, it's verified that the _type_
of the two objects is actually the same.<br/>
Refer to the `EnTT` type system documentation for more details about how
`type_info` works and the possible risks of a comparison.

A particularly interesting feature of this class is that it can also be used as
an opaque container for const and non-const references:

```cpp
int value = 42;

entt::any any{std::in_place_type<int &>(value)};
entt::any cany = entt::make_any<const int &>(value);
entt::any fwd = entt::forward_as_any(value);

any.emplace<const int &>(value);
```

In other words, whenever `any` is explicitly told to construct an _alias_, it
acts as a pointer to the original instance rather than making a copy of it or
moving it internally. The contained object is never destroyed and users must
ensure that its lifetime exceeds that of the container.<br/>
Similarly, it's possible to create non-owning copies of `any` from an existing
object:

```cpp
// aliasing constructor
entt::any ref = other.as_ref();
```

In this case, it doesn't matter if the original container actually holds an
object or is as a reference for unmanaged elements already. The new instance
thus created doesn't create copies and only serves as a reference for the
original item.

It's worth mentioning that, while everything works transparently when it comes
to non-const references, there are some exceptions when it comes to const
references.<br/>
In particular, the `data` member function invoked on a non-const instance of
`any` that wraps a const reference returns a null pointer in all cases.

To cast an instance of `any` to a type, the library offers a set of `any_cast`
functions in all respects similar to their most famous counterparts.<br/>
The only difference is that, in the case of `EnTT`, they won't raise exceptions
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
In other terms, if the size is 0, `any` suppresses the small buffer optimization
and always dynamically allocates objects (except for aliasing cases).

## Alignment requirement

The alignment requirement is optional and by default the most stringent (the
largest) for any object whose size is at most equal to the one provided.<br/>
It's provided as an optional second parameter following the desired size for the
internal storage:

```cpp
using my_any = entt::basic_any<sizeof(double[4]), alignof(double[4])>;
```

The `basic_any` class template inspects the alignment requirements in each case,
even when not provided and may decide not to use the small buffer optimization
in order to meet them.

# Bit

Finding out the population count of an unsigned integral value (`popcount`),
whether a number is a power of two or not (`has_single_bit`) as well as the next
power of two given a random value (`next_power_of_two`) can be useful.<br/>
For example, it helps to allocate memory in pages having a size suitable for the
fast modulus:

```cpp
const std::size_t result = entt::fast_mod(value, modulus);
```

Where `modulus` is necessarily a power of two. Perhaps not everyone knows that
this type of operation is far superior in terms of performance to the basic
modulus and for this reason preferred in many areas.

# Compressed pair

Primarily designed for internal use and far from being feature complete, the
`compressed_pair` class does exactly what it promises: it tries to reduce the
size of a pair by exploiting _Empty Base Class Optimization_ (or _EBCO_).<br/>
This class **is not** a drop-in replacement for `std::pair`. However, it offers
enough functionalities to be a good alternative for when reducing memory usage
is more important than having some cool and probably useless feature.

Although the API is very close to that of `std::pair` (apart from the fact that
the template parameters are inferred from the constructor and therefore there is
no `entt::make_compressed_pair`), the major difference is that `first` and
`second` are functions for implementation requirements:

```cpp
entt::compressed_pair pair{0, 3.};
pair.first() = 42;
```

There isn't much to describe then. It's recommended to rely on documentation and
intuition. At the end of the day, it's just a pair and nothing more.

# Enum as bitmask

Sometimes it's useful to be able to use enums as bitmasks. However, enum classes
aren't really suitable for the purpose. Main problem is that they don't convert
implicitly to their underlying type.<br/>
The choice is then between using old-fashioned enums (with all their problems
that I don't want to discuss here) or writing _ugly_ code.

Fortunately, there is also a third way: adding enough operators in the global
scope to treat enum classes as bitmasks transparently.<br/>
The ultimate goal is to write code like the following (or maybe something more
meaningful, but this should give a grasp and remain simple at the same time):

```cpp
enum class my_flag {
    unknown = 0x01,
    enabled = 0x02,
    disabled = 0x04
};

const my_flag flags = my_flag::enabled;
const bool is_enabled = !!(flags & my_flag::enabled);
```

The problem with adding all operators to the global scope is that these come
into play even when not required, with the risk of introducing errors that are
difficult to deal with.<br/>
However, C++ offers enough tools to get around this problem. In particular, the
library requires users to register the enum classes for which bitmask support
should be enabled:

```cpp
template<>
struct entt::enum_as_bitmask<my_flag>
    : std::true_type
{};
```

This is handy when dealing with enum classes defined by third party libraries
and over which the user has no control. However, it's also verbose and can be
avoided by adding a specific value to the enum class itself:

```cpp
enum class my_flag {
    unknown = 0x01,
    enabled = 0x02,
    disabled = 0x04,
    _entt_enum_as_bitmask
};
```

In this case, there is no need to specialize the `enum_as_bitmask` traits, since
`EnTT` automatically detects the flag and enables the bitmask support.<br/>
Once the enum class is registered (in one way or the other), the most common
operators such as `&`, `|` but also `&=` and `|=` are available for use.

Refer to the official documentation for the full list of operators.

# Hashed strings

Hashed strings are human-readable identifiers in the codebase that turn into
numeric values at runtime, thus without affecting performance.<br/>
The class has an implicit `constexpr` constructor that chews a bunch of
characters. Once created, one can get the original string by means of the `data`
member function or convert the instance into a number.<br/>
A hashed string is well suited wherever a constant expression is required. No
_string-to-number_ conversion will take place at runtime if used carefully.

Example of use:

```cpp
auto load(entt::hashed_string::hash_type resource) {
    // uses the numeric representation of the resource to load and return it
}

auto resource = load(entt::hashed_string{"gui/background"});
```

There is also a _user defined literal_ dedicated to hashed strings to make them
more _user-friendly_:

```cpp
using namespace entt::literals;
constexpr auto str = "text"_hs;
```

User defined literals in `EnTT` are enclosed in the `entt::literals` namespace.
Therefore, the entire namespace or selectively the literal of interest must be
explicitly included before each use, a bit like `std::literals`.<br/>
The class also offers the necessary functionalities to create hashed strings at
runtime:

```cpp
std::string orig{"text"};

// create a full-featured hashed string...
entt::hashed_string str{orig.c_str()};

// ... or compute only the unique identifier
const auto hash = entt::hashed_string::value(orig.c_str());
```

This possibility shouldn't be exploited in tight loops, since the computation
takes place at runtime and no longer at compile-time. It could therefore affect
performance to some degrees.

## Wide characters

The `hashed_string` class is an alias  for `basic_hashed_string<char>`. To use
the C++ type for wide character representations, there exists also the alias
`hashed_wstring` for `basic_hashed_string<wchar_t>`.<br/>
In this case, the user defined literal to use to create hashed strings on the
fly is `_hws`:

```cpp
constexpr auto str = L"text"_hws;
```

The hash type of `hashed_wstring` is the same as its counterpart.

## Conflicts

The hashed string class uses FNV-1a internally to hash strings. Because of the
_pigeonhole principle_, conflicts are possible. This is a fact.<br/>
There is no silver bullet to solve the problem of conflicts when dealing with
hashing functions. In this case, the best solution is likely to give up. That's
all.<br/>
After all, human-readable unique identifiers aren't something strictly defined
and over which users have not the control. Choosing a slightly different
identifier is probably the best solution to make the conflict disappear in this
case.

# Iterators

Writing and working with iterators isn't always easy. More often than not it
also leads to duplicated code.<br/>
`EnTT` tries to overcome this problem by offering some utilities designed to
make this hard work easier.

## Input iterator pointer

When writing an input iterator that returns in-place constructed values if
dereferenced, it's not always straightforward to figure out what `value_type` is
and how to make it behave like a full-fledged pointer.<br/>
Conversely, it would be very useful to have an `operator->` available on the
iterator itself that always works without too much complexity.

The input iterator pointer is meant for this. It's a small class that wraps the
in-place constructed value and adds some functions on top of it to make it
suitable for use with input iterators: 

```cpp
struct iterator_type {
    using value_type = std::pair<first_type, second_type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    // ...
}
```

The library makes extensive use of this class internally. In many cases, the
`value_type` of the returned iterators is just an input iterator pointer.

## Iota iterator

Waiting for C++20, this iterator accepts an integral value and returns all
elements in a certain range:

```cpp
entt::iota_iterator first{0};
entt::iota_iterator last{100};

for(; first != last; ++first) {
    int value = *first;
    // ...
}
```

In the future, views will replace this class. Meanwhile, the library makes some
interesting uses of it when a range of integral values is to be returned to the
user.

## Iterable adaptor

Typically, a container class provides `begin` and `end` member functions (with
their const counterparts) for iteration.<br/>
However, it can happen that a class offers multiple iteration methods or allows
users to iterate different sets of _elements_.

The iterable adaptor is a utility class that makes it easier to use and access
data in this case.<br/>
It accepts a couple of iterators (or an iterator and a sentinel) and offers an
_iterable_ object with all the expected methods like `begin`, `end` and whatnot.

The library uses this class extensively.<br/>
Think for example of views, which can be iterated to access entities but also
offer a method of obtaining an iterable object that returns tuples of entities
and components at once.<br/>
Another example is the registry class which allows users to iterate its storage
by returning an iterable object for the purpose.

# Memory

There are a handful of tools within `EnTT` to interact with memory in one way or
another.<br/>
Some are geared towards simplifying the implementation of (internal or external)
allocator aware containers. Others are designed to help the developer with
everyday problems.

The former are very specific and for niche problems. These are tools designed to
unwrap fancy or plain pointers (`to_address`) or to help forget the meaning of
acronyms like _POCCA_, _POCMA_ or _POCS_.<br/>
I won't describe them here in detail. Instead, I recommend reading the inline
documentation to those interested in the subject.

## Allocator aware unique pointers

A nasty thing in C++ (at least up to C++20) is the fact that shared pointers
support allocators while unique pointers don't.<br/>
There is a proposal at the moment that also shows (among the other things) how
this can be implemented without any compiler support.

The `allocate_unique` function follows this proposal, making a virtue out of
necessity:

```cpp
std::unique_ptr<my_type, entt::allocation_deleter<my_type>> ptr = entt::allocate_unique<my_type>(allocator, arguments);
```

Although the internal implementation is slightly different from what is proposed
for the standard, this function offers an API that is a drop-in replacement for
the same feature.

# Monostate

The monostate pattern is often presented as an alternative to a singleton based
configuration system.<br/>
This is exactly its purpose in `EnTT`. Moreover, this implementation is thread
safe by design (hopefully).

Keys are integral values (easily obtained by hashed strings), values are basic
types like `int`s or `bool`s. Values of different types can be associated with
each key, even more than one at a time.<br/>
Because of this, one should pay attention to use the same type both during an
assignment and when trying to read back the data. Otherwise, there is the risk
to incur in unexpected results.

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

## Built-in RTTI support

Runtime type identification support (or RTTI) is one of the most frequently
disabled features in the C++ world, especially in the gaming sector. Regardless
of the reasons for this, it's often a shame not to be able to rely on opaque
type information at runtime.<br/>
The library tries to fill this gap by offering a built-in system that doesn't
serve as a replacement but comes very close to being one and offers similar
information to that provided by its counterpart.

Basically, the whole system relies on a handful of classes. In particular:

* The unique sequential identifier associated with a given type:

  ```cpp
  auto index = entt::type_index<a_type>::value();
  ```

  The returned value isn't guaranteed to be stable across different runs.<br/>
  However, it can be very useful as index in associative and unordered
  associative containers or for positional accesses in a vector or an array.
  
  An external generator can also be used if needed. In fact, `type_index` can be
  specialized by type and is also _sfinae-friendly_ in order to allow more
  refined specializations such as:
  
  ```cpp
  template<typename Type>
  struct entt::type_index<Type, std::void_d<decltype(Type::index())>> {
      static entt::id_type value() noexcept {
          return Type::index();
      }
  };
  ```
  
  Indexes **must** be sequentially generated in this case.<br/>
  The tool is widely used within `EnTT`. Generating indices not sequentially
  would break an assumption and would likely lead to undesired behaviors.

* The hash value associated with a given type:

  ```cpp
  auto hash = entt::type_hash<a_type>::value();
  ```

  In general, the `value` function exposed by `type_hash` is also `constexpr`
  but this isn't guaranteed for all compilers and platforms (although it's valid
  with the most well-known and popular ones).

  This function **can** use non-standard features of the language for its own
  purposes. This makes it possible to provide compile-time identifiers that
  remain stable across different runs.<br/>
  Users can prevent the library from using these features by means of the
  `ENTT_STANDARD_CPP` definition. In this case, there is no guarantee that
  identifiers remain stable across executions. Moreover, they are generated
  at runtime and are no longer a compile-time thing.

  As it happens with `type_index`, also `type_hash` is a _sfinae-friendly_ class
  that can be specialized in order to customize its behavior globally or on a
  per-type or per-traits basis.

* The name associated with a given type:

  ```cpp
  auto name = entt::type_name<a_type>::value();
  ```

  This value is extracted from some information generally made available by the
  compiler in use. Therefore, it may differ depending on the compiler and may be
  empty in the event that this information isn't available.<br/>
  For example, given the following class:

  ```cpp
  struct my_type { /* ... */ };
  ```

  The name is `my_type` when compiled with GCC or CLang and `struct my_type`
  when MSVC is in use.<br/>
  Most of the time the name is also retrieved at compile-time and is therefore
  always returned through an `std::string_view`. Users can easily access it and
  modify it as needed, for example by removing the word `struct` to normalize
  the result. `EnTT` doesn't do this for obvious reasons, since it would be
  creating a new string at runtime otherwise.

  This function **can** use non-standard features of the language for its own
  purposes. Users can prevent the library from using these features by means of
  the `ENTT_STANDARD_CPP` definition. In this case, the name is just empty.

  As it happens with `type_index`, also `type_name` is a _sfinae-friendly_ class
  that can be specialized in order to customize its behavior globally or on a
  per-type or per-traits basis.

These are then combined into utilities that aim to offer an API that is somewhat
similar to that made available by the standard library.

### Type info

The `type_info` class isn't a drop-in replacement for `std::type_info` but can
provide similar information which are not implementation defined and don't
require to enable RTTI.<br/>
Therefore, they can sometimes be even more reliable than those obtained
otherwise.

Its type defines an opaque class that is also copyable and movable.<br/>
Objects of this type are generally returned by the `type_id` functions:

```cpp
// by type
auto info = entt::type_id<a_type>();

// by value
auto other = entt::type_id(42);
```

All elements thus received are nothing more than const references to instances
of `type_info` with static storage duration.<br/>
This is convenient for saving the entire object aside for the cost of a pointer.
However, nothing prevents from constructing `type_info` objects directly:

```cpp
entt::type_info info{std::in_place_type<int>};
```

These are the information made available by `type_info`:

* The index associated with a given type:

  ```cpp
  auto idx = entt::type_id<a_type>().index();
  ```

  This is also an alias for the following:

  ```cpp
  auto idx = entt::type_index<std::remove_cv_t<std::remove_reference_t<a_type>>>::value();
  ```

* The hash value associated with a given type:

  ```cpp
  auto hash = entt::type_id<a_type>().hash();
  ```

  This is also an alias for the following:

  ```cpp
  auto hash = entt::type_hash<std::remove_cv_t<std::remove_reference_t<a_type>>>::value();
  ```

* The name associated with a given type:

  ```cpp
  auto name = entt::type_id<my_type>().name();
  ```

  This is also an alias for the following:

  ```cpp
  auto name = entt::type_name<std::remove_cv_t<std::remove_reference_t<a_type>>>::value();
  ```

Where all accessed features are available at compile-time, the `type_info` class
is also fully `constexpr`. However, this cannot be guaranteed in advance and
depends mainly on the compiler in use and any specializations of the classes
described above.

### Almost unique identifiers

Since the default non-standard, compile-time implementation of `type_hash` makes
use of hashed strings, it may happen that two types are assigned the same hash
value.<br/>
In fact, although this is quite rare, it's not entirely excluded.

Another case where two types are assigned the same identifier is when classes
from different contexts (for example two or more libraries loaded at runtime)
have the same fully qualified name. In this case, `type_name` returns the same
value for the two types.<br/>
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

The standard operator `sizeof` complains if users provide it with functions or
incomplete types. On the other hand, it's guaranteed that its result is always
non-zero, even if applied to an empty class type.<br/>
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

A utility to easily transfer the constness of a type to another type:

```cpp
// type is const dst_type because of the constness of src_type
using type = entt::constness_as_t<dst_type, const src_type>;
```

The trait is subject to the rules of the language. For example, _transferring_
constness between references won't give the desired effect.

### Member class type

The `auto` template parameter introduced with C++17 made it possible to simplify
many class templates and template functions but also made the class type opaque
when members are passed as template arguments.<br/>
The purpose of this utility is to extract the class type in a few lines of code:

```cpp
template<typename Member>
using clazz = entt::member_class_t<Member>;
```

### N-th argument

A utility to quickly find the n-th argument of a function, member function or
data member (for blind operations on opaque types):

```cpp
using type = entt::nth_argument_t<1u, decltype(&clazz::member)>;
```

Disambiguation of overloaded functions is the responsibility of the user, should
it be needed.

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

Type `id_type` is very important and widely used in `EnTT`. Therefore, there is
a more user-friendly shortcut for the creation of constants based on it.<br/>
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
* `type_list_index[_v]` to get the index of a given element of a type list.
* `type_list_cat[_t]` and a handy `operator+` to concatenate type lists.
* `type_list_unique[_t]` to remove duplicate types from a type list.
* `type_list_contains[_v]` to know if a type list contains a given type.
* `type_list_diff[_t]` to remove types from type lists.
* `type_list_transform[_t]` to _transform_ a range and create another type list.

I'm also pretty sure that more and more utilities will be added over time as
needs become apparent.<br/>
Many of these functionalities also exist in their version dedicated to value
lists. We therefore have `value_list_element[_v]` as well as
`value_list_cat[_t]`and so on.

# Unique sequential identifiers

Sometimes it's useful to be able to give unique, sequential numeric identifiers
to types either at compile-time or runtime.<br/>
There are plenty of different solutions for this out there and I could have used
one of them. However, I decided to spend my time to define a couple of tools
that fully embraces what the modern C++ has to offer.

## Compile-time generator

To generate sequential numeric identifiers at compile-time, `EnTT` offers the
`ident` class template:

```cpp
// defines the identifiers for the given types
using id = entt::ident<a_type, another_type>;

// ...

switch(a_type_identifier) {
case id::value<a_type>:
    // ...
    break;
case id::value<another_type>:
    // ...
    break;
default:
    // ...
}
```

This is what this class template has to offer: a `value` inline variable that
contains a numeric identifier for the given type. It can be used in any context
where constant expressions are required.

As long as the list remains unchanged, identifiers are also guaranteed to be
stable across different runs. In case they have been used in a production
environment and a type has to be removed, one can just use a placeholder to
leave the other identifiers unchanged:

```cpp
template<typename> struct ignore_type {};

using id = entt::ident<
    a_type_still_valid,
    ignore_type<no_longer_valid_type>,
    another_type_still_valid
>;
```

Perhaps a bit ugly to see in a codebase but it gets the job done at least.

## Runtime generator

The `family` class template helps to generate sequential numeric identifiers for
types at runtime:

```cpp
// defines a custom generator
using id = entt::family<struct my_tag>;

// ...

const auto a_type_id = id::value<a_type>;
const auto another_type_id = id::value<another_type>;
```

This is what a _family_ has to offer: a `value` inline variable that contains a
numeric identifier for the given type.<br/>
The generator is customizable, so as to get different _sequences_ for different
purposes if needed.

Identifiers aren't guaranteed to be stable across different runs. Indeed it
mostly depends on the flow of execution.

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
