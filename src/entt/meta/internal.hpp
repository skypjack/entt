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
    enum class operation { COPY, MOVE, DTOR, ADDR, REF };

    using storage_type = std::aligned_storage_t<sizeof(double[2]), alignof(double[2])>;
    using vtable_type = void *(const operation, const meta_storage &, meta_storage *);

    template<typename Type>
    static constexpr auto in_situ = sizeof(Type) <= sizeof(storage_type)
        && std::is_nothrow_move_constructible_v<Type> && std::is_nothrow_copy_constructible_v<Type>;

    template<typename Type>
    static void * basic_vtable(const operation op, const meta_storage &from, meta_storage *to) {
        if constexpr(std::is_void_v<Type>) {
            return nullptr;
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            switch(op) {
            case operation::REF:
                to->vtable = from.vtable;
                [[fallthrough]];
            case operation::COPY:
            case operation::MOVE:
                to->instance = from.instance;
                break;
            case operation::ADDR:
                return from.instance;
            case operation::DTOR:
                break;
            }
        } else if constexpr(in_situ<Type>) {
            auto *instance = std::launder(reinterpret_cast<Type *>(&const_cast<storage_type &>(from.storage)));

            switch(op) {
            case operation::COPY:
                new (&to->storage) Type{std::as_const(*instance)};
                break;
            case operation::MOVE:
                new (&to->storage) Type{std::move(*instance)};
                [[fallthrough]];
            case operation::DTOR:
                instance->~Type();
                break;
            case operation::ADDR:
                return instance;
            case operation::REF:
                to->vtable = basic_vtable<std::add_lvalue_reference_t<Type>>;
                to->instance = instance;
                break;
            }
        } else {
            switch(op) {
            case operation::COPY:
                to->instance = new Type{std::as_const(*static_cast<Type *>(from.instance))};
                break;
            case operation::MOVE:
                to->instance = from.instance;
                break;
            case operation::DTOR:
                delete static_cast<Type *>(from.instance);
                break;
            case operation::ADDR:
                return from.instance;
            case operation::REF:
                to->vtable = basic_vtable<std::add_lvalue_reference_t<Type>>;
                to->instance = from.instance;
                break;
            }
        }

        return nullptr;
    }

public:
    /*! @brief Default constructor. */
    meta_storage() ENTT_NOEXCEPT
        : vtable{&basic_vtable<void>},
          instance{}
    {}

    template<typename Type, typename... Args>
    explicit meta_storage(std::in_place_type_t<Type>, [[maybe_unused]] Args &&... args)
        : vtable{&basic_vtable<Type>}
    {
        if constexpr(!std::is_void_v<Type>) {
            if constexpr(in_situ<Type>) {
                new (&storage) Type{std::forward<Args>(args)...};
            } else {
                instance = new Type{std::forward<Args>(args)...};
            }
        }
    }

    template<typename Type>
    meta_storage(std::reference_wrapper<Type> value)
        : vtable{&basic_vtable<std::add_lvalue_reference_t<Type>>},
          instance{&value.get()}
    {}

    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Type>>, meta_storage>>>
    meta_storage(Type &&value)
        : meta_storage{std::in_place_type<std::remove_cv_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)}
    {}

    meta_storage(const meta_storage &other)
        : meta_storage{}
    {
        vtable = other.vtable;
        vtable(operation::COPY, other, this);
    }

    meta_storage(meta_storage &&other) ENTT_NOEXCEPT
        : meta_storage{}
    {
        vtable = std::exchange(other.vtable, &basic_vtable<void>);
        vtable(operation::MOVE, other, this);
    }

    ~meta_storage() {
        vtable(operation::DTOR, *this, nullptr);
    }

    meta_storage & operator=(meta_storage other) {
        swap(*this, other);
        return *this;
    }

    [[nodiscard]] const void * data() const ENTT_NOEXCEPT {
        return vtable(operation::ADDR, *this, nullptr);
    }

    [[nodiscard]] void * data() ENTT_NOEXCEPT {
        return vtable(operation::ADDR, *this, nullptr);
    }

    template<typename Type, typename... Args>
    void emplace(Args &&... args) {
        *this = meta_storage{std::in_place_type<Type>, std::forward<Args>(args)...};
    }

    [[nodiscard]] meta_storage ref() const ENTT_NOEXCEPT {
        meta_storage other{};
        vtable(operation::REF, *this, &other);
        return other;
    }

    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return !(vtable(operation::ADDR, *this, nullptr) == nullptr);
    }

    friend void swap(meta_storage &lhs, meta_storage &rhs) {
        meta_storage tmp{};
        lhs.vtable(operation::MOVE, lhs, &tmp);
        rhs.vtable(operation::MOVE, rhs, &lhs);
        lhs.vtable(operation::MOVE, tmp, &rhs);
        std::swap(lhs.vtable, rhs.vtable);
    }

private:
    vtable_type *vtable;
    union { void *instance; storage_type storage; };
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
    const type_info info;
    id_type id;
    meta_type_node * next;
    meta_prop_node * prop;
    const size_type size_of;
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
    for(auto &&curr: meta_range{node->*Member}) {
        if(op(&curr)) {
            return &curr;
        }
    }

    for(auto &&curr: meta_range{node->base}) {
        if(auto *ret = find_if<Member>(op, curr.type()); ret) {
            return ret;
        }
    }

    return nullptr;
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
            type_id<Type>(),
            {},
            nullptr,
            nullptr,
            size_of_v<Type>,
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
