# Crash Course: configuration

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Definitions](#definitions)
  * [ENTT_STANDALONE](#entt_standalone)
  * [ENTT_NOEXCEPT](#entt_noexcept)
  * [ENTT_HS_SUFFIX and ENTT_HWS_SUFFIX](#entt_hs_suffix_and_entt_hws_suffix)
  * [ENTT_USE_ATOMIC](#entt_use_atomic)
  * [ENTT_ID_TYPE](#entt_id_type)
  * [ENTT_PAGE_SIZE](#entt_page_size)
  * [ENTT_ASSERT](#entt_assert)
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

## ENTT_STANDALONE

`EnTT` is designed in such a way that it works (almost) everywhere out of the
box. However, this is the result of many refinements over time and a compromise
regarding some optimizations.<br/>
It's worth noting that users can get a small performance boost by passing this
definition to the compiler when the library is used in a standalone application.

## ENTT_NOEXCEPT

The purpose of this parameter is to suppress the use of `noexcept` by this
library.<br/>
To do this, simply define the variable without assigning any value to it.

## ENTT_HS_SUFFIX and ENTT_HWS_SUFFIX

The `hashed_string` class introduces the `_hs` and `_hws` suffixes to accompany
its user defined literals.<br/>
In the case of conflicts or even just to change these suffixes, it's possible to
do so by associating new ones with these definitions.

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
known perhaps is that these are paged to reduce memory consumption in some
corner cases.<br/>
The default size of a page is 32kB but users can adjust it if appropriate. In
all case, the chosen value **must** be a power of 2.

## ENTT_ASSERT

For performance reasons, `EnTT` doesn't use exceptions or any other control
structures. In fact, it offers many features that result in undefined behavior
if not used correctly.<br/>
To get around this, the library relies on a lot of `assert`s for the purpose of
detecting errors in debug builds. However, these assertions may in turn affect
performance to an extent.<br/>
This option is meant to disable all controls.

## ENTT_NO_ETO

In order to reduce memory consumption and increase performance, empty types are
never stored by the ECS module of `EnTT`.<br/>
Use this variable to treat these types like all others and therefore to create a
dedicated storage for them.

## ENTT_STANDARD_CPP

After many adventures, `EnTT` finally works fine across boundaries.<br/>
To do this, the library mixes some non-standard language features with others
that are perfectly compliant.<br/>
This definition will prevent the library from using non-standard techniques,
that is, functionalities that aren't fully compliant with the standard C++.
