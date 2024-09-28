#ifndef ENTT_ENTITY_HANDLE_HPP
#define ENTT_ENTITY_HANDLE_HPP

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename It>
class handle_storage_iterator final {
    template<typename Other>
    friend class handle_storage_iterator;

    using underlying_type = std::remove_reference_t<typename It::value_type::second_type>;
    using entity_type = typename underlying_type::entity_type;

public:
    using value_type = typename std::iterator_traits<It>::value_type;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr handle_storage_iterator() noexcept
        : entt{null},
          it{},
          last{} {}

    constexpr handle_storage_iterator(entity_type value, It from, It to) noexcept
        : entt{value},
          it{from},
          last{to} {
        while(it != last && !it->second.contains(entt)) {
            ++it;
        }
    }

    constexpr handle_storage_iterator &operator++() noexcept {
        while(++it != last && !it->second.contains(entt)) {}
        return *this;
    }

    constexpr handle_storage_iterator operator++(int) noexcept {
        handle_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *it;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    template<typename ILhs, typename IRhs>
    friend constexpr bool operator==(const handle_storage_iterator<ILhs> &, const handle_storage_iterator<IRhs> &) noexcept;

private:
    entity_type entt;
    It it;
    It last;
};

template<typename ILhs, typename IRhs>
[[nodiscard]] constexpr bool operator==(const handle_storage_iterator<ILhs> &lhs, const handle_storage_iterator<IRhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename ILhs, typename IRhs>
[[nodiscard]] constexpr bool operator!=(const handle_storage_iterator<ILhs> &lhs, const handle_storage_iterator<IRhs> &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Non-owning handle to an entity.
 *
 * Tiny wrapper around a registry and an entity.
 *
 * @tparam Registry Basic registry type.
 * @tparam Scope Types to which to restrict the scope of a handle.
 */
template<typename Registry, typename... Scope>
class basic_handle {
    using traits_type = entt_traits<typename Registry::entity_type>;

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<Registry &>(*owner);
    }

public:
    /*! @brief Type of registry accepted by the handle. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::value_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Iterable handle type. */
    using iterable = iterable_adaptor<internal::handle_storage_iterator<typename decltype(std::declval<registry_type>().storage())::iterator>>;

    /*! @brief Constructs an invalid handle. */
    basic_handle() noexcept
        : owner{},
          entt{null} {}

    /**
     * @brief Constructs a handle from a given registry and entity.
     * @param ref An instance of the registry class.
     * @param value A valid identifier.
     */
    basic_handle(registry_type &ref, entity_type value) noexcept
        : owner{&ref},
          entt{value} {}

    /**
     * @brief Returns an iterable object to use to _visit_ a handle.
     *
     * The iterable object returns a pair that contains the name and a reference
     * to the current storage.<br/>
     * Returned storage are those that contain the entity associated with the
     * handle.
     *
     * @return An iterable object to use to _visit_ the handle.
     */
    [[nodiscard]] iterable storage() const noexcept {
        auto underlying = owner_or_assert().storage();
        return iterable{{entt, underlying.begin(), underlying.end()}, {entt, underlying.end(), underlying.end()}};
    }

    /*! @copydoc valid */
    [[nodiscard]] explicit operator bool() const noexcept {
        return owner && owner->valid(entt);
    }

    /**
     * @brief Checks if a handle refers to a valid registry and entity.
     * @return True if the handle refers to a valid registry and entity, false
     * otherwise.
     */
    [[nodiscard]] bool valid() const {
        return static_cast<bool>(*this);
    }

    /**
     * @brief Returns a pointer to the underlying registry, if any.
     * @return A pointer to the underlying registry, if any.
     */
    [[nodiscard]] registry_type *registry() const noexcept {
        return owner;
    }

    /**
     * @brief Returns the entity associated with a handle.
     * @return The entity associated with the handle.
     */
    [[nodiscard]] entity_type entity() const noexcept {
        return entt;
    }

    /*! @copydoc entity */
    [[nodiscard]] operator entity_type() const noexcept {
        return entity();
    }

    /*! @brief Destroys the entity associated with a handle. */
    void destroy() {
        owner_or_assert().destroy(std::exchange(entt, null));
    }

    /**
     * @brief Destroys the entity associated with a handle.
     * @param version A desired version upon destruction.
     */
    void destroy(const version_type version) {
        owner_or_assert().destroy(std::exchange(entt, null), version);
    }

    /**
     * @brief Assigns the given element to a handle.
     * @tparam Type Type of element to create.
     * @tparam Args Types of arguments to use to construct the element.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the newly created element.
     */
    template<typename Type, typename... Args>
    // NOLINTNEXTLINE(modernize-use-nodiscard)
    decltype(auto) emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template emplace<Type>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns or replaces the given element for a handle.
     * @tparam Type Type of element to assign or replace.
     * @tparam Args Types of arguments to use to construct the element.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the newly created element.
     */
    template<typename Type, typename... Args>
    decltype(auto) emplace_or_replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template emplace_or_replace<Type>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given element for a handle.
     * @tparam Type Type of element to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param func Valid function objects.
     * @return A reference to the patched element.
     */
    template<typename Type, typename... Func>
    decltype(auto) patch(Func &&...func) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template patch<Type>(entt, std::forward<Func>(func)...);
    }

    /**
     * @brief Replaces the given element for a handle.
     * @tparam Type Type of element to replace.
     * @tparam Args Types of arguments to use to construct the element.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the element being replaced.
     */
    template<typename Type, typename... Args>
    decltype(auto) replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template replace<Type>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given elements from a handle.
     * @tparam Type Types of elements to remove.
     * @return The number of elements actually removed.
     */
    template<typename... Type>
    // NOLINTNEXTLINE(modernize-use-nodiscard)
    size_type remove() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template remove<Type...>(entt);
    }

