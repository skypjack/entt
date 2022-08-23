#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../container/dense_map.hpp"
#include "../core/attribute.h"
#include "../core/enum.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
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
    is_integral = 0x0008,
    is_signed = 0x0010,
    is_array = 0x0020,
    is_enum = 0x0040,
    is_class = 0x0080,
    is_meta_pointer_like = 0x0100,
    is_meta_sequence_container = 0x0200,
    is_meta_associative_container = 0x0400,
    _entt_enum_as_bitmask
};

struct meta_type_node;

struct meta_prop_node {
    meta_type_node *(*type)() noexcept {nullptr};
    basic_any<0u> value{};
};

struct meta_base_node {
    meta_type_node *(*type)() noexcept;
    meta_any (*cast)(meta_any) noexcept;
};

struct meta_conv_node {
    meta_any (*conv)(const meta_any &);
};

struct meta_ctor_node {
    using size_type = std::size_t;

    size_type arity;
    meta_type (*arg)(const size_type) noexcept;
    meta_any (*invoke)(meta_any *const);
};

struct meta_dtor_node {
    void (*dtor)(void *){nullptr};
};

struct meta_data_node {
    using size_type = std::size_t;

    meta_traits traits;
    size_type arity;
    meta_type_node *(*type)() noexcept;
    meta_type (*arg)(const size_type) noexcept;
    bool (*set)(meta_handle, meta_any);
    meta_any (*get)(meta_handle);
    dense_map<id_type, meta_prop_node, identity> prop{};
};

struct meta_func_node {
    using size_type = std::size_t;

    id_type watermark;
    meta_traits traits;
    size_type arity;
    meta_type_node *(*ret)() noexcept;
    meta_type (*arg)(const size_type) noexcept;
    meta_any (*invoke)(meta_handle, meta_any *const);
    dense_map<id_type, meta_prop_node, identity> prop{};
    std::unique_ptr<meta_func_node> next{};
};

struct meta_template_node {
    using size_type = std::size_t;

    size_type arity{0u};
    meta_type_node *type{nullptr};
    meta_type_node *(*arg)(const size_type) noexcept {nullptr};
};

struct meta_type_node {
    using size_type = std::size_t;

    const type_info *info;
    id_type id;
    const meta_traits traits;
    meta_type_node *next;
    const size_type size_of;
    meta_type_node *(*const remove_pointer)() noexcept;
    meta_any (*const default_constructor)();
    double (*const conversion_helper)(void *, const void *);
    meta_any (*const from_void)(void *, const void *);
    meta_template_node templ;
    dense_map<id_type, meta_prop_node, identity> prop{};
    dense_map<id_type, meta_ctor_node, identity> ctor{};
    dense_map<id_type, meta_base_node, identity> base{};
    dense_map<id_type, meta_conv_node, identity> conv{};
    dense_map<id_type, meta_data_node, identity> data{};
    dense_map<id_type, meta_func_node, identity> func{};
    meta_dtor_node dtor{};
};

template<typename... Args>
meta_type_node *meta_arg_node(type_list<Args...>, const std::size_t index) noexcept;

template<typename Type>
class ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>, "Invalid type");

    [[nodiscard]] static auto *meta_default_constructor() noexcept {
        if constexpr(std::is_default_constructible_v<Type>) {
            return +[]() { return meta_any{std::in_place_type<Type>}; };
        } else {
            return static_cast<std::decay_t<decltype(meta_type_node::default_constructor)>>(nullptr);
        }
    }

    [[nodiscard]] static auto *meta_conversion_helper() noexcept {
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

    [[nodiscard]] static auto *meta_from_void() noexcept {
        if constexpr(std::is_same_v<Type, void> || std::is_function_v<Type>) {
            return static_cast<std::decay_t<decltype(meta_type_node::from_void)>>(nullptr);
        } else {
            return +[](void *element, const void *as_const) {
                using value_type = std::decay_t<Type>;

                return element ? meta_any{std::in_place_type<value_type &>, *static_cast<value_type *>(element)}
                               : meta_any{std::in_place_type<const value_type &>, *static_cast<const value_type *>(as_const)};
            };
        }
    }

    [[nodiscard]] static auto meta_template_info() noexcept {
        if constexpr(is_complete_v<meta_template_traits<Type>>) {
            return meta_template_node{
                meta_template_traits<Type>::args_type::size,
                meta_node<typename meta_template_traits<Type>::class_type>::resolve(),
                +[](const std::size_t index) noexcept -> meta_type_node * { return meta_arg_node(typename meta_template_traits<Type>::args_type{}, index); }};
        } else {
            return meta_template_node{};
        }
    }

public:
    [[nodiscard]] static meta_type_node *resolve() noexcept {
        static meta_type_node node{
            &type_id<Type>(),
            {},
            (std::is_arithmetic_v<Type> ? internal::meta_traits::is_arithmetic : internal::meta_traits::is_none)
                | (std::is_integral_v<Type> ? internal::meta_traits::is_integral : internal::meta_traits::is_none)
                | (std::is_signed_v<Type> ? internal::meta_traits::is_signed : internal::meta_traits::is_none)
                | (std::is_array_v<Type> ? internal::meta_traits::is_array : internal::meta_traits::is_none)
                | (std::is_enum_v<Type> ? internal::meta_traits::is_enum : internal::meta_traits::is_none)
                | (std::is_class_v<Type> ? internal::meta_traits::is_class : internal::meta_traits::is_none)
                | (is_meta_pointer_like_v<Type> ? internal::meta_traits::is_meta_pointer_like : internal::meta_traits::is_none)
                | (is_complete_v<meta_sequence_container_traits<Type>> ? internal::meta_traits::is_meta_sequence_container : internal::meta_traits::is_none)
                | (is_complete_v<meta_associative_container_traits<Type>> ? internal::meta_traits::is_meta_associative_container : internal::meta_traits::is_none),
            nullptr,
            size_of_v<Type>,
            &meta_node<std::remove_cv_t<std::remove_pointer_t<Type>>>::resolve,
            meta_default_constructor(),
            meta_conversion_helper(),
            meta_from_void(),
            meta_template_info()
            // tricks clang-format
        };

        return &node;
    }
};

template<typename... Args>
[[nodiscard]] meta_type_node *meta_arg_node(type_list<Args...>, const std::size_t index) noexcept {
    meta_type_node *args[sizeof...(Args) + 1u]{nullptr, internal::meta_node<std::remove_cv_t<std::remove_reference_t<Args>>>::resolve()...};
    return args[index + 1u];
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

} // namespace entt

#endif
