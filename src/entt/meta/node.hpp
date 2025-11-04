#ifndef ENTT_META_NODE_HPP
#define ENTT_META_NODE_HPP

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/attribute.h"
#include "../core/bit.hpp"
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
class meta_handle;

/*! @cond TURN_OFF_DOXYGEN */
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
    is_pointer = 0x0100,
    is_pointer_like = 0x0200,
    is_sequence_container = 0x0400,
    is_associative_container = 0x0800,
    _user_defined_traits = 0xFFFF,
    _entt_enum_as_bitmask = 0xFFFF
};

template<typename Type>
[[nodiscard]] auto meta_to_user_traits(const meta_traits traits) noexcept {
    static_assert(std::is_enum_v<Type>, "Invalid enum type");
    constexpr auto shift = popcount(static_cast<std::underlying_type_t<meta_traits>>(meta_traits::_user_defined_traits));
    return Type{static_cast<std::underlying_type_t<Type>>(static_cast<std::underlying_type_t<meta_traits>>(traits) >> shift)};
}

template<typename Type>
[[nodiscard]] auto user_to_meta_traits(const Type value) noexcept {
    static_assert(std::is_enum_v<Type>, "Invalid enum type");
    constexpr auto shift = popcount(static_cast<std::underlying_type_t<meta_traits>>(meta_traits::_user_defined_traits));
    const auto traits = static_cast<std::underlying_type_t<internal::meta_traits>>(static_cast<std::underlying_type_t<Type>>(value));
    ENTT_ASSERT(traits < ((~static_cast<std::underlying_type_t<meta_traits>>(meta_traits::_user_defined_traits)) >> shift), "Invalid traits");
    return meta_traits{traits << shift};
}

struct meta_type_node;

struct meta_custom_node {
    id_type type{};
    std::shared_ptr<void> value{};
};

struct meta_base_node {
    id_type type{};
    const meta_type_node &(*resolve)(const meta_context &) noexcept {};
    const void *(*cast)(const void *) noexcept {};
};

struct meta_conv_node {
    id_type type{};
    meta_any (*conv)(const meta_ctx &, const void *){};
};

struct meta_ctor_node {
    using size_type = std::size_t;

    id_type id{};
    size_type arity{0u};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    meta_any (*invoke)(const meta_ctx &, meta_any *const){};
};

struct meta_data_node {
    using size_type = std::size_t;

    id_type id{};
    const char *name{};
    meta_traits traits{meta_traits::is_none};
    size_type arity{0u};
    const meta_type_node &(*type)(const meta_context &) noexcept {};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    bool (*set)(meta_handle, meta_any){};
    meta_any (*get)(meta_handle){};
    meta_custom_node custom{};
};

struct meta_func_node {
    using size_type = std::size_t;

    id_type id{};
    const char *name{};
    meta_traits traits{meta_traits::is_none};
    size_type arity{0u};
    const meta_type_node &(*ret)(const meta_context &) noexcept {};
    meta_type (*arg)(const meta_ctx &, const size_type) noexcept {};
    meta_any (*invoke)(meta_handle, meta_any *const){};
    std::unique_ptr<meta_func_node> next;
    meta_custom_node custom{};
};

struct meta_template_node {
    using size_type = std::size_t;

    size_type arity{0u};
    const meta_type_node &(*resolve)(const meta_context &) noexcept {};
    const meta_type_node &(*arg)(const meta_context &, const size_type) noexcept {};
};

struct meta_type_descriptor {
    std::vector<meta_ctor_node> ctor{};
    std::vector<meta_base_node> base{};
    std::vector<meta_conv_node> conv{};
    std::vector<meta_data_node> data{};
    std::vector<meta_func_node> func{};
};

struct meta_type_node {
    using size_type = std::size_t;

    const type_info *info{};
    id_type id{};
    const char *name{};
    meta_traits traits{meta_traits::is_none};
    size_type size_of{0u};
    const meta_type_node &(*remove_pointer)(const meta_context &) noexcept {};
    meta_any (*default_constructor)(const meta_ctx &){};
    double (*conversion_helper)(void *, const void *){};
    meta_any (*from_void)(const meta_ctx &, void *, const void *){};
    meta_template_node templ{};
    meta_custom_node custom{};
    std::unique_ptr<meta_type_descriptor> details{};
};

template<auto Member, typename Type, typename Value>
[[nodiscard]] auto *find_member(Type &from, const Value value) {
    for(auto &&elem: from) {
        if((elem.*Member) == value) {
            return &elem;
        }
    }

    return static_cast<typename Type::value_type *>(nullptr);
}

[[nodiscard]] inline auto *find_overload(meta_func_node *curr, std::remove_pointer_t<decltype(meta_func_node::invoke)> *const ref) {
    while((curr != nullptr) && (curr->invoke != ref)) { curr = curr->next.get(); }
    return curr;
}

template<auto Member>
[[nodiscard]] auto *look_for(const meta_context &context, const meta_type_node &node, const id_type id, bool recursive) {
    using value_type = typename std::remove_reference_t<decltype((node.details.get()->*Member))>::value_type;

    if(node.details) {
        if(auto *member = find_member<&value_type::id>((node.details.get()->*Member), id); member != nullptr) {
            return member;
        }

        if(recursive) {
            for(auto &&curr: node.details->base) {
                if(auto *elem = look_for<Member>(context, curr.resolve(context), id, recursive); elem) {
                    return elem;
                }
            }
        }
    }

    return static_cast<value_type *>(nullptr);
}

