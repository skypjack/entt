#ifndef ENTT_ENTITY_PROXY_HPP
#define ENTT_ENTITY_PROXY_HPP


#include "registry.hpp"


namespace entt {


/**
 * @brief Non-owning proxy to an entity.
 *
 * Tiny wrapper around an entity and a registry.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_proxy {
public:
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<Entity>;
    /*! @brief Type of registry used internally. */
    using registry_type = std::conditional_t<
        std::is_const_v<Entity>,
        const basic_registry<entity_type>,
        basic_registry<entity_type>
    >;

    basic_proxy() ENTT_NOEXCEPT
        : entt{null}, reg{nullptr}
    {}

    /**
     * @brief Constructs a proxy from a given entity.
     * @param entity A valid entity identifier.
     * @param ref An instance of the registry class.
     */
    basic_proxy(entity_type entity, registry_type &ref) ENTT_NOEXCEPT
        : entt{entity}, reg{&ref}
    {}
    
    /**
     * @brief Constructs a const proxy from a non-const proxy.
     */
    operator basic_proxy<const Entity>() const ENTT_NOEXCEPT {
        return {entt, *reg};
    }

    /**
     * @sa basic_registry::emplace
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace(Args &&... args) const {
        ENTT_ASSERT(reg);
        return reg->template emplace<Component>(entt, std::forward<Args>(args)...);
    }
    
    /**
     * @sa basic_registry::emplace_or_replace
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace_or_replace(Args &&... args) const {
        ENTT_ASSERT(reg);
        return reg->template emplace_or_replace<Component>(entt, std::forward<Args>(args)...);
    }
    
    /**
     * @sa basic_registry::patch
     */
    template<typename Component, typename... Func>
    decltype(auto) patch(Func &&... func) const {
        ENTT_ASSERT(reg);
        return reg->template patch<Component>(entt, std::forward<Func>(func)...);
    }
    
    /**
     * @sa basic_registry::replace
     */
    template<typename Component, typename... Args>
    decltype(auto) replace(Args &&... args) const {
        ENTT_ASSERT(reg);
        return reg->template replace<Component>(entt, std::forward<Args>(args)...);
    }
    
    /**
     * @sa basic_registry::remove
     */
    template<typename... Components>
    void remove() const {
        ENTT_ASSERT(reg);
        reg->template remove<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::remove_if_exists
     */
    template<typename... Components>
    decltype(auto) remove_if_exists() const {
        ENTT_ASSERT(reg);
        return reg->template remove_if_exists<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::remove_all
     */
    template<typename = void>
    void remove_all() const {
        ENTT_ASSERT(reg);
        reg->remove_all(entt);
    }
    
    /**
     * @sa basic_registry::has
     */
    template<typename... Components>
    [[nodiscard]] decltype(auto) has() const {
        ENTT_ASSERT(reg);
        return reg->template has<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::any
     */
    template<typename... Components>
    [[nodiscard]] decltype(auto) any() const {
        ENTT_ASSERT(reg);
        return reg->template any<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::get
     */
    template<typename... Components>
    [[nodiscard]] decltype(auto) get() const {
        ENTT_ASSERT(reg);
        return reg->template get<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::get_or_emplace
     */
    template<typename Component, typename... Args>
    decltype(auto) get_or_emplace(Args &&... args) const {
        ENTT_ASSERT(reg);
        return reg->template get_or_emplace<Component>(entt, std::forward<Args>(args)...);
    }
    
    /**
     * @sa basic_registry::try_get
     */
    template<typename... Components>
    [[nodiscard]] decltype(auto) try_get() const {
        ENTT_ASSERT(reg);
        return reg->template try_get<Components...>(entt);
    }
    
    /**
     * @sa basic_registry::orphan
     */
    [[nodiscard]] bool orphan() const {
        ENTT_ASSERT(reg);
        return reg->orphan(entt);
    }
    
    /**
     * @sa basic_registry::visit
     */
    template<typename Func>
    void visit(Func &&func) const {
        ENTT_ASSERT(reg);
        reg->visit(entt, std::forward<Func>(func));
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    [[nodiscard]] registry_type & registry() const ENTT_NOEXCEPT {
        ENTT_ASSERT(reg);
        return *reg;
    }

    /**
     * @brief Returns the entity associated with a proxy.
     * @return The entity associated with the proxy.
     */
    [[nodiscard]] entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

    /**
     * @brief Checks if a proxy refers to a valid entity or not.
     * @return True if the proxy refers to a valid entity, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
};


template<typename Entity>
basic_proxy(Entity, basic_registry<Entity> &) -> basic_proxy<Entity>;

template<typename Entity>
basic_proxy(Entity, const basic_registry<Entity> &) -> basic_proxy<const Entity>;


}


#endif
