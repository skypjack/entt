# Crash Course: service locator

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Service locator](#service-locator)
  * [Opaque handles](#opaque-handles)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Usually, service locators are tightly bound to the services they expose and it's
hard to define a general purpose solution.<br/>
This tiny class tries to fill the gap and gets rid of the burden of defining a
different specific locator for each application.

# Service locator

The service locator API tries to mimic that of `std::optional` and adds some
extra functionalities on top of it such as allocator support.<br/>
There are a couple of functions to set up a service, namely `emplace` and
`allocate_emplace`:

```cpp
entt::locator<interface>::emplace<service>(argument);
entt::locator<interface>::allocate_emplace<service>(allocator, argument);
```

The difference is that the latter expects an allocator as the first argument and
uses it to allocate the service itself.<br/>
Once a service is set up, it's retrieved using the `value` function:

```cpp
interface &service = entt::locator<interface>::value();
```

Since the service may not be set (and therefore this function may result in an
undefined behavior), the `has_value` and `value_or` functions are also available
to test a service locator and to get a fallback service in case there is none:

```cpp
if(entt::locator<interface>::has_value()) {
    // ...
}

interface &service = entt::locator<interface>::value_or<fallback_impl>(argument);
```

All arguments are used only if necessary, that is, if a service doesn't already
exist and therefore the fallback service is constructed and returned. In all
other cases, they are discarded.<br/>
Finally, to reset a service, use the `reset` function.

## Opaque handles

Sometimes it's useful to _transfer_ a copy of a service to another locator. For
example, when working across boundaries it's common to _share_ a service with a
dynamically loaded module.<br/>
Options aren't much in this case. Among these is the possibility of _exporting_
services and assigning them to a different locator.

This is what the `handle` and `reset` functions are meant for.<br/>
The former returns an opaque object useful for _exporting_ (or rather, obtaining
a reference to) a service. The latter also accepts an optional argument to a
handle which then allows users to reset a service by initializing it with an
opaque handle:

```cpp
auto handle = entt::locator<interface>::handle();
entt::locator<interface>::reset(handle);
```

It's worth noting that it's possible to get handles for uninitialized services
and use them with other locators. Of course, all a user will get is to have an
uninitialized service elsewhere as well.

Note that exporting a service allows users to _share_ the object currently set
in a locator. Replacing it won't replace the element even where a service has
been configured with a handle to the previous item.<br/>
In other words, if an audio service is replaced with a null object to silence an
application and the original service was shared, this operation won't propagate
to the other locators. Therefore, a module that share the ownership of the
original audio service is still able to emit sounds.
