#ifndef ENTT_META_INTERNAL_HPP
#define ENTT_META_INTERNAL_HPP


#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include "../core/attribute.h"
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "type_traits.hpp"


namespace entt {


class meta_any;
struct meta_handle;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


class meta_storage {
    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using copy_fn_type = void(meta_storage &, const meta_storage &);
    using steal_fn_type = void(meta_storage &, meta_storage &);
    using destroy_fn_type = void(meta_storage &);

    template<typename Type, typename = std::void_t<>>
    struct type_traits {
        template<typename... Args>
        static void instance(meta_storage &buffer, Args &&... args) {
            buffer.instance = new Type{std::forward<Args>(args)...};
            new (&buffer.storage) Type *{static_cast<Type *>(buffer.instance)};
        }

        static void destroy(meta_storage &buffer) {
            delete static_cast<Type *>(buffer.instance);
        }

        static void copy(meta_storage &to, const meta_storage &from) {
            to.instance = new Type{*static_cast<const Type *>(from.instance)};
            new (&to.storage) Type *{static_cast<Type *>(to.instance)};
        }

        static void steal(meta_storage &to, meta_storage &from) {
            new (&to.storage) Type *{static_cast<Type *>(from.instance)};
            to.instance = from.instance;
        }
    };

    template<typename Type>
    struct type_traits<Type, std::enable_if_t<sizeof(Type) <= sizeof(void *) && std::is_nothrow_move_constructible_v<Type>>> {
        template<typename... Args>
        static void instance(meta_storage &buffer, Args &&... args) {
            buffer.instance = new (&buffer.storage) Type{std::forward<Args>(args)...};
        }

        static void destroy(meta_storage &buffer) {
            static_cast<Type *>(buffer.instance)->~Type();
        }

        static void copy(meta_storage &to, const meta_storage &from) {
            to.instance = new (&to.storage) Type{*static_cast<const Type *>(from.instance)};
        }

        static void steal(meta_storage &to, meta_storage &from) {
            to.instance = new (&to.storage) Type{std::move(*static_cast<Type *>(from.instance))};
            destroy(from);
        }
    };

public:
    /*! @brief Default constructor. */
    meta_storage() ENTT_NOEXCEPT
        : storage{},
          instance{},
          destroy_fn{},
          copy_fn{},
          steal_fn{}
    {}

    template<typename Type, typename... Args>
    explicit meta_storage(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : meta_storage{}
    {
        if constexpr(!std::is_void_v<Type>) {
            type_traits<Type>::instance(*this, std::forward<Args>(args)...);
            destroy_fn = &type_traits<Type>::destroy;
            copy_fn = &type_traits<Type>::copy;
            steal_fn = &type_traits<Type>::steal;
        }
    }

    template<typename Type>
    meta_storage(std::reference_wrapper<Type> value)
        : meta_storage{}
    {
        instance = &value.get();
    }

    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_storage>>>
    meta_storage(Type &&value)
        : meta_storage{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    meta_storage(const meta_storage &other)
        : meta_storage{}
    {
        (other.copy_fn ? other.copy_fn : [](auto &to, const auto &from) { to.instance = from.instance; })(*this, other);
        destroy_fn = other.destroy_fn;
        copy_fn = other.copy_fn;
        steal_fn = other.steal_fn;
    }

    meta_storage(meta_storage &&other)
        : meta_storage{}
    {
        swap(*this, other);
    }

    ~meta_storage() {
        if(destroy_fn) {
            destroy_fn(*this);
        }
    }

    meta_storage & operator=(meta_storage other) {
        swap(other, *this);
        return *this;
    }

    [[nodiscard]] const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(std::as_const(*this).data());
    }

    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = meta_storage{std::in_place_type<Type>, std::forward<Args>(args)...};
    }

    [[nodiscard]] meta_storage ref() const ENTT_NOEXCEPT {
        meta_storage other{};
        other.instance = instance;
        return other;
    }

    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(instance == nullptr);
    }

