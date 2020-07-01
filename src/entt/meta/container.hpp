#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP


#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "type_traits.hpp"


namespace entt {


namespace detail {


template<typename Container, template<typename> class... Trait>
struct trait_composition: public Trait<Container>... {};


/**
 * @brief Basic STL-compatible container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_container {
    /*! @brief Iterator type of the container. */
    using iterator = typename Container::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename Container::size_type;
    /*! @brief Value type of the container. */
    using value_type = typename Container::value_type;

    /**
     * @brief Returns the size of the given container.
     * @param cont The container for which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const Container &cont) ENTT_NOEXCEPT {
        return cont.size();
    }

    /**
     * @brief Returns an iterator to the first element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(Container &cont) {
        return cont.begin();
    }

    /**
     * @brief Returns an iterator past the last element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(Container &cont) {
        return cont.end();
    }
};


/**
 * @brief Basic STL-compatible associative container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_associative_container {
    /*! @brief Key type of the sequence container. */
    using key_type = typename Container::key_type;

    /**
     * @brief Returns an iterator to the element with key equivalent to the given
     * one, if any.
     * @param cont The container in which to search for the element.
     * @param key The key of the element to search.
     * @return An iterator to the element with the given key, if any.
     */
    [[nodiscard]] static typename Container::iterator find(Container &cont, const key_type &key) {
        return cont.find(key);
    }
};


/**
 * @brief Basic STL-compatible dynamic container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_dynamic_container {
    /**
     * @brief Clears the content of the given container.
     * @param cont The container for which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(Container &cont) {
        return cont.clear(), true;
    }
};


}


/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits<std::vector<Type, Args...>>
    : detail::trait_composition<
          std::vector<Type, Args...>,
          detail::basic_container,
          detail::basic_dynamic_container> {
    /**
     * @brief Resizes a given container to contain a certain number of elements.
     * @param vec The container to resize.
     * @param sz The new size of the container.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool resize(std::vector<Type, Args...> &vec, typename meta_sequence_container_traits::size_type sz) {
        return (vec.resize(sz), true);
    }

    /**
     * @brief Inserts an element at a specified location of a given container.
     * @param vec The container in which to insert the element.
     * @param it Iterator before which the element will be inserted.
     * @param value Element value to insert.
     * @return A pair consisting of an iterator to the inserted element (in case
     * of success) and a bool denoting whether the insertion took place.
     */
    [[nodiscard]] static std::pair<typename meta_sequence_container_traits::iterator, bool> insert(std::vector<Type, Args...> &vec, typename meta_sequence_container_traits::iterator it, const Type &value) {
        return { vec.insert(it, value), true };
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param vec The container from which to remove the element.
     * @param it Iterator to the element to remove.
     * @return A pair consisting of an iterator following the last removed
     * element (in case of success) and a bool denoting whether the insertion
     * took place.
     */
    [[nodiscard]] static std::pair<typename meta_sequence_container_traits::iterator, bool> erase(std::vector<Type, Args...> &vec, typename meta_sequence_container_traits::iterator it) {
        return { vec.erase(it), true };
    }

    /**
     * @brief Returns a reference to the element at a specified location of a
     * given container (no bounds checking is performed).
     * @param vec The container from which to get the element.
     * @param pos The position of the element to return.
     * @return A reference to the requested element.
     */
    [[nodiscard]] static Type & get(std::vector<Type, Args...> &vec, typename meta_sequence_container_traits::size_type pos) {
        return vec[pos];
    }
};


/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type The type of elements.
 * @tparam N The number of elements.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>: detail::basic_container<std::array<Type, N>> {
    /**
     * @brief Does nothing.
     * @return False to indicate failure in all cases.
     */
    [[nodiscard]] static bool resize(const std::array<Type, N> &, typename meta_sequence_container_traits::size_type) {
        return false;
    }

    /**
     * @brief Does nothing.
     * @return False to indicate failure in all cases.
     */
    [[nodiscard]] static bool clear(const std::array<Type, N> &) {
        return false;
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<typename meta_sequence_container_traits::iterator, bool> insert(const std::array<Type, N> &, typename meta_sequence_container_traits::iterator, const Type &) {
        return { {}, false };
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<typename meta_sequence_container_traits::iterator, bool> erase(const std::array<Type, N> &, typename meta_sequence_container_traits::iterator) {
        return { {}, false };
    }

    /**
     * @brief Returns a reference to the element at a specified location of a
     * given container (no bounds checking is performed).
     * @param arr The container from which to get the element.
     * @param pos The position of the element to return.
     * @return A reference to the requested element.
     */
    [[nodiscard]] static Type & get(std::array<Type, N> &arr, typename meta_sequence_container_traits::size_type pos) {
        return arr[pos];
    }
};


/**
 * @brief Meta associative container traits for `std::map`s of any type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::map<Key, Value, Args...>>
    : detail::trait_composition<
          std::map<Key, Value, Args...>,
          detail::basic_container,
          detail::basic_associative_container,
          detail::basic_dynamic_container> {
    /*! @brief Mapped type of the sequence container. */
    using mapped_type = typename std::map<Key, Value, Args...>::mapped_type;

    /**
     * @brief Inserts an element (a key/value pair) into a given container.
     * @param map The container in which to insert the element.
     * @param key The key of the element to insert.
     * @param value The value of the element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::map<Key, Value, Args...> &map, const typename meta_associative_container_traits::key_type &key, const mapped_type &value) {
        return map.insert(std::make_pair(key, value)).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param map The container from which to remove the element.
     * @param key The key of the element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::map<Key, Value, Args...> &map, const typename meta_associative_container_traits::key_type &key) {
        const auto sz = map.size();
        return map.erase(key) != sz;
    }
};


/**
 * @brief Meta associative container traits for `std::unordered_map`s of any
 * type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::unordered_map<Key, Value, Args...>>
    : detail::trait_composition<
          std::unordered_map<Key, Value, Args...>,
          detail::basic_container,
          detail::basic_associative_container,
          detail::basic_dynamic_container> {
    /*! @brief Mapped type of the sequence container. */
    using mapped_type = typename std::unordered_map<Key, Value, Args...>::mapped_type;

    /**
     * @brief Inserts an element (a key/value pair) into a given container.
     * @param map The container in which to insert the element.
     * @param key The key of the element to insert.
     * @param value The value of the element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::unordered_map<Key, Value, Args...> &map, const typename meta_associative_container_traits::key_type &key, const mapped_type &value) {
        return map.insert(std::make_pair(key, value)).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param map The container from which to remove the element.
     * @param key The key of the element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::unordered_map<Key, Value, Args...> &map, const typename meta_associative_container_traits::key_type &key) {
        const auto sz = map.size();
        return map.erase(key) != sz;
    }
};


/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::set<Key, Args...>>
    : detail::trait_composition<
          std::set<Key, Args...>,
          detail::basic_container,
          detail::basic_associative_container,
          detail::basic_dynamic_container> {
    /**
     * @brief Inserts an element into a given container.
     * @param set The container in which to insert the element.
     * @param key The element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::set<Key, Args...> &set, const typename meta_associative_container_traits::key_type &key) {
        return set.insert(key).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param set The container from which to remove the element.
     * @param key The element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::set<Key, Args...> &set, const typename meta_associative_container_traits::key_type &key) {
        const auto sz = set.size();
        return set.erase(key) != sz;
    }
};


/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::unordered_set<Key, Args...>>
    : detail::trait_composition<
          std::unordered_set<Key, Args...>,
          detail::basic_container,
          detail::basic_associative_container,
          detail::basic_dynamic_container> {
    /**
     * @brief Inserts an element into a given container.
     * @param set The container in which to insert the element.
     * @param key The element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::unordered_set<Key, Args...> &set, const typename meta_associative_container_traits::key_type &key) {
        return set.insert(key).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param set The container from which to remove the element.
     * @param key The element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::unordered_set<Key, Args...> &set, const typename meta_associative_container_traits::key_type &key) {
        const auto sz = set.size();
        return set.erase(key) != sz;
    }
};


}


#endif
