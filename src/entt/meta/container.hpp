// IWYU pragma: always_keep

#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "../core/concepts.hpp"
#include "../core/type_traits.hpp"
#include "../stl/iterator.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

template<typename Type>
struct sequence_container_extent: integral_constant<meta_dynamic_extent> {};

template<typename Type>
requires is_complete_v<std::tuple_size<Type>>
struct sequence_container_extent<Type>: integral_constant<std::tuple_size_v<Type>> {};

template<typename Type>
inline constexpr std::size_t sequence_container_extent_v = sequence_container_extent<Type>::value;

template<typename Type>
concept meta_sequence_container_like = requires(Type elem) {
    typename Type::value_type;
    typename Type::iterator;
    requires entt::stl::forward_iterator<typename Type::iterator>;
    { elem.begin() } -> std::same_as<typename Type::iterator>;
    { elem.end() } -> std::same_as<typename Type::iterator>;
    requires !requires { typename Type::key_type; };
    requires !requires { elem.substr(); };
};

template<typename Type>
concept meta_associative_container_like = requires(Type value) {
    typename Type::key_type;
    typename Type::value_type;
    typename Type::iterator;
    requires entt::stl::forward_iterator<typename Type::iterator>;
    { value.begin() } -> std::same_as<typename Type::iterator>;
    { value.end() } -> std::same_as<typename Type::iterator>;
    value.find(std::declval<typename Type::key_type>());
};

} // namespace internal
/*! @endcond */

/**
 * @brief General purpose implementation of meta sequence container traits.
 * @tparam Type Type of underlying sequence container.
 */
template<cvref_unqualified Type>
struct basic_meta_sequence_container_traits {
    /*! @brief Unsigned integer type. */
    using size_type = meta_sequence_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = meta_sequence_container::iterator;

    /*! @brief Number of elements, or `meta_dynamic_extent` if dynamic. */
    static constexpr std::size_t extent = internal::sequence_container_extent_v<Type>;

    /**
     * @brief Returns the number of elements in a container.
     * @param container Opaque pointer to a container of the given type.
     * @return Number of elements.
     */
    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    /**
     * @brief Clears a container.
     * @param container Opaque pointer to a container of the given type.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear([[maybe_unused]] void *container) {
        if constexpr(requires(Type elem) { elem.clear(); }) {
            static_cast<Type *>(container)->clear();
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Increases the capacity of a container.
     * @param container Opaque pointer to a container of the given type.
     * @param sz Desired capacity.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(requires(Type elem) { elem.reserve(sz); }) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Resizes a container.
     * @param container Opaque pointer to a container of the given type.
     * @param sz The new number of elements.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool resize([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(std::is_default_constructible_v<typename Type::value_type> && requires(Type elem) { elem.resize(sz); }) {
            static_cast<Type *>(container)->resize(sz);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Returns a possibly const iterator to the beginning or the end.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @param end False to get a pointer that is past the last element.
     * @return An iterator to the first or past the last element of the
     * container.
     */
    static iterator iter(const meta_ctx &area, void *container, const void *as_const, const bool end) {
        return (container == nullptr)
                   ? iterator{area, end ? static_cast<const Type *>(as_const)->cend() : static_cast<const Type *>(as_const)->cbegin()}
                   : iterator{area, end ? static_cast<Type *>(container)->end() : static_cast<Type *>(container)->begin()};
    }

    /**
     * @brief Assigns one element to a container and constructs its object from
     * a given opaque instance.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param value Optional opaque instance of the object to construct (as
     * value type).
     * @param cref Optional opaque instance of the object to construct (as
     * decayed const reference type).
     * @param it Iterator before which the element will be inserted.
     * @return A possibly invalid iterator to the inserted element.
     */
    [[nodiscard]] static iterator insert([[maybe_unused]] const meta_ctx &area, [[maybe_unused]] void *container, [[maybe_unused]] const void *value, [[maybe_unused]] const void *cref, [[maybe_unused]] const iterator &it) {
        if constexpr(requires(Type elem, typename Type::const_iterator it, Type::value_type instance) { elem.insert(it, instance); }) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            return {area, static_cast<Type *>(container)->insert(
                              non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()),
                              (value != nullptr) ? *static_cast<const Type::value_type *>(value) : *static_cast<const std::remove_reference_t<typename Type::const_reference> *>(cref))};
        } else {
            return iterator{};
        }
    }

    /**
     * @brief Erases an element from a container.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param it An opaque iterator to the element to erase.
     * @return A possibly invalid iterator following the last removed element.
     */
    [[nodiscard]] static iterator erase([[maybe_unused]] const meta_ctx &area, [[maybe_unused]] void *container, [[maybe_unused]] const iterator &it) {
        if constexpr(requires(Type elem, typename Type::const_iterator it) { elem.erase(it); }) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            return {area, static_cast<Type *>(container)->erase(non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()))};
        } else {
            return iterator{};
        }
    }
};

