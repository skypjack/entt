# Crash Course: service locator

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Service locator](#service-locator)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Usually service locators are tightly bound to the services they expose and it's
hard to define a general purpose solution.<br/>
This tiny class tries to fill the gap and to get rid of the burden of defining a
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
Once a service has been set up, it's retrieved using the value function:

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
