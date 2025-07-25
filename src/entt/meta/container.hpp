// IWYU pragma: always_keep

#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP

#include <array>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../container/dense_map.hpp"
#include "../container/dense_set.hpp"
#include "../core/type_traits.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type, typename = void>
struct sequence_container_extent: integral_constant<meta_dynamic_extent> {};

template<typename Type>
struct sequence_container_extent<Type, std::enable_if_t<is_complete_v<std::tuple_size<Type>>>>: integral_constant<std::tuple_size_v<Type>> {};

template<typename Type>
inline constexpr std::size_t sequence_container_extent_v = sequence_container_extent<Type>::value;

template<typename, typename = void>
struct key_only_associative_container: std::true_type {};

template<typename Type>
struct key_only_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename Type>
inline constexpr bool key_only_associative_container_v = key_only_associative_container<Type>::value;

template<typename, typename = void>
struct reserve_aware_container: std::false_type {};

template<typename Type>
struct reserve_aware_container<Type, std::void_t<decltype(&Type::reserve)>>: std::true_type {};

template<typename Type>
inline constexpr bool reserve_aware_container_v = reserve_aware_container<Type>::value;

} // namespace internal
/*! @endcond */

/**
 * @brief General purpose implementation of meta sequence container traits.
 * @tparam Type Type of underlying sequence container.
 */
template<typename Type>
struct basic_meta_sequence_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    /*! @brief Unsigned integer type. */
    using size_type = typename meta_sequence_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = typename meta_sequence_container::iterator;

    /*! @brief Number of elements, or `meta_dynamic_extent` if dynamic. */
    static constexpr std::size_t extent = internal::sequence_container_extent_v<Type>;
    /*! @brief True in case of fixed size containers, false otherwise. */
    [[deprecated("use ::extent instead")]] static constexpr bool fixed_size = (extent != meta_dynamic_extent);

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
        if constexpr(extent == meta_dynamic_extent) {
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
        if constexpr(internal::reserve_aware_container_v<Type>) {
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
        if constexpr((extent == meta_dynamic_extent) && std::is_default_constructible_v<typename Type::value_type>) {
            static_cast<Type *>(container)->resize(sz);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Returns a possibly const iterator to the beginning.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @return An iterator to the first element of the container.
     */
    static iterator begin(const meta_ctx &area, void *container, const void *as_const) {
        return (container != nullptr) ? iterator{area, static_cast<Type *>(container)->begin()}
                                      : iterator{area, static_cast<const Type *>(as_const)->begin()};
    }

    /**
     * @brief Returns a possibly const iterator to the end.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @return An iterator that is past the last element of the container.
     */
    static iterator end(const meta_ctx &area, void *container, const void *as_const) {
        return (container != nullptr) ? iterator{area, static_cast<Type *>(container)->end()}
                                      : iterator{area, static_cast<const Type *>(as_const)->end()};
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
        if constexpr(extent == meta_dynamic_extent) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            return {area, static_cast<Type *>(container)->insert(
                              non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()),
                              (value != nullptr) ? *static_cast<const typename Type::value_type *>(value) : *static_cast<const std::remove_reference_t<typename Type::const_reference> *>(cref))};
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
        if constexpr(extent == meta_dynamic_extent) {
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
template<typename Type>
struct basic_meta_associative_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    /*! @brief Unsigned integer type. */
    using size_type = typename meta_associative_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = typename meta_associative_container::iterator;

    /*! @brief True in case of key-only containers, false otherwise. */
    static constexpr bool key_only = internal::key_only_associative_container_v<Type>;

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
        if constexpr(internal::reserve_aware_container_v<Type>) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Returns a possibly const iterator to the beginning.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @return An iterator to the first element of the container.
     */
    static iterator begin(const meta_ctx &area, void *container, const void *as_const) {
        return (container != nullptr) ? iterator{area, std::bool_constant<key_only>{}, static_cast<Type *>(container)->begin()}
                                      : iterator{area, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->begin()};
    }

    /**
     * @brief Returns a possibly const iterator to the end.
     * @param area The context to pass to the newly created iterator.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const Const opaque pointer fallback.
     * @return An iterator that is past the last element of the container.
     */
    static iterator end(const meta_ctx &area, void *container, const void *as_const) {
        return (container != nullptr) ? iterator{area, std::bool_constant<key_only>{}, static_cast<Type *>(container)->end()}
                                      : iterator{area, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->end()};
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
            return static_cast<Type *>(container)->insert(*static_cast<const typename Type::key_type *>(key)).second;
        } else {
            return static_cast<Type *>(container)->emplace(*static_cast<const typename Type::key_type *>(key), *static_cast<const typename Type::mapped_type *>(value)).second;
        }
    }

    /**
     * @brief Removes an element from a container.
     * @param container Opaque pointer to a container of the given type.
     * @param key An opaque key value of an element to remove.
     * @return Number of elements removed (either 0 or 1).
     */
    [[nodiscard]] static size_type erase(void *container, const void *key) {
        return static_cast<Type *>(container)->erase(*static_cast<const typename Type::key_type *>(key));
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
        return (container != nullptr) ? iterator{area, std::bool_constant<key_only>{}, static_cast<Type *>(container)->find(*static_cast<const typename Type::key_type *>(key))}
                                      : iterator{area, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->find(*static_cast<const typename Type::key_type *>(key))};
    }
};

/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::vector<Args...>>
    : basic_meta_sequence_container_traits<std::vector<Args...>> {};

/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type Template arguments for the container.
 * @tparam N Template arguments for the container.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>
    : basic_meta_sequence_container_traits<std::array<Type, N>> {};

/**
 * @brief Meta sequence container traits for `std::list`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::list<Args...>>
    : basic_meta_sequence_container_traits<std::list<Args...>> {};

/**
 * @brief Meta sequence container traits for `std::deque`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::deque<Args...>>
    : basic_meta_sequence_container_traits<std::deque<Args...>> {};

/**
 * @brief Meta associative container traits for `std::map`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::map<Args...>>
    : basic_meta_associative_container_traits<std::map<Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_map`s of any
 * type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::unordered_map<Args...>>
    : basic_meta_associative_container_traits<std::unordered_map<Args...>> {};

/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::set<Args...>>
    : basic_meta_associative_container_traits<std::set<Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::unordered_set<Args...>>
    : basic_meta_associative_container_traits<std::unordered_set<Args...>> {};

/**
 * @brief Meta associative container traits for `dense_map`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<dense_map<Args...>>
    : basic_meta_associative_container_traits<dense_map<Args...>> {};

/**
 * @brief Meta associative container traits for `dense_set`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<dense_set<Args...>>
    : basic_meta_associative_container_traits<dense_set<Args...>> {};

} // namespace entt

#endif