/**
 * @brief General purpose implementation of meta associative container traits.
 * @tparam Type Type of underlying associative container.
 */
template<cvref_unqualified Type>
struct basic_meta_associative_container_traits {
    /*! @brief Unsigned integer type. */
    using size_type = meta_associative_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = meta_associative_container::iterator;

    /*! @brief True in case of key-only containers, false otherwise. */
    static constexpr bool key_only = !requires { typename Type::mapped_type; };

    /**
     * @brief Returns the number of elements in a container.
     * @param container Opaque pointer to a container of the given type.
     * @return Number of elements.
     */
    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    /**
     * @brief Clears a container.
     * @param container Opaque pointer to a container of the given type.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(void *container) {
        static_cast<Type *>(container)->clear();
        return true;
    }

    /**
     * @brief Increases the capacity of a container.
     * @param container Opaque pointer to a container of the given type.
     * @param sz Desired capacity.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(requires(Type elem) { elem.reserve(sz); }) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Returns a possibly const iterator to the beginning or the end.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @param end False to get a pointer that is past the last element.
     * @return An iterator to the first or past the last element of the
     * container.
     */
    static iterator iter(const meta_ctx &area, void *container, const void *as_const, const bool end) {
        return (container == nullptr)
                   ? iterator{area, std::bool_constant<key_only>{}, end ? static_cast<const Type *>(as_const)->cend() : static_cast<const Type *>(as_const)->cbegin()}
                   : iterator{area, std::bool_constant<key_only>{}, end ? static_cast<Type *>(container)->end() : static_cast<Type *>(container)->begin()};
    }

    /**
     * @brief Inserts an element into a container, if the key does not exist.
     * @param container Opaque pointer to a container of the given type.
     * @param key An opaque key value of an element to insert.
     * @param value Optional opaque value to insert (key-value containers).
     * @return True if the insertion took place, false otherwise.
     */
    [[nodiscard]] static bool insert(void *container, const void *key, [[maybe_unused]] const void *value) {
        if constexpr(key_only) {
            return static_cast<Type *>(container)->insert(*static_cast<const Type::key_type *>(key)).second;
        } else {
            return static_cast<Type *>(container)->emplace(*static_cast<const Type::key_type *>(key), *static_cast<const Type::mapped_type *>(value)).second;
        }
    }

    /**
     * @brief Removes an element from a container.
     * @param container Opaque pointer to a container of the given type.
     * @param key An opaque key value of an element to remove.
     * @return Number of elements removed (either 0 or 1).
     */
    [[nodiscard]] static size_type erase(void *container, const void *key) {
        return static_cast<Type *>(container)->erase(*static_cast<const Type::key_type *>(key));
    }

    /**
     * @brief Finds an element with a given key.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @param key Opaque key value of an element to search for.
     * @return An iterator to the element with the given key, if any.
     */
    static iterator find(const meta_ctx &area, void *container, const void *as_const, const void *key) {
        return (container != nullptr) ? iterator{area, std::bool_constant<key_only>{}, static_cast<Type *>(container)->find(*static_cast<const Type::key_type *>(key))}
                                      : iterator{area, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->find(*static_cast<const Type::key_type *>(key))};
    }
};

/**
 * @brief Traits meta sequence container like types.
 * @tparam Type Container type to inspect.
 */
template<internal::meta_sequence_container_like Type>
struct meta_sequence_container_traits<Type>: basic_meta_sequence_container_traits<Type> {};

/**
 * @brief Traits for meta associative container like types.
 * @tparam Type Container type to inspect.
 */
template<internal::meta_associative_container_like Type>
struct meta_associative_container_traits<Type>: basic_meta_associative_container_traits<Type> {};

} // namespace entt

#endif