template<typename Type>
const meta_type_node &resolve(const meta_context &) noexcept;

template<typename... Args>
[[nodiscard]] const meta_type_node &meta_arg_node(const meta_context &context, type_list<Args...>, const std::size_t index) noexcept {
    using resolve_type = const meta_type_node &(*)(const meta_context &) noexcept;
    constexpr std::array<resolve_type, sizeof...(Args)> list{&resolve<std::remove_const_t<std::remove_reference_t<Args>>>...};
    ENTT_ASSERT(index < sizeof...(Args), "Out of bounds");
    return list[index](context);
}

[[nodiscard]] inline const void *try_cast(const meta_context &context, const meta_type_node &from, const id_type to, const void *instance) noexcept {
    if(from.details) {
        for(auto &&curr: from.details->base) {
            if(const void *other = curr.cast(instance); curr.type == to) {
                return other;
            } else if(const void *elem = try_cast(context, curr.resolve(context), to, other); elem) {
                return elem;
            }
        }
    }

    return nullptr;
}

template<typename Type>
auto setup_node_for() noexcept {
    meta_type_node node{
        &type_id<Type>(),
        type_id<Type>().hash(),
        nullptr,
        (std::is_arithmetic_v<Type> ? meta_traits::is_arithmetic : meta_traits::is_none)
            | (std::is_integral_v<Type> ? meta_traits::is_integral : meta_traits::is_none)
            | (std::is_signed_v<Type> ? meta_traits::is_signed : meta_traits::is_none)
            | (std::is_array_v<Type> ? meta_traits::is_array : meta_traits::is_none)
            | (std::is_enum_v<Type> ? meta_traits::is_enum : meta_traits::is_none)
            | (std::is_class_v<Type> ? meta_traits::is_class : meta_traits::is_none)
            | (std::is_pointer_v<Type> ? meta_traits::is_pointer : meta_traits::is_none)
            | (is_meta_pointer_like_v<Type> ? meta_traits::is_pointer_like : meta_traits::is_none)
            | (is_complete_v<meta_sequence_container_traits<Type>> ? meta_traits::is_sequence_container : meta_traits::is_none)
            | (is_complete_v<meta_associative_container_traits<Type>> ? meta_traits::is_associative_container : meta_traits::is_none),
        size_of_v<Type>,
        &resolve<std::remove_const_t<std::remove_pointer_t<Type>>>};

    if constexpr(std::is_default_constructible_v<Type>) {
        node.default_constructor = +[](const meta_ctx &ctx) {
            return meta_any{ctx, std::in_place_type<Type>};
        };
    }

    if constexpr(std::is_arithmetic_v<Type>) {
        node.conversion_helper = +[](void *lhs, const void *rhs) {
            return lhs ? static_cast<double>(*static_cast<Type *>(lhs) = static_cast<Type>(*static_cast<const double *>(rhs))) : static_cast<double>(*static_cast<const Type *>(rhs));
        };
    } else if constexpr(std::is_enum_v<Type>) {
        node.conversion_helper = +[](void *lhs, const void *rhs) {
            return lhs ? static_cast<double>(*static_cast<Type *>(lhs) = static_cast<Type>(static_cast<std::underlying_type_t<Type>>(*static_cast<const double *>(rhs)))) : static_cast<double>(*static_cast<const Type *>(rhs));
        };
    }

    if constexpr(!std::is_void_v<Type> && !std::is_function_v<Type>) {
        node.from_void = +[](const meta_ctx &ctx, void *elem, const void *celem) {
            if(elem && celem) { // ownership construction request
                return meta_any{ctx, std::in_place, static_cast<std::decay_t<Type> *>(elem)};
            }

            if(elem) { // non-const reference construction request
                return meta_any{ctx, std::in_place_type<std::decay_t<Type> &>, *static_cast<std::decay_t<Type> *>(elem)};
            }

            // const reference construction request
            return meta_any{ctx, std::in_place_type<const std::decay_t<Type> &>, *static_cast<const std::decay_t<Type> *>(celem)};
        };
    }

    if constexpr(is_complete_v<meta_template_traits<Type>>) {
        node.templ = meta_template_node{
            meta_template_traits<Type>::args_type::size,
            &resolve<typename meta_template_traits<Type>::class_type>,
            +[](const meta_context &area, const std::size_t index) noexcept -> decltype(auto) { return meta_arg_node(area, typename meta_template_traits<Type>::args_type{}, index); }};
    }

    return node;
}

[[nodiscard]] inline const meta_type_node *try_resolve(const meta_context &context, const type_info &info) noexcept {
    const auto it = context.value.find(info.hash());
    return it != context.value.end() ? it->second.get() : nullptr;
}

template<typename Type>
[[nodiscard]] const meta_type_node &resolve(const meta_context &context) noexcept {
    static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>, "Invalid type");
    static const meta_type_node node = setup_node_for<Type>();
    const auto *elem = try_resolve(context, *node.info);
    return (elem == nullptr) ? node : *elem;
}

} // namespace internal
/*! @endcond */

} // namespace entt

#endif
