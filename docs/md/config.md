# Crash Course: configuration

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Definitions](#definitions)
  * [ENTT_NOEXCEPT](#entt_noexcept)
  * [ENTT_USE_ATOMIC](#entt_use_atomic)
  * [ENTT_ID_TYPE](#entt_id_type)
  * [ENTT_PAGE_SIZE](#entt_page_size)
  * [ENTT_ASSERT](#entt_assert)
    * [ENTT_DISABLE_ASSERT](#entt_disable_assert)
  * [ENTT_NO_ETO](#entt_no_eto)
  * [ENTT_STANDARD_CPP](#entt_standard_cpp)

<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

`EnTT` doesn't offer many hooks for customization but it certainly offers
some.<br/>
In the vast majority of cases, users will have no interest in changing the
default parameters. For all other cases, the list of possible configurations
with which it's possible to adjust the behavior of the library at runtime can be
found below.

# Definitions

All options are intended as parameters to the compiler (or user-defined macros
within the compilation units, if preferred).<br/>
Each parameter can result in internal library definitions. It's not recommended
to try to also modify these definitions, since there is no guarantee that they
will remain stable over time unlike the options below.

## ENTT_NOEXCEPT

The purpose of this parameter is to suppress the use of `noexcept` by this
library.<br/>
To do this, simply define the variable without assigning any value to it.

## ENTT_USE_ATOMIC

In general, `EnTT` doesn't offer primitives to support multi-threading. Many of
the features can be split over multiple threads without any explicit control and
the user is the only one who knows if and when a synchronization point is
required.<br/>
However, some features aren't easily accessible to users and can be made
thread-safe by means of this definition.

## ENTT_ID_TYPE

`entt::id_type` is directly controlled by this definition and widely used within
the library.<br/>
By default, its type is `std::uint32_t`. However, users can define a different
default type if necessary.

## ENTT_PAGE_SIZE

As is known, the ECS module of `EnTT` is based on _sparse sets_. What is less
known perhaps is that these are paged to reduce memory consumption.<br/>
Default size of pages (that is, the number of elements they contain) is 4096 but
users can adjust it if appropriate. In all case, the chosen value **must** be a
power of 2.

## ENTT_ASSERT

For performance reasons, `EnTT` doesn't use exceptions or any other control
structures. In fact, it offers many features that result in undefined behavior
if not used correctly.<br/>
To get around this, the library relies on a lot of asserts for the purpose of
detecting errors in debug builds. By default, it uses `assert` internally, but
users are allowed to overwrite its behavior by setting this variable.

### ENTT_DISABLE_ASSERT

Assertions may in turn affect performance to an extent when enabled. Whether
`ENTT_ASSERT` is redefined or not, all asserts can be disabled at once by means
of this definition.<br/>
Note that `ENTT_DISABLE_ASSERT` takes precedence over the redefinition of
`ENTT_ASSERT` and is therefore meant to disable all controls no matter what.

## ENTT_NO_ETO

In order to reduce memory consumption and increase performance, empty types are
never stored by the ECS module of `EnTT`.<br/>
Use this variable to treat these types like all others and therefore to create a
dedicated storage for them.

## ENTT_STANDARD_CPP

`EnTT` mixes non-standard language features with others that are perfectly
compliant to offer some of its functionalities.<br/>
This definition will prevent the library from using non-standard techniques,
that is, functionalities that aren't fully compliant with the standard C++.<br/>
While there are no known portability issues at the time of this writing, this
should make the library fully portable anyway if needed.
