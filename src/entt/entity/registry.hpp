#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include "../core/family.hpp"
#include "../signal/sigh.hpp"
#include "sparse_set.hpp"
#include "traits.hpp"
#include "view.hpp"


namespace entt {


/**
 * @brief TODO
 *
 * TODO
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Registry {
    using component_family = Family<struct InternalRegistryComponentFamily>;
    using view_family = Family<struct InternalRegistryViewFamily>;
    using traits_type = entt_traits<Entity>;

    template<typename Component>
    struct Pool: SparseSet<Entity, Component> {
        SigH<void(Entity)> constructed;
        SigH<void(Entity)> destroyed;

        template<typename... Args>
        Component & construct(Entity entity, Args&&... args) {
            auto &component = SparseSet<Entity, Component>::construct(entity, std::forward<Args>(args)...);
            constructed.publish(entity);
            return component;
        }

        void destroy(Entity entity) override {
            SparseSet<Entity, Component>::destroy(entity);
            destroyed.publish(entity);
        }
    };

    template<typename... Component>
    struct PoolHandler: SparseSet<Entity> {
        static_assert(sizeof...(Component) > 1, "!");

        PoolHandler(Pool<Component> &... pools)
            : pools{pools...}
        {}

        void candidate(Entity entity) {
            using accumulator_type = bool[];
            bool match = true;
            accumulator_type accumulator = { (match = match && std::get<Pool<Component> &>(pools).has(entity))... };
            if(match) { SparseSet<Entity>::construct(entity); }
            (void)accumulator;
        }

        void release(Entity entity) {
            if(SparseSet<Entity>::has(entity)) {
                SparseSet<Entity>::destroy(entity);
            }
        }

    private:
        const std::tuple<Pool<Component> &...> pools;
    };

    template<typename Component>
    bool managed() const noexcept {
        const auto ctype = component_family::type<Component>();
        return ctype < pools.size() && pools[ctype];
    }

    template<typename Component>
    const Pool<Component> & pool() const noexcept {
        assert(managed<Component>());
        return static_cast<Pool<Component> &>(*pools[component_family::type<Component>()]);
    }

    template<typename Component>
    Pool<Component> & pool() noexcept {
        assert(managed<Component>());
        return const_cast<Pool<Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    Pool<Component> & ensure() {
        const auto ctype = component_family::type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        if(!pools[ctype]) {
            pools[ctype] = std::make_unique<Pool<Component>>();
        }

        return pool<Component>();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor, explicit on purpose. */
    explicit Registry() = default;
    /*! @brief Default destructor. */
    ~Registry() = default;

