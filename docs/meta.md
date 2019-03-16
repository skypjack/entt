# Crash Course: reflection system

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Reflection in a nutshell](#reflection-in-a-nutshell)
* [Any as in any type](#any-as-in-any-type)
* [Enjoy the runtime](#enjoy-the-runtime)
* [Named constants and enums](#named-constants-and-enums)
* [Properties and meta objects](#properties-and-meta-objects)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Reflection (or rather, its lack) is a trending topic in the C++ world and, in
the specific case of `EnTT`, a tool that can unlock a lot of other features. I
looked for a third-party library that met my needs on the subject, but I always
came across some details that I didn't like: macros, being intrusive, too many
allocations. In one word: unsatisfactory.<br/>
I finally decided to write a built-in, non-intrusive and macro-free runtime
reflection system for `EnTT`. Maybe I didn't do better than others or maybe yes,
time will tell me, but at least I can model this tool around the library to
which it belongs and not vice versa.

# Reflection in a nutshell

Reflection always starts from real types (users cannot reflect imaginary types
and it would not make much sense, we wouldn't be talking about reflection
anymore).<br/>
To _reflect_ a type, the library provides the `reflect` function:

```cpp
auto factory = entt::reflect<my_type>("reflected_type");
```

It accepts the type to reflect as a template parameter and an optional name as
an argument. Names are important because users can retrieve meta types at
runtime by searching for them by name. However, there are cases in which users
can be interested in adding features to a reflected type so that the reflection
system can use it correctly under the hood, but they don't want to allow
searching the type by name.<br/>
In both cases, the returned value is a factory object to use to continue
building the meta type.

A factory is such that all its member functions returns the factory itself.
It can be used to extend the reflected type and add the following:

* _Constructors_. Actual constructors can be assigned to a reflected type by
  specifying their list of arguments. Free functions (namely, factories) can be
  used as well, as long as the return type is the expected one. From a client's
  point of view, nothing changes if a constructor is a free function or an
  actual constructor.<br/>
  Use the `ctor` member function for this purpose:

  ```cpp
  entt::reflect<my_type>("reflected").ctor<int, char>().ctor<&factory>();
  ```

* _Destructors_. Free functions can be set as destructors of reflected types.
  The purpose is to give users the ability to free up resources that require
  special treatment before an object is actually destroyed.<br/>
  Use the `dtor` member function for this purpose:

  ```cpp
  entt::reflect<my_type>("reflected").dtor<&destroy>();
  ```

* _Data members_. Both real data members of the underlying type and static and
  global variables, as well as constants of any kind, can be attached to a meta
  type. From a client's point of view, all the variables associated with the
  reflected type will appear as if they were part of the type itself.<br/>
  Use the `data` member function for this purpose:

  ```cpp
  entt::reflect<my_type>("reflected")
      .data<&my_type::static_variable>("static")
      .data<&my_type::data_member>("member")
      .data<&global_variable>("global");
  ```

  This function requires as an argument the name to give to the meta data once
  created. Users can then access meta data at runtime by searching for them by
  name.<br/>
  Data members can be set also by means of a couple of functions, namely a
  setter and a getter. Setters and getters can be either free functions, member
  functions or mixed ones, as long as they respect the required signatures.<br/>
  Refer to the inline documentation for all the details.

* _Member functions_. Both real member functions of the underlying type and free
  functions can be attached to a meta type. From a client's point of view, all
  the functions associated with the reflected type will appear as if they were
  part of the type itself.<br/>
  Use the `func` member function for this purpose:

  ```cpp
  entt::reflect<my_type>("reflected")
      .func<&my_type::static_function>("static")
      .func<&my_type::member_function>("member")
      .func<&free_function>("free");
  ```

  This function requires as an argument the name to give to the meta function
  once created. Users can then access meta functions at runtime by searching for
  them by name.

* _Base classes_. A base class is such that the underlying type is actually
  derived from it. In this case, the reflection system tracks the relationship
  and allows for implicit casts at runtime when required.<br/>
  Use the `base` member function for this purpose:

  ```cpp
  entt::reflect<derived_type>("derived").base<base_type>();
  ```

  From now on, wherever a `base_type` is required, an instance of `derived_type`
  will also be accepted.

* _Conversion functions_. Actual types can be converted, this is a fact. Just
  think of the relationship between a `double` and an `int` to see it. Similar
  to bases, conversion functions allow users to define conversions that will be
  implicitly performed by the reflection system when required.<br/>
  Use the `conv` member function for this purpose:

  ```cpp
  entt::reflect<double>().conv<int>();
  ```

That's all, everything users need to create meta types and enjoy the reflection
system. At first glance it may not seem that much, but users usually learn to
appreciate it over time.<br/>
Also, do not forget what these few lines hide under the hood: a built-in,
non-intrusive and macro-free system for reflection in C++. Features that are
definitely worth the price, at least for me.

# Any as in any type

The reflection system comes with its own meta any type. It may seem redundant
since C++17 introduced `std::any`, but it is not.<br/>
In fact, the _type_ returned by an `std::any` is a const reference to an
`std::type_info`, an implementation defined class that's not something everyone
wants to see in a software. Furthermore, the class `std::type_info` suffers from
some design flaws and there is even no way to _convert_ an `std::type_info` into
a meta type, thus linking the two worlds.

A meta any object provides an API similar to that of its most famous counterpart
and serves the same purpose of being an opaque container for any type of
value.<br/>
It minimizes the allocations required, which are almost absent thanks to _SBO_
techniques. In fact, unless users deal with _fat types_ and create instances of
them though the reflection system, allocations are at zero.

A meta any object can be created by any other object or as an empty container
to initialize later:

```cpp
// a meta any object that contains an int
entt::meta_any any{0};

// an empty meta any object
entt::meta_any empty{};
```

It can be constructed or assigned by copy and move and it takes the burden of
destroying the contained object when required.<br/>
A meta any object has a `type` member function that returns the meta type of the
contained value, if any. The member functions `can_cast` and `can_convert` are
used to know if the underlying object has a given type as a base or if it can be
converted implicitly to it. Similarly, `cast` and `convert` do what they promise
and return the expected value.

# Enjoy the runtime

Once the web of reflected types has been constructed, it's a matter of using it
at runtime where required.<br/>
All this has the great merit that, unlike the vast majority of the things
present in this library and closely linked to the compile-time, the reflection
system stands in fact as a non-intrusive tool for the runtime.

To search for a reflected type there are two options: by type or by name. In
both cases, the search can be done by means of the `resolve` function:

```cpp
// search for a reflected type by type
auto by_type = entt::resolve<my_type>();

// search for a reflected type by name
auto by_name = entt::resolve("reflected_type");
```

There exits also a third overload of the `resolve` function to use to iterate
all the reflected types at once:

```cpp
resolve([](auto type) {
    // ...
});
```

In all cases, the returned value is an instance of `meta_type`. This type of
objects offer an API to know the _runtime name_ of the type, to iterate all the
meta objects associated with them and even to build or destroy instances of the
underlying type.<br/>
Refer to the inline documentation for all the details.

The meta objects that compose a meta type are accessed in the following ways:

* _Meta constructors_. They are accessed by types of arguments:

  ```cpp
  auto ctor = entt::resolve<my_type>().ctor<int, char>();
  ```

  The returned type is `meta_ctor` and may be invalid if there is no constructor
  that accepts the supplied arguments or at least some types from which they are
  derived or to which they can be converted.<br/>
  A meta constructor offers an API to know the number of arguments, the expected
  meta types and to invoke it, therefore to construct a new instance of the
  underlying type.

* _Meta destructor_. It's returned by a dedicated function:

  ```cpp
  auto dtor = entt::resolve<my_type>().dtor();
  ```

  The returned type is `meta_dtor` and may be invalid if there is no custom
  destructor set for the given meta type.<br/>
  All what a meta destructor has to offer is a way to invoke it on a given
  instance. Be aware that the result may not be what is expected.

* _Meta data_. They are accessed by name:

  ```cpp
  auto data = entt::resolve<my_type>().data("member");
  ```

  The returned type is `meta_data` and may be invalid if there is no meta data
  object associated with the given name.<br/>
  A meta data object offers an API to query the underlying type (ie to know if
  it's a const or a static one), to get the meta type of the variable and to set
  or get the contained value.

* _Meta functions_. They are accessed by name:

  ```cpp
  auto func = entt::resolve<my_type>().func("member");
  ```

  The returned type is `meta_func` and may be invalid if there is no meta
  function object associated with the given name.<br/>
  A meta function object offers an API to query the underlying type (ie to know
  if it's a const or a static function), to know the number of arguments, the
  meta return type and the meta types of the parameters. In addition, a meta
  function object can be used to invoke the underlying function and then get the
  return value in the form of meta any object.


* _Meta bases_. They are accessed through the name of the base types:

  ```cpp
  auto base = entt::resolve<derived_type>().base("base");
  ```

  The returned type is `meta_base` and may be invalid if there is no meta base
  object associated with the given name.<br/>
  Meta bases aren't meant to be used directly, even though they are freely
  accessible. They expose only a few methods to use to know the meta type of the
  base class and to convert a raw pointer between types.

* _Meta conversion functions_. They are accessed by type:

  ```cpp
  auto conv = entt::resolve<double>().conv<int>();
  ```

  The returned type is `meta_conv` and may be invalid if there is no meta
  conversion function associated with the given type.<br/>
  The meta conversion functions are as thin as the meta bases and with a very
  similar interface. The sole difference is that they return a newly created
  instance wrapped in a meta any object when they convert between different
  types.

All the objects thus obtained as well as the meta types can be explicitly
converted to a boolean value to check if they are valid:

```cpp
auto func = entt::resolve<my_type>().func("member");

if(func) {
    // ...
}
```

Furthermore, all meta objects with the exception of meta destructors can be
iterated through an overload that accepts a callback through which to return
them. As an example:

```cpp
entt::resolve<my_type>().data([](auto data) {
    // ...
});
```

A meta type can also be used to `construct` or `destroy` actual instances of the
underlying type.<br/>
In particular, the `construct` member function accepts a variable number of
arguments and searches for a match. It returns a `meta_any` object that may or
may not be initialized, depending on whether a suitable constructor has been
found or not. On the other side, the `destroy` member function accepts instances
of `meta_any` as well as actual objects by reference and invokes the registered
destructor if any or a default one.<br/>
Be aware that the result of a call to `destroy` may not be what is expected.

Meta types and meta objects in general contain much more than what is said: a
plethora of functions in addition to those listed whose purposes and uses go
unfortunately beyond the scope of this document.<br/>
I invite anyone interested in the subject to look at the code, experiment and
read the official documentation to get the best out of this powerful tool.

# Named constants and enums

A special mention should be made for constant values and enums. It wouldn't be
necessary, but it will help distracted readers.

As mentioned, the `data` member function can be used to reflect constants of any
type among the other things.<br/>
This allows users to create meta types for enums that will work exactly like any
other meta type built from a class. Similarly, arithmetic types can be enriched
with constants of special meaning where required.<br/>
Personally, I find it very useful not to export what is the difference between
enums and classes in C++ directly in the space of the reflected types.

All the values thus exported will appear to users as if they were constant data
members of the reflected types.

Exporting constant values or elements from an enum is as simple as ever:

```cpp
entt::reflect<my_enum>()
        .data<my_enum::a_value>("a_value")
        .data<my_enum::another_value>("another_value");

entt::reflect<int>().data<2048>("max_int");
```

It goes without saying that accessing them is trivial as well. It's a matter of
doing the following, as with any other data member of a meta type:

```cpp
auto value = entt::resolve<my_enum>().data("a_value").get({}).cast<my_enum>();
auto max = entt::resolve<int>().data("max_int").get({}).cast<int>();
```

As a side note, remember that all this happens behind the scenes without any
allocation because of the small object optimization performed by the meta any
class.

# Properties and meta objects

Sometimes (ie when it comes to creating an editor) it might be useful to be able
to attach properties to the meta objects created. Fortunately, this is possible
for most of them.<br/>
To attach a property to a meta object, no matter what as long as it supports
properties, it is sufficient to provide an object at the time of construction
such that `std::get<0>` and `std::get<1>` are valid for it. In other terms, the
properties are nothing more than key/value pairs users can put in an
`std::pair`. As an example:

```cpp
entt::reflect<my_type>("reflected", std::make_pair("tooltip"_hs, "message"));
```

The meta objects that support properties offer then a couple of member functions
named `prop` to iterate them at once and to search a specific property by key:

```cpp
// iterate all the properties of a meta type
entt::resolve<my_type>().prop([](auto prop) {
    // ...
});

// search for a given property by name
auto prop = entt::resolve<my_type>().prop("tooltip"_hs);
```

Meta properties are objects having a fairly poor interface, all in all. They
only provide the `key` and the `value` member functions to be used to retrieve
the key and the value contained in the form of meta any objects, respectively.
