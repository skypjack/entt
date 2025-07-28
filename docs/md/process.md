# Crash Course: cooperative scheduler

# Table of Contents

* [Introduction](#introduction)
* [The process](#the-process)
  * [Continuation](#continuation)
  * [Shared process](#shared-process)
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
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    my_process(const allocator_type &allocator, delta_type delay)
        : entt::process{allocator},
          remaining{delay}
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

## Continuation

A process may be followed by other processes upon successful termination.<br/>
This pairing can be set up at creation time, keeping the processes conceptually
separate from each other while still combining them at runtime:

```cpp
my_process process{};
process.then<my_other_process>();
```

This approach allows processes to be developed in isolation and combined to
define complex actions.<br/>
For example, a delayed operation where a parent process (such as a timer)
_schedules_ a child process (the deferred task) once the time is over.

The `then` function also accepts lambdas, which are associated with a dedicated
process internally:

```cpp
process.then([](entt::process &proc, std::uint32_t delta, void *data) {
    // ...
})
```

The lambda function is such that it accepts a reference to the process that
manages it (to be able to terminate it, pause it and so on), plus the usual
values also passed to the `update` function.

## Shared process

All processes inherit from `std::enable_shared_from_this` to allow sharing with
the caller.<br/>
The returned smart pointer was created using the allocator associated with the
scheduler and therefore all its processes. This same allocator is available by
invoking `get_allocator` on the process itself.

As far as possible, sharing a process is not intended to allow the caller to
manage it. This could actually compromise the proper functioning of the
scheduler and the process itself.<br/>
Rather, the purpose is to allow the callers to save a valid reference to the
process, allowing them to intervene in its lifecycle through calls like `pause`
and the like.

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

To attach a process to a scheduler, invoke the `attach` function with a process
type and the arguments to use to construct it:

```cpp
scheduler.attach<my_process>(_1000u);
```

The scheduler will also provide the process with its allocator as the first
argument.<br>
In case of lambdas or functors, the required signature is the one already seen
for the `then` function of a process:

```cpp
scheduler.attach([](entt::process &, std::uint32_t, void *){ /* ... */ });
```

In both cases, the newly created process is returned by reference and its `then`
member function is used to create chains of processes to run sequentially.<br/>
As a minimal example of use:

```cpp
// schedules a task in the form of a lambda function
scheduler.attach([](entt::process &, std::uint32_t, void *) {
    // ...
})
// appends a child in the form of another lambda function
.then([](entt::process &, std::uint32_t, void *) {
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
that is used to discard all the running processes at once:

```cpp
// aborts all the processes abruptly ...
scheduler.abort(true);

// ... or gracefully during the next tick
scheduler.abort();
```

The argument passed to the `abort` function indicates whether execution should
be stopped immediately or processes should be notified on the next tick.
