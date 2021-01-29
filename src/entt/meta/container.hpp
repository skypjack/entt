#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP


#include <array>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "type_traits.hpp"


namespace entt {


/**
 * @brief Container traits.
 * @tparam Container Type of the underlying container.
 * @tparam Trait Traits associated with the underlying container.
 */
template<typename Container, template<typename> class... Trait>
struct meta_container_traits: public Trait<Container>... {
    /*! @brief Type of container. */
    using type = Container;
};


/**
 * @brief Basic STL-compatible container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_container {
    /**
     * @brief Returns the size of the given container.
     * @param cont The container for which to return the size.
     * @return The size of the given container.
     */
    [[nodiscard]] static typename Container::size_type size(const Container &cont) ENTT_NOEXCEPT {
        return cont.size();
    }

    /**
     * @brief Returns an iterator to the first element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static typename Container::iterator begin(Container &cont) {
        return cont.begin();
    }

    /**
     * @brief Returns an iterator to the first element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator to the first element of the given container.
     */
    [[nodiscard]] static typename Container::const_iterator cbegin(const Container &cont) {
        return cont.begin();
    }

    /**
     * @brief Returns an iterator past the last element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static typename Container::iterator end(Container &cont) {
        return cont.end();
    }

    /**
     * @brief Returns an iterator past the last element of the given container.
     * @param cont The container for which to return the iterator.
     * @return An iterator past the last element of the given container.
     */
    [[nodiscard]] static typename Container::const_iterator cend(const Container &cont) {
        return cont.end();
    }
};


/**
 * @brief Basic STL-compatible associative container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_associative_container {
    /**
     * @brief Returns an iterator to the element with key equivalent to the
     * given one, if any.
     * @param cont The container in which to search for the element.
     * @param key The key of the element to search.
     * @return An iterator to the element with the given key, if any.
     */
    [[nodiscard]] static typename Container::iterator find(Container &cont, const typename Container::key_type &key) {
        return cont.find(key);
    }

    /*! @copydoc find */
    [[nodiscard]] static typename Container::const_iterator cfind(const Container &cont, const typename Container::key_type &key) {
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
    [[nodiscard]] static bool clear([[maybe_unused]] Container &cont) {
        return cont.clear(), true;
    }
};


/**
 * @brief Basic STL-compatible dynamic associative container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_dynamic_associative_container {
    /**
     * @brief Removes the specified element from the given container.
     * @param cont The container from which to remove the element.
     * @param key The element to remove.
     * @return A bool denoting whether the removal took place.
     */
    [[nodiscard]] static bool erase([[maybe_unused]] Container &cont, [[maybe_unused]] const typename Container::key_type &key) {
        const auto sz = cont.size();
        return cont.erase(key) != sz;
    }
};


/**
 * @brief Basic STL-compatible sequence container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct basic_sequence_container {
    /**
     * @brief Returns a reference to the element at the specified location of
     * the given container (no bounds checking is performed).
     * @param cont The container from which to get the element.
     * @param pos The position of the element to return.
     * @return A reference to the requested element.
     */
    [[nodiscard]] static typename Container::reference get(Container &cont, typename Container::size_type pos) {
        return cont[pos];
    }

    /*! @copydoc get */
    [[nodiscard]] static typename Container::const_reference cget(const Container &cont, typename Container::size_type pos) {
        return cont[pos];
    }
};


/**
 * @brief STL-compatible dynamic associative key-only container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct dynamic_associative_key_only_container {
    /**
     * @brief Inserts an element into the given container.
     * @param cont The container in which to insert the element.
     * @param key The element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert([[maybe_unused]] Container &cont, [[maybe_unused]] const typename Container::key_type &key) {
        return cont.insert(key).second;
    }
};


/**
 * @brief STL-compatible dynamic key-value associative container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct dynamic_associative_key_value_container {
    /**
     * @brief Inserts an element (a key/value pair) into the given container.
     * @param cont The container in which to insert the element.
     * @param key The key of the element to insert.
     * @param value The value of the element to insert.
     * @return A bool denoting whether the insertion took place.
     */
    [[nodiscard]] static bool insert([[maybe_unused]] Container &cont, [[maybe_unused]] const typename Container::key_type &key, [[maybe_unused]] const typename Container::mapped_type &value) {
        return cont.insert(std::make_pair(key, value)).second;
    }
};


