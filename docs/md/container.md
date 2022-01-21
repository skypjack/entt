# Crash Course: containers

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Containers](#containers)
  * [Dense map](#dense-map)
  * [Dense set](#dense-set)

<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

The standard C++ library offers a wide range of containers and it's really
difficult to do better (although it's very easy to do worse, as many examples
available online demonstrate).<br/>
`EnTT` doesn't try in any way to replace what is offered by the standard. Quite
the opposite, given the widespread use that is made of standard containers.<br/>
However, the library also tries to fill a gap in features and functionality by
making available some containers initially developed for internal use.

This section of the library is likely to grow larger over time. However, for the
moment it's quite small and mainly aimed at satisfying some internal needs.<br/>
For all containers made available, full test coverage and stability over time is
guaranteed as usual.

# Containers

## Dense map

The dense map made available in `EnTT` is a hash map that aims to return a
packed array of elements, so as to reduce the number of jumps in memory during
iterations.<br/>
The implementation is based on _sparse sets_ and each bucket is identified by an
implicit list within the packed array itself.

The interface is in all respects similar to its counterpart in the standard
library, that is, `std::unordered_map`.<br/>
Therefore, there is no need to go into the API description.

## Dense set

The dense set made available in `EnTT` is a hash set that aims to return a
packed array of elements, so as to reduce the number of jumps in memory during
iterations.<br/>
The implementation is based on _sparse sets_ and each bucket is identified by an
implicit list within the packed array itself.

The interface is in all respects similar to its counterpart in the standard
library, that is, `std::unordered_set`.<br/>
Therefore, there is no need to go into the API description.
