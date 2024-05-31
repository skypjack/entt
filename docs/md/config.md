# Crash Course: configuration

# Table of Contents

* [Introduction](#introduction)
* [Definitions](#definitions)
  * [ENTT_NOEXCEPTION](#entt_noexception)
  * [ENTT_USE_ATOMIC](#entt_use_atomic)
  * [ENTT_ID_TYPE](#entt_id_type)
  * [ENTT_SPARSE_PAGE](#entt_sparse_page)
  * [ENTT_PACKED_PAGE](#entt_packed_page)
  * [ENTT_ASSERT](#entt_assert)
    * [ENTT_ASSERT_CONSTEXPR](#entt_assert_constexpr)
    * [ENTT_DISABLE_ASSERT](#entt_disable_assert)
  * [ENTT_NO_ETO](#entt_no_eto)
  * [ENTT_STANDARD_CPP](#entt_standard_cpp)

# Introduction

`EnTT` has become almost completely customizable over time, in many
respects. These variables are just one of the many ways to customize how it
works.<br/>
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

## ENTT_NOEXCEPTION

Define this variable without assigning any value to it to turn off exception
handling in `EnTT`.<br/>
This is roughly equivalent to setting the compiler flag `-fno-exceptions` but is
also limited to this library only.

## ENTT_USE_ATOMIC

In general, `EnTT` doesn't offer primitives to support multi-threading. Many of
the features can be split over multiple threads without any explicit control and
the user is the one who knows if a synchronization point is required.<br/>
However, some features aren't easily accessible to users and are made
thread-safe by means of this definition.

## ENTT_ID_TYPE

`entt::id_type` is directly controlled by this definition and widely used within
the library.<br/>
By default, its type is `std::uint32_t`. However, users can define a different
default type if necessary.

## ENTT_SPARSE_PAGE

It's known that the ECS module of `EnTT` is based on _sparse sets_. What is less
known perhaps is that the sparse arrays are paged to reduce memory usage.<br/>
Default size of pages (that is, the number of elements they contain) is 4096 but
users can adjust it if appropriate. In all case, the chosen value **must** be a
power of 2.

## ENTT_PACKED_PAGE

As it happens with sparse arrays, packed arrays are also paginated. However, in
this case the aim isn't to reduce memory usage but to have pointer stability
upon component creation.<br/>
Default size of pages (that is, the number of elements they contain) is 1024 but
users can adjust it if appropriate. In all case, the chosen value **must** be a
power of 2.

## ENTT_ASSERT

For performance reasons, `EnTT` doesn't use exceptions or any other control
structures. In fact, it offers many features that result in undefined behavior
if not used correctly.<br/>
To get around this, the library relies on a lot of asserts for the purpose of
detecting errors in debug builds. By default, it uses `assert` internally. Users
are allowed to overwrite its behavior by setting this variable.

### ENTT_ASSERT_CONSTEXPR

Usually, an assert within a `constexpr` function isn't a big deal. However, in
case of extreme customizations, it might be useful to differentiate.<br/>
For this purpose, `EnTT` introduces an admittedly badly named variable to make
the job easier in this regard. By default, this variable forwards its arguments
to `ENTT_ASSERT`.

### ENTT_DISABLE_ASSERT

Assertions may in turn affect performance to an extent when enabled. Whether
`ENTT_ASSERT` and `ENTT_ASSERT_CONSTEXPR` are redefined or not, all asserts can
be disabled at once by means of this definition.<br/>
Note that `ENTT_DISABLE_ASSERT` takes precedence over the redefinition of the
other variables and is therefore meant to disable all controls no matter what.

## ENTT_NO_ETO

In order to reduce memory consumption and increase performance, empty types are
never instantiated nor stored by the ECS module of `EnTT`.<br/>
Use this variable to treat these types like all others and therefore to create a
dedicated storage for them.

## ENTT_STANDARD_CPP

`EnTT` mixes non-standard language features with others that are perfectly
compliant to offer some of its functionalities.<br/>
This definition prevents the library from using non-standard techniques, that
is, functionalities that aren't fully compliant with the standard C++.<br/>
While there are no known portability issues at the time of this writing, this
should make the library fully portable anyway if needed.
