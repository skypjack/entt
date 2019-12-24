# Push EnTT across boundaries

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [The EnTT way](#the-entt-way)
* [Meta context](#meta-context)
* [Memory management](#memory-management)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` has historically had a limit when used across boundaries on Windows in
general and on GNU/Linux when default visibility was set to hidden. The
limitation was mainly due to a custom utility used to assign unique, sequential
identifiers to different types.<br/>
Fortunately, nowadays using `EnTT` across boundaries is straightforward. In
fact, everything just works transparently in almost all cases. There are only a
few exceptions, easy to deal with anyway.

# The EnTT way

Many classes in `EnTT` make extensive use of type erasure for their purposes.
This isn't a problem in itself (in fact, it's the basis of an API so convenient
to use). However, a way is needed to recognize the objects whose type has been
erased on the other side of a boundary.<br/>
The `type_info` class template is how identifiers are generated and thus made
available to the rest of the library.

The only case in which this may arouse some interest is in case of conflicts
between identifiers (definitely uncommon though) or where the default solution
proposed by `EnTT` is not suitable for the user's purposes.<br/>
Please refer to the dedicated section for more details.

# Meta context

The runtime reflection system deserves a special mention when it comes to using
it across boundaries.<br/>
Since it's linked to a static context to which the visible components are
attached and different contexts don't relate to each other, they must be
_shared_ to allow the use of meta types across boundaries.

Sharing a context is trivial though. First of all, the local one must be
acquired in the main space:

```cpp
entt::meta_ctx ctx{};
```

Then, it must passed to the receiving space that will set it as its global
context, thus releasing the local one that remains available but is no longer
referred to by the runtime reflection system:

```cpp
entt::meta_ctx::bind(ctx);
```

From now on, both spaces will refer to the same context and on it will be
attached the new visible meta types, no matter where they are created.<br/>
A context can also be reset and then associated again locally as:

```cpp
entt::meta_ctx::bind{entt::meta_ctx{});
```

This is allowed because local and global contexts are separated. Therefore, it's
always possible to make the local context the current one again.

Before to release a context, all locally registered types should be reset to
avoid dangling references. Otherwise, if a type is accessed from another space
by name, there could be an attempt to address its parts that are no longer
available.

# Memory Management

There is another subtle problem due to memory management that can lead to
headaches.<br/>
This can occur where there are pools of objects (such as components or events)
dynamically created on demand.

As an example, imagine creating an instance of `registry` in the main executable
and share it with a plugin. If the latter starts working with a component that
is unknown to the former, a dedicated pool is created within the registry on
first use.<br/>
As one can guess, this pool is instantiated on a different side of the boundary
from the `registry`. Therefore, the instance is now managing memory from
different spaces and this can quickly lead to crashes if not properly addressed.

Fortunately, all classes that could potentially suffer from this problem offer
also a `discard` member function to get rid of these pools:

```cpp
registry.discard<local_type>();
```

This is all there is to do to get around this. Again, `discard` is only to be
invoked if it's certain that the container and pools are instantiated on
different sides of the boundary.

If in doubts or to avoid risks, simply invoke the `prepare` member function or
any of the other functions that refer to the desired type to force the
generation of the pools that are used on both sides of the boundary.<br/>
This is something to be done usually in the main context when needed.
