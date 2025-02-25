# Crash Course: containers

# Table of Contents

* [Introduction](#introduction)
* [Containers](#containers)
  * [Dense map](#dense-map)
  * [Dense set](#dense-set)
* [Adaptors](#adaptors)
  * [Table](#table)

# Introduction

The standard C++ library offers a wide range of containers and adaptors already.
It is really difficult to do better (although it is very easy to do worse, as
many examples available online demonstrate).<br/>
`EnTT` does not try in any way to replace what is offered by the standard. Quite
the opposite, given the widespread use that is made of standard containers.<br/>
However, the library also tries to fill a gap in features and functionalities by
making available some containers and adaptors initially developed for internal
use.

This section of the library is likely to grow larger over time. However, for the
moment it is quite small and mainly aimed at satisfying some internal
needs.<br/>
For all containers and adaptors made available, full test coverage and stability
over time is guaranteed as usual.

# Containers

## Dense map

The dense map made available in `EnTT` is a hash map that aims to return a
packed array of elements, so as to reduce the number of jumps in memory during
iterations.<br/>
The implementation is based on _sparse sets_ and each bucket is identified by an
implicit list within the packed array itself.

The interface is very close to its counterpart in the standard library, that is,
the `std::unordered_map` class.<br/>
However, both local and non-local iterators returned by a dense map belong to
the input iterator category although they respectively model the concepts of a
_forward iterator_ type and a _random access iterator_ type.<br/>
This is because they return a pair of references rather than a reference to a
pair. In other words, dense maps return a so called _proxy iterator_ the value
type of which is:

* `std::pair<const Key &, Type &>` for non-const iterator types.
* `std::pair<const Key &, const Type &>` for const iterator types.

This is quite different from what any standard library map returns and should be
taken into account when looking for a drop-in replacement.

## Dense set

The dense set made available in `EnTT` is a hash set that aims to return a
packed array of elements, so as to reduce the number of jumps in memory during
iterations.<br/>
The implementation is based on _sparse sets_ and each bucket is identified by an
implicit list within the packed array itself.

The interface is in all respects similar to its counterpart in the standard
library, that is, the `std::unordered_set` class.<br/>
However, this type of set also supports reverse iteration and therefore offers
all the functions necessary for the purpose (such as `rbegin` and `rend`).

# Adaptors

## Table

The `basic_table` class is a container adaptor which manages multiple sequential
containers together, treating them as different columns of the same table.<br/>
The `table` alias allows users to provide only the types to handle, using
`std::vector` as the default sequential container.

Only a small set of functions is provided, although very close to what the API
of the `std::vector` class offers.<br/>
The internal implementation is purposely supported by a tuple of containers
rather than a container of tuples. The purpose is to allow efficient access to
single columns and not just access to the entire data set of the table.

When a row is accessed, all data is returned in the form of a tuple containing
(possibly const) references to the elements of the row itself.<br/>
Similarly, when a table is iterated, tuples of references to table elements are
returned for each row.
