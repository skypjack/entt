# Crash Course: poly

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

* [Introduction](#introduction)
  * [Other libraries](#other-libraries)
* [Concept and implementation](#concept-and-implementation)
  * [Deduced interface](#deduced-interface)
  * [Defined interface](#defined-interface)
  * [Fullfill a concept](#fullfill-a-concept)
* [Inheritance](#inheritance)
* [Static polymorphism in the wild](#static-polymorphism-in-the-wild)
* [Storage size and alignment requirement](#storage-size-and-alignment-requirement)
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

Since the `poly` class template makes use of `entt::any` internally, it also
supports most of its feature. Among the most important, the possibility to
create aliases to existing and thus unmanaged objects. This allows users to
exploit the static polymorphism while maintaining ownership of objects.<br/>
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
I've some doubts about the usefulness of some feature in real world projects,
but perhaps my lack of experience comes into play here. In my opinion, its only
flaw is the API which I find slightly more cumbersome than other solutions.<br/>
The latter was undoubtedly a source of inspiration for this module, although I
opted for different choices in the implementation of both the final API and some
feature.

Either way, the authors are gurus of the C++ community, people I only have to
learn from.

# Concept and implementation

The first thing to do to create a _type-erasing polymorphic object wrapper_ (to
use the terminology introduced by Eric Niebler) is to define a _concept_ that
types will have to adhere to.<br/>
For this purpose, the library offers a single class that supports both deduced
and fully defined interfaces. Although having interfaces deduced automatically
is convenient and allows users to write less code in most cases, this has some
limitations and it's therefore useful to be able to get around the deduction by
providing a custom definition for the static virtual table.

Once the interface is defined, it will be sufficient to provide a generic
implementation to fullfill the concept.<br/>
Also in this case, the library allows customizations based on types or families
of types, so as to be able to go beyond the generic case where necessary.

## Deduced interface

This is how a concept with a deduced interface is introduced:

```cpp
struct Drawable: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        void draw() { this->template invoke<0>(*this); }
    };

    // ...
};
```

It's recognizable by the fact that it inherits from an empty type list.<br/>
Functions can also be const, accept any number of paramters and return a type
other than `void`:

```cpp
struct Drawable: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        bool draw(int pt) const { return this->template invoke<0>(*this, pt); }
    };

    // ...
};
```

In this case, all parameters must be passed to `invoke` after the reference to
`this` and the return value is whatever the internal call returns.<br/>
As for `invoke`, this is a name that is injected into the _concept_ through
`Base`, from which one must necessarily inherit. Since it's also a dependent
name, the `this-> template` form is unfortunately necessary due to the rules of
the language. However, there exists also an alternative that goes through an
external call:

```cpp
struct Drawable: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        bool draw() const { entt::poly_call<0>(*this); }
    };

    // ...
};
```

Once the _concept_ is defined, users must provide a generic implementation of it
in order to tell the system how any type can satisfy its requirements. This is
done via an alias template within the concept itself.<br/>
The index passed as a template parameter to either `invoke` or `poly_call`
refers to how this alias is defined.

## Defined interface

A fully defined concept is no different to one for which the interface is
deduced, with the only difference that the list of types is not empty this time:

```cpp
struct Drawable: entt::type_list<void()> {
    template<typename Base>
    struct type: Base {
        void draw() { entt::poly_call<0>(*this); }
    };

    // ...
};
```

Again, parameters and return values other than `void` are allowed. Also, the
function type must be const when the method to bind to it is const:

```cpp
struct Drawable: entt::type_list<bool(int) const> {
    template<typename Base>
    struct type: Base {
        bool draw(int pt) const { return entt::poly_call<0>(*this, pt); }
    };

    // ...
};
```

Why should a user fully define a concept if the function types are the same as
the deduced ones?<br>
Because, in fact, this is exactly the limitation that can be worked around by
manually defining the static virtual table.

When things are deduced, there is an implicit constraint.<br/>
If the concept exposes a member function called `draw` with function type
`void()`, a concept can be satisfied:

* Either by a class that exposes a member function with the same name and the
  same signature.

* Or through a lambda that makes use of existing member functions from the
  interface itself.

In other words, it's not possible to make use of functions not belonging to the
interface, even if they are present in the types that fulfill the concept.<br/>
Similarly, it's not possible to deduce a function in the static virtual table
with a function type different from that of the associated member function in
the interface itself.

Explicitly defining a static virtual table suppresses the deduction step and
allows maximum flexibility when providing the implementation for a concept.

## Fullfill a concept

The `impl` alias template of a concept is used to define how it's fulfilled:

```cpp
struct Drawable: entt::type_list<> {
    // ...

    template<typename Type>
    using impl = entt::value_list<&Type::draw>;
};
```

In this case, it's stated that the `draw` method of a generic type will be
enough to satisfy the requirements of the `Drawable` concept.<br/>
Both member functions and free functions are supported to fullfill concepts:

```cpp
template<typename Type>
void print(Type &self) { self.print(); }

struct Drawable: entt::type_list<void()> {
    // ...

    template<typename Type>
    using impl = entt::value_list<&print<Type>>;
};
```

Likewise, as long as the parameter types and return type support conversions to
and from those of the function type referenced in the static virtual table, the
actual implementation may differ in its function type since it's erased
internally.<br/>
Moreover, the `self` parameter isn't strictly required by the system and can be
left out for free functions if not required.

Refer to the inline documentation for more details.

# Inheritance

_Concept inheritance_ is straightforward due to how poly looks like in `EnTT`.
Therefore, it's quite easy to build hierarchies of concepts if necessary.<br/>
The only constraint is that all concepts in a hierarchy must belong to the same
_family_, that is, they must be either all deduced or all defined.

For a deduced concept, inheritance is achieved in a few steps:

```cpp
struct DrawableAndErasable: entt::type_list<> {
    template<typename Base>
    struct type: typename Drawable::template type<Base> {
        static constexpr auto base = std::tuple_size_v<typename entt::poly_vtable<Drawable>::type>;
        void erase() { entt::poly_call<base + 0>(*this); }
    };

    template<typename Type>
    using impl = entt::value_list_cat_t<
        typename Drawable::impl<Type>,
        entt::value_list<&Type::erase>
    >;
};
```

The static virtual table is empty and must remain so.<br/>
On the other hand, `type` no longer inherits from `Base` and instead forwards
its template parameter to the type exposed by the _base class_. Internally, the
size of the static virtual table of the base class is used as an offset for the
local indexes.<br/>
Finally, by means of the `value_list_cat_t` utility, the implementation consists
in appending the new functions to the previous list.

As for a defined concept instead, also the list of types must be extended, in a
similar way to what is shown for the implementation of the above concept.<br/>
To do this, it's useful to declare a function that allows to convert a _concept_
into its underlying `type_list` object:

```cpp
template<typename... Type>
entt::type_list<Type...> as_type_list(const entt::type_list<Type...> &);
```

The definition isn't strictly required, since the function will only be used
through a `decltype` as it follows:

```cpp
struct DrawableAndErasable: entt::type_list_cat_t<
    decltype(as_type_list(std::declval<Drawable>())),
    entt::type_list<void()>
> {
    // ...
};
```

Similar to above, `type_list_cat_t` is used to concatenate the underlying static
virtual table with the new function types.<br/>
Everything else is the same as already shown instead.

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

drawable instance{circle{}};
instance->draw();

instance = square{};
instance->draw();
```

The `poly` class template offers a wide range of constructors, from the default
one (which will return an uninitialized `poly` object) to the copy and move
constructors, as well as the ability to create objects in-place.<br/>
Among others, there is also a constructor that allows users to wrap unmanaged
objects in a `poly` instance (either const or non-const ones):

```cpp
circle shape;
drawable instance{std::in_place_type<circle &>, shape};
```

Similarly, it's possible to create non-owning copies of `poly` from an existing
object:

```cpp
drawable other = instance.as_ref();
```

In both cases, although the interface of the `poly` object doesn't change, it
won't construct any element or take care of destroying the referenced objects.

Note also how the underlying concept is accessed via a call to `operator->` and
not directly as `instance.draw()`.<br/>
This allows users to decouple the API of the wrapper from that of the concept.
Therefore, where `instance.data()` will invoke the `data` member function of the
poly object, `instance->data()` will map directly to the functionality exposed
by the underlying concept.

# Storage size and alignment requirement

Under the hood, the `poly` class template makes use of `entt::any`. Therefore,
it can take advantage of the possibility of defining at compile-time the size of
the storage suitable for the small buffer optimization as well as the alignment
requirements:

```cpp
entt::basic_poly<Drawable, sizeof(double[4]), alignof(double[4])>
```

The default size is `sizeof(double[2])`, which seems like a good compromise
between a buffer that is too large and one unable to hold anything larger than
an integer. The alignment requirement is optional instead and by default such
that it's the most stringent (the largest) for any object whose size is at most
equal to the one provided.<br/>
It's worth noting that providing a size of 0 (which is an accepted value in all
respects) will force the system to dynamically allocate the contained objects in
all cases.
