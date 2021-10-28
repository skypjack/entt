#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP

#include <array>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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
struct is_dynamic_sequence_container<Type, std::void_t<decltype(&Type::reserve)>>: std::true_type {};

template<typename, typename = void>
struct is_key_only_meta_associative_container: std::true_type {};

template<typename Type>
struct is_key_only_meta_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename Type>
struct basic_meta_sequence_container_traits {
    using iterator = meta_sequence_container::iterator;
    using size_type = std::size_t;

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
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

    [[nodiscard]] static bool clear([[maybe_unused]] any &container) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                cont->clear();
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{std::begin(*cont)};
        }

        return iterator{std::begin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{std::end(*cont)};
        }

        return iterator{std::end(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator insert([[maybe_unused]] any &container, [[maybe_unused]] iterator it, [[maybe_unused]] meta_any &value) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                // this abomination is necessary because only on macos value_type and const_reference are different types for std::vector<bool>
                if(value.allow_cast<typename Type::const_reference>() || value.allow_cast<typename Type::value_type>()) {
                    const auto *element = value.try_cast<std::remove_reference_t<typename Type::const_reference>>();
                    return iterator{cont->insert(any_cast<const typename Type::iterator &>(it.base()), element ? *element : value.cast<typename Type::value_type>())};
                }
            }
        }

        return {};
    }

    [[nodiscard]] static iterator erase([[maybe_unused]] any &container, [[maybe_unused]] iterator it) {
        if constexpr(is_dynamic_sequence_container<Type>::value) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                return iterator{cont->erase(any_cast<const typename Type::iterator &>(it.base()))};
            }
        }

        return {};
    }

    [[nodiscard]] static meta_any get(any &container, size_type pos) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return meta_any{std::in_place_type<typename Type::reference>, (*cont)[pos]};
        }

        return meta_any{std::in_place_type<typename Type::const_reference>, any_cast<const Type &>(container)[pos]};
    }
};

template<typename Type>
struct basic_meta_associative_container_traits {
    using iterator = meta_associative_container::iterator;
    using size_type = std::size_t;

    static constexpr auto key_only = is_key_only_meta_associative_container<Type>::value;

    [[nodiscard]] static size_type size(const any &container) ENTT_NOEXCEPT {
        return any_cast<const Type &>(container).size();
    }

    [[nodiscard]] static bool clear(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            cont->clear();
            return true;
        }

        return false;
    }

    [[nodiscard]] static iterator begin(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{std::integral_constant<bool, key_only>{}, cont->begin()};
        }

        return iterator{std::integral_constant<bool, key_only>{}, std::begin(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static iterator end(any &container) {
        if(auto *const cont = any_cast<Type>(&container); cont) {
            return iterator{std::integral_constant<bool, key_only>{}, cont->end()};
        }

        return iterator{std::integral_constant<bool, key_only>{}, std::end(any_cast<const Type &>(container))};
    }

    [[nodiscard]] static bool insert(any &container, meta_any &key, [[maybe_unused]] meta_any &value) {
        if(auto *const cont = any_cast<Type>(&container); cont && key.allow_cast<const typename Type::key_type &>()) {
            if constexpr(is_key_only_meta_associative_container<Type>::value) {
                return cont->insert(key.cast<const typename Type::key_type &>()).second;
            } else {
                if(value.allow_cast<const typename Type::mapped_type &>()) {
                    return cont->emplace(key.cast<const typename Type::key_type &>(), value.cast<const typename Type::mapped_type &>()).second;
                }
            }
        }

        return false;
    }

    [[nodiscard]] static bool erase(any &container, meta_any &key) {
        if(auto *const cont = any_cast<Type>(&container); cont && key.allow_cast<const typename Type::key_type &>()) {
            return (cont->erase(key.cast<const typename Type::key_type &>()) != cont->size());
        }

        return false;
    }

    [[nodiscard]] static iterator find(any &container, meta_any &key) {
        if(key.allow_cast<const typename Type::key_type &>()) {
            if(auto *const cont = any_cast<Type>(&container); cont) {
                return iterator{std::integral_constant<bool, key_only>{}, cont->find(key.cast<const typename Type::key_type &>())};
            }

            return iterator{std::integral_constant<bool, key_only>{}, any_cast<const Type &>(container).find(key.cast<const typename Type::key_type &>())};
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

} // namespace entt

#endif
