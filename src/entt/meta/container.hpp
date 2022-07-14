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
struct is_dynamic_sequence_container: std::false_type {};

template<typename Type>
struct is_dynamic_sequence_container<Type, std::void_t<decltype(&Type::clear)>>: std::true_type {};

template<typename, typename = void>
struct is_key_only_meta_associative_container: std::true_type {};

template<typename Type>
struct is_key_only_meta_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename Type>
struct basic_meta_sequence_container_traits {
    using iterator = meta_sequence_container::iterator;
    using size_type = std::size_t;

    [[nodiscard]] static size_type size(const any &container) noexcept {
        return any_cast<const Type &>(container).size();
    }

    [[nodiscard]] static bool resize([[maybe_unused]] any &container, [[maybe_unused]] size_type sz) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                cont->resize(sz);
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] static iterator iter(any &container, const bool as_end) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{as_end ? cont->end() : cont->begin()};
        }

        const Type &as_const = any_cast<const Type &>(container);
        return iterator{as_end ? as_const.end() : as_const.begin()};
    }

    [[nodiscard]] static iterator insert_or_erase([[maybe_unused]] any &container, [[maybe_unused]] const any &handle, [[maybe_unused]] meta_any &value) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                typename Type::const_iterator it{};

                if(auto *non_const = any_cast<typename Type::iterator>(&handle); non_const) {
                    it = *non_const;
                } else {
                    it = any_cast<const typename Type::const_iterator &>(handle);
                }

                if(value) {
                    // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
                    if(value.allow_cast<typename Type::const_reference>() || value.allow_cast<typename Type::value_type>()) {
                        const auto *element = value.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                        return iterator{cont->insert(it, element ? *element : value.cast<typename Type::value_type>())};
                    }
                } else {
                    return iterator{cont->erase(it)};
                }
            }
        }

        return {};
    }
};

template<typename Type>
struct basic_meta_associative_container_traits {
    using iterator = meta_associative_container::iterator;
    using size_type = std::size_t;

    static constexpr auto key_only = is_key_only_meta_associative_container<Type>::value;

    [[nodiscard]] static size_type size(const any &container) noexcept {
        return any_cast<const Type &>(container).size();
    }

    [[nodiscard]] static bool clear(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            cont->clear();
            return true;
        }

        return false;
    }

    [[nodiscard]] static iterator iter(any &container, const bool as_end) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{std::bool_constant<key_only>{}, as_end ? cont->end() : cont->begin()};
        }

        const auto &as_const = any_cast<const Type &>(container);
        return iterator{std::bool_constant<key_only>{}, as_end ? as_const.end() : as_const.begin()};
    }

    [[nodiscard]] static size_type insert_or_erase(any &container, meta_any &key, meta_any &value) {
        if(auto *const cont = any_cast<Type>(&container); cont && key.allow_cast<const typename Type::key_type &>()) {
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

    [[nodiscard]] static iterator find(any &container, meta_any &key) {
        if(key.allow_cast<const typename Type::key_type &>()) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                return iterator{std::bool_constant<key_only>{}, cont->find(key.cast<const typename Type::key_type &>())};
            }

            return iterator{std::bool_constant<key_only>{}, any_cast<const Type &>(container).find(key.cast<const typename Type::key_type &>())};
        }

        return {};
    }
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits<std::vector<Type, Args...>>
    : internal::basic_meta_sequence_container_traits<std::vector<Type, Args...>> {};

/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type The type of elements.
 * @tparam N The number of elements.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>
    : internal::basic_meta_sequence_container_traits<std::array<Type, N>> {};

/**
 * @brief Meta sequence container traits for `std::list`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits<std::list<Type, Args...>>
    : internal::basic_meta_sequence_container_traits<std::list<Type, Args...>> {};

/**
 * @brief Meta sequence container traits for `std::deque`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits<std::deque<Type, Args...>>
    : internal::basic_meta_sequence_container_traits<std::deque<Type, Args...>> {};

/**
 * @brief Meta associative container traits for `std::map`s of any type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::map<Key, Value, Args...>>
    : internal::basic_meta_associative_container_traits<std::map<Key, Value, Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_map`s of any
 * type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::unordered_map<Key, Value, Args...>>
    : internal::basic_meta_associative_container_traits<std::unordered_map<Key, Value, Args...>> {};

/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::set<Key, Args...>>
    : internal::basic_meta_associative_container_traits<std::set<Key, Args...>> {};

/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::unordered_set<Key, Args...>>
    : internal::basic_meta_associative_container_traits<std::unordered_set<Key, Args...>> {};

/**
 * @brief Meta associative container traits for `dense_map`s of any type.
 * @tparam Key The key type of the elements.
 * @tparam Type The value type of the elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Type, typename... Args>
struct meta_associative_container_traits<dense_map<Key, Type, Args...>>
    : internal::basic_meta_associative_container_traits<dense_map<Key, Type, Args...>> {};

/**
 * @brief Meta associative container traits for `dense_set`s of any type.
 * @tparam Type The value type of the elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_associative_container_traits<dense_set<Type, Args...>>
    : internal::basic_meta_associative_container_traits<dense_set<Type, Args...>> {};

} // namespace entt

#endif
