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
struct is_dynamic_sequence_container: std::false_type {};

template<typename Type>
struct is_dynamic_sequence_container<Type, std::void_t<decltype(&Type::clear)>>: std::true_type {};

template<typename, typename = void>
struct is_key_only_meta_associative_container: std::true_type {};

template<typename Type>
struct is_key_only_meta_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename Type>
struct basic_meta_sequence_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    using iterator = meta_sequence_container::iterator;
    using size_type = std::size_t;

    [[nodiscard]] static size_type size(const void *container) noexcept {
        return static_cast<const Type *>(container)->size();
    }

    [[nodiscard]] static bool resize([[maybe_unused]] void *container, [[maybe_unused]] size_type sz) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            static_cast<Type *>(container)->resize(sz);
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] static iterator iter(const meta_ctx &ctx, void *container, const void *as_const, const bool as_end) {
        if(auto *const cont = static_cast<Type *>(container); cont) {
            return iterator{ctx, as_end ? cont->end() : cont->begin()};
        }

        auto *const cont = static_cast<const Type *>(as_const);
        return iterator{ctx, as_end ? cont->end() : cont->begin()};
    }

    [[nodiscard]] static iterator insert_or_erase([[maybe_unused]] const meta_ctx &ctx, [[maybe_unused]] void *container, [[maybe_unused]] const any &handle, [[maybe_unused]] meta_any &value) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            typename Type::const_iterator it{};

            if(auto *const non_const = any_cast<typename Type::iterator>(&handle); non_const) {
                it = *non_const;
            } else {
                it = any_cast<const typename Type::const_iterator &>(handle);
            }

            if(auto *const cont = static_cast<Type *>(container); value) {
                // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
                if(value.allow_cast<typename Type::const_reference>() || value.allow_cast<typename Type::value_type>()) {
                    const auto *element = value.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                    return iterator{ctx, cont->insert(it, element ? *element : value.cast<typename Type::value_type>())};
                }
            } else {
                return iterator{ctx, cont->erase(it)};
            }
        }

        return iterator{};
    }
};

template<typename Type>
struct basic_meta_associative_container_traits {
    static_assert(std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>, "Unexpected type");

    using iterator = meta_associative_container::iterator;
    using size_type = std::size_t;

    static constexpr auto key_only = is_key_only_meta_associative_container<Type>::value;

    [[nodiscard]] static size_type size(const void *container) noexcept {
        return static_cast<const Type *>(container)->size();
    }

    [[nodiscard]] static bool clear(void *container) {
        static_cast<Type *>(container)->clear();
        return true;
    }

    [[nodiscard]] static iterator iter(const meta_ctx &ctx, void *container, const void *as_const, const bool as_end) {
        if(auto *const cont = static_cast<Type *>(container); cont) {
            return iterator{ctx, std::bool_constant<key_only>{}, as_end ? cont->end() : cont->begin()};
        }

        auto *const cont = static_cast<const Type *>(as_const);
        return iterator{ctx, std::bool_constant<key_only>{}, as_end ? cont->end() : cont->begin()};
    }

    [[nodiscard]] static size_type insert_or_erase(void *container, meta_any &key, meta_any &value) {
        if(auto *const cont = static_cast<Type *>(container); key.allow_cast<const typename Type::key_type &>()) {
            if(value) {
                if constexpr(key_only) {
                    return cont->insert(key.cast<const typename Type::key_type &>()).second;
                } else {
                    return value.allow_cast<const typename Type::mapped_type &>() && cont->emplace(key.cast<const typename Type::key_type &>(), value.cast<const typename Type::mapped_type &>()).second;
                }
            } else {
                return cont->erase(key.cast<const typename Type::key_type &>());
            }
        }

        return 0u;
    }

    [[nodiscard]] static iterator find(const meta_ctx &ctx, void *container, const void *as_const, meta_any &key) {
        if(key.allow_cast<const typename Type::key_type &>()) {
            if(auto *const cont = static_cast<Type *>(container); cont) {
                return iterator{ctx, std::bool_constant<key_only>{}, cont->find(key.cast<const typename Type::key_type &>())};
            }

            return iterator{ctx, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->find(key.cast<const typename Type::key_type &>())};
        }

        return iterator{};
    }
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::vector<Args...>>
    : internal::basic_meta_sequence_container_traits<std::vector<Args...>> {};

/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type Template arguments for the container.
 * @tparam N Template arguments for the container.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>
    : internal::basic_meta_sequence_container_traits<std::array<Type, N>> {};

/**
 * @brief Meta sequence container traits for `std::list`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::list<Args...>>
    : internal::basic_meta_sequence_container_traits<std::list<Args...>> {};

/**
 * @brief Meta sequence container traits for `std::deque`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_sequence_container_traits<std::deque<Args...>>
    : internal::basic_meta_sequence_container_traits<std::deque<Args...>> {};

/**
 * @brief Meta associative container traits for `std::map`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::map<Args...>>
    : internal::basic_meta_associative_container_traits<std::map<Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_map`s of any
 * type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::unordered_map<Args...>>
    : internal::basic_meta_associative_container_traits<std::unordered_map<Args...>> {};

/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::set<Args...>>
    : internal::basic_meta_associative_container_traits<std::set<Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<std::unordered_set<Args...>>
    : internal::basic_meta_associative_container_traits<std::unordered_set<Args...>> {};

/**
 * @brief Meta associative container traits for `dense_map`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<dense_map<Args...>>
    : internal::basic_meta_associative_container_traits<dense_map<Args...>> {};

/**
 * @brief Meta associative container traits for `dense_set`s of any type.
 * @tparam Args Template arguments for the container.
 */
template<typename... Args>
struct meta_associative_container_traits<dense_set<Args...>>
    : internal::basic_meta_associative_container_traits<dense_set<Args...>> {};

} // namespace entt

#endif
