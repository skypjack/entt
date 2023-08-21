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
struct dynamic_sequence_container: std::false_type {};

template<typename Type>
struct dynamic_sequence_container<Type, std::void_t<decltype(&Type::clear)>>: std::true_type {};

template<typename, typename = void>
struct key_only_associative_container: std::true_type {};

template<typename Type>
struct key_only_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename, typename = void>
struct reserve_aware_container: std::false_type {};

template<typename Type>
struct reserve_aware_container<Type, std::void_t<decltype(&Type::reserve)>>: std::true_type {};

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
class basic_meta_sequence_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    friend meta_sequence_container;

    using size_type = typename meta_sequence_container::size_type;
    using iterator = typename meta_sequence_container::iterator;

    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    [[nodiscard]] static bool clear([[maybe_unused]] void *container) {
        if constexpr(internal::dynamic_sequence_container<Type>::value) {
            static_cast<Type *>(container)->clear();
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(internal::reserve_aware_container<Type>::value) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] static bool resize([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(internal::dynamic_sequence_container<Type>::value) {
            static_cast<Type *>(container)->resize(sz);
            return true;
        } else {
            return false;
        }
    }

    static void begin(const void *container, const bool as_const, iterator &it) {
        if(as_const) {
            it.rebind(static_cast<const Type *>(container)->begin());
        } else {
            it.rebind(static_cast<Type *>(const_cast<void *>(container))->begin());
        }
    }

    static void end(const void *container, const bool as_const, iterator &it) {
        if(as_const) {
            it.rebind(static_cast<const Type *>(container)->end());
        } else {
            it.rebind(static_cast<Type *>(const_cast<void *>(container))->end());
        }
    }

    [[nodiscard]] static bool insert([[maybe_unused]] void *container, [[maybe_unused]] meta_any &elem, [[maybe_unused]] iterator &it) {
        if constexpr(internal::dynamic_sequence_container<Type>::value) {
            // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
            if(elem.allow_cast<typename Type::const_reference>() || elem.allow_cast<typename Type::value_type>()) {
                auto *const non_const = any_cast<typename Type::iterator>(&it.base());
                const auto *element = elem.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                it.rebind(static_cast<Type *>(container)->insert(non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()), element ? *element : elem.cast<typename Type::value_type>()));
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] static bool erase([[maybe_unused]] void *container, [[maybe_unused]] iterator &it) {
        if constexpr(internal::dynamic_sequence_container<Type>::value) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            it.rebind(static_cast<Type *>(container)->erase(non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base())));
            return true;
        } else {
            return false;
        }
    }
};

/**
 * @brief General purpose implementation of meta associative container traits.
 * @tparam Type Type of underlying associative container.
 */
template<typename Type>
class basic_meta_associative_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    friend meta_associative_container;

    static constexpr auto key_only = internal::key_only_associative_container<Type>::value;

    using size_type = typename meta_associative_container::size_type;
    using iterator = typename meta_associative_container::iterator;

    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    [[nodiscard]] static bool clear(void *container) {
        static_cast<Type *>(container)->clear();
        return true;
    }

    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(internal::reserve_aware_container<Type>::value) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    static void begin(const void *container, const bool as_const, iterator &it) {
        if(as_const) {
            it.rebind<key_only>(static_cast<const Type *>(container)->begin());
        } else {
            it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->begin());
        }
    }

    static void end(const void *container, const bool as_const, iterator &it) {
        if(as_const) {
            it.rebind<key_only>(static_cast<const Type *>(container)->end());
        } else {
            it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->end());
        }
    }

    [[nodiscard]] static bool insert(void *container, const void *key, [[maybe_unused]] const void *value) {
        if constexpr(internal::key_only_associative_container<Type>::value) {
            return static_cast<Type *>(container)->insert(*static_cast<const typename Type::key_type *>(key)).second;
        } else {
            return static_cast<Type *>(container)->emplace(*static_cast<const typename Type::key_type *>(key), *static_cast<const typename Type::mapped_type *>(value)).second;
        }
    }

    [[nodiscard]] static size_type erase(void *container, const void *key) {
        return static_cast<Type *>(container)->erase(*static_cast<const typename Type::key_type *>(key));
    }

    static void find(const void *container, const bool as_const, const void *key, iterator &it) {
        if(as_const) {
            it.rebind<key_only>(static_cast<const Type *>(container)->find(*static_cast<const typename Type::key_type *>(key)));
        } else {
            it.rebind<key_only>(static_cast<Type *>(const_cast<void *>(container))->find(*static_cast<const typename Type::key_type *>(key)));
        }
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
