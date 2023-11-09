#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP

#include <memory>
#include <type_traits>
#include <utility>
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "../signal/delegate.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "storage.hpp"
#include "view.hpp"

namespace entt {

/**
 * @brief Converts a registry to a view.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class as_view {
    template<typename... Get, typename... Exclude>
    auto dispatch(get_t<Get...>, exclude_t<Exclude...>) const {
        return reg.template view<constness_as_t<typename Get::value_type, Get>...>(exclude_t<constness_as_t<typename Exclude::value_type, Exclude>...>{});
    }

public:
    /*! @brief Type of registry to convert. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_view(registry_type &source) noexcept
        : reg{source} {}

    /**
     * @brief Conversion function from a registry to a view.
     * @tparam Get Type of storage used to construct the view.
     * @tparam Exclude Types of storage used to filter the view.
     * @return A newly created view.
     */
    template<typename Get, typename Exclude>
    operator basic_view<Get, Exclude>() const {
        return dispatch(Get{}, Exclude{});
    }

private:
    registry_type &reg;
};

/**
 * @brief Converts a registry to a group.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class as_group {
    template<typename... Owned, typename... Get, typename... Exclude>
    auto dispatch(owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>) const {
        if constexpr(std::is_const_v<registry_type>) {
            return reg.template group_if_exists<typename Owned::value_type...>(get_t<typename Get::value_type...>{}, exclude_t<typename Exclude::value_type...>{});
        } else {
            return reg.template group<constness_as_t<typename Owned::value_type, Owned>...>(get_t<constness_as_t<typename Get::value_type, Get>...>{}, exclude_t<constness_as_t<typename Exclude::value_type, Exclude>...>{});
        }
    }

public:
    /*! @brief Type of registry to convert. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_group(registry_type &source) noexcept
        : reg{source} {}

    /**
     * @brief Conversion function from a registry to a group.
     * @tparam Owned Types of _owned_ by the group.
     * @tparam Get Types of storage _observed_ by the group.
     * @tparam Exclude Types of storage used to filter the group.
     * @return A newly created group.
     */
    template<typename Owned, typename Get, typename Exclude>
    operator basic_group<Owned, Get, Exclude>() const {
        return dispatch(Owned{}, Get{}, Exclude{});
    }

private:
    registry_type &reg;
};

/**
 * @brief Helper to create a listener that directly invokes a member function.
 * @tparam Member Member function to invoke on a component of the given type.
 * @tparam Registry Basic registry type.
 * @param reg A registry that contains the given entity and its components.
 * @param entt Entity from which to get the component.
 */
template<auto Member, typename Registry = std::decay_t<nth_argument_t<0u, decltype(Member)>>>
void invoke(Registry &reg, const typename Registry::entity_type entt) {
    static_assert(std::is_member_function_pointer_v<decltype(Member)>, "Invalid pointer to non-static member function");
    delegate<void(Registry &, const typename Registry::entity_type)> func;
    func.template connect<Member>(reg.template get<member_class_t<decltype(Member)>>(entt));
    func(reg, entt);
}

/**
 * @brief Returns the entity associated with a given component.
 *
 * @warning
 * Currently, this function only works correctly with the default storage as it
 * makes assumptions about how the components are laid out.
 *
 * @tparam Args Storage type template parameters.
 * @param storage A storage that contains the given component.
 * @param instance A valid component instance.
 * @return The entity associated with the given component.
 */
template<typename... Args>
auto to_entity(const basic_storage<Args...> &storage, const typename basic_storage<Args...>::value_type &instance) -> typename basic_storage<Args...>::entity_type {
    constexpr auto page_size = basic_storage<Args...>::traits_type::page_size;
    const typename basic_storage<Args...>::base_type &base = storage;
    const auto *addr = std::addressof(instance);

    for(auto it = base.rbegin(), last = base.rend(); it < last; it += page_size) {
        if(const auto dist = (addr - std::addressof(storage.get(*it))); dist >= 0 && dist < static_cast<decltype(dist)>(page_size)) {
            return *(it + dist);
        }
    }

    return null;
}

/**
 * @copybrief to_entity
 * @tparam Args Registry type template parameters.
 * @tparam Component Type of component.
 * @param reg A registry that contains the given entity and its components.
 * @param instance A valid component instance.
 * @return The entity associated with the given component.
 */
template<typename... Args, typename Component>
[[deprecated("use storage based to_entity instead")]] typename basic_registry<Args...>::entity_type to_entity(const basic_registry<Args...> &reg, const Component &instance) {
    if(const auto *storage = reg.template storage<Component>(); storage) {
        return to_entity(*storage, instance);
    }

    return null;
}

/*! @brief Primary template isn't defined on purpose. */
template<typename...>
struct sigh_helper;

/**
 * @brief Signal connection helper for registries.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
struct sigh_helper<Registry> {
    /*! @brief Registry type. */
    using registry_type = Registry;

    /**
     * @brief Constructs a helper for a given registry.
     * @param ref A valid reference to a registry.
     */
    sigh_helper(registry_type &ref)
        : bucket{&ref} {}

    /**
     * @brief Binds a properly initialized helper to a given signal type.
     * @tparam Type Type of signal to bind the helper to.
     * @param id Optional name for the underlying storage to use.
     * @return A helper for a given registry and signal type.
     */
    template<typename Type>
    auto with(const id_type id = type_hash<Type>::value()) noexcept {
        return sigh_helper<registry_type, Type>{*bucket, id};
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    [[nodiscard]] registry_type &registry() noexcept {
        return *bucket;
    }

private:
    registry_type *bucket;
};

/**
 * @brief Signal connection helper for registries.
 * @tparam Registry Basic registry type.
 * @tparam Type Type of signal to connect listeners to.
 */
template<typename Registry, typename Type>
struct sigh_helper<Registry, Type> final: sigh_helper<Registry> {
    /*! @brief Registry type. */
    using registry_type = Registry;

    /**
     * @brief Constructs a helper for a given registry.
     * @param ref A valid reference to a registry.
     * @param id Optional name for the underlying storage to use.
     */
    sigh_helper(registry_type &ref, const id_type id = type_hash<Type>::value())
        : sigh_helper<Registry>{ref},
          name{id} {}

    /**
     * @brief Forwards the call to `on_construct` on the underlying storage.
     * @tparam Candidate Function or member to connect.
     * @tparam Args Type of class or type of payload, if any.
     * @param args A valid object that fits the purpose, if any.
     * @return This helper.
     */
    template<auto Candidate, typename... Args>
    auto on_construct(Args &&...args) {
        this->registry().template on_construct<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Forwards the call to `on_update` on the underlying storage.
     * @tparam Candidate Function or member to connect.
     * @tparam Args Type of class or type of payload, if any.
     * @param args A valid object that fits the purpose, if any.
     * @return This helper.
     */
    template<auto Candidate, typename... Args>
    auto on_update(Args &&...args) {
        this->registry().template on_update<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Forwards the call to `on_destroy` on the underlying storage.
     * @tparam Candidate Function or member to connect.
     * @tparam Args Type of class or type of payload, if any.
     * @param args A valid object that fits the purpose, if any.
     * @return This helper.
     */
    template<auto Candidate, typename... Args>
    auto on_destroy(Args &&...args) {
        this->registry().template on_destroy<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

private:
    id_type name;
};

/**
 * @brief Deduction guide.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
sigh_helper(Registry &) -> sigh_helper<Registry>;

} // namespace entt

#endif
