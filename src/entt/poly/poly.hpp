#ifndef ENTT_POLY_POLY_HPP
#define ENTT_POLY_POLY_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/*! @brief Inspector class used to infer the type of the virtual table. */
struct poly_inspector {
    /**
     * @brief Generic conversion operator (definition only).
     * @tparam Type Type to which conversion is requested.
     */
    template<class Type>
    operator Type &&() const;

    /**
     * @brief Dummy invocation function (definition only).
     * @tparam Member Index of the function to invoke.
     * @tparam Args Types of arguments to pass to the function.
     * @param args The arguments to pass to the function.
     * @return A poly inspector convertible to any type.
     */
    template<auto Member, typename... Args>
    poly_inspector invoke(Args &&...args) const;

    /*! @copydoc invoke */
    template<auto Member, typename... Args>
    poly_inspector invoke(Args &&...args);
};

/**
 * @brief Static virtual table factory.
 * @tparam Concept Concept descriptor.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Alignment requirement.
 */
template<typename Concept, std::size_t Len, std::size_t Align>
class poly_vtable {
    using inspector = typename Concept::template type<poly_inspector>;

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (*)(inspector &, Args...)) -> Ret (*)(basic_any<Len, Align> &, Args...);

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (*)(const inspector &, Args...)) -> Ret (*)(const basic_any<Len, Align> &, Args...);

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (*)(Args...)) -> Ret (*)(const basic_any<Len, Align> &, Args...);

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (inspector::*)(Args...)) -> Ret (*)(basic_any<Len, Align> &, Args...);

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (inspector::*)(Args...) const) -> Ret (*)(const basic_any<Len, Align> &, Args...);

    template<auto... Candidate>
    static auto make_vtable(value_list<Candidate...>)
        -> decltype(std::make_tuple(vtable_entry(Candidate)...));

    template<typename... Func>
    [[nodiscard]] static constexpr auto make_vtable(type_list<Func...>) {
        if constexpr(sizeof...(Func) == 0u) {
            return decltype(make_vtable(typename Concept::template impl<inspector>{})){};
        } else if constexpr((std::is_function_v<Func> && ...)) {
            return decltype(std::make_tuple(vtable_entry(std::declval<Func inspector::*>())...)){};
        }
    }

    template<typename Type, auto Candidate, typename Ret, typename Any, typename... Args>
    static void fill_vtable_entry(Ret (*&entry)(Any &, Args...)) {
        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>) {
            entry = +[](Any &, Args... args) -> Ret {
                return std::invoke(Candidate, std::forward<Args>(args)...);
            };
        } else {
            entry = +[](Any &instance, Args... args) -> Ret {
                return static_cast<Ret>(std::invoke(Candidate, any_cast<constness_as_t<Type, Any> &>(instance), std::forward<Args>(args)...));
            };
        }
    }

    template<typename Type, auto... Index>
    [[nodiscard]] static auto fill_vtable(std::index_sequence<Index...>) {
        vtable_type impl{};
        (fill_vtable_entry<Type, value_list_element_v<Index, typename Concept::template impl<Type>>>(std::get<Index>(impl)), ...);
        return impl;
    }

    using vtable_type = decltype(make_vtable(Concept{}));
    static constexpr bool is_mono_v = std::tuple_size_v<vtable_type> == 1u;

public:
    /*! @brief Virtual table type. */
    using type = std::conditional_t<is_mono_v, std::tuple_element_t<0u, vtable_type>, const vtable_type *>;

    /**
     * @brief Returns a static virtual table for a specific concept and type.
     * @tparam Type The type for which to generate the virtual table.
     * @return A static virtual table for the given concept and type.
     */
    template<typename Type>
    [[nodiscard]] static type instance() {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Type differs from its decayed form");
        static const vtable_type vtable = fill_vtable<Type>(std::make_index_sequence<Concept::template impl<Type>::size>{});

        if constexpr(is_mono_v) {
            return std::get<0>(vtable);
        } else {
            return &vtable;
        }
    }
};

/**
 * @brief Poly base class used to inject functionalities into concepts.
 * @tparam Poly The outermost poly class.
 */
template<typename Poly>
struct poly_base {
    /**
     * @brief Invokes a function from the static virtual table.
     * @tparam Member Index of the function to invoke.
     * @tparam Args Types of arguments to pass to the function.
     * @param self A reference to the poly object that made the call.
     * @param args The arguments to pass to the function.
     * @return The return value of the invoked function, if any.
     */
    template<auto Member, typename... Args>
    [[nodiscard]] decltype(auto) invoke(const poly_base &self, Args &&...args) const {
        const auto &poly = static_cast<const Poly &>(self);

        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(poly.vtable)>>) {
            return poly.vtable(poly.storage, std::forward<Args>(args)...);
        } else {
            return std::get<Member>(*poly.vtable)(poly.storage, std::forward<Args>(args)...);
        }
    }

    /*! @copydoc invoke */
    template<auto Member, typename... Args>
    [[nodiscard]] decltype(auto) invoke(poly_base &self, Args &&...args) {
        auto &poly = static_cast<Poly &>(self);

        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(poly.vtable)>>) {
            return poly.vtable(poly.storage, std::forward<Args>(args)...);
        } else {
            return std::get<Member>(*poly.vtable)(poly.storage, std::forward<Args>(args)...);
        }
    }
};

