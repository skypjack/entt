# Crash Course: resource management

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [The resource, the loader and the cache](#the-resource-the-loader-and-the-cache)
  * [Resource handle](#resource-handle)
  * [Loaders](#loader)
  * [The cache class](#the-cache)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Resource management is usually one of the most critical parts of a game.
Solutions are often tuned to the particular application. There exist several
approaches and all of them are perfectly fine as long as they fit the
requirements of the piece of software in which they are used.<br/>
Examples are loading everything on start, loading on request, predictive
loading, and so on.

`EnTT` doesn't pretend to offer a _one-fits-all_ solution for the different
cases.<br/>
Instead, the library comes with a minimal, general purpose resource cache that
might be useful in many cases.

# The resource, the loader and the cache

Resource, loader and cache are the three main actors for the purpose.<br/>
The _resource_ is an image, an audio, a video or any other type:

```cpp
struct my_resource { const int value; };
```

The _loader_ is a callable type the aim of which is to load a specific resource:

```cpp
struct my_loader final {
    using result_type = std::shared_ptr<my_resource>;

    result_type operator()(int value) const {
        // ...
        return std::make_shared<my_resource>(value);
    }
};
```

Its function operator can accept any arguments and should return a value of the
declared result type (`std::shared_ptr<my_resource>` in the example).<br/>
A loader can also overload its function call operator to make it possible to
construct the same or another resource from different lists of arguments.

Finally, a cache is a specialization of a class template tailored to a specific
resource and (optionally) a loader:

```cpp
using my_cache = entt::resource_cache<my_resource, my_loader>;

// ...

my_cache cache{};
```

The class is designed to create different caches for different resource types
and to manage each one independently in the most appropriate way.<br/>
As a (very) trivial example, audio tracks can survive in most of the scenes of
an application while meshes can be associated with a single scene only, then
discarded when a player leaves it.

## Resource handle

Resources aren't returned directly to the caller. Instead, they are wrapped in a
_resource handle_, an instance of the `entt::resource` class template.<br/>
For those who know the _flyweight design pattern_ already, that's exactly what
it is. To all others, this is the time to brush up on some notions instead.

A shared pointer could have been used as a resource handle. In fact, the default
implementation mostly maps the interface of its standard counterpart and only
adds a few things on top of it.<br/>
However, the handle in `EnTT` is designed as a standalone class template. This
is due to the fact that specializing a class in the standard library is often
undefined behavior while having the ability to specialize the handle for one,
more or all resource types could help over time.

## Loaders

A loader is responsible for _loading_ resources (quite obviously).<br/>
By default, it's just a callable object that forwards its arguments to the
resource itself. That is, a _passthrough type_. All the work is demanded to the
constructor(s) of the resource itself.<br/>
Loaders also are fully customizable as expected.

A custom loader is a class with at least one function call operator and a member
type named `result_type`.<br/>
The loader isn't required to return a resource handle. As long as `return_type`
is suitable for constructing a handle, that's fine.

When using the default handle, it expects a resource type which is convertible
to or suitable for constructing an `std::shared_ptr<Type>` (where `Type` is the
actual resource type).<br/>
In other terms, the loader should return shared pointers to the given resource
type. However, this isn't mandatory. Users can easily get around this constraint
by specializing both the handle and the loader.

A cache forwards all its arguments to the loader if required. This means that
loaders can also support tag dispatching to offer different loading policies:

```cpp
struct my_loader {
    using result_type = std::shared_ptr<my_resource>;

    struct from_disk_tag{};
    struct from_network_tag{};

    template<typename Args>
    result_type operator()(from_disk_tag, Args&&... args) {
        // ...
        return std::make_shared<my_resource>(std::forward<Args>(args)...);
    }

    template<typename Args>
    result_type operator()(from_network_tag, Args&&... args) {
        // ...
        return std::make_shared<my_resource>(std::forward<Args>(args)...);
    }
}
```

This makes the whole loading logic quite flexible and easy to extend over time.

## The cache class

The cache is the class that is asked to _connect the dots_.<br/>
It loads the resources, stores them aside and returns handles as needed:

```cpp
entt::resource_cache<my_resource, my_loader> cache{};
```

Under the hood, a cache is nothing more than a map where the key value has type
`entt::id_type` while the mapped value is whatever type its loader returns.<br/>
For this reason, it offers most of the functionalities a user would expect from
a map, such as `empty` or `size` and so on. Similarly, it's an iterable type
that also supports indexing by resource id:

```cpp
for(auto [id, res]: cache) {
    // ...
}

if(entt::resource<my_resource> res = cache["resource/id"_hs]; res) {
    // ...
}
```

Please, refer to the inline documentation for all the details about the other
functions (such as `contains` or `erase`).

Set aside the part of the API that this class _shares_ with a map, it also adds
something on top of it in order to address the most common requirements of a
resource cache.<br/>
In particular, it doesn't have an `emplace` member function which is replaced by
`load` and `force_load` instead (where the former loads a new resource only if
not present while the second triggers a forced loading in any case):

```cpp
auto ret = cache.load("resource/id"_hs);

// true only if the resource was not already present
const bool loaded = ret.second;

// takes the resource handle pointed to by the returned iterator
entt::resource<my_resource> res = ret.first->second;
```

Note that the hashed string is used for convenience in the example above.<br/>
Resource identifiers are nothing more than integral values. Therefore, plain
numbers as well as non-class enum value are accepted.

It's worth mentioning that the iterators of a cache as well as its indexing
operators return resource handles rather than instances of the mapped type.<br/>
Since the cache has no control over the loader and a resource isn't required to
also be convertible to bool, these handles can be invalid. This usually means an
error in the user logic but it may also be an _expected_ event.<br/>
It's therefore recommended to verify handles validity with a check in debug (for
example, when loading) or an appropriate logic in retail.
