#ifndef ENTT_ENTITY_POLY_STORAGE_MIXIN_HPP
#define ENTT_ENTITY_POLY_STORAGE_MIXIN_HPP

#include "storage.hpp"
#include "poly_type_traits.hpp"


namespace entt {

template<typename Entity, typename Type>
class poly_type;

/**
 * @brief used to add signal support for polymorphic storages
 * @tparam Entity storage entity type
 * @tparam Type storage value type
 */
template<typename Entity, typename Type>
class poly_sigh_holder {
public:
    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance is created and assigned to an entity.<br/>
     * Listeners are invoked after the object has been assigned to the entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
        return sink{construction};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is explicitly updated.<br/>
     * Listeners are invoked after the object has been updated.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
        return sink{update};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is removed from an entity and thus destroyed.<br/>
     * Listeners are invoked before the object has been removed from the entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
        return sink{destruction};
    }

private:
    template<typename... T>
    inline void publish_construction([[maybe_unused]] type_list<T...>, basic_registry<Entity>& reg, const Entity ent) {
        (reg.template storage<T>().construction.publish(reg, ent), ...);
    }

    inline void publish_construction(basic_registry<Entity>& reg, const Entity ent) {
        publish_construction(poly_parent_types_t<Type>{}, reg, ent);
        publish_construction(type_list<Type>{}, reg, ent);
    }

    template<typename... T>
    inline void publish_update([[maybe_unused]] type_list<T...>, basic_registry<Entity>& reg, const Entity ent) {
        (reg.template storage<T>().update.publish(reg, ent), ...);
    }

    inline void publish_update(basic_registry<Entity>& reg, const Entity ent) {
        publish_update(poly_parent_types_t<Type>{}, reg, ent);
        publish_update(type_list<Type>{}, reg, ent);
    }

    template<typename... T>
    inline void publish_destruction([[maybe_unused]] type_list<T...>, basic_registry<Entity>& reg, const Entity ent) {
        (reg.template storage<T>().destruction.publish(reg, ent), ...);
    }

    inline void publish_destruction(basic_registry<Entity>& reg, const Entity ent) {
        publish_destruction(poly_parent_types_t<Type>{}, reg, ent);
        publish_destruction(type_list<Type>{}, reg, ent);
    }

private:
    sigh<void(basic_registry<Entity>&, const Entity)> construction{};
    sigh<void(basic_registry<Entity>&, const Entity)> update{};
    sigh<void(basic_registry<Entity>&, const Entity)> destruction{};

    template<typename, typename>
    friend class poly_sigh_holder;

    template<typename, typename, typename, typename>
    friend class poly_storage_mixin;
};


/**
 * @brief Storage mixin for polymorphic component types
 * @tparam Storage underlying storage type
 * @tparam Entity entity type
 * @tparam Type value type
 */
template<typename Storage, typename Entity, typename Type, typename = void>
class poly_storage_mixin : public Storage, public poly_sigh_holder<Entity, Type> {template<typename Func>
    void notify_destruction(typename Storage::basic_iterator first, typename Storage::basic_iterator last, Func func) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        for(; first != last; ++first) {
            const auto entt = *first;
            this->publish_destruction(*owner, entt);
            const auto it = Storage::find(entt);
            func(it, it + 1u);
        }
    }

    void swap_and_pop(typename Storage::basic_iterator first, typename Storage::basic_iterator last) final {
        notify_destruction(std::move(first), std::move(last), [this](auto... args) { Storage::swap_and_pop(args...); });
    }

    void in_place_pop(typename Storage::basic_iterator first, typename Storage::basic_iterator last) final {
        notify_destruction(std::move(first), std::move(last), [this](auto... args) { Storage::in_place_pop(args...); });
    }

    typename Storage::basic_iterator try_emplace(const typename Storage::entity_type entt, const bool force_back, const void *value) final {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Storage::try_emplace(entt, force_back, value);
        this->publish_construction(*owner, entt);
        return Storage::find(entt);
    }

public:
    /*! @brief Underlying value type. */
    using value_type = typename Storage::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Storage::entity_type;

    /*! @brief Inherited constructors. */
    using Storage::Storage;

    /**
     * @brief Assigns entities to a storage.
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Storage::emplace(entt, std::forward<Args>(args)...);
        this->publish_construction(*owner, entt);
        return this->get(entt);
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Storage::patch(entt, std::forward<Func>(func)...);
        this->publish_update(*owner, entt);
        return this->get(entt);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects assigned
     * to the entities.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects assigned to the
     * entities.
     */
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&...args) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Storage::insert(first, last, std::forward<Args>(args)...);

        for(auto it = last; it != last; ++it) {
            publish_construction(*owner, *it);
        }
    }

    /**
     * @brief Forwards variables to mixins, if any.
     * @param value A variable wrapped in an opaque container.
     */
    void bind(any value) ENTT_NOEXCEPT override {
        if(auto *reg = any_cast<basic_registry<entity_type>>(&value); reg) {
            owner = reg;
            register_for_all_parent_types(*reg, poly_parent_types_t<value_type>{});
        }
        Storage::bind(std::move(value));
    }

    template<typename ParentType>
    void add_self_for_type(basic_registry<entity_type>& reg) {
        reg.ctx().template emplace<poly_type<entity_type, ParentType>>().bind_child_storage(this);
    }

    template<typename... ParentTypes>
    void register_for_all_parent_types(basic_registry<entity_type>& reg, [[maybe_unused]] type_list<ParentTypes...>) {
        (add_self_for_type<ParentTypes>(reg), ...);
        add_self_for_type<value_type>(reg);
    }

    template<typename, typename, typename, typename>
    friend class poly_storage_mixin;

private:
    basic_registry<entity_type> *owner{};
};


/** @copydoc poly_storage_mixin */
template<typename Storage, typename Entity, typename Type>
class poly_storage_mixin<Storage, Entity, Type, std::enable_if_t<!(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>)>>:
    public basic_sparse_set<Entity>, public poly_sigh_holder<Entity, Type> {

public:
    using base_type = basic_sparse_set<Entity>;
    using entity_type = Entity;
    using value_type = Type;

    poly_storage_mixin() : base_type(type_id<value_type>()) {};
};

} // entt

#endif // ENTT_ENTITY_POLY_STORAGE_MIXIN_HPP
