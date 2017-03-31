# EnTT - Entity-Component System in modern C++

# Introduction

`EnTT` is a header-only, tiny and easy to use entity-component system in modern C++.<br/>
ECS is an architectural pattern used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

## Code Example

```
#include <iostream>
#include <registry.hpp>

struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

using ECS = entt::DefaultRegistry<Position, Velocity>;

int main() {
    ECS ecs;

    for(auto i = 0; i < 10; ++i) {
        auto entity = ecs.create();
        ecs.assign<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { ecs.assign<Velocity>(entity, i * .1f, i * .1f); }
    }

    std::cout << "single component view" << std::endl;

    for(auto entity: ecs.view<Position>()) {
        auto &position = ecs.get<Position>(entity);
        std::cout << position.x << "," << position.y << std::endl;
    }

    std::cout << "multi component view" << std::endl;

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        std::cout << position.x << "," << position.y << " - " << velocity.dx << "," << velocity.dy << std::endl;
        if(entity % 4) { ecs.remove<Velocity>(entity); }
        else { ecs.destroy(entity); }
    }

    std::cout << "single component view" << std::endl;

    for(auto entity: ecs.view<Position>()) {
        auto &position = ecs.get<Position>(entity);
        std::cout << position.x << "," << position.y << std::endl;
    }

    std::cout << "multi component view" << std::endl;

    for(auto entity: ecs.view<Position, Velocity>()) {
        auto &position = ecs.get<Position>(entity);
        auto &velocity = ecs.get<Velocity>(entity);
        std::cout << position.x << "," << position.y << " - " << velocity.dx << "," << velocity.dy << std::endl;
        if(entity % 4) { ecs.remove<Velocity>(entity); }
        else { ecs.destroy(entity); }
    }

    ecs.reset();
}
```

## Motivation

