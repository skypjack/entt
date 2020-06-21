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


/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits_t<std::vector<Type, Args...>> {
    /*! @brief Iterator type of the sequence container. */
    using iterator = typename std::vector<Type, Args...>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<Type, Args...>::size_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::vector<Type, Args...>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param vec The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::vector<Type, Args...> &vec) ENTT_NOEXCEPT {
        return vec.size();
    }

    /**
     * @brief Resizes a given container to contain a certain number of elements.
     * @param vec The container to resize.
     * @param sz The new size of the container.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool resize(std::vector<Type, Args...> &vec, size_type sz) ENTT_NOEXCEPT {
        return (vec.resize(sz), true);
    }

    /**
     * @brief Clears the content of a given container.
     * @param vec The container of which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(std::vector<Type, Args...> &vec) {
        return vec.clear(), true;
    }

    /**
     * @brief Returns an iterator to the first element of a given container.
     * @param vec The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::vector<Type, Args...> &vec) {
        return vec.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param vec The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::vector<Type, Args...> &vec) {
        return vec.end();
    }

    /**
     * @brief Inserts an element at a specified location of a given container.
     * @param vec The container in which to insert the element.
     * @param it Iterator before which the element will be inserted.
     * @param value Element value to insert.
     * @return A pair consisting of an iterator to the inserted element (in case
     * of success) and a bool denoting whether the insertion took place.
     */
    [[nodiscard]] static std::pair<iterator, bool> insert(std::vector<Type, Args...> &vec, iterator it, const Type &value) {
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
    [[nodiscard]] static std::pair<iterator, bool> erase(std::vector<Type, Args...> &vec, iterator it) {
        return { vec.erase(it), true };
    }

    /**
     * @brief Returns a reference to the element at a specified location of a
     * given container (no bounds checking is performed).
     * @param vec The container from which to get the element.
     * @param pos The position of the element to return.
     * @return A reference to the requested element.
     */
    [[nodiscard]] static Type & get(std::vector<Type, Args...> &vec, size_type pos) {
        return vec[pos];
    }
};


