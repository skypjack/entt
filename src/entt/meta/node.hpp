#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/attribute.h"
#include "../core/enum.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "context.hpp"
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
    meta_type_node (*type)(const meta_context &) noexcept {};
    std::shared_ptr<void> value{};
};

struct meta_base_node {
    meta_type_node (*type)(const meta_context &) noexcept {};
    const void *(*cast)(const void *) noexcept {};
};

struct meta_conv_node {
    meta_any (*conv)(const meta_ctx &, const void *){};
};

struct meta_ctor_node {
    using size_type = std::size_t;

    size_type arity{0u};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    meta_any (*invoke)(const meta_ctx &, meta_any *const){};
};

struct meta_dtor_node {
    void (*dtor)(void *){};
};

struct meta_data_node {
    using size_type = std::size_t;

    meta_traits traits{meta_traits::is_none};
    size_type arity{0u};
    meta_type_node (*type)(const meta_context &) noexcept {};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    bool (*set)(meta_handle, meta_any){};
    meta_any (*get)(const meta_ctx &, meta_handle){};
    dense_map<id_type, meta_prop_node, identity> prop{};
};

struct meta_func_node {
    using size_type = std::size_t;

    meta_traits traits{meta_traits::is_none};
    size_type arity{0u};
    meta_type_node (*ret)(const meta_context &) noexcept {};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    meta_any (*invoke)(const meta_ctx &, meta_handle, meta_any *const){};
    std::shared_ptr<meta_func_node> next{};
    dense_map<id_type, meta_prop_node, identity> prop{};
};

struct meta_template_node {
    using size_type = std::size_t;

    size_type arity{0u};
    meta_type_node (*type)(const meta_context &) noexcept {};
    meta_type_node (*arg)(const meta_context &, const size_type) noexcept {};
};

struct meta_type_descriptor {
    dense_map<id_type, meta_ctor_node, identity> ctor{};
    dense_map<id_type, meta_base_node, identity> base{};
    dense_map<id_type, meta_conv_node, identity> conv{};
    dense_map<id_type, meta_data_node, identity> data{};
    dense_map<id_type, meta_func_node, identity> func{};
    dense_map<id_type, meta_prop_node, identity> prop{};
};

struct meta_type_node {
    using size_type = std::size_t;

    const type_info *info{};
    id_type id{};
    meta_traits traits{meta_traits::is_none};
    size_type size_of{0u};
    meta_type_node (*resolve)(const meta_context &) noexcept {};
    meta_type_node (*remove_pointer)(const meta_context &) noexcept {};
    meta_any (*default_constructor)(const meta_ctx &){};
    double (*conversion_helper)(void *, const void *){};
    meta_any (*from_void)(const meta_ctx &, void *, const void *){};
    meta_template_node templ{};
    meta_dtor_node dtor{};
    std::shared_ptr<meta_type_descriptor> details{};
};

template<typename Type>
meta_type_node resolve(const meta_context &) noexcept;

template<typename... Args>
[[nodiscard]] auto meta_arg_node(const meta_context &context, type_list<Args...>, [[maybe_unused]] const std::size_t index) noexcept {
    [[maybe_unused]] std::size_t pos{};
    meta_type_node (*value)(const meta_context &) noexcept = nullptr;
    ((value = (pos++ == index ? &resolve<std::remove_cv_t<std::remove_reference_t<Args>>> : value)), ...);
    ENTT_ASSERT(value != nullptr, "Out of bounds");
    return value(context);
}

[[nodiscard]] inline const void *try_cast(const meta_context &context, const meta_type_node &from, const meta_type_node &to, const void *instance) noexcept {
    if(from.info && to.info && *from.info == *to.info) {
        return instance;
    }

    if(from.details) {
        for(auto &&curr: from.details->base) {
            if(const void *elem = try_cast(context, curr.second.type(context), to, curr.second.cast(instance)); elem) {
                return elem;
            }
        }
    }

    return nullptr;
}

[[nodiscard]] inline const meta_type_node *try_resolve(const meta_context &context, const type_info &info) noexcept {
    const auto it = context.value.find(info.hash());
    return it != context.value.end() ? &it->second : nullptr;
}

template<typename Type>
[[nodiscard]] meta_type_node resolve(const meta_context &context) noexcept {
    static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>, "Invalid type");

    if(auto *elem = try_resolve(context, type_id<Type>()); elem) {
        return *elem;
    }

    meta_type_node node{
        &type_id<Type>(),
        type_id<Type>().hash(),
        (std::is_arithmetic_v<Type> ? meta_traits::is_arithmetic : meta_traits::is_none)
            | (std::is_integral_v<Type> ? meta_traits::is_integral : meta_traits::is_none)
            | (std::is_signed_v<Type> ? meta_traits::is_signed : meta_traits::is_none)
            | (std::is_array_v<Type> ? meta_traits::is_array : meta_traits::is_none)
            | (std::is_enum_v<Type> ? meta_traits::is_enum : meta_traits::is_none)
            | (std::is_class_v<Type> ? meta_traits::is_class : meta_traits::is_none)
            | (is_meta_pointer_like_v<Type> ? meta_traits::is_meta_pointer_like : meta_traits::is_none)
            | (is_complete_v<meta_sequence_container_traits<Type>> ? meta_traits::is_meta_sequence_container : meta_traits::is_none)
            | (is_complete_v<meta_associative_container_traits<Type>> ? meta_traits::is_meta_associative_container : meta_traits::is_none),
        size_of_v<Type>,
        &resolve<Type>,
        &resolve<std::remove_cv_t<std::remove_pointer_t<Type>>>};

    if constexpr(std::is_default_constructible_v<Type>) {
        node.default_constructor = +[](const meta_ctx &ctx) {
            return meta_any{ctx, std::in_place_type<Type>};
        };
    }

    if constexpr(std::is_arithmetic_v<Type>) {
        node.conversion_helper = +[](void *bin, const void *value) {
            return bin ? static_cast<double>(*static_cast<Type *>(bin) = static_cast<Type>(*static_cast<const double *>(value))) : static_cast<double>(*static_cast<const Type *>(value));
        };
    } else if constexpr(std::is_enum_v<Type>) {
        node.conversion_helper = +[](void *bin, const void *value) {
            return bin ? static_cast<double>(*static_cast<Type *>(bin) = static_cast<Type>(static_cast<std::underlying_type_t<Type>>(*static_cast<const double *>(value)))) : static_cast<double>(*static_cast<const Type *>(value));
        };
    }

    if constexpr(!std::is_same_v<Type, void> && !std::is_function_v<Type>) {
        node.from_void = +[](const meta_ctx &ctx, void *element, const void *as_const) {
            if(element) {
                return meta_any{ctx, std::in_place_type<std::decay_t<Type> &>, *static_cast<std::decay_t<Type> *>(element)};
            }

            return meta_any{ctx, std::in_place_type<const std::decay_t<Type> &>, *static_cast<const std::decay_t<Type> *>(as_const)};
        };
    }

    if constexpr(is_complete_v<meta_template_traits<Type>>) {
        node.templ = meta_template_node{
            meta_template_traits<Type>::args_type::size,
            &resolve<typename meta_template_traits<Type>::class_type>,
            +[](const meta_context &area, const std::size_t index) noexcept { return meta_arg_node(area, typename meta_template_traits<Type>::args_type{}, index); }};
    }

    return node;
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

} // namespace entt

#endif
