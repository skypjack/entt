#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include "../core/family.hpp"
#include "entt_traits.hpp"
#include "sparse_set.hpp"
#include "view.hpp"


namespace entt {


/**
 * @brief Fast and reliable entity-component system.
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
    using tag_family = Family<struct InternalRegistryTagFamily>;
    using component_family = Family<struct InternalRegistryComponentFamily>;
    using view_family = Family<struct InternalRegistryViewFamily>;
    using traits_type = entt_traits<Entity>;

    struct Attachee {
        Entity entity;
    };

    template<typename Tag>
    struct Attaching: Attachee {
        // requirements for aggregates are relaxed only since C++17
        template<typename... Args>
        Attaching(Entity entity, Tag tag)
            : Attachee{entity}, tag{std::move(tag)}
        {}

        Tag tag;
    };

    template<typename Component>
    struct Pool: SparseSet<Entity, Component> {
        using test_fn_type = bool(Registry::*)(Entity) const;

        template<typename... Args>
        Component & construct(Registry &registry, Entity entity, Args &&... args) {
            auto &component = SparseSet<Entity, Component>::construct(entity, std::forward<Args>(args)...);

            for(auto &&listener: listeners) {
                if((registry.*listener.second)(entity)) {
                    listener.first->construct(entity);
                }
            }

            return component;
        }

        void destroy(Entity entity) override {
            SparseSet<Entity, Component>::destroy(entity);

            for(auto &&listener: listeners) {
                auto *handler = listener.first;

                if(handler->has(entity)) {
                    handler->destroy(entity);
                }
            }
        }

        inline void append(SparseSet<Entity> *handler, test_fn_type fn) {
            listeners.emplace_back(handler, fn);
        }

        inline void remove(SparseSet<Entity> *handler) {
            listeners.erase(std::remove_if(listeners.begin(), listeners.end(), [handler](auto &listener) {
                return listener.first == handler;
            }), listeners.end());
        }

    private:
        std::vector<std::pair<SparseSet<Entity> *, test_fn_type>> listeners;
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

    template<typename... Component>
    SparseSet<Entity> & handler() {
        static_assert(sizeof...(Component) > 1, "!");
        const auto vtype = view_family::type<Component...>();

        if(!(vtype < handlers.size())) {
            handlers.resize(vtype + 1);
        }

        if(!handlers[vtype]) {
            using accumulator_type = int[];
            auto set = std::make_unique<SparseSet<Entity>>();

            for(auto entity: view<Component...>()) {
                set->construct(entity);
            }

            accumulator_type accumulator = {
                (ensure<Component>().append(set.get(), &Registry::has<Component...>), 0)...
            };

            handlers[vtype] = std::move(set);
            (void)accumulator;
        }

        return *handlers[vtype];
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Unsigned integer type. */
    using tag_type = typename tag_family::family_type;
    /*! @brief Unsigned integer type. */
    using component_type = typename component_family::family_type;

    /*! @brief Default constructor. */
    Registry() = default;

    /*! @brief Copying a registry isn't allowed. */
    Registry(const Registry &) = delete;
    /*! @brief Default move constructor. */
    Registry(Registry &&) = default;

    /*! @brief Copying a registry isn't allowed. @return This registry. */
    Registry & operator=(const Registry &) = delete;
    /*! @brief Default move assignment operator. @return This registry. */
    Registry & operator=(Registry &&) = default;

    /**
     * @brief Returns the numeric identifier of a type of tag at runtime.
     *
     * The given tag doesn't need to be necessarily in use. However, the
     * registry could decide to prepare internal data structures for it for
     * later uses.<br/>
     * Do not use this functionality to provide numeric identifiers to types at
     * runtime.
     *
     * @tparam Tag Type of tag to query.
     * @return Runtime numeric identifier of the given type of tag.
     */
    template<typename Tag>
    tag_type tag() const noexcept {
        return tag_family::type<Tag>();
    }

