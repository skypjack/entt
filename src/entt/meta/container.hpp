#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP

#include <array>
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
#include "meta.hpp"
#include "type_traits.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename, typename = void>
struct fixed_size_sequence_container: std::true_type {};

template<typename Type>
struct fixed_size_sequence_container<Type, std::void_t<decltype(&Type::clear)>>: std::false_type {};

template<typename Type>
inline constexpr bool fixed_size_sequence_container_v = fixed_size_sequence_container<Type>::value;

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

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief General purpose implementation of meta sequence container traits.
 * @tparam Type Type of underlying sequence container.
 */
template<typename Type>
struct basic_meta_sequence_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    /*! @brief True in case of key-only containers, false otherwise. */
    static constexpr bool fixed_size = internal::fixed_size_sequence_container_v<Type>;

    /*! @brief Unsigned integer type. */
    using size_type = typename meta_sequence_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = typename meta_sequence_container::iterator;

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
        if constexpr(fixed_size) {
            return false;
        } else {
            static_cast<Type *>(container)->clear();
            return true;
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
        if constexpr(fixed_size || !std::is_default_constructible_v<typename Type::value_type>) {
            return false;
        } else {
            static_cast<Type *>(container)->resize(sz);
            return true;
        }
    }

    /**
     * @brief Returns a possibly const iterator to the beginning.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const True for const-only containers, false otherwise.
     * @param it The meta iterator to rebind the underlying iterator to.
     */
    static void begin(const void *container, const bool as_const, iterator &it) {
        if(as_const) {
            it.rebind(static_cast<const Type *>(container)->begin());
        } else {
            it.rebind(static_cast<Type *>(const_cast<void *>(container))->begin());
        }
    }

    /**
     * @brief Returns a possibly const iterator to the end.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const True for const-only containers, false otherwise.
     * @param it The meta iterator to rebind the underlying iterator to.
     */
    static void end(const void *container, const bool as_const, iterator &it) {
        as_const ? it.rebind(static_cast<const Type *>(container)->end()) : it.rebind(static_cast<Type *>(const_cast<void *>(container))->end());
    }

    /**
     * @brief Assigns one element to a container and constructs its object from
     * a given opaque instance.
     * @param container Opaque pointer to a container of the given type.
     * @param value Optional opaque instance of the object to construct (as
     * value type).
     * @param cref Optional opaque instance of the object to construct (as
     * decayed const reference type).
     * @param it The meta iterator to rebind the underlying iterator to.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool insert([[maybe_unused]] void *container, [[maybe_unused]] const void *value, [[maybe_unused]] const void *cref, [[maybe_unused]] iterator &it) {
        if constexpr(fixed_size) {
            return false;
        } else {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());

            it.rebind(static_cast<Type *>(container)->insert(
                non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()),
                value ? *static_cast<const typename Type::value_type *>(value) : *static_cast<const std::remove_reference_t<typename Type::const_reference> *>(cref)));

            return true;
        }
    }

    /**
     * @brief Erases an element from a container.
     * @param container Opaque pointer to a container of the given type.
     * @param it An opaque iterator to the element to erase.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool erase([[maybe_unused]] void *container, [[maybe_unused]] iterator &it) {
        if constexpr(fixed_size) {
            return false;
        } else {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            it.rebind(static_cast<Type *>(container)->erase(non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base())));
            return true;
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

    /*! @brief True in case of key-only containers, false otherwise. */
    static constexpr bool key_only = internal::key_only_associative_container_v<Type>;

    /*! @brief Unsigned integer type. */
    using size_type = typename meta_associative_container::size_type;
    /*! @brief Meta iterator type. */
    using iterator = typename meta_associative_container::iterator;

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
     * @param container Opaque pointer to a container of the given type.
     * @param as_const True for const-only containers, false otherwise.
     * @param it The meta iterator to rebind the underlying iterator to.
     */
    static void begin(const void *container, const bool as_const, iterator &it) {
        as_const ? it.rebind<key_only>(static_cast<const Type *>(container)->begin()) : it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->begin());
    }

    /**
     * @brief Returns a possibly const iterator to the end.
     * @param container Opaque pointer to a container of the given type.
     * @param as_const True for const-only containers, false otherwise.
     * @param it The meta iterator to rebind the underlying iterator to.
     */
    static void end(const void *container, const bool as_const, iterator &it) {
        as_const ? it.rebind<key_only>(static_cast<const Type *>(container)->end()) : it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->end());
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
     * @param container Opaque pointer to a container of the given type.
     * @param as_const True for const-only containers, false otherwise.
     * @param key Opaque key value of an element to search for.
     * @param it The meta iterator to rebind the underlying iterator to.
     */
    static void find(const void *container, const bool as_const, const void *key, iterator &it) {
        const auto &elem = *static_cast<const typename Type::key_type *>(key);
        as_const ? it.rebind<key_only>(static_cast<const Type *>(container)->find(elem)) : it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->find(elem));
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
