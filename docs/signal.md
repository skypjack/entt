# Crash Course: events, signals and everything in between

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
* [Delegate](#delegate)
* [Signals](#signals)
* [Event dispatcher](#event-dispatcher)
* [Event emitter](#event-emitter)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Signals are usually a core part of games and software architectures in
general.<br/>
Roughly speaking, they help to decouple the various parts of a system while
allowing them to communicate with each other somehow.

The so called _modern C++_ comes with a tool that can be useful in these terms,
the `std::function`. As an example, it can be used to create delegates.<br/>
However, there is no guarantee that an `std::function` does not perform
allocations under the hood and this could be problematic sometimes. Furthermore,
it solves a problem but may not adapt well to other requirements that may arise
from time to time.

In case that the flexibility and potential of an `std::function` are not
required or where you are looking for something different, `EnTT` offers a full
set of classes to solve completely different problems.

# Delegate

A delegate can be used as a general purpose invoker with no memory overhead for
free functions and members provided along with an instance on which to invoke
them.<br/>
It does not claim to be a drop-in replacement for an `std::function`, so do not
expect to use it whenever an `std::function` fits well. However, it can be used
to send opaque delegates around to be used to invoke functions as needed.

The interface is trivial. It offers a default constructor to create empty
delegates:

```cpp
entt::delegate<int(int)> delegate{};
```

All what is needed to create an instance is to specify the type of the function
the delegate will _contain_, that is the signature of the free function or the
member function one wants to assign to it.

Attempting to use an empty delegate by invoking its function call operator
results in undefined behavior or most likely a crash. Before to use a delegate,
it must be initialized.<br/>
There exists a bunch of overloads of the `connect` member function to do that.
As an example of use:

```cpp
int f(int i) { return i; }

struct my_struct {
    int f(const int &i) { return i }
};

// bind a free function to the delegate
delegate.connect<&f>();

// bind a member function to the delegate
my_struct instance;
delegate.connect<&my_struct::f>(&instance);
```

The delegate class accepts also data members, if needed. In this case, the
function type of the delegate is such that the parameter list is empty and the
value of the data member is at least convertible to the return type.<br/>
Functions having type equivalent to `void(T *, args...)` are accepted as well.
In this case, `T *` is considered a payload and the function will receive it
back every time it's invoked. In other terms, this works just fine with the
above definition:

```cpp
void g(const char *c, int i) { /* ... */ }
const char c = 'c';

delegate.connect<&g>(&c);
delegate(42);
```

The function `g` will be invoked with a pointer to `c` and `42`. However, the
function type of the delegate is still `void(int)`, mainly because this is also
the signature of its function call operator.

To create and initialize a delegate at once, there are also some specialized
constructors. Because of the rules of the language, the listener is provided by
means of the `entt::connect_arg` variable template:

```cpp
entt::delegate<int(int)> func{entt::connect_arg<&f>};
```

Aside `connect`, a `disconnect` counterpart isn't provided. Instead, there
exists a `reset` member function to use to clear a delegate.<br/>
To know if a delegate is empty, it can be used explicitly in every conditional
statement:

```cpp
if(delegate) {
    // ...
}
```

Finally, to invoke a delegate, the function call operator is the way to go as
usual:

```cpp
auto ret = delegate(42);
```

As shown above, listeners do not have to strictly follow the signature of the
delegate. As long as a listener can be invoked with the given arguments to yield
a result that is convertible to the given result type, everything works just
fine.

Probably too much small and pretty poor of functionalities, but the delegate
class can help in a lot of cases and it has shown that it is worth keeping it
within the library.

# Signals

Signal handlers work with naked pointers, function pointers and pointers to
member functions. Listeners can be any kind of objects and users are in charge
of connecting and disconnecting them from a signal to avoid crashes due to
different lifetimes. On the other side, performance shouldn't be affected that
much by the presence of such a signal handler.<br/>
A signal handler can be used as a private data member without exposing any
_publish_ functionality to the clients of a class. The basic idea is to impose a
clear separation between the signal itself and its _sink_ class, that is a tool
to be used to connect and disconnect listeners on the fly.

The API of a signal handler is straightforward. The most important thing is that
it comes in two forms: with and without a collector. In case a signal is
associated with a collector, all the values returned by the listeners can be
literally _collected_ and used later by the caller. Otherwise it works just like
a plain signal that emits events from time to time.<br/>

**Note**: collectors are allowed only in case of function types whose the return
type isn't `void` for obvious reasons.

To create instances of signal handlers there exist mainly two ways:

```cpp
// no collector type
entt::sigh<void(int, char)> signal;

// explicit collector type
entt::sigh<void(int, char), my_collector<bool>> collector;
```

As expected, they offer all the basic functionalities required to know how many
listeners they contain (`size`) or if they contain at least a listener (`empty`)
and even to swap two signal handlers (`swap`).

Besides them, there are member functions to use both to connect and disconnect
listeners in all their forms by means of a sink:

```cpp
void foo(int, char) { /* ... */ }

struct listener {
    void bar(const int &, char) { /* ... */ }
};

// ...

listener instance;

signal.sink().connect<&foo>();
signal.sink().connect<&listener::bar>(&instance);

// ...

// disconnects a free function
signal.sink().disconnect<&foo>();

// disconnect a member function of an instance
signal.sink().disconnect<&listener::bar>(&instance);

// discards all the listeners at once
signal.sink().disconnect();
```

As shown above, listeners do not have to strictly follow the signature of the
signal. As long as a listener can be invoked with the given arguments to yield a
result that is convertible to the given result type, everything works just fine.

Once listeners are attached (or even if there are no listeners at all), events
and data in general can be published through a signal by means of the `publish`
member function:

```cpp
signal.publish(42, 'c');
```

To collect data, the `collect` member function should be used instead. Below is
a minimal example to show how to use it:

```cpp
struct my_collector {
    std::vector<int> vec{};

    bool operator()(int v) noexcept {
        vec.push_back(v);
        return true;
    }
};

int f() { return 0; }
int g() { return 1; }

// ...

entt::sigh<int(), my_collector<int>> signal;

signal.sink().connect<&f>();
signal.sink().connect<&g>();

my_collector collector = signal.collect();

assert(collector.vec[0] == 0);
assert(collector.vec[1] == 1);
```

A collector must expose a function operator that accepts as an argument a type
to which the return type of the listeners can be converted. Moreover, it has to
return a boolean value that is false to stop collecting data, true otherwise.
This way one can avoid calling all the listeners in case it isn't necessary.

# Event dispatcher

The event dispatcher class is designed so as to be used in a loop. It allows
users both to trigger immediate events or to queue events to be published all
together once per tick.<br/>
This class shares part of its API with the one of the signal handler, but it
doesn't require that all the types of events are specified when declared:

```cpp
// define a general purpose dispatcher that works with naked pointers
entt::dispatcher dispatcher{};
```

In order to register an instance of a class to a dispatcher, its type must
expose one or more member functions the arguments of which are such that
`const E &` can be converted to them for each type of event `E`, no matter what
the return value is.<br/>
The name of the member function aimed to receive the event must be provided to
the `connect` member function of the sink in charge for the specific event:

```cpp
struct an_event { int value; };
struct another_event {};

struct listener
{
    void receive(const an_event &) { /* ... */ }
    void method(const another_event &) { /* ... */ }
};

// ...

listener listener;
dispatcher.sink<an_event>().connect<&listener::receive>(&listener);
dispatcher.sink<another_event>().connect<&listener::method>(&listener);
```

The `disconnect` member function follows the same pattern and can be used to
selectively remove listeners:

```cpp
dispatcher.sink<an_event>().disconnect<&listener::receive>(&listener);
dispatcher.sink<another_event>().disconnect<&listener::method>(&listener);
```

The `trigger` member function serves the purpose of sending an immediate event
to all the listeners registered so far. It offers a convenient approach that
relieves users from having to create the event itself. Instead, it's enough to
specify the type of event and provide all the parameters required to construct
it.<br/>
As an example:

```cpp
dispatcher.trigger<an_event>(42);
dispatcher.trigger<another_event>();
```

Listeners are invoked immediately, order of execution isn't guaranteed. This
method can be used to push around urgent messages like an _is terminating_
notification on a mobile app.

On the other hand, the `enqueue` member function queues messages together and
allows to maintain control over the moment they are sent to listeners. The
signature of this method is more or less the same of `trigger`:

```cpp
dispatcher.enqueue<an_event>(42);
dispatcher.enqueue<another_event>();
```

Events are stored aside until the `update` member function is invoked, then all
the messages that are still pending are sent to the listeners at once:

```cpp
// emits all the events of the given type at once
dispatcher.update<my_event>();

// emits all the events queued so far at once
dispatcher.update();
```

This way users can embed the dispatcher in a loop and literally dispatch events
once per tick to their systems.

# Event emitter

A general purpose event emitter thought mainly for those cases where it comes to
working with asynchronous stuff.<br/>
Originally designed to fit the requirements of
[`uvw`](https://github.com/skypjack/uvw) (a wrapper for `libuv` written in
modern C++), it was adapted later to be included in this library.

To create a custom emitter type, derived classes must inherit directly from the
base class as:

```cpp
struct my_emitter: emitter<my_emitter> {
    // ...
}
```

The full list of accepted types of events isn't required. Handlers are created
internally on the fly and thus each type of event is accepted by default.

Whenever an event is published, an emitter provides the listeners with a
reference to itself along with a const reference to the event. Therefore
listeners have an handy way to work with it without incurring in the need of
capturing a reference to the emitter itself.<br/>
In addition, an opaque object is returned each time a connection is established
between an emitter and a listener, allowing the caller to disconnect them at a
later time.<br/>
The opaque object used to handle connections is both movable and copyable. On
the other side, an event emitter is movable but not copyable by default.

To create new instances of an emitter, no arguments are required:

```cpp
my_emitter emitter{};
```

Listeners must be movable and callable objects (free functions, lambdas,
functors, `std::function`s, whatever) whose function type is:

```cpp
void(const Event &, my_emitter &)
```

Where `Event` is the type of event they want to listen.<br/>
There are two ways to attach a listener to an event emitter that differ
slightly from each other:

* To register a long-lived listener, use the `on` member function. It is meant
  to register a listener designed to be invoked more than once for the given
  event type.<br/>
  As an example:

  ```cpp
  auto conn = emitter.on<my_event>([](const my_event &event, my_emitter &emitter) {
      // ...
  });
  ```

  The connection object can be freely discarded. Otherwise, it can be used later
  to disconnect the listener if required.

* To register a short-lived listener, use the `once` member function. It is
  meant to register a listener designed to be invoked only once for the given
  event type. The listener is automatically disconnected after the first
  invocation.<br/>
  As an example:

  ```cpp
  auto conn = emitter.once<my_event>([](const my_event &event, my_emitter &emitter) {
      // ...
  });
  ```

  The connection object can be freely discarded. Otherwise, it can be used later
  to disconnect the listener if required.

In both cases, the connection object can be used with the `erase` member
function:

```cpp
emitter.erase(conn);
```

There are also two member functions to use either to disconnect all the
listeners for a given type of event or to clear the emitter:

```cpp
// removes all the listener for the specific event
emitter.clear<my_event>();

// removes all the listeners registered so far
emitter.clear();
```

To send an event to all the listeners that are interested in it, the `publish`
member function offers a convenient approach that relieves users from having to
create the event:

```cpp
struct my_event { int i; };

// ...

emitter.publish<my_event>(42);
```

Finally, the `empty` member function tests if there exists at least either a
listener registered with the event emitter or to a given type of event:

```cpp
bool empty;

// checks if there is any listener registered for the specific event
empty = emitter.empty<my_event>();

// checks it there are listeners registered with the event emitter
empty = emitter.empty();
```

In general, the event emitter is a handy tool when the derived classes _wrap_
asynchronous operations, because it introduces a _nice-to-have_ model based on
events and listeners that kindly hides the complexity behind the scenes. However
it is not limited to such uses.