    /**
     * @brief Returns the numeric identifier of a type of component at runtime.
     *
     * The given component doesn't need to be necessarily in use. However, the
     * registry could decide to prepare internal data structures for it for
     * later uses.<br/>
     * Do not use this functionality to provide numeric identifiers to types at
     * runtime.
     *
     * @tparam Component Type of component to query.
     * @return Runtime numeric identifier of the given type of component.
     */
    template<typename Component>
    component_type component() const noexcept {
        return component_family::type<Component>();
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const noexcept {
        return managed<Component>() ? pool<Component>().size() : size_type{};
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return Number of entities still in use.
     */
    size_type size() const noexcept {
        return entities.size() - available;
    }

    /**
     * @brief Increases the capacity of the pool for a given component.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @tparam Component Type of component for which to reserve storage.
     * @param cap Desired capacity.
     */
    template<typename Component>
    void reserve(size_type cap) {
        ensure<Component>().reserve(cap);
    }

    /**
     * @brief Increases the capacity of a registry in terms of entities.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(size_type cap) {
        entities.reserve(cap);
    }

    /**
     * @brief Returns the number of entities ever created.
     * @return Number of entities ever created.
     */
    size_type capacity() const noexcept {
        return entities.size();
    }

    /**
     * @brief Checks whether the pool for the given component is empty.
     * @tparam Component Type of component in which one is interested.
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
        return entities.size() == available;
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    bool valid(entity_type entity) const noexcept {
        const auto pos = size_type(entity & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     *
     * Alternative version of `valid`. It accesses the internal data structures
     * without bounds checking and thus it's both unsafe and risky to use.<br/>
     * You should not invoke directly this function unless you know exactly what
     * you are doing. Prefer the `valid` member function instead.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the registry can
     * result in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * bounds violation.
     *
     * @param entity A valid entity identifier.
     * @return True if the identifier is valid, false otherwise.
     */
    bool fast(entity_type entity) const noexcept {
        const auto pos = size_type(entity & traits_type::entity_mask);
        assert(pos < entities.size());
        // the in-use control bit permits to avoid accessing the direct vector
        return (entities[pos] == entity);
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return Version stored along with the given entity identifier.
     */
    version_type version(entity_type entity) const noexcept {
        return version_type((entity >> traits_type::entity_shift) & traits_type::version_mask);
    }

    /**
     * @brief Returns the actual version for an entity identifier.
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
     * @return Actual version for the given entity identifier.
     */
    version_type current(entity_type entity) const noexcept {
        const auto pos = size_type(entity & traits_type::entity_mask);
        assert(pos < entities.size());
        return version_type((entities[pos] >> traits_type::entity_shift) & traits_type::version_mask);
    }

    /**
     * @brief Returns a new entity initialized with the given components.
     *
     * There are two kinds of entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled one with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `current` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * The returned entity has fully initialized components assigned.
     *
     * @tparam Component A list of components to assign to the entity.
     * @param components Instances with which to initialize components.
     * @return A valid entity identifier.
     */
    template<typename... Component>
    entity_type create(Component &&... components) noexcept {
        using accumulator_type = int[];
        const auto entity = create();
        accumulator_type accumulator = { 0, (ensure<std::decay_t<Component>>().construct(*this, entity, std::forward<Component>(components)), 0)... };
        (void)accumulator;
        return entity;
    }

    /**
     * @brief Returns a new entity to which the given components are assigned.
     *
     * There are two kinds of entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled one with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `current` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * The returned entity has default initialized components assigned.
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
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled one with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `current` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * The returned entity has no components assigned.
     *
     * @return A valid entity identifier.
     */
    entity_type create() noexcept {
        entity_type entity;

        if(available) {
            const auto entt = next;
            const auto version = entities[entt] & (~traits_type::entity_mask);

            entity = entt | version;
            next = entities[entt] & traits_type::entity_mask;
            entities[entt] = entity;
            --available;
        } else {
            entity = entity_type(entities.size());
            assert(entity < traits_type::entity_mask);
            entities.push_back(entity);
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
        const auto version = (((entity >> traits_type::entity_shift) + 1) & traits_type::version_mask) << traits_type::entity_shift;
        const auto node = (available ? next : ((entt + 1) & traits_type::entity_mask)) | version;

        entities[entt] = node;
        next = entt;
        ++available;

        for(auto &&cpool: pools) {
            if(cpool && cpool->has(entity)) {
                cpool->destroy(entity);
            }
        }
    }

    /**
     * @brief Attaches a tag to an entity.
     *
     * Usually, pools of components allocate enough memory to store a bunch of
     * elements even if only one of them is used. On the other hand, there are
     * cases where all what is needed is a single instance component to attach
     * to an entity.<br/>
     * Tags are the right tool to achieve the purpose.
     *
     * @warning
     * Attempting to use an invalid entity or to attach to an entity a tag that
     * already has an owner results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the tag has been already attached to another entity.
     *
     * @tparam Tag Type of tag to create.
     * @tparam Args Types of arguments to use to construct the tag.
     * @param entity A valid entity identifier
     * @param args Parameters to use to initialize the tag.
     * @return A reference to the newly created tag.
     */
    template<typename Tag, typename... Args>
    Tag & attach(entity_type entity, Args &&... args) {
        assert(valid(entity));
        assert(!has<Tag>());
        const auto ttype = tag_family::type<Tag>();

        if(!(ttype < tags.size())) {
            tags.resize(ttype + 1);
        }

        tags[ttype].reset(new Attaching<Tag>{entity, Tag{std::forward<Args>(args)...}});

        return static_cast<Attaching<Tag> *>(tags[ttype].get())->tag;
    }

    /**
     * @brief Removes a tag from its owner, if any.
     * @tparam Tag Type of tag to remove.
     */
    template<typename Tag>
    void remove() {
        if(has<Tag>()) {
            tags[tag_family::type<Tag>()].reset();
        }
    }

    /**
     * @brief Checks if a tag has an owner.
     * @tparam Tag Type of tag for which to perform the check.
     * @return True if the tag already has an owner, false otherwise.
     */
    template<typename Tag>
    bool has() const noexcept {
        const auto ttype = tag_family::type<Tag>();
        return (ttype < tags.size() &&
                // it's a valid tag
                tags[ttype] &&
                // the associated entity hasn't been destroyed in the meantime
                tags[ttype]->entity == (entities[tags[ttype]->entity & traits_type::entity_mask]));
    }

    /**
     * @brief Returns a reference to a tag.
     *
     * @warning
     * Attempting to get a tag that hasn't an owner results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * tag hasn't been previously attached to an entity.
     *
     * @tparam Tag Type of tag to get.
     * @return A reference to the tag.
     */
    template<typename Tag>
    const Tag & get() const noexcept {
        assert(has<Tag>());
        return static_cast<Attaching<Tag> *>(tags[tag_family::type<Tag>()].get())->tag;
    }

    /**
     * @brief Returns a reference to a tag.
     *
     * @warning
     * Attempting to get a tag that hasn't an owner results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * tag hasn't been previously attached to an entity.
     *
     * @tparam Tag Type of tag to get.
     * @return A reference to the tag.
     */
    template<typename Tag>
    Tag & get() noexcept {
        return const_cast<Tag &>(const_cast<const Registry *>(this)->get<Tag>());
    }

    /**
     * @brief Gets the owner of a tag, if any.
     *
     * @warning
     * Attempting to get the owner of a tag that hasn't been previously attached
     * to an entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * tag hasn't an owner.
     *
     * @tparam Tag Type of tag of which to get the owner.
     * @return A valid entity identifier.
     */
    template<typename Tag>
    entity_type attachee() const noexcept {
        assert(has<Tag>());
        return tags[tag_family::type<Tag>()]->entity;
    }

    /**
     * @brief Assigns the given component to an entity.
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
     * @tparam Component Type of component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & assign(entity_type entity, Args &&... args) {
        assert(valid(entity));
        return ensure<Component>().construct(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to remove a component from an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to remove.
     * @param entity A valid entity identifier.
     */
    template<typename Component>
    void remove(entity_type entity) {
        assert(valid(entity));
        pool<Component>().destroy(entity);
    }

    /**
     * @brief Checks if an entity has all the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has all the components, false otherwise.
     */
    template<typename... Component>
    bool has(entity_type entity) const noexcept {
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { all, (all = all && managed<Component>() && pool<Component>().has(entity))... };
        (void)accumulator;
        return all;
    }

    /**
     * @brief Returns a reference to the given component for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to get.
     * @param entity A valid entity identifier.
     * @return A reference to the component owned by the entity.
     */
    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return pool<Component>().get(entity);
    }

    /**
     * @brief Returns a reference to the given component for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to get.
     * @param entity A valid entity identifier.
     * @return A reference to the component owned by the entity.
     */
    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const Registry *>(this)->get<Component>(entity));
    }

    /**
     * @brief Returns a reference to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get components from an entity
     * that doesn't own them results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own instances of the given
     * components.
     *
     * @tparam Component Type of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    std::enable_if_t<(sizeof...(Component) > 1), std::tuple<const Component &...>>
    get(entity_type entity) const noexcept {
        return std::tuple<const Component &...>{get<Component>(entity)...};
    }

    /**
     * @brief Returns a reference to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get components from an entity
     * that doesn't own them results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own instances of the given
     * components.
     *
     * @tparam Component Type of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    std::enable_if_t<(sizeof...(Component) > 1), std::tuple<Component &...>>
    get(entity_type entity) noexcept {
        return std::tuple<Component &...>{get<Component>(entity)...};
    }

    /**
     * @brief Replaces the given component for an entity.
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
     * @tparam Component Type of component to replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & replace(entity_type entity, Args &&... args) {
        assert(valid(entity));
        return (pool<Component>().get(entity) = Component{std::forward<Args>(args)...});
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
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
     * Prefer this function anyway because it has slightly better
     * performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & accommodate(entity_type entity, Args &&... args) {
        assert(valid(entity));
        auto &cpool = ensure<Component>();

        return (cpool.has(entity)
                ? (cpool.get(entity) = Component{std::forward<Args>(args)...})
                : cpool.construct(*this, entity, std::forward<Args>(args)...));
    }

    /**
     * @brief Sorts the pool of entities for the given component.
     *
     * The order of the elements in a pool is highly affected by assignments
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
     * bool(const Component &, const Component &)
     * @endcode
     *
     * @tparam Component Type of components to sort.
     * @tparam Compare Type of comparison function object.
     * @param compare A valid comparison function object.
     */
    template<typename Component, typename Compare>
    void sort(Compare compare) {
        ensure<Component>().sort(std::move(compare));
    }

    /**
     * @brief Sorts two pools of components in the same way.
     *
     * The order of the elements in a pool is highly affected by assignments
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
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ensure<To>().respect(ensure<From>());
    }

    /**
     * @brief Resets the given component for an entity.
     *
     * If the entity has an instance of the component, this function removes the
     * component from the entity. Otherwise it does nothing.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to reset.
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
     * @tparam Component Type of component whose pool must be reset.
     */
    template<typename Component>
    void reset() {
        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            each([&cpool](auto entity) {
                if(cpool.has(entity)) {
                    cpool.destroy(entity);
                }
            });
        }
    }

