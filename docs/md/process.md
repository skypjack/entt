# Crash Course: cooperative scheduler

# Table of Contents

* [Introduction](#introduction)
* [The process](#the-process)
  * [Adaptor](#adaptor)
  * [Continuation](#continuation)
* [The scheduler](#the-scheduler)

# Introduction

Processes are a useful tool to work around the strict definition of a system and
introduce logic in a different way, usually without resorting to other component
types.<br/>
`EnTT` offers minimal support to this paradigm by introducing a few classes used
to define and execute cooperative processes.

# The process

A typical task inherits from the `process` class template. Derived classes also
specify what the intended type for elapsed times is.

A process should implement the following member functions whether needed (note
that it is not required to define a function unless the derived class wants to
_override_ the default behavior):

* `void update(Delta, void *);`

  This is invoked once per tick until a process is explicitly aborted or ends
  either with or without errors. Each process should at least define it to work
  _properly_. The `void *` parameter is an opaque pointer to user data (if any)
  forwarded directly to the process during an update.

* `void succeeded();`

  This is invoked in case of success, immediately after an update and during the
  same tick.

* `void failed();`

  This is invoked in case of errors, immediately after an update and during the
  same tick.

* `void aborted();`

  This is invoked only if a process is explicitly aborted. There is no guarantee
  that it executes in the same tick. It depends solely on whether the process is
  aborted immediately or not.

A class can also change the state of a process by invoking `succeed` and `fail`,
as well as `pause` and `unpause` the process itself.<br/>
All these are public member functions made available to manage the life cycle of
a process easily.

Here is a minimal example for the sake of curiosity:

```cpp
struct my_process: entt::process {
    using delta_type = typename entt::process::delta_type;

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

Lambdas and functors cannot be used directly with a scheduler because they are
not properly defined processes with managed life cycles.<br/>
This class helps in filling the gap and turning lambdas and functors into
full-featured processes usable by a scheduler.

Function call operators have signatures similar to that of the `update` member
function of a process, except that they receive a reference to the handle to
manage its lifecycle as needed:

```cpp
void(entt::process &handle, delta_type delta, void *data);
```

Parameters have the following meaning:

* `handle` is a reference to the process handle itself.
* `delta` is the elapsed time.
* `data` is an opaque pointer to user data if any, `nullptr` otherwise.

The library also provides the `process_from` function to simplify the creation
of processes starting from lambdas or the like.

## Continuation

A process may be followed by other processes upon successful termination.<br/>
This pairing can be set up at creation time, keeping the processes conceptually
separate from each other while still combining them at runtime:

```cpp
my_process process{};
process.then(std::make_shared<my_other_process>());
```

This approach allows processes to be developed in isolation and combined to
define complex actions.<br/>
For example, a delayed operation where a parent process (such as a timer)
_schedules_ a child process (the deferred task) once the time is over.

# The scheduler

A cooperative scheduler runs different processes and helps manage their life
cycles.

Each process is invoked once per tick. If it terminates, it is removed
automatically from the scheduler, and it is never invoked again. Otherwise,
it is a good candidate to run one more time the next tick.<br/>
A process can also have a _child_. In this case, the parent process is replaced
with its child when it terminates and only if it returns with success. In case
of errors, both the parent process and its child are discarded. This way, it is
easy to create a _chain of processes_ to run sequentially.

Using a scheduler is straightforward. To create it, users must provide only the
type for the elapsed times and no arguments at all:

```cpp
entt::basic_scheduler<std::uint64_t> scheduler;
```

Otherwise, the `scheduler` alias is also available for the most common cases. It
uses `std::uint32_t` as a default type:

```cpp
entt::scheduler scheduler{};
```

The class has member functions to query its internal data structures, like
`empty` or `size`, as well as a `clear` utility to reset it to a clean state:

```cpp
// checks if there are processes still running
const auto empty = scheduler.empty();

// gets the number of processes still running
entt::scheduler::size_type size = scheduler.size();

// resets the scheduler to its initial state and discards all the processes
scheduler.clear();
```

To attach a process to a scheduler, it is enough to invoke the `attach` function
by providing the process itself as an argument:

```cpp
scheduler.attach(std::make_shared<my_process>(_1000u));
```

In case of a lambda or a functor, `process_from` should get the job done:

```cpp
scheduler.attach(entt::process_from([](entt::process &, std::uint32_t, void *){ /* ... */ }));
```

In both cases, the newly created process is returned by reference and its `then`
member function is used to create chains of processes to run sequentially.<br/>
As a minimal example of use:

```cpp
// schedules a task in the form of a lambda function
scheduler.attach(entt::process_from([](entt::process &, std::uint32_t, void *) {
    // ...
}))
// appends a child in the form of another lambda function
.then(entt::process_from([](entt::process &, std::uint32_t, void *) {
    // ...
}))
// appends a child in the form of a process class
.then(std::make_shared<my_process>(1000u));
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
that is used to discard all the running processes at once:

```cpp
// aborts all the processes abruptly ...
scheduler.abort(true);

// ... or gracefully during the next tick
scheduler.abort();
```

The argument passed to the `abort` function indicates whether execution should
be stopped immediately or processes should be notified on the next tick.
