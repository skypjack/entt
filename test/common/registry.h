#ifndef ENTT_COMMON_REGISTRY_H
#define ENTT_COMMON_REGISTRY_H

#include <entt/entity/registry.hpp>

namespace test {

/// Custom registry that exposes a subset of basic registry methods
template<typename Entity>
class basic_custom_registry: protected entt::basic_registry<Entity> {
    using registry_type = entt::basic_registry<Entity>;

  public:
    using entity_type = typename registry_type::entity_type;
    using allocator_type = typename registry_type::allocator_type;

    // Forwarding desired methods to be exposed

    [[nodiscard]] Entity create() { return registry_type::create(); }

    template<typename Type, typename It, typename... Args>
    void insert(It begin, It end, Args &&... args)
    {
        registry_type::template insert<Type>(begin, end, std::forward<Args>(args)...);
    }

    template<typename Type>
    decltype(auto) storage(const entt::id_type id = entt::type_hash<Type>::value())
    {
        return registry_type::template storage<Type>(id);
    }

    template<typename Type>
    decltype(auto) storage(const entt::id_type id = entt::type_hash<Type>::value()) const
    {
        return registry_type::template storage<Type>(id);
    }
};

} // namespace test

#endif
