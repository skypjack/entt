# Crash Course: poly

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
  * [Other libraries](#other-libraries)
* [Concept and implementation](#concept-and-implementation)
* [Static polymorphism in the wild](#static-polymorphism-in-the-wild)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Static polymorphism is a very powerful tool in C++, albeit sometimes cumbersome
to obtain.<br/>
This module aims to make it simple and easy to use.

The library allows to define _concepts_ as interfaces to fullfill with concrete
classes withouth having to inherit from a common base.<br/>
This is, among others, one of the advantages of static polymorphism in general
and of a generic wrapper like that offered by the `poly` class template in
particular.<br/>
What users get is an object that can be passed around as such and not through a
reference or a pointer, as happens when it comes to working with dynamic
polymorphism.

Since the `poly` class template makes use of `entt::any` internally, it supports
most of its features. Among the most important, the possibility to create
aliases to existing objects and therefore not managed directly. This allows
users to exploit the static polymorphism while maintaining ownership of their
objects.<br/>
Likewise, the `poly` class template also benefits from the small buffer
optimization offered by the `entt::any` class and therefore minimizes the number
of allocations, avoiding them altogether where possible.

## Other libraries

There are some very interesting libraries regarding static polymorphism.<br/>
Among all, the two that I prefer are:

* [`dyno`](https://github.com/ldionne/dyno): runtime polymorphism done right.
* [`Poly`](https://github.com/facebook/folly/blob/master/folly/docs/Poly.md):
  a class template that makes it easy to define a type-erasing polymorphic
  object wrapper.

The former is admittedly an experimental library, with many interesting ideas.
I've some doubts about the usefulness of some features in real world projects,
but perhaps my ignorance comes into play here. In my opinion, its only flaw is
the API which I find slightly more cumbersome than other solutions.<br/>
The latter was undoubtedly a source of inspiration for this module, although I
opted for different choices in the implementation of both the final API and some
features.

Either way, the authors are gurus of the C++ community, people I only have to
learn from.

# Concept and implementation

The first thing to do to create a _type-erasing polymorphic object wrapper_ (to
use the terminology introduced by Eric Niebler) is to define a _concept_ that
types will have to adhere to.<br/>
In `EnTT`, this translates into the definition of a template class as follows:

```cpp
template<typename Base>
struct Drawable: Base {
    void draw() { this->template invoke<0>(*this); }
};
```

The example is purposely minimal but the functions can receive values and return
arguments. The former will be returned by the call to `invoke`, the latter must
be passed to the same function after the reference to `this` instead.<br/>
As for `invoke`, this is a name that is injected into the _concept_ through
`Base`, from which one must necessarily inherit. Since it's also a dependent
name, the `this-> template` form is unfortunately necessary due to the rules of
the language. However, there exists also an alternative that goes through an
external call:

```cpp
template<typename Base>
struct Drawable: Base {
    void draw() { entt::poly_call<0>(*this); }
};
```

Once the _concept_ is defined, users need to specialize a template variable to
tell the system how any type can satisfy its requirements:

```cpp
template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, Type> = entt::value_list<&Type::draw>{};
```

In this case, it's stated that the `draw` method of a generic type will be
enough to satisfy the requirements of the `Drawable` concept.<br/>
The `poly_impl` variable template can be specialized in a generic way as in the
example above, or for a specific type where this satisfies the requirements
differently. Moreover, it's easy to specialize it for families of types:

```cpp
template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, std::vector<Type>> = entt::value_list<&std::vector<Type>::size>{};
```

Finally, an implementation doesn't have to consist of just member functions.
Free functions are an alternative to fill any gaps in the interface of a type:

```cpp
template<typename Type>
void print(Type &self) { self.print(); }

template<typename Type>
inline constexpr auto entt::poly_impl<Drawable, Type> = entt::value_list<&print<Type>>{};
```

Refer to the variable template definition for more details.

# Static polymorphism in the wild

Once the _concept_ and implementation have been introduced, it will be possible
to use the `poly` class template to contain instances that meet the
requirements:

```cpp
using drawable = entt::poly<Drawable>;

struct circle {
    void draw() { /* ... */ }
};

struct square {
    void draw() { /* ... */ }
};

// ...

drawable d{circle{}};
d.draw();

d = square{};
d.draw();
```

The `poly` class template offers a wide range of constructors, from the default
one (which will return an uninitialized `poly` object) to the copy and move
constructor, as well as the ability to create objects in-place.<br/>
Among others, there is a constructor that allows users to wrap unmanaged objects
in a `poly` instance:

```cpp
circle c;
drawable d{std::ref(c)};
```

In this case, although the interface of the `poly` object doesn't change, it
won't construct any element or take care of destroying the referenced object.
