#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP


#include <cstddef>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/attribute.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "type_traits.hpp"


namespace entt {


class meta_any;
class meta_type;
struct meta_handle;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


enum meta_trait: std::uint32_t {
    IS_NONE = 0x0000,
    IS_CONST = 0x0001,
    IS_STATIC = 0x0002,
    IS_INTEGRAL = 0x0004,
    IS_FLOATING_POINT = 0x0008,
    IS_ARRAY = 0x0010,
    IS_ENUM = 0x0020,
    IS_UNION = 0x0040,
    IS_CLASS = 0x0080,
    IS_POINTER = 0x0100,
    IS_FUNCTION_POINTER = 0x0200,
    IS_MEMBER_OBJECT_POINTER = 0x0400,
    IS_MEMBER_FUNCTION_POINTER = 0x0800,
    IS_META_POINTER_LIKE = 0x1000,
    IS_META_SEQUENCE_CONTAINER = 0x2000,
    IS_META_ASSOCIATIVE_CONTAINER = 0x4000
};


struct meta_type_node;


struct meta_prop_node {
    meta_prop_node * next;
    const meta_any &id;
    meta_any &value;
};


struct meta_base_node {
    meta_base_node * next;
    meta_type_node * const type;
    const void *(* const cast)(const void *) ENTT_NOEXCEPT;
};


struct meta_conv_node {
    meta_conv_node * next;
    meta_type_node * const type;
    meta_any(* const conv)(const void *);
};


struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node * next;
    meta_prop_node * prop;
    const size_type arity;
    meta_type(* const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_any * const);
};


struct meta_data_node {
    id_type id;
    meta_data_node * next;
    meta_prop_node * prop;
    std::underlying_type_t<meta_trait> traits;
    meta_type_node * const type;
    bool(* const set)(meta_handle, meta_any);
    meta_any(* const get)(meta_handle);
};


struct meta_func_node {
    using size_type = std::size_t;
    id_type id;
    meta_func_node * next;
    meta_prop_node * prop;
    const size_type arity;
    std::underlying_type_t<meta_trait> traits;
    meta_type_node * const ret;
    meta_type(* const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any(* const invoke)(meta_handle, meta_any * const);
};


struct meta_template_node {
    using size_type = std::size_t;
    const size_type arity;
    meta_type_node * const type;
    meta_type_node *(* const arg)(const size_type) ENTT_NOEXCEPT;
};


struct meta_type_node {
    using size_type = std::size_t;
    const type_info info;
    id_type id;
    meta_type_node * next;
    meta_prop_node * prop;
    const size_type size_of;
    std::underlying_type_t<meta_trait> traits;
    const size_type rank;
    size_type(* const extent)(const size_type) ENTT_NOEXCEPT ;
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    const meta_template_node *const templ;
    meta_ctor_node * const def_ctor;
    meta_ctor_node *ctor{nullptr};
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
    void(* dtor)(void *){nullptr};
};


template<typename... Args>
meta_type_node * meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT;


template<typename Type>
class ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Invalid type");

    template<std::size_t... Index>
    [[nodiscard]] static auto extent(const meta_type_node::size_type dim, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        meta_type_node::size_type ext{};
        ((ext = (dim == Index ? std::extent_v<Type, Index> : ext)), ...);
        return ext;
    }

    [[nodiscard]] static meta_ctor_node * meta_default_constructor([[maybe_unused]] meta_type_node *type) ENTT_NOEXCEPT {
        if constexpr(std::is_default_constructible_v<Type>) {
            static meta_ctor_node node{
                nullptr,
                nullptr,
                0u,
                nullptr,
                [](meta_any * const) { return meta_any{std::in_place_type<Type>}; }
            };

            return &node;
        } else {
            return nullptr;
        }
    }

