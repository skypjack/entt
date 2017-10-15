#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include "../core/family.hpp"
#include "sparse_set.hpp"
#include "traits.hpp"
#include "view.hpp"


namespace entt {


/**
 * @brief A repository class for entities and components.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components and thus
 * create views to iterate them.
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
        using test_fn_type = bool(Registry::*)(Entity) const;

        template<typename... Args>
        Component & construct(Registry &registry, Entity entity, Args&&... args) {
            auto &component = SparseSet<Entity, Component>::construct(entity, std::forward<Args>(args)...);

            for(auto &&listener: listeners) {
                if((registry.*listener.second)(entity)) {
                    listener.first.construct(entity);
                }
            }

            return component;
        }

        void destroy(Entity entity) override {
            SparseSet<Entity, Component>::destroy(entity);

            for(auto &&listener: listeners) {
                auto &handler = listener.first;

                if(handler.has(entity)) {
                    handler.destroy(entity);
                }
            }
        }

        void append(SparseSet<Entity> &handler, test_fn_type fn) {
            listeners.emplace_back(handler, fn);
        }

    private:
        std::vector<std::pair<SparseSet<Entity> &, test_fn_type>> listeners;
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
     * @warning
     * Attempting to use an entity that doesn't belong to the registry results
     * in undefined behavior. An entity belongs to the registry even if it has
     * been previously destroyed and/or recycled.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * registry doesn't own the given entity.
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
        accumulator_type accumulator = { 0, (ensure<Component>().construct(*this, entity), 0)... };
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
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
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
     * @warning
     * Attempting to use an invalid entity or to assign a component to an entity
     * that already owns it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity already owns an instance of the given
     * component.
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
        return ensure<Component>().construct(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from the given entity.
     *
     * @warning
     * Attempting to use an invalid entity or to remove a component from an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
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
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
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
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
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
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
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
     * @warning
     * Attempting to use an invalid entity or to replace a component of an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
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
     *
     * @code{.cpp}
     * if(registry.has<Component>(entity)) {
     *     registry.replace<Component>(entity, args...);
     * } else {
     *     registry.assign<Component>(entity, args...);
     * }
     * @endcode
     *
     * Prefer this function anyway because it has slighlty better
     * performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
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
                : cpool.construct(*this, entity, std::forward<Args>(args)...));
    }

    /**
     * @brief Sorts the pool of the given component.
     *
     * The order of the elements in a pool is highly affected by assignements
     * of components to entities and deletions. Components are arranged to
     * maximize the performance during iterations and users should not make any
     * assumption on the order.<br/>
     * This function can be used to impose an order to the elements in the pool
     * for the given component. The order is kept valid until a component of the
     * given type is assigned or removed from an entity.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(auto e1, auto e2)
     * @endcode
     *
     * Where `e1` and `e2` are valid entity identifiers.
     *
     * @tparam Component The type of the components to sort.
     * @tparam Compare The type of the comparison function object.
     * @param compare A valid comparison function object.
     */
    template<typename Component, typename Compare>
    void sort(Compare compare) {
        auto &cpool = ensure<Component>();

        cpool.sort([&cpool, compare = std::move(compare)](auto lhs, auto rhs) {
            return compare(static_cast<const Component &>(cpool.get(lhs)), static_cast<const Component &>(cpool.get(rhs)));
        });
    }

