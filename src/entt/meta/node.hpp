#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/attribute.h"
#include "../core/enum.hpp"
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

enum class meta_traits : std::uint32_t {
    is_none = 0x0000,
    is_const = 0x0001,
    is_static = 0x0002,
    is_arithmetic = 0x0004,
    is_array = 0x0008,
    is_enum = 0x0010,
    is_class = 0x0020,
    is_pointer = 0x0040,
    is_meta_pointer_like = 0x0080,
    is_meta_sequence_container = 0x0100,
    is_meta_associative_container = 0x0200,
    _entt_enum_as_bitmask
};

struct meta_type_node;

struct meta_prop_node {
    meta_prop_node *next;
    const meta_any &id;
    meta_any &value;
};

struct meta_base_node {
    meta_base_node *next;
    meta_type_node *const type;
    meta_any (*const cast)(meta_any) ENTT_NOEXCEPT;
};

struct meta_conv_node {
    meta_conv_node *next;
    meta_type_node *const type;
    meta_any (*const conv)(const meta_any &);
};

struct meta_ctor_node {
    using size_type = std::size_t;
    meta_ctor_node *next;
    const size_type arity;
    meta_type (*const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any (*const invoke)(meta_any *const);
};

struct meta_data_node {
    using size_type = std::size_t;
    id_type id;
    const meta_traits traits;
    meta_data_node *next;
    meta_prop_node *prop;
    const size_type arity;
    meta_type_node *const type;
    meta_type (*const arg)(const size_type) ENTT_NOEXCEPT;
    bool (*const set)(meta_handle, meta_any);
    meta_any (*const get)(meta_handle);
};

struct meta_func_node {
    using size_type = std::size_t;
    id_type id;
    const meta_traits traits;
    meta_func_node *next;
    meta_prop_node *prop;
    const size_type arity;
    meta_type_node *const ret;
    meta_type (*const arg)(const size_type) ENTT_NOEXCEPT;
    meta_any (*const invoke)(meta_handle, meta_any *const);
};

struct meta_template_node {
    using size_type = std::size_t;
    const size_type arity;
    meta_type_node *const type;
    meta_type_node *(*const arg)(const size_type)ENTT_NOEXCEPT;
};

struct meta_type_node {
    using size_type = std::size_t;
    const type_info *info;
    id_type id;
    const meta_traits traits;
    meta_type_node *next;
    meta_prop_node *prop;
    const size_type size_of;
    meta_type_node *(*const remove_pointer)() ENTT_NOEXCEPT;
    meta_any (*const default_constructor)();
    double (*const conversion_helper)(void *, const void *);
    const meta_template_node *const templ;
    meta_ctor_node *ctor{nullptr};
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
    void (*dtor)(void *){nullptr};
};

template<typename... Args>
meta_type_node *meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT;

template<typename Type>
class ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Invalid type");

    [[nodiscard]] static auto *meta_default_constructor() ENTT_NOEXCEPT {
        if constexpr(std::is_default_constructible_v<Type>) {
            return +[]() { return meta_any{std::in_place_type<Type>}; };
        } else {
            return static_cast<std::decay_t<decltype(meta_type_node::default_constructor)>>(nullptr);
        }
    }

    [[nodiscard]] static auto *meta_conversion_helper() ENTT_NOEXCEPT {
        if constexpr(std::is_arithmetic_v<Type>) {
            return +[](void *bin, const void *value) {
                return bin ? static_cast<double>(*static_cast<Type *>(bin) = static_cast<Type>(*static_cast<const double *>(value))) : static_cast<double>(*static_cast<const Type *>(value));
            };
        } else if constexpr(std::is_enum_v<Type>) {
            return +[](void *bin, const void *value) {
                return bin ? static_cast<double>(*static_cast<Type *>(bin) = static_cast<Type>(static_cast<std::underlying_type_t<Type>>(*static_cast<const double *>(value)))) : static_cast<double>(*static_cast<const Type *>(value));
            };
        } else {
            return static_cast<std::decay_t<decltype(meta_type_node::conversion_helper)>>(nullptr);
        }
    }

    [[nodiscard]] static meta_template_node *meta_template_info() ENTT_NOEXCEPT {
        if constexpr(is_complete_v<meta_template_traits<Type>>) {
            static meta_template_node node{
                meta_template_traits<Type>::args_type::size,
                meta_node<typename meta_template_traits<Type>::class_type>::resolve(),
                [](const std::size_t index) ENTT_NOEXCEPT { return meta_arg_node(typename meta_template_traits<Type>::args_type{}, index); }
                // tricks clang-format
            };

            return &node;
        } else {
            return nullptr;
        }
    }

public:
    [[nodiscard]] static meta_type_node *resolve() ENTT_NOEXCEPT {
        static meta_type_node node{
            &type_id<Type>(),
            {},
            internal::meta_traits::is_none
                | (std::is_arithmetic_v<Type> ? internal::meta_traits::is_arithmetic : internal::meta_traits::is_none)
                | (std::is_array_v<Type> ? internal::meta_traits::is_array : internal::meta_traits::is_none)
                | (std::is_enum_v<Type> ? internal::meta_traits::is_enum : internal::meta_traits::is_none)
                | (std::is_class_v<Type> ? internal::meta_traits::is_class : internal::meta_traits::is_none)
                | (std::is_pointer_v<Type> ? internal::meta_traits::is_pointer : internal::meta_traits::is_none)
                | (is_meta_pointer_like_v<Type> ? internal::meta_traits::is_meta_pointer_like : internal::meta_traits::is_none)
                | (is_complete_v<meta_sequence_container_traits<Type>> ? internal::meta_traits::is_meta_sequence_container : internal::meta_traits::is_none)
                | (is_complete_v<meta_associative_container_traits<Type>> ? internal::meta_traits::is_meta_associative_container : internal::meta_traits::is_none),
            nullptr,
            nullptr,
            size_of_v<Type>,
            &meta_node<std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<Type>>>>::resolve,
            meta_default_constructor(),
            meta_conversion_helper(),
            meta_template_info()
            // tricks clang-format
        };

        return &node;
    }
};

template<typename... Args>
[[nodiscard]] meta_type_node *meta_arg_node(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT {
    meta_type_node *args[sizeof...(Args) + 1u]{nullptr, internal::meta_node<std::remove_cv_t<std::remove_reference_t<Args>>>::resolve()...};
    return args[index + 1u];
}

template<auto Member, typename Type>
[[nodiscard]] static std::decay_t<decltype(std::declval<internal::meta_type_node>().*Member)> find_by(const Type &info_or_id, const internal::meta_type_node *node) ENTT_NOEXCEPT {
    for(auto *curr = node->*Member; curr; curr = curr->next) {
        if constexpr(std::is_same_v<Type, type_info>) {
            if(*curr->type->info == info_or_id) {
                return curr;
            }
        } else if constexpr(std::is_same_v<decltype(curr), meta_base_node *>) {
            if(curr->type->id == info_or_id) {
                return curr;
            }
        } else {
            if(curr->id == info_or_id) {
                return curr;
            }
        }
    }

    for(auto *curr = node->base; curr; curr = curr->next) {
        if(auto *ret = find_by<Member>(info_or_id, curr->type); ret) {
            return ret;
        }
    }

    return nullptr;
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

} // namespace entt

#endif
