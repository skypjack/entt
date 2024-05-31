# Crash Course: events, signals and everything in between

# Table of Contents

* [Introduction](#introduction)
* [Delegate](#delegate)
  * [Runtime arguments](#runtime-arguments)
  * [Lambda support](#lambda-support)
  * [Raw access](#raw-access)
* [Signals](#signals)
* [Event dispatcher](#event-dispatcher)
  * [Named queues](#named-queues)
* [Event emitter](#event-emitter)

# Introduction

Signals are more often than not a core part of games and software architectures
in general.<br/>
They help to decouple the various parts of a system while allowing them to
communicate with each other somehow.

The so called _modern C++_ comes with a tool that can be useful in this regard,
the `std::function`. As an example, it can be used to create delegates.<br/>
However, there is no guarantee that an `std::function` doesn't perform
allocations under the hood and this could be problematic sometimes. Furthermore,
it solves a problem but may not adapt well to other requirements that may arise
from time to time.

In case that the flexibility and power of an `std::function` isn't required or
if the price to pay for them is too high, `EnTT` offers a complete set of
lightweight classes to solve the same and many other problems.

# Delegate

A delegate can be used as a general purpose invoker with no memory overhead for
free functions, lambdas and members provided along with an instance on which to
invoke them.<br/>
It doesn't claim to be a drop-in replacement for an `std::function`, so don't
expect to use it whenever an `std::function` fits well. That said, it's most
likely even a better fit than an `std::function` in a lot of cases, so expect to
use it quite a lot anyway.

The interface is trivial. It offers a default constructor to create empty
delegates:

```cpp
entt::delegate<int(int)> delegate{};
```

What is needed to create an instance is to specify the type of the function the
delegate _accepts_, that is the signature of the functions it models.<br/>
However, attempting to use an empty delegate by invoking its function call
operator results in undefined behavior or most likely a crash.

There exist a few overloads of the `connect` member function to initialize a
delegate:

```cpp
int f(int i) { return i; }

struct my_struct {
    int f(const int &i) const { return i; }
};

// bind a free function to the delegate
delegate.connect<&f>();

// bind a member function to the delegate
my_struct instance;
delegate.connect<&my_struct::f>(instance);
```

The delegate class also accepts data members, if needed. In this case, the
function type of the delegate is such that the parameter list is empty and the
value of the data member is at least convertible to the return type.

Free functions having type equivalent to `void(T &, args...)` are accepted as
well. The first argument `T &` is considered a payload and the function will
receive it back every time it's invoked. In other terms, this works just fine
with the above definition:

```cpp
void g(const char &c, int i) { /* ... */ }
const char c = 'c';

delegate.connect<&g>(c);
delegate(42);
```

Function `g` is invoked with a reference to `c` and `42`. However, the function
type of the delegate is still `void(int)`. This is also the signature of its
function call operator.<br/>
Another interesting aspect of the delegate class is that it accepts functions
with a list of parameters that is shorter than that of its function type:

```cpp
void g() { /* ... */ }
delegate.connect<&g>();
delegate(42);
```

Where the function type of the delegate is `void(int)` as above. It goes without
saying that the extra arguments are silently discarded internally. This is a
nice-to-have feature in a lot of cases, as an example when the `delegate` class
is used as a building block of a signal-slot system.<br/>
In fact, this filtering works both ways. The class tries to pass its first
_count_ arguments **first**, then the last _count_. Watch out for conversion
rules if in doubt when connecting a listener!<br/>
Arbitrary functions that pull random arguments from the delegate list aren't
supported instead. Other feature were preferred, such as support for functions
with compatible argument lists although not equal to those of the delegate.

To create and initialize a delegate at once, there are a few specialized
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
already shown in the examples above:

```cpp
auto ret = delegate(42);
```

In all cases, listeners don't have to strictly follow the signature of the
delegate. As long as a listener can be invoked with the given arguments to yield
a result that is convertible to the given result type, everything works just
fine.

As a side note, members of classes may or may not be associated with instances.
If they are not, the first argument of the function type must be that of the
class on which the members operate and an instance of this class must obviously
be passed when invoking the delegate:

```cpp
entt::delegate<void(my_struct &, int)> delegate;
delegate.connect<&my_struct::f>();

my_struct instance;
delegate(instance, 42);
```

In this case, it's not possible to _deduce_ the function type since the first
argument doesn't necessarily have to be a reference (for example, it can be a
pointer, as well as a const reference).<br/>
Therefore, the function type must be declared explicitly for unbound members.

## Runtime arguments

The `delegate` class is meant to be used primarily with template arguments.
However, as a consequence of its design, it also offers minimal support for
runtime arguments.<br/>
When used like this, some features aren't supported though. In particular:

* Curried functions aren't accepted.
* Functions with an argument list that differs from that of the delegate aren't
  supported.
* Return type and types of arguments **must** coincide with those of the
  delegate and _being at least convertible_ isn't enough anymore.

Moreover, for a given function type `Ret(Args...)`, the signature of the
functions connected at runtime must necessarily be `Ret(const void *, Args...)`.

Runtime arguments can be passed both to the constructor of a delegate and to the
`connect` member function. An optional parameter is also accepted in both cases.
This argument is used to pass arbitrary user data back and forth as a
`const void *` upon invocation.<br/>
To connect a function to a delegate _in the hard way_:

```cpp
int func(const void *ptr, int i) { return *static_cast<const int *>(ptr) * i; }
const int value = 42;

// use the constructor ...
entt::delegate delegate{&func, &value};

// ... or the connect member function
delegate.connect(&func, &value);
```

The type of the delegate is deduced from the function if possible. In this case,
since the first argument is an implementation detail, the deduced function type
is `int(int)`.<br/>
Invoking a delegate built in this way follows the same rules as previously
explained.

## Lambda support

In general, the `delegate` class doesn't fully support lambda functions in all
their nuances. The reason is pretty simple: a `delegate` isn't a drop-in
replacement for an `std::function`. Instead, it tries to overcome the problems
with the latter.<br/>
That being said, non-capturing lambda functions are supported, even though some
features aren't available in this case.

This is a logical consequence of the support for connecting functions at
runtime. Therefore, lambda functions undergo the same rules and
limitations.<br/>
In fact, since non-capturing lambda functions decay to pointers to functions,
they can be used with a `delegate` as if they were _normal functions_ with
optional payload:

```cpp
my_struct instance;

// use the constructor ...
entt::delegate delegate{+[](const void *ptr, int value) {
    return static_cast<const my_struct *>(ptr)->f(value);
}, &instance};

// ... or the connect member function
delegate.connect([](const void *ptr, int value) {
    return static_cast<const my_struct *>(ptr)->f(value);
}, &instance);
```

As above, the first parameter (`const void *`) isn't part of the function type
of the delegate and is used to dispatch arbitrary user data back and forth. In
other terms, the function type of the delegate above is `int(int)`.

## Raw access

While not recommended, a delegate also allows direct access to the stored
callable function target and underlying data, if any.<br/>
This makes it possible to bypass the behavior of the delegate itself and force
calls on different instances:

```cpp
my_struct other;
delegate.target(&other, 42);
```

It goes without saying that this type of approach is **very** risky, especially
since there is no way of knowing whether the contained function was originally a
member function of some class, a free function or a lambda.<br/>
Another possible (and meaningful) use of this feature is that of identifying a
particular delegate through its descriptive _traits_ instead.

# Signals

Signal handlers work with references to classes, function pointers and pointers
to members. Listeners can be any kind of objects and users are in charge of
connecting and disconnecting them from a signal to avoid crashes due to
different lifetimes. On the other side, performance shouldn't be affected that
much by the presence of such a signal handler.<br/>
Signals make use of delegates internally and therefore they undergo the same
rules and offer similar functionalities. It may be a good idea to consult the
documentation of the `delegate` class for further information.

A signal handler is can be used as a private data member without exposing any
_publish_ functionality to the clients of a class.<br/>
The basic idea is to impose a clear separation between the signal itself and the
`sink` class, that is a tool to be used to connect and disconnect listeners on
the fly.

The API of a signal handler is straightforward. If a collector is supplied to
the signal when something is published, all the values returned by its listeners
are literally _collected_ and used later by the caller. Otherwise, the class
works just like a plain signal that emits events from time to time.<br/>
To create instances of signal handlers it's sufficient to provide the type of
function to which they refer:

```cpp
entt::sigh<void(int, char)> signal;
```

Signals offer all the basic functionalities required to know how many listeners
they contain (`size`) or if they contain at least a listener (`empty`), as well
as a function to use to swap handlers (`swap`).

Besides them, there are member functions to use both to connect and disconnect
listeners in all their forms by means of a sink:

```cpp
void foo(int, char) { /* ... */ }

struct listener {
    void bar(const int &, char) { /* ... */ }
};

// ...

entt::sink sink{signal};
listener instance;

sink.connect<&foo>();
sink.connect<&listener::bar>(instance);

// ...

// disconnects a free function
sink.disconnect<&foo>();

// disconnect a member function of an instance
sink.disconnect<&listener::bar>(instance);

// disconnect all member functions of an instance, if any
sink.disconnect(&instance);

// discards all listeners at once
sink.disconnect();
```

As shown above, listeners don't have to strictly follow the signature of the
signal. As long as a listener can be invoked with the given arguments to yield a
result that is convertible to the given return type, everything works just
fine.<br/>
In all cases, the `connect` member function returns by default a `connection`
object to be used as an alternative to break a connection by means of its
`release` member function.<br/>
A `scoped_connection` can also be created from a connection. In this case, the
link is broken automatically as soon as the object goes out of scope.

Once listeners are attached (or even if there are no listeners at all), events
and data in general are published through a signal by means of the `publish`
member function:

```cpp
signal.publish(42, 'c');
```

To collect data, the `collect` member function is used instead:

```cpp
int f() { return 0; }
int g() { return 1; }

// ...

entt::sigh<int()> signal;
entt::sink sink{signal};

sink.connect<&f>();
sink.connect<&g>();

std::vector<int> vec{};
signal.collect([&vec](int value) { vec.push_back(value); });

assert(vec[0] == 0);
assert(vec[1] == 1);
```

A collector must expose a function operator that accepts as an argument a type
to which the return type of the listeners can be converted. Moreover, it can
optionally return a boolean value that is true to stop collecting data, false
otherwise. This way one can avoid calling all the listeners in case it isn't
necessary.<br/>
Functors can also be used in place of a lambda. Since the collector is copied
when invoking the `collect` member function, `std::ref` is the way to go in this
case:

```cpp
struct my_collector {
    std::vector<int> vec{};

    bool operator()(int v) {
        vec.push_back(v);
        return true;
    }
};

// ...

my_collector collector;
signal.collect(std::ref(collector));
```

# Event dispatcher

The event dispatcher class allows users to trigger immediate events or to queue
and publish them all together later.<br/>
This class lazily instantiates its queues. Therefore, it's not necessary to
_announce_ the event types in advance:

```cpp
// define a general purpose dispatcher
entt::dispatcher dispatcher{};
```

A listener registered with a dispatcher is such that its type offers one or more
member functions that take arguments of type `Event &` for any type of event,
regardless of the return value.<br/>
These functions are linked directly via `connect` to a _sink_:

```cpp
struct an_event { int value; };
struct another_event {};

struct listener {
    void receive(const an_event &) { /* ... */ }
    void method(const another_event &) { /* ... */ }
};

// ...

listener listener;
dispatcher.sink<an_event>().connect<&listener::receive>(listener);
dispatcher.sink<another_event>().connect<&listener::method>(listener);
```

Note that connecting listeners within event handlers can result in undefined
behavior.<br/>
The `disconnect` member function is used to remove one listener at a time or all
of them at once:

```cpp
dispatcher.sink<an_event>().disconnect<&listener::receive>(listener);
dispatcher.sink<another_event>().disconnect(&listener);
```

The `trigger` member function serves the purpose of sending an immediate event
to all the listeners registered so far:

```cpp
dispatcher.trigger(an_event{42});
dispatcher.trigger<another_event>();
```

Listeners are invoked immediately, order of execution isn't guaranteed. This
method can be used to push around urgent messages like an _is terminating_
notification on a mobile app.

On the other hand, the `enqueue` member function queues messages together and
helps to maintain control over the moment they are sent to listeners:

```cpp
dispatcher.enqueue<an_event>(42);
dispatcher.enqueue(another_event{});
```

Events are stored aside until the `update` member function is invoked:

```cpp
// emits all the events of the given type at once
dispatcher.update<an_event>();

// emits all the events queued so far at once
dispatcher.update();
```

This way users can embed the dispatcher in a loop and literally dispatch events
once per tick to their systems.

## Named queues

All queues within a dispatcher are associated by default with an event type and
then retrieved from it.<br/>
However, it's possible to create queues with different _names_ (and therefore
also multiple queues for a single type). In fact, more or less all functions
also take an additional parameter. As an example:

```cpp
dispatcher.sink<an_event>("custom"_hs).connect<&listener::receive>(listener);
```

In this case, the term _name_ is misused as these are actual numeric identifiers
of type `id_type`.<br/>
An exception to this rule is the `enqueue` function. There is no additional
parameter for it but rather a different function:

```cpp
dispatcher.enqueue_hint<an_event>("custom"_hs, 42);
```

This is mainly due to the template argument deduction rules and unfortunately
there is no real (elegant) way to avoid it.

# Event emitter

A general purpose event emitter thought mainly for those cases where it comes to
working with asynchronous stuff.<br/>
Originally designed to fit the requirements of
[`uvw`](https://github.com/skypjack/uvw) (a wrapper for `libuv` written in
modern C++), it was adapted later to be included in this library.

To create an emitter type, derived classes must inherit from the base as:

```cpp
struct my_emitter: emitter<my_emitter> {
    // ...
}
```

Handlers for the different events are created internally on the fly. It's not
required to specify in advance the full list of accepted events.<br/>
Moreover, whenever an event is published, an emitter also passes a reference
to itself to its listeners.

To create new instances of an emitter, no arguments are required:

```cpp
my_emitter emitter{};
```

Listeners are movable and callable objects (free functions, lambdas, functors,
`std::function`s, whatever) whose function type is compatible with:

```cpp
void(Type &, my_emitter &)
```

Where `Type` is the type of event they want to receive.<br/>
To attach a listener to an emitter, there exists the `on` member function:

```cpp
emitter.on<my_event>([](const my_event &event, my_emitter &emitter) {
    // ...
});
```

Similarly, the `reset` member function is used to disconnect listeners given a
type while `clear` is used to disconnect all listeners at once:

```cpp
// resets the listener for my_event
emitter.erase<my_event>();

// resets all listeners
emitter.clear()
```

To send an event to the listener registered on a given type, the `publish`
function is the way to go:

```cpp
struct my_event { int i; };

// ...

emitter.publish(my_event{42});
```

Finally, the `empty` member function tests if there exists at least a listener
registered with the event emitter while `contains` is used to check if a given
event type is associated with a valid listener:

```cpp
if(emitter.contains<my_event>()) {
    // ...
}
```

This class introduces a _nice-to-have_ model based on events and listeners.<br/>
More in general, it's a handy tool when the derived classes _wrap_ asynchronous
operations but it's not limited to such uses.
