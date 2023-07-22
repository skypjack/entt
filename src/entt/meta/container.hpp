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
#include "context.hpp"
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

    using operation = internal::meta_container_operation;
    using size_type = typename meta_sequence_container::size_type;
    using iterator = typename meta_sequence_container::iterator;

    static size_type basic_vtable(const operation op, const meta_ctx &ctx, const void *cvalue, void *value, iterator *it) {
        switch(op) {
        case operation::size:
            return static_cast<const Type *>(cvalue)->size();
        case operation::clear:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                static_cast<Type *>(value)->clear();
                return true;
            } else {
                break;
            }
        case operation::reserve:
            if constexpr(internal::reserve_aware_container<Type>::value) {
                static_cast<Type *>(value)->reserve(*static_cast<const size_type *>(cvalue));
                return true;
            } else {
                break;
            }
        case operation::resize:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                static_cast<Type *>(value)->resize(*static_cast<const size_type *>(cvalue));
                return true;
            } else {
                break;
            }
        case operation::begin:
            if(value) {
                *it = iterator{ctx, static_cast<Type *>(value)->begin()};
            } else {
                *it = iterator{ctx, static_cast<const Type *>(cvalue)->begin()};
            }

            return true;
        case operation::end:
            if(value) {
                *it = iterator{ctx, static_cast<Type *>(value)->end()};
            } else {
                *it = iterator{ctx, static_cast<const Type *>(cvalue)->end()};
            }

            return true;
        case operation::insert:
        case operation::erase:
            if constexpr(internal::dynamic_sequence_container<Type>::value) {
                auto *const non_const = any_cast<typename Type::iterator>(&it->base());
                typename Type::const_iterator underlying{non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it->base())};

                if(op == operation::insert) {
                    // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
                    if(auto &any = *static_cast<meta_any *>(const_cast<void *>(cvalue)); any.allow_cast<typename Type::const_reference>() || any.allow_cast<typename Type::value_type>()) {
                        const auto *element = any.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                        *it = iterator{ctx, static_cast<Type *>(value)->insert(underlying, element ? *element : any.cast<typename Type::value_type>())};
                        return true;
                    }
                } else {
                    *it = iterator{ctx, static_cast<Type *>(value)->erase(underlying)};
                    return true;
                }
            }

            break;
        default:
            // nothing to do here
            break;
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

    using operation = internal::meta_container_operation;
    using size_type = typename meta_associative_container::size_type;
    using iterator = typename meta_associative_container::iterator;

    static size_type basic_vtable(const operation op, const meta_ctx &ctx, const void *cvalue, void *value, meta_any *key, iterator *it) {
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
                *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<Type *>(value)->begin()};
            } else {
                *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<const Type *>(cvalue)->begin()};
            }

            return true;
        case operation::end:
            if(value) {
                *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<Type *>(value)->end()};
            } else {
                *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<const Type *>(cvalue)->end()};
            }

            return true;
        case operation::insert:
            if(key->allow_cast<const typename Type::key_type &>()) {
                if constexpr(key_only) {
                    return static_cast<Type *>(value)->insert(key->cast<const typename Type::key_type &>()).second;
                } else {
                    auto &any = *static_cast<meta_any *>(const_cast<void *>(cvalue));
                    return any.allow_cast<const typename Type::mapped_type &>() && static_cast<Type *>(value)->emplace(key->cast<const typename Type::key_type &>(), any.cast<const typename Type::mapped_type &>()).second;
                }
            }

            break;
        case operation::erase:
            if(key->allow_cast<const typename Type::key_type &>()) {
                return static_cast<Type *>(value)->erase(key->cast<const typename Type::key_type &>());
            }

            break;
        case operation::find:
            if(key->allow_cast<const typename Type::key_type &>()) {
                if(value) {
                    *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<Type *>(value)->find(key->cast<const typename Type::key_type &>())};
                } else {
                    *it = iterator{ctx, std::bool_constant<key_only>{}, static_cast<const Type *>(cvalue)->find(key->cast<const typename Type::key_type &>())};
                }

                return true;
            }

            break;
        default:
            // nothing to do here
            break;
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
