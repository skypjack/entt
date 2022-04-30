#ifndef ENTT_ENTITY_POLY_STORAGE_MIXIN_HPP
#define ENTT_ENTITY_POLY_STORAGE_MIXIN_HPP

#include "storage.hpp"
#include "poly_type_traits.hpp"
#include "sigh_storage_mixin.hpp"


namespace entt {

template<typename Entity, typename Type, typename Allocator>
class poly_type;

/**
 * @brief Storage mixin for polymorphic component types
 * @tparam Storage underlying storage type
 * @tparam Entity entity type
 * @tparam Type value type
 */
template<typename Storage, typename = void>
struct poly_storage_mixin : Storage {
    /*! @brief Underlying value type. */
    using value_type = typename Storage::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Storage::entity_type;

    /*! @brief Inherited constructors. */
    using Storage::Storage;

    /**
     * @brief Forwards variables to mixins, if any.
     * @param value A variable wrapped in an opaque container.
     */
    void bind(any value) ENTT_NOEXCEPT override {
        if(auto *reg = any_cast<basic_registry<entity_type>>(&value); reg) {
            bind_all_parent_types(*reg, poly_parent_types_t<value_type>{});
        }
        Storage::bind(std::move(value));
    }

private:
    template<typename... ParentTypes>
    void bind_all_parent_types(basic_registry<entity_type>& reg, [[maybe_unused]] type_list<ParentTypes...>) {
        (reg.ctx().template emplace<poly_type<entity_type, ParentTypes, poly_type_allocator_t<ParentTypes>>>().bind_child_storage(this), ...);
        reg.ctx().template emplace<poly_type<entity_type, value_type, poly_type_allocator_t<value_type>>>().bind_child_storage(this);
    }
};


} // entt

#endif