    friend void swap(meta_storage &lhs, meta_storage &rhs) {
        using std::swap;

        if(lhs.steal_fn && rhs.steal_fn) {
            meta_storage buffer{};
            lhs.steal_fn(buffer, lhs);
            rhs.steal_fn(lhs, rhs);
            lhs.steal_fn(rhs, buffer);
        } else if(lhs.steal_fn) {
            lhs.steal_fn(rhs, lhs);
        } else if(rhs.steal_fn) {
            rhs.steal_fn(lhs, rhs);
        } else {
            swap(lhs.instance, rhs.instance);
        }

        swap(lhs.destroy_fn, rhs.destroy_fn);
        swap(lhs.copy_fn, rhs.copy_fn);
        swap(lhs.steal_fn, rhs.steal_fn);
    }

private:
    storage_type storage;
    void *instance;
    destroy_fn_type *destroy_fn;
    copy_fn_type *copy_fn;
    steal_fn_type *steal_fn;
};


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
    const void *(* const cast)(const void *) ENTT_NOEXCEPT;
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


struct meta_data_node {
    id_type id;
    meta_type_node * const parent;
    meta_data_node * next;
    meta_prop_node * prop;
    const bool is_static;
    meta_type_node *(* const type)() ENTT_NOEXCEPT;
    bool(* const set)(meta_handle, meta_any);
    meta_any(* const get)(meta_handle);
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
    meta_any(* const invoke)(meta_handle, meta_any *);
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
    const bool is_pointer_like;
    const bool is_sequence_container;
    const bool is_associative_container;
    const size_type rank;
    size_type(* const extent)(size_type);
    bool(* const compare)(const void *, const void *);
    meta_type_node *(* const remove_pointer)() ENTT_NOEXCEPT;
    meta_type_node *(* const remove_extent)() ENTT_NOEXCEPT;
    meta_base_node *base{nullptr};
    meta_conv_node *conv{nullptr};
    meta_ctor_node *ctor{nullptr};
    meta_data_node *data{nullptr};
    meta_func_node *func{nullptr};
    void(* dtor)(void *){nullptr};
};


template<typename Node>
class meta_range {
    struct range_iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::forward_iterator_tag;

        range_iterator() ENTT_NOEXCEPT = default;

        range_iterator(Node *head) ENTT_NOEXCEPT
            : node{head}
        {}

        range_iterator & operator++() ENTT_NOEXCEPT {
            return node = node->next, *this;
        }

        range_iterator operator++(int) ENTT_NOEXCEPT {
            range_iterator orig = *this;
            return ++(*this), orig;
        }

        [[nodiscard]] bool operator==(const range_iterator &other) const ENTT_NOEXCEPT {
            return other.node == node;
        }

        [[nodiscard]] bool operator!=(const range_iterator &other) const ENTT_NOEXCEPT {
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

public:
    using iterator = range_iterator;

    meta_range() ENTT_NOEXCEPT = default;

    meta_range(Node *head)
        : node{head}
    {}

    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return iterator{node};
    }

    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{};
    }

private:
    Node *node{nullptr};
};


template<auto Member, typename Op>
auto find_if(const Op &op, const meta_type_node *node)
-> std::decay_t<decltype(node->*Member)> {
    std::decay_t<decltype(node->*Member)> ret = nullptr;

    for(auto &&curr: meta_range{node->*Member}) {
        if(op(&curr)) {
            ret = &curr;
            break;
        }
    }

    if(!ret) {
        for(auto &&curr: meta_range{node->base}) {
            if(ret = find_if<Member>(op, curr.type()); ret) {
                break;
            }
        }
    }

    return ret;
}


template<typename Type>
class ENTT_API meta_node {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Invalid type");

    [[nodiscard]] static bool compare(const void *lhs, const void *rhs) {
        if constexpr(!std::is_function_v<Type> && is_equality_comparable_v<Type>) {
            return *static_cast<const Type *>(lhs) == *static_cast<const Type *>(rhs);
        } else {
            return lhs == rhs;
        }
    }

    template<std::size_t... Index>
    [[nodiscard]] static auto extent(meta_type_node::size_type dim, std::index_sequence<Index...>) {
        meta_type_node::size_type ext{};
        ((ext = (dim == Index ? std::extent_v<Type, Index> : ext)), ...);
        return ext;
    }

public:
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
            is_meta_pointer_like_v<Type>,
            has_meta_sequence_container_traits_v<Type>,
            has_meta_associative_container_traits_v<Type>,
            std::rank_v<Type>,
            [](meta_type_node::size_type dim) {
                return extent(dim, std::make_index_sequence<std::rank_v<Type>>{});
            },
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
 * @endcond
 */


}


#endif