I started using another well known Entity Component System named [entityx](https://github.com/alecthomas/entityx).<br/>
While I was playing with it, I found that I didn't like that much the way it manages its memory.
Moreover, I was pretty sure that one could achieve better performance with a slightly modified pool under the hood.<br/>
That's also the reason for which the interface is quite similar to the one of _entityx_, so that `EnTT` can be used as a
drop-in replacement for it with a minimal effort.

### Performance

As it stands right now, `EnTT` is just fast enough for my requirements if compared to my first choice (that was already
amazingly fast):

| Benchmark | EntityX (master) | EntityX (experimental/compile_time) | EnTT |
|-----------|-------------|-------------|-------------|
| Creating 10M entities | 0.281481s | 0.213988s | 0.00542235s |
| Destroying 10M entities | 0.166156s | 0.0673857s | 0.0582367s |
| Iterating over 10M entities, unpacking one component | 0.047039s | 0.0297941s | 9.3e-08s |
| Iterating over 10M entities, unpacking two components | 0.0701693s | 0.0412988s | 0.0206747s |

See [benchmark.cpp](https://github.com/skypjack/entt/blob/master/test/benchmark.cpp) for further details.<br/>
Of course, I'll try to get out of it more features and better performance anyway in the future, mainly for fun.
If you want to contribute and have any suggestion, feel free to make a PR or open an issue to discuss them.

# Build Instructions

## Requirements

To be able to use `EnTT`, users must provide a full-featured compiler that supports at least C++14.<br/>
CMake version 3.4 or later is mandatory to compile the tests, you don't have to install it otherwise.

## Library

`EnTT` is a header-only library. This means that including the `registry.hpp` header is enough to use it.<br/>
It's a matter of adding the following line at the top of a file:

    #include <registry.hpp>

Then pass the proper `-I` argument to the compiler to add the `src` directory to the include paths.<br/>

## Documentation

### API Reference

`EnTT` contains three main actors: the *registry*, the *view* and the *pool*.<br/>
Unless you have specific requirements of memory management, the default registry (that used the pool provided with
`EnTT`) should be good enough for any use. Customization is an option anyway, so that you can use your own pool as
long as it offers the expected interface.

#### The Registry

There are two options to instantiate your own registry:

* By using the default one:

    ```
    auto registry = entt::DefaultRegistry<Components...>{args...};
    ```

  That is, you must provide the whole list of components to be registered with the default registry.

* By using your own pool:

    ```
    auto registry = entt::Registry<YourOwnPool<Components...>{args...};
    ```

  Note that the registry expects a class template where the template parameters are the components to be managed.

In both cases, `args...` parameters are forwarded to the underlying pool during the construction.<br/>
There are no requirements for the components but to be moveable, therefore POD types are just fine.

Once you have created a registry, the followings are the exposed member functions:

* `size`: returns the number of entities still alive.
* `capacity`: returns the maximum number of entities created till now.
* `empty`: returns `true` if all the entities have been destroyed, `false` otherwise.
* `create`: creates a new entity and returns it, no components assigned.
* `create<Components...>`: creates a new entity and assigns it the given components, then returns the entity.
* `destroy`: destroys the entity and all its components.
* `assign<Component>(entity, args...)`: assigns the given component to the entity and uses `args...` to initialize it.
* `remove<Component>(entity)`: removes the given component from the entity.
* `has<Component>(entity)`: returns `true` if the entity has the given component, `false` otherwise.
* `get<Component>(entity)`: returns a reference to the given component for the entity (undefined behaviour if the entity has not the component).
* `replace<Component>(entity, args...)`: replaces the given component for the entity, using `args...` to create the new component.
* `clone(entity)`: clones an entity and all its components, then returns the new entity identifier.
* `copy<Component>(from, to)`: copies a component from an entity to another one (both the entities must already have been assigned the component, undefined behaviour otherwise).
* `copy(from, to)`: copies all the components and their contents from an entity to another one (comoonents are created or destroyed if needed).
* `reset()`: resets the pool and destroys all the entities and their components.
* `view<Components...>()`: gets a view of the entities that have the given components (see below for further details).

Note that entities are numbers and nothing more. They are not classes and they have no memeber functions at all.

#### The View

There are two different kinds of view, each one with a slighlty different interface:

* The _single component view_.
* The _multi component view_.

Both of them are iterable, that is both of them have `begin` and `end` member functions that are suitable for a range-based for loop:

```
auto view = registry.view<Position, Velocity>();

for(auto entity: view) {
    // do whatever you want with your entities
}
```

Iterators are extremely poor, they are meant exclusively to be used to iterate over a set of entities.<br/>
Exposed member functions are:

* `operator++()`
* `operator++(int)`
* `operator==()`
* `operator!=()`
* `operator*()`.

The single component view has an additional member function:

* `size()`: returns the exact number of expected entities.

The multi component view has an additional member function:

* `reset()`: reorganizes internal data so as to further create optimized iterators (use it whenever the data within the registry are known to be changed).

Both the views can be used more than once. They return newly created and correctly initialized iterators whenever
`begin` or `end` is invoked. Anyway views and iterators are tiny objects and the time to construct them can be safely ignored.
I'd suggest not to store them anywhere and to invoke the `Registry::view` member function at each iteration to get a properly
initialized view over which to iterate.

**Note**: An important feature (usually missed by other well known ECS) is that users can create and destroy entities, as
well as assign or remove components while iterating and neither the views nor the iterators will be invalidated.<br/>
Therefore, unless one tries to access a destroyed entity through an iterator that hasn't been advanced (in this case, of course,
it's an undefined behaviour), users can freely interact with the registry and keep views and iterators consistent.<br/>
On the other side, iterators aren't thread safe. Do no try to iterate over a set of components and modify them concurrently.
That being said, as long as a thread iterates over the entities that have the component `X` or assign and removes
that component from a set of entities and another thread does something similar with components `Y` and `Z`, it shouldn't be a
problem at all.<br/>
As an example, that means that users can freely run the rendering system over the renderable entities and update the physics
concurrently on a separate thread if needed.

#### The Pool

Custom pools for a given component can be defined as a specialization of the class template `ComponentPool`.<br/>
In particular:

```
template<>
struct ComponentPool<MyComponent> final {
    // ...
};
```

A custom pool should expose at least the following member functions:

* `bool empty() const noexcept;`
* `size_type capacity() const noexcept;`
* `size_type size() const noexcept;`
* `const entity_type * entities() const noexcept;`
* `bool has(entity_type entity) const noexcept;`
* `const component_type & get(entity_type entity) const noexcept;`
* `component_type & get(entity_type entity) noexcept;`
* template<typename... Args> component_type & construct(entity_type entity, Args&&... args);`
* `void destroy(entity_type entity);`
* `void reset();`

This is a fast and easy way to define a custom pool specialization for a given component (as an example, if the
component `X` requires to be ordered internally somehow during construction or destruction operations) and to use the
default pool for all the other components.<br/>
It's a mattrer of including the given specialization along with the registry, so that it can find it during the instantiation.<br/>
In this case, users are not required to use the more explicit `Registry` class. Instead, they can still use `entt::DefaultRegistry`.

In cases when the per-component pools are not good enough, the registry can be initialized with a custom pool.<br/>
In other terms, `entt::Registry` has a template template parameter that can be used to provide both the pool and the list of
components:

```
auto registry = entt::Registry<MyCustomPool<Component1, Component2>>{};
```

Even thoug the underlying pool doesn't store the components separately, the registry must know them to be able to do
specific actions (like `destroy` or `copy`). That's why they must be explicitly specified.<br/>
A generic pool should expose at least the following memeber functions:

* `template<typename Comp> bool empty() const noexcept;`
* `template<typename Comp> size_type capacity() const noexcept;`
* `template<typename Comp> size_type size() const noexcept;`
* `template<typename Comp> const entity_type * entities() const noexcept;`
* `template<typename Comp> bool has(entity_type entity) const noexcept;`
* `template<typename Comp> const Comp & get(entity_type entity) const noexcept;`
* `template<typename Comp> Comp & get(entity_type entity) noexcept;`
* `template<typename Comp, typename... Args> Comp & construct(entity_type entity, Args&&... args);`
* `template<typename Comp> void destroy(entity_type entity);`
* `void reset();`

Good luck. If you come out with a more performant components pool, do not forget to make a PR so that I can add it to
the list of available ones. I would be glad to have such a contribution to the project!!

## Tests

To compile and run the tests, `EnTT` requires *googletest*.<br/>
Run the script `deps.sh` to download it. It is good practice to do it every time one pull the project.

Then, to build the tests:

* `$ cd build`
* `$ cmake ..`
* `$ make`
* `$ make test`

# Contributors

If you want to contribute, please send patches as pull requests against the branch master.<br/>
Check the [contributors list](https://github.com/skypjack/entt/blob/master/AUTHORS) to see who has partecipated so far.

# License

Code and documentation Copyright (c) 2017 Michele Caini.<br/>
Code released under [the MIT license](https://github.com/skypjack/entt/blob/master/LICENSE).
