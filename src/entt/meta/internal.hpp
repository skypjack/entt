#ifndef ENTT_META_INTERNAL_HPP
#define ENTT_META_INTERNAL_HPP


#include <cstddef>
#include <type_traits>
#include "../core/attribute.h"
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"


namespace entt {


class meta_any;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_type_node;


struct meta_prop_node {
    meta_prop_node * next;
    meta_any(* const key)();
    meta_any(* const value)();
};


struct meta_base_node {
    meta_type_node * const parent;
    meta_base_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    void *(* const cast)(void *) ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_type_node * const parent;
    meta_conv_node * next;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    meta_any(* const conv)(const void *);
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_type_node * const parent;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type size;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
};


struct meta_dtor_node {
    meta_type_node * const parent;
    void(* const invoke)(void *);
};


struct meta_data_node {
    id_type id;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_any, meta_any, meta_any);
    meta_any(* const get)(meta_any, meta_any);
};


struct meta_func_node {
    using size_type = std::size_t;
    id_type id;
    meta_type_node * const parent;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const ret)() ENTT_NOEXCEPT;
    meta_type_node *(* const arg)(size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any, meta_any *);
};


struct meta_type_node {
    using size_type = std::size_t;
    const id_type type_id;
    id_type id;
    meta_type_node * next;
    meta_prop_node * prop;
    const bool is_void;
    const bool is_integral;
    const bool is_floating_point;
    const bool is_array;
    const bool is_enum;
    const bool is_union;
    const bool is_class;
    const bool is_pointer;
    const bool is_function_pointer;
    const bool is_member_object_pointer;
    const bool is_member_function_pointer;
    const size_type extent;
    bool(* const compare)(const void *, const void *);
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_dtor_node *dtor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
};


template<typename Type, typename Op, typename Node>
void visit(Op &op, Node *node) {
    while(node) {
        op(Type{node});
        node = node->next;
    }
}


template<auto Member, typename Type, typename Op>
void visit(Op &op, const internal::meta_type_node *node) {
    if(node) {
        internal::visit<Type>(op, node->*Member);
        auto *next = node->base;

        while(next) {
            visit<Member, Type>(op, next->type());
            next = next->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(const Op &op, Node *node) {
    while(node && !op(node)) {
        node = node->next;
    }

    return node;
}


template<auto Member, typename Op>
auto find_if(const Op &op, const meta_type_node *node)
-> decltype(find_if(op, node->*Member)) {
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *next = node->base;

        while(next && !ret) {
            ret = find_if<Member>(op, next->type());
            next = next->next;
        }
    }

    return ret;
}


template<typename Type>
struct ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Invalid type");

    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    [[nodiscard]] static meta_type_node * resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            type_info<Type>::id(),
            {},
            nullptr,
            nullptr,
            std::is_void_v<Type>,
            std::is_integral_v<Type>,
            std::is_floating_point_v<Type>,
            std::is_array_v<Type>,
            std::is_enum_v<Type>,
            std::is_union_v<Type>,
            std::is_class_v<Type>,
            std::is_pointer_v<Type>,
            std::is_pointer_v<Type> && std::is_function_v<std::remove_pointer_t<Type>>,
            std::is_member_object_pointer_v<Type>,
            std::is_member_function_pointer_v<Type>,
            std::extent_v<Type>,
            &compare, // workaround for an issue with VS2017
            &meta_node<std::remove_const_t<std::remove_pointer_t<Type>>>::resolve,
            &meta_node<std::remove_const_t<std::remove_extent_t<Type>>>::resolve
        };

        return &node;
    }
};


template<typename... Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>...> {};


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


}


#endif