    /**
     * @brief Resets a whole registry.
     *
     * Destroys all the entities. After a call to `reset`, all the entities
     * still in use are recycled with a new version number. In case entity
     * identifers are stored around, the `current` member function can be used
     * to know if they are still valid.
     */
    void reset() {
        each([this](auto entity) {
            destroy(entity);
        });
    }

    /**
     * @brief Iterates all the entities that are still in use.
     *
     * The function object is invoked for each entity that is still in use.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type);
     * @endcode
     *
     * This function is fairly slow and should not be used frequently.<br/>
     * Consider using a view if the goal is to iterate entities that have a
     * determinate set of components. A view is usually faster than combining
     * this function with a bunch of custom tests.<br/>
     * On the other side, this function can be used to iterate all the entities
     * that are in use, regardless of their components.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if(available) {
            for(auto pos = entities.size(); pos; --pos) {
                const entity_type curr = pos - 1;
                const auto entt = entities[curr] & traits_type::entity_mask;

                if(curr == entt) {
                    func(entities[curr]);
                }
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        }
    }

    /**
     * @brief Checks if an entity is an orphan.
     *
     * An orphan is an entity that has neither assigned components nor
     * tags.
     *
     * @param entity A valid entity identifier.
     * @return True if the entity is an orphan, false otherwise.
     */
    bool orphan(entity_type entity) const {
        assert(valid(entity));
        bool orphan = true;

        for(std::size_t i = 0; i < pools.size() && orphan; ++i) {
            const auto &pool = pools[i];
            orphan = !(pool && pool->has(entity));
        }

        for(std::size_t i = 0; i < tags.size() && orphan; ++i) {
            const auto &tag = tags[i];
            orphan = !(tag && (tag->entity == entity));
        }

        return orphan;
    }