    [[nodiscard]] static meta_template_node * meta_template_info() ENTT_NOEXCEPT {
        if constexpr(is_complete_v<meta_template_traits<Type>>) {
            static meta_template_node node{
                meta_template_traits<Type>::args_type::size,
                meta_node<typename meta_template_traits<Type>::class_type>::resolve(),
                [](const std::size_t index) ENTT_NOEXCEPT {
                    return meta_arg_node(typename meta_template_traits<Type>::args_type{}, index);
                }
            };

            return &node;
        } else {
            return nullptr;
        }
    }

public:
    [[nodiscard]] static meta_type_node * resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            type_id<Type>(),
            {},
            nullptr,
            nullptr,
            size_of_v<Type>,
            internal::meta_trait::IS_NONE
                | (std::is_integral_v<Type> ? internal::meta_trait::IS_INTEGRAL : internal::meta_trait::IS_NONE)
                | (std::is_floating_point_v<Type> ? internal::meta_trait::IS_FLOATING_POINT : internal::meta_trait::IS_NONE)
                | (std::is_array_v<Type> ? internal::meta_trait::IS_ARRAY : internal::meta_trait::IS_NONE)
                | (std::is_enum_v<Type> ? internal::meta_trait::IS_ENUM : internal::meta_trait::IS_NONE)
                | (std::is_union_v<Type> ? internal::meta_trait::IS_UNION : internal::meta_trait::IS_NONE)
                | (std::is_class_v<Type> ? internal::meta_trait::IS_CLASS : internal::meta_trait::IS_NONE)
                | (std::is_pointer_v<Type> ? internal::meta_trait::IS_POINTER : internal::meta_trait::IS_NONE)
                | (std::is_pointer_v<Type> && std::is_function_v<std::remove_pointer_t<Type>> ? internal::meta_trait::IS_FUNCTION_POINTER : internal::meta_trait::IS_NONE)
                | (std::is_member_object_pointer_v<Type> ? internal::meta_trait::IS_MEMBER_OBJECT_POINTER : internal::meta_trait::IS_NONE)
                | (std::is_member_function_pointer_v<Type> ? internal::meta_trait::IS_MEMBER_FUNCTION_POINTER : internal::meta_trait::IS_NONE)
                | (is_meta_pointer_like_v<Type> ? internal::meta_trait::IS_META_POINTER_LIKE : internal::meta_trait::IS_NONE)
                | (is_complete_v<meta_sequence_container_traits<Type>> ? internal::meta_trait::IS_META_SEQUENCE_CONTAINER : internal::meta_trait::IS_NONE)
                | (is_complete_v<meta_associative_container_traits<Type>> ? internal::meta_trait::IS_META_ASSOCIATIVE_CONTAINER : internal::meta_trait::IS_NONE),
            std::rank_v<Type>,
            [](meta_type_node::size_type dim) ENTT_NOEXCEPT { return extent(dim, std::make_index_sequence<std::rank_v<Type>>{}); },
            &meta_node<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<Type>>>>::resolve,
            &meta_node<std::remove_cv_t<std::remove_reference_t<std::remove_extent_t<Type>>>>::resolve,
            meta_template_info(),
            meta_default_constructor(&node),
            meta_default_constructor(&node)
        };

        return &node;
    }
};


template<typename Type>
struct meta_info: meta_node<std::remove_cv_t<std::remove_reference_t<Type>>> {};


template<typename... Args>
[[nodiscard]] meta_type_node * meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT {
    meta_type_node *args[sizeof...(Args) + 1u]{nullptr, internal::meta_info<Args>::resolve()...};
    return args[index + 1u];
}


template<auto Member, typename Op>
[[nodiscard]] static std::decay_t<decltype(std::declval<internal::meta_type_node>().*Member)> visit(const Op &op, const internal::meta_type_node *node) {
    for(auto *curr = node->*Member; curr; curr = curr->next) {
        if(op(curr)) {
            return curr;
        }
    }

    for(auto *curr = node->base; curr; curr = curr->next) {
        if(auto *ret = visit<Member>(op, curr->type); ret) {
            return ret;
        }
    }

    return nullptr;
}


}


/**
 * Internal details not to be documented.
 * @endcond
 */


}


#endif