    /**
     * @brief Sorts two pools of components in the same way.
     *
     * The order of the elements in a pool is highly affected by assignements
     * of components to entities and deletions. Components are arranged to
     * maximize the performance during iterations and users should not make any
     * assumption on the order.
     *
     * It happens that different pools of components must be sorted the same way
     * because of runtime and/or performance constraints. This function can be
     * used to order a pool of components according to the order between the
     * entities in another pool of components.
     *
     * @b How @b it @b works
     *
     * Being `A` and `B` the two sets where `B` is the master (the one the order
     * of which rules) and `A` is the slave (the one to sort), after a call to
     * this function an iterator for `A` will return the entities according to
     * the following rules:
     *
     * * All the entities in `A` that are also in `B` are returned first
     * according to the order they have in `B`.
     * * All the entities in `A` that are not in `B` are returned in no
     * particular order after all the other entities.
     *
     * Any subsequent change to `B` won't affect the order in `A`.
     *
     * @tparam To The type of the components to sort.
     * @tparam From The type of the components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ensure<To>().respect(ensure<From>());
    }

    /**
     * @brief Resets the given component for the given entity.
     *
     * If the entity has an instance of the component, this function removes the
     * component from the entity. Otherwise it does nothing.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component The component to reset.
     * @param entity A valid entity identifier.
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
     * @brief Resets the pool of the given component.
     *
     * For each entity that has an instance of the given component, the
     * component itself is removed and thus destroyed.
     *
     * @tparam Component The component whose pool must be reset.
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
     * @brief Resets the whole registry.
     *
     * Destroys all the entities. After a call to `reset`, all the entities
     * previously created are recycled with a new version number. In case entity
     * identifers are stored around, the `current` member function can be used
     * to know if they are still valid.
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
     * @brief Returns a standard view for the given components.
     *
     * This kind of views are created on the fly and share with the registry its
     * internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Standard views do their best to iterate the smallest set of candidate
     * entites. In particular:
     * * Single component views are incredibly fast and iterate a packed array
     * of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     * component and pick up a reference to the smallest set of candidates to
     * test for the given components.
     *
     * @note
     * Multi component views are pretty fast. However their performance tend to
     * degenerate when the number of components to iterate grows up and the most
     * of the entities have all the given components.<br/>
     * To get a performance boost, consider using a PersistentView instead.
     *
     * @see View
     * @see View<Entity, Component>
     * @see PersistentView
     *
     * @tparam Component The type of the components used to construct the view.
     * @return A newly created standard view.
     */
    template<typename... Component>
    View<Entity, Component...> view() {
        return View<Entity, Component...>{ensure<Component>()...};
    }

    /**
     * @brief Prepares the internal data structures used by persistent views.
     *
     * Persistent views are an incredibly fast tool used to iterate a packed
     * array of entities all of which have specific components.<br/>
     * The initialization of a persistent view is also a pretty cheap operation,
     * but for the first time they are created. That's mainly because of the
     * internal data structures of the registry that are dedicated to this kind
     * of views and that don't exist yet the very first time they are
     * requested.<br/>
     * To avoid costly operations, internal data structures for persistent views
     * can be prepared with this function. Just use the same set of components
     * that would have been used otherwise to contruct the view.
     *
     * @tparam Component The types of the components used to prepare the view.
     */
    template<typename... Component>
    void prepare() {
        static_assert(sizeof...(Component) > 1, "!");
        const auto vtype = view_family::type<Component...>();

        if(!(vtype < handlers.size())) {
            handlers.resize(vtype + 1);
        }

        if(!handlers[vtype]) {
            using accumulator_type = int[];

            auto handler = std::make_unique<SparseSet<Entity>>();

            for(auto entity: view<Component...>()) {
                handler->construct(entity);
            }

            accumulator_type accumulator = {
                (ensure<Component>().append(*handler, &Registry::has<Component...>), 0)...
            };

            handlers[vtype] = std::move(handler);
            (void)accumulator;
        }
    }

    /**
     * @brief Returns a persistent view for the given components.
     *
     * This kind of views are created on the fly and share with the registry its
     * internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Persistent views are the right choice to iterate entites when the number
     * of components grows up and the most of the entities have all the given
     * components.<br/>
     * However they have also drawbacks:
     * * Each kind of persistent view requires a dedicated data structure that
     * is allocated within the registry and it increases memory pressure.
     * * Internal data structures used to construct persistent views must be
     * kept updated and it affects slightly construction and destruction of
     * entities and components.
     *
     * That being said, persistent views are an incredibly powerful tool if used
     * with care and offer a boost of performance undoubtedly.
     *
     * @note
     * Consider to use the `prepare` member function to initialize the internal
     * data structures used by persistent views when the registry is still
     * empty. Initialization could be a costly operation otherwise and it will
     * be performed the very first time each view is created.
     *
     * @see View
     * @see View<Entity, Component>
     * @see PersistentView
     *
     * @tparam Component The types of the components used to construct the view.
     * @return A newly created persistent view.
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
 * @brief Default registry class.
 *
 * The default registry is the best choice for almost all the applications.<br/>
 * Users should have a really good reason to choose something different.
 */
using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_ENTITY_REGISTRY_HPP