    /**
     * @brief Erases the given elements from a handle.
     * @tparam Type Types of elements to erase.
     */
    template<typename... Type>
    void erase() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        owner_or_assert().template erase<Type...>(entt);
    }

    /**
     * @brief Checks if a handle has all the given elements.
     * @tparam Type Elements for which to perform the check.
     * @return True if the handle has all the elements, false otherwise.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) all_of() const {
        return owner_or_assert().template all_of<Type...>(entt);
    }

    /**
     * @brief Checks if a handle has at least one of the given elements.
     * @tparam Type Elements for which to perform the check.
     * @return True if the handle has at least one of the given elements,
     * false otherwise.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) any_of() const {
        return owner_or_assert().template any_of<Type...>(entt);
    }

    /**
     * @brief Returns references to the given elements for a handle.
     * @tparam Type Types of elements to get.
     * @return References to the elements owned by the handle.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template get<Type...>(entt);
    }

    /**
     * @brief Returns a reference to the given element for a handle.
     * @tparam Type Type of element to get.
     * @tparam Args Types of arguments to use to construct the element.
     * @param args Parameters to use to initialize the element.
     * @return Reference to the element owned by the handle.
     */
    template<typename Type, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template get_or_emplace<Type>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given elements for a handle.
     * @tparam Type Types of elements to get.
     * @return Pointers to the elements owned by the handle.
     */
    template<typename... Type>
    [[nodiscard]] auto try_get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template try_get<Type...>(entt);
    }

    /**
     * @brief Checks if a handle has elements assigned.
     * @return True if the handle has no elements assigned, false otherwise.
     */
    [[nodiscard]] bool orphan() const {
        return owner_or_assert().orphan(entt);
    }

    /**
     * @brief Returns a const handle from a non-const one.
     * @tparam Other A valid entity type.
     * @tparam Args Scope of the handle to construct.
     * @return A const handle referring to the same registry and the same
     * entity.
     */
    template<typename Other, typename... Args>
    operator basic_handle<Other, Args...>() const noexcept {
        static_assert(std::is_same_v<Other, Registry> || std::is_same_v<std::remove_const_t<Other>, Registry>, "Invalid conversion between different handles");
        static_assert((sizeof...(Scope) == 0 || ((sizeof...(Args) != 0 && sizeof...(Args) <= sizeof...(Scope)) && ... && (type_list_contains_v<type_list<Scope...>, Args>))), "Invalid conversion between different handles");
        return owner ? basic_handle<Other, Args...>{*owner, entt} : basic_handle<Other, Args...>{};
    }

private:
    registry_type *owner;
    entity_type entt;
};

/**
 * @brief Compares two handles.
 * @tparam Args Scope of the first handle.
 * @tparam Other Scope of the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return True if both handles refer to the same registry and the same
 * entity, false otherwise.
 */
template<typename... Args, typename... Other>
[[nodiscard]] bool operator==(const basic_handle<Args...> &lhs, const basic_handle<Other...> &rhs) noexcept {
    return lhs.registry() == rhs.registry() && lhs.entity() == rhs.entity();
}

/**
 * @brief Compares two handles.
 * @tparam Args Scope of the first handle.
 * @tparam Other Scope of the second handle.
 * @param lhs A valid handle.
 * @param rhs A valid handle.
 * @return False if both handles refer to the same registry and the same
 * entity, true otherwise.
 */
template<typename... Args, typename... Other>
[[nodiscard]] bool operator!=(const basic_handle<Args...> &lhs, const basic_handle<Other...> &rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief Compares a handle with the null object.
 * @tparam Args Scope of the handle.
 * @param lhs A valid handle.
 * @param rhs A null object yet to be converted.
 * @return False if the two elements differ, true otherwise.
 */
template<typename... Args>
[[nodiscard]] constexpr bool operator==(const basic_handle<Args...> &lhs, const null_t rhs) noexcept {
    return (lhs.entity() == rhs);
}

/**
 * @brief Compares a handle with the null object.
 * @tparam Args Scope of the handle.
 * @param lhs A null object yet to be converted.
 * @param rhs A valid handle.
 * @return False if the two elements differ, true otherwise.
 */
template<typename... Args>
[[nodiscard]] constexpr bool operator==(const null_t lhs, const basic_handle<Args...> &rhs) noexcept {
    return (rhs == lhs);
}

/**
 * @brief Compares a handle with the null object.
 * @tparam Args Scope of the handle.
 * @param lhs A valid handle.
 * @param rhs A null object yet to be converted.
 * @return True if the two elements differ, false otherwise.
 */
template<typename... Args>
[[nodiscard]] constexpr bool operator!=(const basic_handle<Args...> &lhs, const null_t rhs) noexcept {
    return (lhs.entity() != rhs);
}

/**
 * @brief Compares a handle with the null object.
 * @tparam Args Scope of the handle.
 * @param lhs A null object yet to be converted.
 * @param rhs A valid handle.
 * @return True if the two elements differ, false otherwise.
 */
template<typename... Args>
[[nodiscard]] constexpr bool operator!=(const null_t lhs, const basic_handle<Args...> &rhs) noexcept {
    return (rhs != lhs);
}

} // namespace entt

#endif
