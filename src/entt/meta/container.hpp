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

    using operation = internal::meta_sequence_container_operation;
    using size_type = typename meta_sequence_container::size_type;
    using iterator = typename meta_sequence_container::iterator;

    static size_type basic_vtable(const operation op, const void *container, const void *value, iterator *it) {
        switch(op) {
        case operation::size:
            return static_cast<const Type *>(container)->size();
        case operation::clear:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                static_cast<Type *>(const_cast<void *>(container))->clear();
                return true;
            } else {
                break;
            }
        case operation::reserve:
            if constexpr(internal::reserve_aware_container<Type>::value) {
                static_cast<Type *>(const_cast<void *>(container))->reserve(*static_cast<const size_type *>(value));
                return true;
            } else {
                break;
            }
        case operation::resize:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                static_cast<Type *>(const_cast<void *>(container))->resize(*static_cast<const size_type *>(value));
                return true;
            } else {
                break;
            }
        case operation::begin:
            it->rebind(static_cast<Type *>(const_cast<void *>(container))->begin());
            return true;
        case operation::cbegin:
            it->rebind(static_cast<const Type *>(container)->begin());
            return true;
        case operation::end:
            it->rebind(static_cast<Type *>(const_cast<void *>(container))->end());
            return true;
        case operation::cend:
            it->rebind(static_cast<const Type *>(container)->end());
            return true;
        case operation::insert:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                auto *const non_const = any_cast<typename Type::iterator>(&it->base());
                typename Type::const_iterator underlying{non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it->base())};

                // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
                if(auto &as_any = *static_cast<meta_any *>(const_cast<void *>(value)); as_any.allow_cast<typename Type::const_reference>() || as_any.allow_cast<typename Type::value_type>()) {
                    const auto *element = as_any.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                    it->rebind(static_cast<Type *>(const_cast<void *>(container))->insert(underlying, element ? *element : as_any.cast<typename Type::value_type>()));
                    return true;
                }
            }

            break;
        case operation::erase:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                auto *const non_const = any_cast<typename Type::iterator>(&it->base());
                typename Type::const_iterator underlying{non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it->base())};
                it->rebind(static_cast<Type *>(const_cast<void *>(container))->erase(underlying));
                return true;
            } else {
                break;
            }
        }

        return false;
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

    using operation = internal::meta_associative_container_operation;
    using size_type = typename meta_associative_container::size_type;
    using iterator = typename meta_associative_container::iterator;

    static size_type basic_vtable(const operation op, const void *cvalue, void *value, const void *key, iterator *it) {
        switch(op) {
        case operation::size:
            return static_cast<const Type *>(cvalue)->size();
        case operation::clear:
            static_cast<Type *>(value)->clear();
            return true;
        case operation::reserve:
            if constexpr(internal::reserve_aware_container<Type>::value) {
                static_cast<Type *>(value)->reserve(*static_cast<const size_type *>(cvalue));
                return true;
            } else {
                break;
            }
        case operation::begin:
            if(value) {
                it->rebind<key_only>(static_cast<Type *>(value)->begin());
            } else {
                it->rebind<key_only>(static_cast<const Type *>(cvalue)->begin());
            }

            return true;
        case operation::end:
            if(value) {
                it->rebind<key_only>(static_cast<Type *>(value)->end());
            } else {
                it->rebind<key_only>(static_cast<const Type *>(cvalue)->end());
            }

            return true;
        case operation::insert:
            if constexpr(key_only) {
                return static_cast<Type *>(value)->insert(*static_cast<const typename Type::key_type *>(key)).second;
            } else {
                return static_cast<Type *>(value)->emplace(*static_cast<const typename Type::key_type *>(key), *static_cast<const typename Type::mapped_type *>(cvalue)).second;
            }
        case operation::erase:
            return static_cast<Type *>(value)->erase(*static_cast<const typename Type::key_type *>(key));
        case operation::find:
            if(value) {
                it->rebind<key_only>(static_cast<Type *>(value)->find(*static_cast<const typename Type::key_type *>(key)));
            } else {
                it->rebind<key_only>(static_cast<const Type *>(cvalue)->find(*static_cast<const typename Type::key_type *>(key)));
            }

            return true;
        }

        return false;
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
