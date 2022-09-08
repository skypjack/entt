#ifndef ENTT_ENTITY_HANDLE_HPP
#define ENTT_ENTITY_HANDLE_HPP

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../core/iterator.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

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

    constexpr handle_storage_iterator() noexcept
        : entt{null},
          it{},
          last{} {}

    constexpr handle_storage_iterator(entity_type value, It from, It to) noexcept
        : entt{value},
          it{from},
          last{to} {
        while(it != last && !it->second.contains(entt)) { ++it; }
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

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Non-owning handle to an entity.
 *
 * Tiny wrapper around a registry and an entity.
 *
 * @tparam Registry Basic registry type.
 * @tparam Scope Types to which to restrict the scope of a handle.
 */
template<typename Registry, typename... Scope>
struct basic_handle {
    /*! @brief Type of registry accepted by the handle. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename registry_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename registry_type::size_type;

    /*! @brief Constructs an invalid handle. */
    basic_handle() noexcept
        : reg{},
          entt{null} {}

    /**
     * @brief Constructs a handle from a given registry and entity.
     * @param ref An instance of the registry class.
     * @param value A valid identifier.
     */
    basic_handle(registry_type &ref, entity_type value) noexcept
        : reg{&ref},
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
    [[nodiscard]] auto storage() const noexcept {
        auto iterable = reg->storage();
        using iterator_type = internal::handle_storage_iterator<typename decltype(iterable)::iterator>;
        return iterable_adaptor{iterator_type{entt, iterable.begin(), iterable.end()}, iterator_type{entt, iterable.end(), iterable.end()}};
    }

    /**
     * @brief Constructs a const handle from a non-const one.
     * @tparam Other A valid entity type (see entt_traits for more details).
     * @tparam Args Scope of the handle to construct.
     * @return A const handle referring to the same registry and the same
     * entity.
     */
    template<typename Other, typename... Args>
    operator basic_handle<Other, Args...>() const noexcept {
        static_assert(std::is_same_v<Other, Registry> || std::is_same_v<std::remove_const_t<Other>, Registry>, "Invalid conversion between different handles");
        static_assert((sizeof...(Scope) == 0 || ((sizeof...(Args) != 0 && sizeof...(Args) <= sizeof...(Scope)) && ... && (type_list_contains_v<type_list<Scope...>, Args>))), "Invalid conversion between different handles");

        return reg ? basic_handle<Other, Args...>{*reg, entt} : basic_handle<Other, Args...>{};
    }

    /**
     * @brief Converts a handle to its underlying entity.
     * @return The contained identifier.
     */
    [[nodiscard]] operator entity_type() const noexcept {
        return entity();
    }

    /**
     * @brief Checks if a handle refers to non-null registry pointer and entity.
     * @return True if the handle refers to non-null registry and entity, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return reg && reg->valid(entt);
    }

    /**
     * @brief Checks if a handle refers to a valid entity or not.
     * @return True if the handle refers to a valid entity, false otherwise.
     */
    [[nodiscard]] bool valid() const {
        return reg->valid(entt);
    }

    /**
     * @brief Returns a pointer to the underlying registry, if any.
     * @return A pointer to the underlying registry, if any.
     */
    [[nodiscard]] registry_type *registry() const noexcept {
        return reg;
    }

    /**
     * @brief Returns the entity associated with a handle.
     * @return The entity associated with the handle.
     */
    [[nodiscard]] entity_type entity() const noexcept {
        return entt;
    }

    /*! @brief Destroys the entity associated with a handle. */
    void destroy() {
        reg->destroy(entt);
    }

    /**
     * @brief Destroys the entity associated with a handle.
     * @param version A desired version upon destruction.
     */
    void destroy(const version_type version) {
        reg->destroy(entt, version);
    }

    /**
     * @brief Assigns the given component to a handle.
     * @tparam Component Type of component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Component, Scope>), "Invalid type");
        return reg->template emplace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns or replaces the given component for a handle.
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace_or_replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Component, Scope>), "Invalid type");
        return reg->template emplace_or_replace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given component for a handle.
     * @tparam Component Type of component to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param func Valid function objects.
     * @return A reference to the patched component.
     */
    template<typename Component, typename... Func>
    decltype(auto) patch(Func &&...func) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Component, Scope>), "Invalid type");
        return reg->template patch<Component>(entt, std::forward<Func>(func)...);
    }

    /**
     * @brief Replaces the given component for a handle.
     * @tparam Component Type of component to replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the component being replaced.
     */
    template<typename Component, typename... Args>
    decltype(auto) replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Component, Scope>), "Invalid type");
        return reg->template replace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given components from a handle.
     * @tparam Component Types of components to remove.
     * @return The number of components actually removed.
     */
    template<typename... Component>
    size_type remove() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Component> && ...), "Invalid type");
        return reg->template remove<Component...>(entt);
    }

    /**
     * @brief Erases the given components from a handle.
     * @tparam Component Types of components to erase.
     */
    template<typename... Component>
    void erase() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Component> && ...), "Invalid type");
        reg->template erase<Component...>(entt);
    }

    /**
     * @brief Checks if a handle has all the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the handle has all the components, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) all_of() const {
        return reg->template all_of<Component...>(entt);
    }

    /**
     * @brief Checks if a handle has at least one of the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the handle has at least one of the given components,
     * false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) any_of() const {
        return reg->template any_of<Component...>(entt);
    }

    /**
     * @brief Returns references to the given components for a handle.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the handle.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Component> && ...), "Invalid type");
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the given component for a handle.
     * @tparam Component Type of component to get.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return Reference to the component owned by the handle.
     */
    template<typename Component, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Component, Scope>), "Invalid type");
        return reg->template get_or_emplace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given components for a handle.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the handle.
     */
    template<typename... Component>
    [[nodiscard]] auto try_get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Component> && ...), "Invalid type");
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Checks if a handle has components assigned.
     * @return True if the handle has no components assigned, false otherwise.
     */
    [[nodiscard]] bool orphan() const {
        return reg->orphan(entt);
    }

private:
    registry_type *reg;
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

} // namespace entt

#endif
