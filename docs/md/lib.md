# Push EnTT across boundaries

# Table of Contents

* [Working across boundaries](#working-across-boundaries)
  * [Smooth until proven otherwise](#smooth-until-proven-otherwise)
  * [Meta context](#meta-context)
  * [Memory management](#memory-management)

# Working across boundaries

`EnTT` has historically had a limit when used across boundaries on Windows in
general and on GNU/Linux when default visibility was set to hidden. The
limitation was mainly due to a custom utility used to assign unique, sequential
identifiers with different types.<br/>
Fortunately, nowadays `EnTT` works smoothly across boundaries.

## Smooth until proven otherwise

Many classes in `EnTT` make extensive use of type erasure for their purposes.
This raises the need to identify objects whose type has been erased.<br/>
The `type_hash` class template is how identifiers are generated and thus made
available to the rest of the library. In general, this class arouses little
interest. The only exception is when a conflict between identifiers occurs
(definitely uncommon though) or when the default solution proposed by `EnTT` is
not suitable for the user's purposes.<br/>
The section dedicated to `type_info` contains all the details to get around the
issue in a concise and elegant way. Please refer to the specific documentation.

When working with linked libraries, compile definitions `ENTT_API_EXPORT` and
`ENTT_API_IMPORT` are to import or export symbols, so as to make everything work
nicely across boundaries.<br/>
On the other hand, everything should run smoothly when working with plugins or
shared libraries that do not export any symbols.

For those who need more details, the test suite contains many examples covering
the most common cases (see the `lib` directory for all details).<br/>
It goes without saying that it is impossible to cover **all** possible cases.
However, what is offered should hopefully serve as a basis for all of them.

## Meta context

The runtime reflection system deserves a special mention when it comes to using
it across boundaries.<br/>
Since it is linked already to a static context to which the elements are
attached and different contexts do not relate to each other, they must be
_shared_ to allow the use of meta types across boundaries.

Fortunately, sharing a context is also trivial to do. First of all, the local
one is acquired in the main space:

```cpp
auto handle = entt::locator<entt::meta_ctx>::handle();
```

Then, it is passed to the receiving space that sets it as its default context,
thus discarding or storing aside the local one:

```cpp
entt::locator<entt::meta_ctx>::reset(handle);
```

From now on, both spaces refer to the same context and to it are all new meta
types attached, no matter where they are created.<br/>
Note that _replacing_ the main context does not also propagate changes across
boundaries. In other words, replacing a context results in the decoupling of the
two sides and therefore a divergence in the contents.

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
different spaces, and this can quickly lead to crashes if not properly
addressed.

To overcome the risk, it is recommended to use well-defined interfaces that make
fundamental types pass through the boundaries, isolating the instances of the
`EnTT` classes from time to time and as appropriate.<br/>
Refer to the test suite for some examples, read the documentation available
online about this type of issues or consult someone who has already had such
experiences to avoid problems.
