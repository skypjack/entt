# Frequently Asked Questions

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [FAQ](#faq)
  * [Why is my debug build on Windows so slow?](#why-is-my-debug-build-on-windows-so-slow)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

This is a constantly updated section where I'll try to put the answers to the
most frequently asked questions.<br/>
If you don't find your answer here, there are two cases: nobody has done it yet
or this section needs updating. In both cases, try to
[open a new issue](https://github.com/skypjack/entt/issues/new) or enter the
[gitter channel](https://gitter.im/skypjack/entt) and ask your question.
Probably someone already has an answer for you and we can then integrate this
part of the documentation.

# FAQ

## Why is my debug build on Windows so slow?

`EnTT` is an experimental project that I also use to keep me up-to-date with the
latest revision of the language and the standard library. For this reason, it's
likely that some classes you're working with are using standard containers under
the hood.<br/>
Unfortunately, it's known that the standard containers aren't particularly
performing in debugging (the reasons for this go beyond this document) and are
even less so on Windows apparently. Fortunately this can also be mitigated a
lot, achieving good results in many cases.

First of all, there are two things to do in a Windows project:

* Disable the [`/JMC`](https://docs.microsoft.com/cpp/build/reference/jmc)
  option (_Just My Code_ debugging), available starting in Visual Studio 2017
  version 15.8.

* Set the [`_ITERATOR_DEBUG_LEVEL`](https://docs.microsoft.com/cpp/standard-library/iterator-debug-level)
  macro to 0. This will disable checked iterators and iterator debugging.

Moreover, the macro `ENTT_DISABLE_ASSERT` should be defined to disable internal
checks made by `EnTT` in debug. These are asserts introduced to help the users,
but require to access to the underlying containers and therefore risk ruining
the performance in some cases.

With these changes, debug performance should increase enough for most cases. If
you want something more, you can can also switch to an optimization level `O0`
or preferably `O1`.