/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type The type of elements.
 * @tparam N The number of elements.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits_t<std::array<Type, N>> {
    /*! @brief Iterator type of the sequence container. */
    using iterator = typename std::array<Type, N>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::array<Type, N>::size_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::array<Type, N>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param arr The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::array<Type, N> &arr) ENTT_NOEXCEPT {
        return arr.size();
    }

    /**
     * @brief Does nothing.
     * @return False to indicate failure in all cases.
     */
    [[nodiscard]] static bool resize(const std::array<Type, N> &, size_type) ENTT_NOEXCEPT {
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
     * @brief Returns an iterator to the first element of a given container.
     * @param arr The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::array<Type, N> &arr) {
        return arr.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param arr The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::array<Type, N> &arr) {
        return arr.end();
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<iterator, bool> insert(const std::array<Type, N> &, iterator, const Type &) {
        return { {}, false };
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<iterator, bool> erase(const std::array<Type, N> &, iterator) {
        return { {}, false };
    }

    /**
     * @brief Returns a reference to the element at a specified location of a
     * given container (no bounds checking is performed).
     * @param arr The container from which to get the element.
     * @param pos The position of the element to return.
     * @return A reference to the requested element.
     */
    [[nodiscard]] static Type & get(std::array<Type, N> &arr, size_type pos) {
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
struct meta_associative_container_traits_t<std::map<Key, Value, Args...>> {
    /*! @brief Iterator type of the associative container. */
    using iterator = typename std::map<Key, Value, Args...>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::map<Key, Value, Args...>::size_type;
    /*! @brief Key type of the sequence container. */
    using key_type = typename std::map<Key, Value, Args...>::key_type;
    /*! @brief Mapped type of the sequence container. */
    using mapped_type = typename std::map<Key, Value, Args...>::mapped_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::map<Key, Value, Args...>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param map The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::map<Key, Value, Args...> &map) ENTT_NOEXCEPT {
        return map.size();
    }

    /**
     * @brief Clears the content of a given container.
     * @param map The container of which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(std::map<Key, Value, Args...> &map) {
        return map.clear(), true;
    }

    /**
     * @brief Returns an iterator to the first element of a given container.
     * @param map The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::map<Key, Value, Args...> &map) {
        return map.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param map The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::map<Key, Value, Args...> &map) {
        return map.end();
    }

    /**
     * @brief Inserts an element (a key/value pair) into a given container.
     * @param map The container in which to insert the element.
     * @param key The key of the element to insert.
     * @param value The value of the element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::map<Key, Value, Args...> &map, const key_type &key, const mapped_type &value) {
        return map.insert(std::make_pair(key, value)).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param map The container from which to remove the element.
     * @param key The key of the element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::map<Key, Value, Args...> &map, const key_type &key) {
        const auto sz = map.size();
        return map.erase(key) != sz;
    }

    /**
     * @brief Returns an iterator to the element with key equivalent to a given
     * one, if any.
     * @param map The container in which to search for the element.
     * @param key The key of the element to search.
     * @return An iterator to the element with the given key, if any.
     */
    [[nodiscard]] static iterator find(std::map<Key, Value, Args...> &map, const key_type &key) {
        return map.find(key);
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
struct meta_associative_container_traits_t<std::unordered_map<Key, Value, Args...>> {
    /*! @brief Iterator type of the associative container. */
    using iterator = typename std::unordered_map<Key, Value, Args...>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::unordered_map<Key, Value, Args...>::size_type;
    /*! @brief Key type of the sequence container. */
    using key_type = typename std::unordered_map<Key, Value, Args...>::key_type;
    /*! @brief Mapped type of the sequence container. */
    using mapped_type = typename std::unordered_map<Key, Value, Args...>::mapped_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::unordered_map<Key, Value, Args...>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param map The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::unordered_map<Key, Value, Args...> &map) ENTT_NOEXCEPT {
        return map.size();
    }

    /**
     * @brief Clears the content of a given container.
     * @param map The container of which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(std::unordered_map<Key, Value, Args...> &map) {
        return map.clear(), true;
    }

    /**
     * @brief Returns an iterator to the first element of a given container.
     * @param map The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::unordered_map<Key, Value, Args...> &map) {
        return map.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param map The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::unordered_map<Key, Value, Args...> &map) {
        return map.end();
    }

    /**
     * @brief Inserts an element (a key/value pair) into a given container.
     * @param map The container in which to insert the element.
     * @param key The key of the element to insert.
     * @param value The value of the element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::unordered_map<Key, Value, Args...> &map, const key_type &key, const mapped_type &value) {
        return map.insert(std::make_pair(key, value)).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param map The container from which to remove the element.
     * @param key The key of the element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::unordered_map<Key, Value, Args...> &map, const key_type &key) {
        const auto sz = map.size();
        return map.erase(key) != sz;
    }

    /**
     * @brief Returns an iterator to the element with key equivalent to a given
     * one, if any.
     * @param map The container in which to search for the element.
     * @param key The key of the element to search.
     * @return An iterator to the element with the given key, if any.
     */
    [[nodiscard]] static iterator find(std::unordered_map<Key, Value, Args...> &map, const key_type &key) {
        return map.find(key);
    }
};


/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits_t<std::set<Key, Args...>> {
    /*! @brief Iterator type of the associative container. */
    using iterator = typename std::set<Key, Args...>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::set<Key, Args...>::size_type;
    /*! @brief Key type of the sequence container. */
    using key_type = typename std::set<Key, Args...>::key_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::set<Key, Args...>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param set The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::set<Key, Args...> &set) ENTT_NOEXCEPT {
        return set.size();
    }

    /**
     * @brief Clears the content of a given container.
     * @param set The container of which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(std::set<Key, Args...> &set) {
        return set.clear(), true;
    }

    /**
     * @brief Returns an iterator to the first element of a given container.
     * @param set The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::set<Key, Args...> &set) {
        return set.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param set The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::set<Key, Args...> &set) {
        return set.end();
    }

    /**
     * @brief Inserts an element into a given container.
     * @param set The container in which to insert the element.
     * @param key The element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::set<Key, Args...> &set, const key_type &key) {
        return set.insert(key).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param set The container from which to remove the element.
     * @param key The element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::set<Key, Args...> &set, const key_type &key) {
        const auto sz = set.size();
        return set.erase(key) != sz;
    }

    /**
     * @brief Returns an iterator to a given element, if any.
     * @param set The container in which to search for the element.
     * @param key The element to search.
     * @return An iterator to the given element, if any.
     */
    [[nodiscard]] static iterator find(std::set<Key, Args...> &set, const key_type &key) {
        return set.find(key);
    }
};


/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits_t<std::unordered_set<Key, Args...>> {
    /*! @brief Iterator type of the associative container. */
    using iterator = typename std::unordered_set<Key, Args...>::iterator;
    /*! @brief Unsigned integer type. */
    using size_type = typename std::unordered_set<Key, Args...>::size_type;
    /*! @brief Key type of the sequence container. */
    using key_type = typename std::unordered_set<Key, Args...>::key_type;
    /*! @brief Value type of the sequence container. */
    using value_type = typename std::unordered_set<Key, Args...>::value_type;

    /**
     * @brief Returns the size of given a container.
     * @param set The container of which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static size_type size(const std::unordered_set<Key, Args...> &set) ENTT_NOEXCEPT {
        return set.size();
    }

    /**
     * @brief Clears the content of a given container.
     * @param set The container of which to clear the content.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool clear(std::unordered_set<Key, Args...> &set) {
        return set.clear(), true;
    }

    /**
     * @brief Returns an iterator to the first element of a given container.
     * @param set The container of which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static iterator begin(std::unordered_set<Key, Args...> &set) {
        return set.begin();
    }

    /**
     * @brief Returns an iterator past the last element of a given container.
     * @param set The container of which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static iterator end(std::unordered_set<Key, Args...> &set) {
        return set.end();
    }

    /**
     * @brief Inserts an element into a given container.
     * @param set The container in which to insert the element.
     * @param key The element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert(std::unordered_set<Key, Args...> &set, const key_type &key) {
        return set.insert(key).second;
    }

    /**
     * @brief Removes the specified element from a given container.
     * @param set The container from which to remove the element.
     * @param key The element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase(std::unordered_set<Key, Args...> &set, const key_type &key) {
        const auto sz = set.size();
        return set.erase(key) != sz;
    }

    /**
     * @brief Returns an iterator to a given element, if any.
     * @param set The container in which to search for the element.
     * @param key The element to search.
     * @return An iterator to the given element, if any.
     */
    [[nodiscard]] static iterator find(std::unordered_set<Key, Args...> &set, const key_type &key) {
        return set.find(key);
    }
};


}


#endif