    /**
     * @brief Iterates orphans and applies them the given function object.
     *
     * The function object is invoked for each entity that is still in use and
     * has neither assigned components nor tags.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type);
     * @endcode
     *
     * This function can be very slow and should not be used frequently.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void orphans(Func func) const {
        each([func = std::move(func), this](auto entity) {
            if(orphan(entity)) {
                func(entity);
            }
        });
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
     * entities. In particular:
     *
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
     * @tparam Component Type of components used to construct the view.
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
     * that would have been used otherwise to construct the view.
     *
     * @tparam Component Types of components used to prepare the view.
     */
    template<typename... Component>
    void prepare() {
        handler<Component...>();
    }

    /**
     * @brief Discards all the data structures used for a given persitent view.
     *
     * Persistent views occupy memory, no matter if they are in use or not.<br/>
     * This function can be used to discard all the internal data structures
     * dedicated to a specific persisten view, with the goal of reducing the
     * memory pressure.
     *
     * @warning
     * Attempting to use a persistent view created before calling this function
     * results in undefined behavior. No assertion available in this case,
     * neither in debug mode nor in release mode.
     *
     * @tparam Component Types of components of the persistent view.
     */
    template<typename... Component>
    void discard() {
        if(contains<Component...>()) {
            using accumulator_type = int[];
            const auto vtype = view_family::type<Component...>();
            auto *set = handlers[vtype].get();
            // if a set exists, pools have already been created for it
            accumulator_type accumulator = { (pool<Component>().remove(set), 0)... };
            handlers[vtype].reset();
            (void)accumulator;
        }
    }

    /**
     * @brief Checks if a persistent view has already been prepared.
     * @tparam Component Types of components of the persistent view.
     * @return True if the view has already been prepared, false otherwise.
     */
    template<typename... Component>
    bool contains() const noexcept {
        static_assert(sizeof...(Component) > 1, "!");
        const auto vtype = view_family::type<Component...>();
        return vtype < handlers.size() && handlers[vtype];
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
     * Persistent views are the right choice to iterate entities when the number
     * of components grows up and the most of the entities have all the given
     * components.<br/>
     * However they have also drawbacks:
     *
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
     * @tparam Component Types of components used to construct the view.
     * @return A newly created persistent view.
     */
    template<typename... Component>
    PersistentView<Entity, Component...> persistent() {
        // after the calls to handler, pools have already been created
        return PersistentView<Entity, Component...>{handler<Component...>(), pool<Component>()...};
    }

private:
    std::vector<std::unique_ptr<SparseSet<Entity>>> handlers;
    std::vector<std::unique_ptr<SparseSet<Entity>>> pools;
    std::vector<std::unique_ptr<Attachee>> tags;
    std::vector<entity_type> entities;
    size_type available{};
    entity_type next{};
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
