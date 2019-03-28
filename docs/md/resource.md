# Crash Course: resource management

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [The resource, the loader and the cache](#the-resource-the-loader-and-the-cache)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Resource management is usually one of the most critical part of a software like
a game. Solutions are often tuned to the particular application. There exist
several approaches and all of them are perfectly fine as long as they fit the
requirements of the piece of software in which they are used.<br/>
Examples are loading everything on start, loading on request, predictive
loading, and so on.

`EnTT` doesn't pretend to offer a _one-fits-all_ solution for the different
cases. Instead, it offers a minimal and perhaps trivial cache that can be useful
most of the time during prototyping and sometimes even in a production
environment.<br/>
For those interested in the subject, the plan is to improve it considerably over
time in terms of performance, memory usage and functionalities. Hoping to make
it, of course, one step at a time.

# The resource, the loader and the cache

There are three main actors in the model: the resource, the loader and the
cache.

The _resource_ is whatever users want it to be. An image, a video, an audio,
whatever. There are no limits.<br/>
As a minimal example:

```cpp
struct my_resource { const int value; };
```

A _loader_ is a class the aim of which is to load a specific resource. It has to
inherit directly from the dedicated base class as in the following example:

```cpp
struct my_loader final: entt::resource_loader<my_loader, my_resource> {
    // ...
};
```

Where `my_resource` is the type of resources it creates.<br/>
A resource loader must also expose a public const member function named `load`
that accepts a variable number of arguments and returns a shared pointer to a
resource.<br/>
As an example:

```cpp
struct my_loader: entt::resource_loader<my_loader, my_resource> {
    std::shared_ptr<my_resource> load(int value) const {
        // ...
        return std::shared_ptr<my_resource>(new my_resource{ value });
    }
};
```

In general, resource loaders should not have a state or retain data of any type.
They should let the cache manage their resources instead.<br/>
As a side note, base class and CRTP idiom aren't strictly required with the
current implementation. One could argue that a cache can easily work with
loaders of any type. However, future changes won't be breaking ones by forcing
the use of a base class today and that's why the model is already in its place.

Finally, a cache is a specialization of a class template tailored to a specific
resource:

```cpp
using my_resource_cache = entt::resource_cache<my_resource>;

// ...

my_resource_cache cache{};
```

The idea is to create different caches for different types of resources and to
manage each one independently and in the most appropriate way.<br/>
As a (very) trivial example, audio tracks can survive in most of the scenes of
an application while meshes can be associated with a single scene and then
discarded when users leave it.

A cache offers a set of basic functionalities to query its internal state and to
_organize_ it:

```cpp
// gets the number of resources managed by a cache
const auto size = cache.size();

// checks if a cache contains at least a valid resource
const auto empty = cache.empty();

// clears a cache and discards its content
cache.clear();
```

Besides these member functions, it contains what is needed to load, use and
discard resources of the given type.<br/>
Before to explore this part of the interface, it makes sense to mention how
resources are identified. The type of the identifiers to use is defined as:

```cpp
entt::resource_cache<resource>::resource_type
```

Where `resource_type` is an alias for `entt::hashed_string`. Therefore, resource
identifiers are created explicitly as in the following example:

```cpp
constexpr auto identifier = entt::resource_cache<resource>::resource_type{"my/resource/identifier"};
// this is equivalent to the following
constexpr auto hs = entt::hashed_string{"my/resource/identifier"};
```

The class `hashed_string` is described in a dedicated section, so I won't do in
details here.

Resources are loaded and thus stored in a cache through the `load` member
function. It accepts the loader to use as a template parameter, the resource
identifier and the parameters used to construct the resource as arguments:

```cpp
// uses the identifier declared above
cache.load<my_loader>(identifier, 0);

// uses a const char * directly as an identifier
cache.load<my_loader>("another/identifier", 42);
```

The return value can be used to know if the resource has been loaded correctly.
In case the loader returns an invalid pointer or the resource already exists in
the cache, a false value is returned:

```cpp
if(!cache.load<my_loader>("another/identifier", 42)) {
    // ...
}
```

Unfortunately, in this case there is no way to know what was the problem
exactly. However, before trying to load a resource or after an error, one can
use the `contains` member function to know if a cache already contains a
specific resource:

```cpp
auto exists = cache.contains("my/identifier");
```

There exists also a member function to use to force a reload of an already
existing resource if needed:

```cpp
auto result = cache.reload<my_loader>("another/identifier", 42);
```

As above, the function returns true in case of success, false otherwise. The
sole difference in this case is that an error necessarily means that the loader
has failed for some reasons to load the resource.<br/>
Note that the `reload` member function is a kind of alias of the following
snippet:

```cpp
cache.discard(identifier);
cache.load<my_loader>(identifier, 42);
```

Where the `discard` member function is used to get rid of a resource if loaded.
In case the cache doesn't contain a resource for the given identifier, the
function does nothing and returns immediately.

So far, so good. Resources are finally loaded and stored within the cache.<br/>
They are returned to users in the form of handles. To get one of them:

```cpp
auto handle = cache.handle("my/identifier");
```

The idea behind a handle is the same of the flyweight pattern. In other terms,
resources aren't copied around. Instead, instances are shared between handles.
Users of a resource owns a handle and it guarantees that a resource isn't
destroyed until all the handles are destroyed, even if the resource itself is
removed from the cache.<br/>
Handles are tiny objects both movable and copyable. They returns the contained
resource as a const reference on request:

* By means of the `get` member function:

  ```cpp
  const auto &resource = handle.get();
  ```

* Using the proper cast operator:

  ```cpp
  const auto &resource = handle;
  ```

* Through the dereference operator:

  ```cpp
  const auto &resource = *handle;
  ```

The resource can also be accessed directly using the arrow operator if required:

```cpp
auto value = handle->value;
```

To test if a handle is still valid, the cast operator to `bool` allows users to
use it in a guard:

```cpp
if(handle) {
    // ...
}
```

Finally, in case there is the need to load a resource and thus to get a handle
without storing the resource itself in the cache, users can rely on the `temp`
member function template.<br/>
The declaration is similar to the one of `load` but for the fact that it doesn't
return a boolean value. Instead, it returns a (possibly invalid) handle for the
resource:

```cpp
auto handle = cache.temp<my_loader>("another/identifier", 42);
```

Do not forget to test the handle for validity. Otherwise, getting the reference
to the resource it points may result in undefined behavior.
