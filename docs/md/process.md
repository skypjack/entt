# Crash Course: cooperative scheduler

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [The process](#the-process)
  * [Adaptor](#adaptor)
* [The scheduler](#the-scheduler)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Sometimes processes are a useful tool to work around the strict definition of a
system and introduce logic in a different way, usually without resorting to the
introduction of other components.

`EnTT` offers a minimal support to this paradigm by introducing a few classes
that users can use to define and execute cooperative processes.

# The process

A typical process must inherit from the `process` class template that stays true
to the CRTP idiom. Moreover, derived classes must specify what's the intended
type for elapsed times.

A process should expose publicly the following member functions whether needed
(note that it isn't required to define a function unless the derived class wants
to _override_ the default behavior):

* `void update(Delta, void *);`

  It's invoked once per tick until a process is explicitly aborted or it
  terminates either with or without errors. Even though it's not mandatory to
  declare this member function, as a rule of thumb each process should at
  least define it to work properly. The `void *` parameter is an opaque pointer
  to user data (if any) forwarded directly to the process during an update.

* `void init();`

  It's invoked when the process joins the running queue of a scheduler. This
  happens as soon as it's attached to the scheduler if the process is a top
  level one, otherwise when it replaces its parent if the process is a
  continuation.

* `void succeeded();`

  It's invoked in case of success, immediately after an update and during the
  same tick.

* `void failed();`

  It's invoked in case of errors, immediately after an update and during the
  same tick.

* `void aborted();`

  It's invoked only if a process is explicitly aborted. There is no guarantee
  that it executes in the same tick, this depends solely on whether the
  process is aborted immediately or not.

Derived classes can also change the internal state of a process by invoking
`succeed` and `fail`, as well as `pause` and `unpause` the process itself. All
these are protected member functions made available to be able to manage the
life cycle of a process from a derived class.

Here is a minimal example for the sake of curiosity:

```cpp
struct my_process: entt::process<my_process, std::uint32_t> {
    using delta_type = std::uint32_t;

    my_process(delta_type delay)
        : remaining{delay}
    {}

    void update(delta_type delta, void *) {
        remaining -= std::min(remaining, delta);

        // ...

        if(!remaining) {
            succeed();
        }
    }

private:
    delta_type remaining;
};
```

## Adaptor

Lambdas and functors can't be used directly with a scheduler for they are not
properly defined processes with managed life cycles.<br/>
This class helps in filling the gap and turning lambdas and functors into
full featured processes usable by a scheduler.

The function call operator has a signature similar to the one of the `update`
function of a process but for the fact that it receives two extra arguments to
call whenever a process is terminated with success or with an error:

```cpp
void(Delta delta, void *data, auto succeed, auto fail);
```

Parameters have the following meaning:

* `delta` is the elapsed time.
* `data` is an opaque pointer to user data if any, `nullptr` otherwise.
* `succeed` is a function to call when a process terminates with success.
* `fail` is a function to call when a process terminates with errors.

Both `succeed` and `fail` accept no parameters at all.

Note that usually users shouldn't worry about creating adaptors at all. A
scheduler creates them internally each and every time a lambda or a functor is
used as a process.

# The scheduler

A cooperative scheduler runs different processes and helps managing their life
cycles.

Each process is invoked once per tick. If it terminates, it's removed
automatically from the scheduler and it's never invoked again. Otherwise it's
a good candidate to run one more time the next tick.<br/>
A process can also have a child. In this case, the parent process is replaced
with its child when it terminates and only if it returns with success. In case
of errors, both the parent process and its child are discarded. This way, it's
easy to create chain of processes to run sequentially.

Using a scheduler is straightforward. To create it, users must provide only the
type for the elapsed times and no arguments at all:

```cpp
entt::scheduler<std::uint32_t> scheduler;
```

It has member functions to query its internal data structures, like `empty` or
`size`, as well as a `clear` utility to reset it to a clean state:

```cpp
// checks if there are processes still running
const auto empty = scheduler.empty();

// gets the number of processes still running
entt::scheduler<std::uint32_t>::size_type size = scheduler.size();

// resets the scheduler to its initial state and discards all the processes
scheduler.clear();
```

To attach a process to a scheduler there are mainly two ways:

* If the process inherits from the `process` class template, it's enough to
  indicate its type and submit all the parameters required to construct it to
  the `attach` member function:

  ```cpp
  scheduler.attach<my_process>(1000u);
  ```

* Otherwise, in case of a lambda or a functor, it's enough to provide an
  instance of the class to the `attach` member function:

  ```cpp
  scheduler.attach([](auto...){ /* ... */ });
  ```

In both cases, the return value is an opaque object that offers a `then` member
function to use to create chains of processes to run sequentially.<br/>
As a minimal example of use:

```cpp
// schedules a task in the form of a lambda function
scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
    // ...
})
// appends a child in the form of another lambda function
.then([](auto delta, void *, auto succeed, auto fail) {
    // ...
})
// appends a child in the form of a process class
.then<my_process>(1000u);
```

To update a scheduler and therefore all its processes, the `update` member
function is the way to go:

```cpp
// updates all the processes, no user data are provided
scheduler.update(delta);

// updates all the processes and provides them with custom data
scheduler.update(delta, &data);
```

In addition to these functions, the scheduler offers an `abort` member function
that can be used to discard all the running processes at once:

```cpp
// aborts all the processes abruptly ...
scheduler.abort(true);

// ... or gracefully during the next tick
scheduler.abort();
```
