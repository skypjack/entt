# Crash Course: graph

# Table of Contents

* [Introduction](#introduction)
* [Data structures](#data-structures)
  * [Adjacency matrix](#adjacency-matrix)
  * [Graphviz dot language](#graphviz-dot-language)
* [Flow builder](#flow-builder)
  * [Tasks and resources](#tasks-and-resources)
  * [Fake resources and order of execution](#fake-resources-and-order-of-execution)
  * [Sync points](#sync-points)
  * [Execution graph](#execution-graph)

# Introduction

`EnTT` doesn't aim to offer everything one needs to work with graphs. Therefore,
anyone looking for this in the _graph_ submodule will be disappointed.<br/>
Quite the opposite is true though. This submodule is minimal and contains only
the data structures and algorithms strictly necessary for the development of
some tools such as the _flow builder_.

# Data structures

As anticipated in the introduction, the aim isn't to offer all possible data
structures suitable for representing and working with graphs. Many will likely
be added or refined over time. However I want to discourage anyone expecting
tight scheduling on the subject.<br/>
The data structures presented in this section are mainly useful for the
development and support of some tools which are also part of the same submodule.

## Adjacency matrix

The adjacency matrix is designed to represent either a directed or an undirected
graph:

```cpp
entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{};
```

The `directed_tag` type _creates_ the graph as directed. There is also an
`undirected_tag` counterpart which creates it as undirected.<br/>
The interface deviates slightly from the typical double indexing of C and offers
an API that is perhaps more familiar to a C++ programmer. Therefore, the access
and modification of an element takes place via the `contains`, `insert` and
`erase` functions rather than a double call to an `operator[]`:

```cpp
if(adjacency_matrix.contains(0u, 1u)) {
    adjacency_matrix.erase(0u, 1u);
} else {
    adjacency_matrix.insert(0u, 1u);
}
```

Both `insert` and` erase` are _idempotent_ functions which have no effect if the
element already exists or has already been deleted.<br/>
The first one returns an `std::pair` containing the iterator to the element and
a boolean value indicating whether the element was newly inserted or not. The
second one returns the number of deleted elements (0 or 1).

An adjacency matrix is initialized with the number of elements (vertices) when
constructing it but can also be resized later using the `resize` function:

```cpp
entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
```

To visit all vertices, the class offers a function named `vertices` that returns
an iterable object suitable for the purpose:

```cpp
for(auto &&vertex: adjacency_matrix.vertices()) {
    // ...
}
```

The same result is obtained with the following snippet, since the vertices are
plain unsigned integral values:

```cpp
for(auto last = adjacency_matrix.size(), pos = {}; pos < last; ++pos) {
    // ...
}
```

As for visiting the edges, a few functions are available.<br/>
When the purpose is to visit all the edges of a given adjacency matrix, the
`edges` function returns an iterable object that is used to get them as pairs of
vertices:

```cpp
for(auto [lhs, rhs]: adjacency_matrix.edges()) {
    // ...
}
```

If the goal is to visit all the in- or out-edges of a given vertex instead, the
`in_edges` and `out_edges` functions are meant for that:

```cpp
for(auto [lhs, rhs]: adjacency_matrix.out_edges(3u)) {
    // ...
}
```

Both the functions expect the vertex to visit (that is, to return the in- or
out-edges for) as an argument.<br/>
Finally, the adjacency matrix is an allocator-aware container and offers most of
the functionalities one would expect from this type of containers, such as
`clear` or 'get_allocator` and so on.

## Graphviz dot language

As it's one of the most popular formats, the library offers minimal support for
converting a graph to a Graphviz dot snippet.<br/>
The simplest way is to pass both an output stream and a graph to the `dot`
function:

```cpp
std::ostringstream output{};
entt::dot(output, adjacency_matrix);
```

It's also possible to provide a callback to which the vertices are passed and
which can be used to add (`dot`) properties to the output as needed:

```cpp
std::ostringstream output{};

entt::dot(output, adjacency_matrix, [](auto &output, auto vertex) {
    out << "label=\"v\"" << vertex << ",shape=\"box\"";
});
```

This second mode is particularly convenient when the user wants to associate
externally managed data to the graph being converted.

# Flow builder

A flow builder is used to create execution graphs from tasks and resources.<br/>
The implementation is as generic as possible and doesn't bind to any other part
of the library.

This class is designed as a sort of _state machine_ to which a specific task is
attached for which the resources accessed in read-only or read-write mode are
specified.<br/>
Most of the functions in the API also return the flow builder itself, according
to what is the common sense API when it comes to builder classes.

Once all tasks are registered and resources assigned to them, an execution graph
in the form of an adjacency matrix is returned to the user.<br/>
This graph contains all the tasks assigned to the flow builder in the form of
_vertices_. The _vertex_ itself is used as an index to get the identifier passed
during registration.

## Tasks and resources

Although these terms are used extensively in the documentation, the flow builder
has no real concept of tasks and resources.<br/>
This class works mainly with _identifiers_, that is, values of type `id_type`.
In other terms, both tasks and resources are identified by integral values.<br/>
This allows not to couple the class itself to the rest of the library or to any
particular data structure. On the other hand, it requires the user to keep track
of the association between identifiers and operations or actual data.

Once a flow builder is created (which requires no constructor arguments), the
first thing to do is to bind a task. This tells to the builder _who_ intends to
consume the resources that are specified immediately after:

```cpp
entt::flow builder{};
builder.bind("task_1"_hs);
```

The example uses the `EnTT` hashed string to generate an identifier for the
task.<br/>
Indeed, the use of `id_type` as an identifier type isn't by accident. In fact,
it matches well with the internal hashed string class. Moreover, it's also the
same type returned by the hash function of the internal RTTI system, in case the
user wants to rely on that.<br/>
However, being an integral value, it leaves the user full freedom to rely on his
own tools if necessary.

Once a task is associated with the flow builder, it's also assigned read-only or
read-write resources as appropriate:

```cpp
builder
    .bind("task_1"_hs)
        .ro("resource_1"_hs)
        .ro("resource_2"_hs)
    .bind("task_2"_hs)
        .rw("resource_2"_hs)
```

As mentioned, many functions return the builder itself and it's therefore easy
to concatenate the different calls.<br/>
Also in the case of resources, they are identified by numeric values of type
`id_type`. As above, the choice is not entirely random. This goes well with the
tools offered by the library while leaving room for maximum flexibility.

Finally, both the `ro` and` rw` functions also offer an overload that accepts a
pair of iterators, so that one can pass a range of resources in one go.

### Rebinding

The `flow` class is resource based rather than task based. This means that graph
generation is driven by resources and not by the order of _appearance_ of tasks
during flow definition.<br/>
Although this concept is particularly important, it's almost irrelevant for the
vast majority of cases. However, it becomes relevant when _rebinding_ resources
or tasks.

In fact, nothing prevents rebinding elements to a flow.<br/>
However, the behavior changes slightly from case to case and has some nuances
that it's worth knowing about.

Directly rebinding a resource without the task being replaced trivially results
in the task's access mode for that resource being updated:

```cpp
builder.bind("task"_hs).rw("resource"_hs).ro("resource"_hs)
```

In this case, the resource is accessed in read-only mode, regardless of the
first call to `rw`.<br/>
Behind the scenes, the call doesn't actually _replace_ the previous one but is
queued after it instead, overwriting it when generating the graph. Thus, a large
number of resource rebindings may even impact processing times (very difficult
to observe but theoretically possible).

Rebinding resources and also combining it with changes to tasks has far more
implications instead.<br/>
As mentioned, graph generation takes place starting from resources and not from
tasks. Therefore, the result may not be as expected:

```cpp
builder
    .bind("task_1"_hs)
        .ro("resource"_hs)
    .bind("task_2"_hs)
        .ro("resource"_hs)
    .bind("task_1"_hs)
        .rw("resource"_hs);
```

What happens here is that the resource first _sees_ a read-only access request
from the first task, then a read-only request from the second task and finally
a read-write request from the first task.<br/>
Although this definition would probably be counted as an error, the resulting
graph may be unexpected. This in fact consists of a single edge outgoing from
the second task and directed to the first task.<br/>
To intuitively understand what happens, it's enough to think of the fact that a
task never has an edge pointing to itself.

While not obvious, this approach has its pros and cons like any other solution.
For example, creating loops is actually simple in the context of resource-based
graph generations:

```cpp
builder
    .bind("task_1"_hs)
        .rw("resource"_hs)
    .bind("task_2"_hs)
        .rw("resource"_hs)
    .bind("task_1"_hs)
        .rw("resource"_hs);
```

As expected, this definition leads to the creation of two edges that define a
loop between the two tasks.

As a general rule, rebinding resources and tasks is highly discouraged because
it could lead to subtle bugs if users don't know what they're doing.<br/>
However, once the mechanisms of resource-based graph generation are understood,
it can offer to the expert user a flexibility and a range of possibilities
otherwise inaccessible.

## Fake resources and order of execution

The flow builder doesn't offer the ability to specify when a task should execute
before or after another task.<br/>
In fact, the order of _registration_ on the resources also determines the order
in which the tasks are processed during the generation of the execution graph.

However, there is a way to _force_ the execution order of two processes.<br/>
Briefly, since accessing a resource in opposite modes requires sequential rather
than parallel scheduling, it's possible to make use of fake resources to rule on
the execution order:

```cpp
builder
    .bind("task_1"_hs)
        .ro("resource_1"_hs)
        .rw("fake"_hs)
    .bind("task_2"_hs)
        .ro("resource_2"_hs)
        .ro("fake"_hs)
    .bind("task_3"_hs)
        .ro("resource_2"_hs)
        .ro("fake"_hs)
```

This snippet forces the execution of `task_1` **before** `task_2` and `task_3`.
This is due to the fact that the former sets a read-write requirement on a fake
resource that the other tasks also want to access in read-only mode.<br/>
Similarly, it's possible to force a task to run **after** a certain group:

```cpp
builder
    .bind("task_1"_hs)
        .ro("resource_1"_hs)
        .ro("fake"_hs)
    .bind("task_2"_hs)
        .ro("resource_1"_hs)
        .ro("fake"_hs)
    .bind("task_3"_hs)
        .ro("resource_2"_hs)
        .rw("fake"_hs)
```

In this case, since there are a number of processes that want to read a specific
resource, they will do so in parallel by forcing `task_3` to run after all the
others tasks.

## Sync points

Sometimes it's useful to assign the role of _sync point_ to a node.<br/>
Whether it accesses new resources or is simply a watershed, the procedure for
assigning this role to a vertex is always the same. First it's tied to the flow
builder, then the `sync` function is invoked:

```cpp
builder.bind("sync_point"_hs).sync();
```

The choice to assign an _identity_ to this type of nodes lies in the fact that,
more often than not, they also perform operations on resources.<br/>
If this isn't the case, it will still be possible to create no-op vertices to
which empty tasks are assigned.

## Execution graph

Once both the resources and their consumers have been properly registered, the
purpose of this tool is to generate an execution graph that takes into account
all specified constraints to return the best scheduling for the vertices:

```cpp
entt::adjacency_matrix<entt::directed_tag> graph = builder.graph();
```

Searching for the main vertices (that is, those without in-edges) is usually the
first thing required:

```cpp
for(auto &&vertex: graph) {
    if(auto in_edges = graph.in_edges(vertex); in_edges.begin() == in_edges.end()) {
        // ...
    }
}
```

Then it's possible to instantiate an execution graph by means of other functions
such as `out_edges` to retrieve the children of a given task or `edges` to get
the identifiers.
