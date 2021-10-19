# Push EnTT across boundaries

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Working across boundaries](#working-across-boundaries)
  * [Smooth until proven otherwise](#smooth-until-proven-otherwise)
  * [Meta context](#meta-context)
  * [Memory management](#memory-management)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Working across boundaries

`EnTT` has historically had a limit when used across boundaries on Windows in
general and on GNU/Linux when default visibility was set to hidden. The
limitation was mainly due to a custom utility used to assign unique, sequential
identifiers with different types.<br/>
Fortunately, nowadays using `EnTT` across boundaries is much easier.

## Smooth until proven otherwise

Many classes in `EnTT` make extensive use of type erasure for their purposes.
This isn't a problem on itself (in fact, it's the basis of an API so convenient
to use). However, a way is needed to recognize the objects whose type has been
erased on the other side of a boundary.<br/>
The `type_hash` class template is how identifiers are generated and thus made
available to the rest of the library. In general, this class doesn't arouse much
interest. The only exception is when a conflict between identifiers occurs
(definitely uncommon though) or when the default solution proposed by `EnTT`
isn't suitable for the user's purposes.<br/>
The section dedicated to `type_info` contains all the details to get around the
issue in a concise and elegant way. Please refer to the specific documentation.

When working with linked libraries, compile definitions `ENTT_API_EXPORT` and
`ENTT_API_IMPORT` can be used where there is a need to import or export symbols,
so as to make everything work nicely across boundaries.<br/>
On the other hand, everything should run smoothly when working with plugins or
shared libraries that don't export any symbols.

For anyone who needs more details, the test suite contains multiple examples
covering the most common cases (see the `lib` directory for all details).<br/>
It goes without saying that it's impossible to cover **all** possible cases.
However, what is offered should hopefully serve as a basis for all of them.

## Meta context

The runtime reflection system deserves a special mention when it comes to using
it across boundaries.<br/>
Since it's linked already to a static context to which the visible components
are attached and different contexts don't relate to each other, they must be
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
entt::meta_ctx::bind(entt::meta_ctx{});
```

This is allowed because local and global contexts are separated. Therefore, it's
always possible to make the local context the current one again.

Before to release a context, all locally registered types should be reset to
avoid dangling references. Otherwise, if a type is accessed from another space
by name, there could be an attempt to address its parts that are no longer
available.

## Memory Management

There is another subtle problem due to memory management that can lead to
headaches.<br/>
It can occur where there are pools of objects (such as components or events)
dynamically created on demand. This is usually not a problem when working with
linked libraries that rely on the same dynamic runtime. However, it can occur in
the case of plugins or statically linked runtimes.

As an example, imagine creating an instance of `registry` in the main executable
and sharing it with a plugin. If the latter starts working with a component that
is unknown to the former, a dedicated pool is created within the registry on
first use.<br/>
As one can guess, this pool is instantiated on a different side of the boundary
from the `registry`. Therefore, the instance is now managing memory from
different spaces and this can quickly lead to crashes if not properly addressed.

To overcome the risk, it's recommended to use well-defined interfaces that make
fundamental types pass through the boundaries, isolating the instances of the
`EnTT` classes from time to time and as appropriate.<br/>
Refer to the test suite for some examples, read the documentation available
online about this type of issues or consult someone who has already had such
experiences to avoid problems.
