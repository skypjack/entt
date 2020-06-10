#ifndef ENTT_META_INTERNAL_HPP
#define ENTT_META_INTERNAL_HPP


#include <algorithm>
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


template<typename Node>
struct meta_iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = Node;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;

    meta_iterator() ENTT_NOEXCEPT = default;

    meta_iterator(Node *head) ENTT_NOEXCEPT
        : node{head}
    {}

    meta_iterator & operator++() ENTT_NOEXCEPT {
        return node = node->next, *this;
    }

    meta_iterator operator++(int) ENTT_NOEXCEPT {
        meta_iterator orig = *this;
        return operator++(), orig;
    }

    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return other.node == node;
    }

    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
        return node;
    }

    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return *operator->();
    }

private:
    Node *node{nullptr};
};


template<typename Node>
struct meta_range {
    using iterator = meta_iterator<Node>;
    using const_iterator = meta_iterator<const Node>;

    meta_range() ENTT_NOEXCEPT = default;

    meta_range(Node *head)
        : node{head}
    {}

    iterator begin() ENTT_NOEXCEPT {
        return iterator{node};
    }

    const_iterator begin() const ENTT_NOEXCEPT {
        return const_iterator{node};
    }

    const_iterator cbegin() const ENTT_NOEXCEPT {
        return begin();
    }

    iterator end() ENTT_NOEXCEPT {
        return iterator{};
    }

    const_iterator end() const ENTT_NOEXCEPT {
        return const_iterator{};
    }

    const_iterator cend() const ENTT_NOEXCEPT {
        return end();
    }

private:
    Node *node{nullptr};
};


template<auto Member, typename Type, typename Op>
void visit(Op &op, const internal::meta_type_node *node) {
    for(auto &&curr: meta_range{node->*Member}) {
        op(Type{&curr});
    }

    for(auto &&base: meta_range{node->base}) {
        visit<Member, Type>(op, base.type());
    }
}


template<auto Member, typename Op>
auto find_if(const Op &op, const meta_type_node *node)
-> std::decay_t<decltype(node->*Member)> {
    std::decay_t<decltype(node->*Member)> ret = nullptr;
    meta_range range{node->*Member};

    if(ret = std::find_if(range.begin(), range.end(), [&op](const auto &curr) { return op(&curr); }).operator->(); !ret) {
        meta_range base{node->base};

        for(auto first = base.begin(), last = base.end(); first != last && !ret; ++first) {
            ret = find_if<Member>(op, first->type());
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