/**
 * @brief STL-compatible dynamic sequence container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct dynamic_sequence_container {
    /**
     * @brief Resizes the given container to contain the given number of
     * elements.
     * @param cont The container to resize.
     * @param sz The new size of the container.
     * @return True in case of success, false otherwise.
     */
    [[nodiscard]] static bool resize([[maybe_unused]] Container &cont, [[maybe_unused]] typename Container::size_type sz) {
        return cont.resize(sz), true;
    }

    /**
     * @brief Inserts an element at the specified location of the given
     * container.
     * @param cont The container into which to insert the element.
     * @param it Iterator before which the element will be inserted.
     * @param value Element value to insert.
     * @return A pair consisting of an iterator to the inserted element (in case
     * of success) and a bool denoting whether the insertion took place.
     */
    [[nodiscard]] static std::pair<typename Container::iterator, bool> insert([[maybe_unused]] Container &cont, [[maybe_unused]] typename Container::const_iterator it, [[maybe_unused]] const typename Container::value_type &value) {
        return { cont.insert(it, value), true };
    }

    /**
     * @brief Removes the element at the specified location from the given
     * container.
     * @param cont The container from which to remove the element.
     * @param it Iterator to the element to remove.
     * @return A pair consisting of an iterator following the last removed
     * element (in case of success) and a bool denoting whether the insertion
     * took place.
     */
    [[nodiscard]] static std::pair<typename Container::iterator, bool> erase([[maybe_unused]] Container &cont, [[maybe_unused]] typename Container::const_iterator it) {
        return { cont.erase(it), true };
    }
};


/**
 * @brief STL-compatible fixed sequence container traits
 * @tparam Container The type of the container.
 */
template<typename Container>
struct fixed_sequence_container {
    /**
     * @brief Does nothing.
     * @return False to indicate failure in all cases.
     */
    [[nodiscard]] static bool resize(const Container &, typename Container::size_type) {
        return false;
    }

    /**
     * @brief Does nothing.
     * @return False to indicate failure in all cases.
     */
    [[nodiscard]] static bool clear(const Container &) {
        return false;
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<typename Container::iterator, bool> insert(const Container &, typename Container::const_iterator, const typename Container::value_type &) {
        return { {}, false };
    }

    /**
     * @brief Does nothing.
     * @return A pair consisting of an invalid iterator and a false value to
     * indicate failure in all cases.
     */
    [[nodiscard]] static std::pair<typename Container::iterator, bool> erase(const Container &, typename Container::const_iterator) {
        return { {}, false };
    }
};


/**
 * @brief Meta sequence container traits for `std::vector`s of any type.
 * @tparam Type The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Type, typename... Args>
struct meta_sequence_container_traits<std::vector<Type, Args...>>
        : meta_container_traits<
              std::vector<Type, Args...>,
              basic_container,
              basic_dynamic_container,
              basic_sequence_container,
              dynamic_sequence_container
          >
{};


/**
 * @brief Meta sequence container traits for `std::array`s of any type.
 * @tparam Type The type of elements.
 * @tparam N The number of elements.
 */
template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>
        : meta_container_traits<
              std::array<Type, N>,
              basic_container,
              basic_sequence_container,
              fixed_sequence_container
          >
{};


/**
 * @brief Meta associative container traits for `std::map`s of any type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::map<Key, Value, Args...>>
        : meta_container_traits<
              std::map<Key, Value, Args...>,
              basic_container,
              basic_associative_container,
              basic_dynamic_container,
              basic_dynamic_associative_container,
              dynamic_associative_key_value_container
          >
{};


/**
 * @brief Meta associative container traits for `std::unordered_map`s of any
 * type.
 * @tparam Key The key type of elements.
 * @tparam Value The value type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename Value, typename... Args>
struct meta_associative_container_traits<std::unordered_map<Key, Value, Args...>>
        : meta_container_traits<
              std::unordered_map<Key, Value, Args...>,
              basic_container,
              basic_associative_container,
              basic_dynamic_container,
              basic_dynamic_associative_container,
              dynamic_associative_key_value_container
          >
{};


/**
 * @brief Meta associative container traits for `std::set`s of any type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::set<Key, Args...>>
        : meta_container_traits<
              std::set<Key, Args...>,
              basic_container,
              basic_associative_container,
              basic_dynamic_container,
              basic_dynamic_associative_container,
              dynamic_associative_key_only_container
          >
{};


/**
 * @brief Meta associative container traits for `std::unordered_set`s of any
 * type.
 * @tparam Key The type of elements.
 * @tparam Args Other arguments.
 */
template<typename Key, typename... Args>
struct meta_associative_container_traits<std::unordered_set<Key, Args...>>
        : meta_container_traits<
              std::unordered_set<Key, Args...>,
              basic_container,
              basic_associative_container,
              basic_dynamic_container,
              basic_dynamic_associative_container,
              dynamic_associative_key_only_container
          >
{};


}


#endif
