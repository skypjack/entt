#ifndef ENTT_COMMON_REGISTRY_H
#define ENTT_COMMON_REGISTRY_H

#include <entt/entity/registry.hpp>

namespace test {

template<typename Entity>
struct custom_registry: private entt::basic_registry<Entity> {
    using base_type = entt::basic_registry<Entity>;

    template<typename, typename>
    friend class entt::basic_sigh_mixin;

    template<typename, typename>
    friend class entt::basic_reactive_mixin;

public:
    using allocator_type = typename base_type::allocator_type;
    using entity_type = typename base_type::entity_type;

    using base_type::base_type;

    using base_type::storage;
    using base_type::create;
    using base_type::emplace;
    using base_type::insert;
};

} // namespace test

#endif