    /*! @brief Copying a sparse set isn't allowed. */
    Registry(const Registry &) = delete;
    /*! @brief Moving a sparse set isn't allowed. */
    Registry(Registry &&) = delete;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    Registry & operator=(const Registry &) = delete;
    /*! @brief Moving a sparse set isn't allowed. @return This sparse set. */
    Registry & operator=(Registry &&) = delete;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component The type of the component to which to return the size.
     * @return The number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const noexcept {
        return managed<Component>() ? pool<Component>().size() : size_type{};
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return The number of entities still in use.
     */
    size_type size() const noexcept {
        return entities.size() - available.size();
    }

    /**
     * @brief Returns the number of entities ever created.
     * @return The number of entities ever created.
     */
    size_type capacity() const noexcept {
        return entities.size();
    }

    /**
     * @brief Checks whether the pool for the given component is empty.
     * @tparam Component The type of the component in which one is interested.
     * @return True if the pool for the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const noexcept {
        return managed<Component>() ? pool<Component>().empty() : true;
    }

    /**
     * @brief Checks if there exists at least an entity still in use.
     * @return True if at least an entity is still in use, false otherwise.
     */
    bool empty() const noexcept {
        return entities.size() == available.size();
    }

    /**
     * @brief Verifies if the entity identifier still refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is still valid, false otherwise.
     */
    bool valid(entity_type entity) const noexcept {
        const auto entt = entity & traits_type::entity_mask;
        return (entt < entities.size() && entities[entt] == entity);
    }

    /**
     * @brief Returns the version stored along with the given entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    version_type version(entity_type entity) const noexcept {
        return version_type((entity >> traits_type::version_shift) & traits_type::version_mask);
    }

    /**
     * @brief Returns the actual version for the given entity identifier.
     *
     * In case entity identifers are stored around, this function can be used to
     * know if they are still valid or the entity has been destroyed and
     * potentially recycled.
     *
     * Attempting to use an entity that doesn't belong to the registry results
     * in undefined behavior. An entity belongs to the registry even if it has
     * been previously destroyed and/or recycled.
     *
     * @note An assertion will abort the execution at runtime in debug mode if
     * the registry doesn't own the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The actual version for the given entity identifier.
     */
    version_type current(entity_type entity) const noexcept {
        const auto entt = entity & traits_type::entity_mask;
        assert(entt < entities.size());
        return version_type((entities[entt] >> traits_type::version_shift) & traits_type::version_mask);
    }

    /**
     * @brief Returns a new entity to which the given components are assigned.
     *
     * There are two kinds of entity identifiers:
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled one with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `current` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * @tparam Component A list of components to assign to the entity.
     * @return A valid entity identifier.
     */
    template<typename... Component>
    entity_type create() noexcept {
        using accumulator_type = int[];
        const auto entity = create();
        accumulator_type accumulator = { 0, (ensure<Component>().construct(entity), 0)... };
        (void)accumulator;
        return entity;
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of entity identifiers:
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled one with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `current` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * @return A valid entity identifier.
     */
    entity_type create() noexcept {
        entity_type entity;

        if(available.empty()) {
            entity = entity_type(entities.size());
            assert((entity >> traits_type::version_shift) == entity_type{});
            entities.push_back(entity);
        } else {
            entity = available.back();
            available.pop_back();
        }

        return entity;
    }

    /**
     * @brief Destroys an entity and lets the registry recycle the identifier.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time. In case entity identifers are stored around,
     * the `current` member function can be used to know if they are still valid
     * or the entity has been destroyed and potentially recycled.
     *
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity.
     *
     * @param entity A valid entity identifier
     */
    void destroy(entity_type entity) {
        assert(valid(entity));

        const auto entt = entity & traits_type::entity_mask;
        const auto version = 1 + ((entity >> traits_type::version_shift) & traits_type::version_mask);
        entities[entt] = entt | (version << traits_type::version_shift);
        available.push_back(entity);

        for(auto &&cpool: pools) {
            if(cpool && cpool->has(entity)) {
                cpool->destroy(entity);
            }
        }
    }

    /**
     * @brief Assigns the given component to the given entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * Attempting to assign a component to an entity that already owns it
     * results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity or if the entity already owns an instance of the
     * given component.
     *
     * @tparam Component The type of the component to create.
     * @tparam Args The types of the arguments used to construct the component.
     * @param entity A valid entity identifier.
     * @param args The parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & assign(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return ensure<Component>().construct(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from the given entity.
     *
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * Attempting to remove a component from an entity that doesn't own it
     * results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity or if the entity doesn't own an instance of the
     * given component.
     *
     * @tparam Component The type of the component to remove.
     * @param entity A valid entity identifier.
     */
    template<typename Component>
    void remove(entity_type entity) {
        assert(valid(entity));
        return pool<Component>().destroy(entity);
    }

    /**
     * @brief Checks if the given entity has all the given components.
     *
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity.
     *
     * @tparam Component The components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has all the components, false otherwise.
     */
    template<typename... Component>
    bool has(entity_type entity) const noexcept {
        static_assert(sizeof...(Component) > 0, "!");
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { (all = all && managed<Component>() && pool<Component>().has(entity))... };
        (void)accumulator;
        return all;
    }

    /**
     * @brief Gets a reference to the given component owned by the given entity.
     *
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * Attempting to get a component from an entity that doesn't own it
     * results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity or if the entity doesn't own an instance of the
     * given component.
     *
     * @tparam Component The type of the component to get.
     * @param entity A valid entity identifier.
     * @return A reference to the instance of the component owned by the entity.
     */
    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return pool<Component>().get(entity);
    }

    /**
     * @brief Gets a reference to the given component owned by the given entity.
     *
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * Attempting to get a component from an entity that doesn't own it
     * results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity or if the entity doesn't own an instance of the
     * given component.
     *
     * @tparam Component The type of the component to get.
     * @param entity A valid entity identifier.
     * @return A reference to the instance of the component owned by the entity.
     */
    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const Registry *>(this)->get<Component>(entity));
    }

    /**
     * @brief Replaces the given component for the given entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * Attempting to replace a component of an entity that doesn't own it
     * results in undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode in
     * case of invalid entity or if the entity doesn't own an instance of the
     * given component.
     *
     * @tparam Component The type of the component to replace.
     * @tparam Args The types of the arguments used to construct the component.
     * @param entity A valid entity identifier.
     * @param args The parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & replace(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return (pool<Component>().get(entity) = Component{std::forward<Args>(args)...});
    }

    /**
     * @brief Assigns or replaces the given component to the given entity.
     *
     * Equivalent to the following snippet (pseudocode):
     * @code{.cpp}
     * if(registry.has<Component>(entity)) {
     *     registry.replace<Component>(entity, args...);
     * } else {
     *     registry.assign<Component>(entity, args...);
     * }
     * @endcode
     *
     * Prefer this function anyway because it has slighlty better performance.
     *
     * @tparam Component The type of the component to assign or replace.
     * @tparam Args The types of the arguments used to construct the component.
     * @param entity A valid entity identifier.
     * @param args The parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & accomodate(entity_type entity, Args&&... args) {
        assert(valid(entity));
        auto &cpool = ensure<Component>();

        return (cpool.has(entity)
                ? (cpool.get(entity) = Component{std::forward<Args>(args)...})
                : cpool.construct(entity, std::forward<Args>(args)...));
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     * @tparam Compare TODO
     * @param compare TODO
     */
    template<typename Component, typename Compare>
    void sort(Compare compare) {
        auto &cpool = ensure<Component>();

        cpool.sort([&cpool, compare = std::move(compare)](auto lhs, auto rhs) {
            return compare(static_cast<const Component &>(cpool.get(lhs)), static_cast<const Component &>(cpool.get(rhs)));
        });
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam To TODO
     * @tparam From TODO
     */
    template<typename To, typename From>
    void sort() {
        ensure<To>().respect(ensure<From>());
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     * @param entity TODO
     */
    template<typename Component>
    void reset(entity_type entity) {
        assert(valid(entity));

        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            if(cpool.has(entity)) {
                cpool.destroy(entity);
            }
        }
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     */
    template<typename Component>
    void reset() {
        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            for(auto entity: entities) {
                if(cpool.has(entity)) {
                    cpool.destroy(entity);
                }
            }
        }
    }

    /**
     * @brief TODO
     *
     * TODO
     */
    void reset() {
        available.clear();
        pools.clear();

        for(auto &&entity: entities) {
            const auto version = 1 + ((entity >> traits_type::version_shift) & traits_type::version_mask);
            entity = (entity & traits_type::entity_mask) | (version << traits_type::version_shift);
            available.push_back(entity);
        }
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     * @return TODO
     */
    template<typename... Component>
    View<Entity, Component...> view() {
        return View<Entity, Component...>{ensure<Component>()...};
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     */
    template<typename... Component>
    void prepare() {
        static_assert(sizeof...(Component) > 1, "!");
        const auto vtype = view_family::type<Component...>();

        if(!(vtype < handlers.size())) {
            handlers.resize(vtype + 1);
        }

        if(!handlers[vtype]) {
            using handler_type = PoolHandler<Component...>;
            using accumulator_type = int[];

            auto handler = std::make_unique<handler_type>(ensure<Component>()...);

            for(auto entity: view<Component...>()) {
                handler->construct(entity);
            }

            auto *ptr = handler.get();
            handlers[vtype] = std::move(handler);

            accumulator_type accumulator = {
                (ensure<Component>().constructed.template connect<handler_type, &handler_type::candidate>(ptr), 0)...,
                (ensure<Component>().destroyed.template connect<handler_type, &handler_type::release>(ptr), 0)...
            };

            (void)accumulator;
        }
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component TODO
     * @return TODO
     */
    template<typename... Component>
    PersistentView<Entity, Component...> persistent() {
        static_assert(sizeof...(Component) > 1, "!");
        prepare<Component...>();
        return PersistentView<Entity, Component...>{*handlers[view_family::type<Component...>()], ensure<Component>()...};
    }

private:
    std::vector<std::unique_ptr<SparseSet<Entity>>> handlers;
    std::vector<std::unique_ptr<SparseSet<Entity>>> pools;
    std::vector<entity_type> available;
    std::vector<entity_type> entities;
};


/**
 * @brief TODO
 *
 * TODO
 */
using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_ENTITY_REGISTRY_HPP