/**
 * @brief Shortcut for calling `poly_base<Type>::invoke`.
 * @tparam Member Index of the function to invoke.
 * @tparam Poly A fully defined poly object.
 * @tparam Args Types of arguments to pass to the function.
 * @param self A reference to the poly object that made the call.
 * @param args The arguments to pass to the function.
 * @return The return value of the invoked function, if any.
 */
template<auto Member, typename Poly, typename... Args>
decltype(auto) poly_call(Poly &&self, Args &&...args) {
    return std::forward<Poly>(self).template invoke<Member>(self, std::forward<Args>(args)...);
}

/**
 * @brief Static polymorphism made simple and within everyone's reach.
 *
 * Static polymorphism is a very powerful tool in C++, albeit sometimes
 * cumbersome to obtain.<br/>
 * This class aims to make it simple and easy to use.
 *
 * @note
 * Both deduced and defined static virtual tables are supported.<br/>
 * Moreover, the `poly` class template also works with unmanaged objects.
 *
 * @tparam Concept Concept descriptor.
 * @tparam Len Size of the storage reserved for the small buffer optimization.
 * @tparam Align Optional alignment requirement.
 */
template<typename Concept, std::size_t Len, std::size_t Align>
class basic_poly: private Concept::template type<poly_base<basic_poly<Concept, Len, Align>>> {
    /*! @brief A poly base is allowed to snoop into a poly object. */
    friend struct poly_base<basic_poly>;

public:
    /*! @brief Concept type. */
    using concept_type = typename Concept::template type<poly_base<basic_poly>>;
    /*! @brief Virtual table type. */
    using vtable_type = typename poly_vtable<Concept, Len, Align>::type;

    /*! @brief Default constructor. */
    basic_poly() ENTT_NOEXCEPT
        : storage{},
          vtable{} {}

    /**
     * @brief Constructs a poly by directly initializing the new object.
     * @tparam Type Type of object to use to initialize the poly.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    explicit basic_poly(std::in_place_type_t<Type>, Args &&...args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          vtable{poly_vtable<Concept, Len, Align>::template instance<std::remove_const_t<std::remove_reference_t<Type>>>()} {}

    /**
     * @brief Constructs a poly from a given value.
     * @tparam Type Type of object to use to initialize the poly.
     * @param value An instance of an object to use to initialize the poly.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, basic_poly>>>
    basic_poly(Type &&value) ENTT_NOEXCEPT
        : basic_poly{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)} {}

    /**
     * @brief Returns the object type if any, `type_id<void>()` otherwise.
     * @return The object type if any, `type_id<void>()` otherwise.
     */
    [[nodiscard]] const type_info &type() const ENTT_NOEXCEPT {
        return storage.type();
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    [[nodiscard]] const void *data() const ENTT_NOEXCEPT {
        return storage.data();
    }

    /*! @copydoc data */
    [[nodiscard]] void *data() ENTT_NOEXCEPT {
        return storage.data();
    }

    /**
     * @brief Replaces the contained object by creating a new instance directly.
     * @tparam Type Type of object to use to initialize the poly.
     * @tparam Args Types of arguments to use to construct the new instance.
     * @param args Parameters to use to construct the instance.
     */
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        storage.template emplace<Type>(std::forward<Args>(args)...);
        vtable = poly_vtable<Concept, Len, Align>::template instance<std::remove_const_t<std::remove_reference_t<Type>>>();
    }

    /*! @brief Destroys contained object */
    void reset() {
        storage.reset();
        vtable = {};
    }

    /**
     * @brief Returns false if a poly is empty, true otherwise.
     * @return False if the poly is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(storage);
    }

    /**
     * @brief Returns a pointer to the underlying concept.
     * @return A pointer to the underlying concept.
     */
    [[nodiscard]] concept_type *operator->() ENTT_NOEXCEPT {
        return this;
    }

    /*! @copydoc operator-> */
    [[nodiscard]] const concept_type *operator->() const ENTT_NOEXCEPT {
        return this;
    }

    /**
     * @brief Aliasing constructor.
     * @return A poly that shares a reference to an unmanaged object.
     */
    [[nodiscard]] basic_poly as_ref() ENTT_NOEXCEPT {
        basic_poly ref{};
        ref.storage = storage.as_ref();
        ref.vtable = vtable;
        return ref;
    }

    /*! @copydoc as_ref */
    [[nodiscard]] basic_poly as_ref() const ENTT_NOEXCEPT {
        basic_poly ref{};
        ref.storage = storage.as_ref();
        ref.vtable = vtable;
        return ref;
    }

private:
    basic_any<Len, Align> storage;
    vtable_type vtable;
};

} // namespace entt

#endif
