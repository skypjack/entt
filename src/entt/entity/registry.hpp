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
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/family.hpp"
#include "../signal/sigh.hpp"
#include "entt_traits.hpp"
#include "snapshot.hpp"
#include "sparse_set.hpp"
#include "utility.hpp"
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
    using handler_family = Family<struct InternalRegistryHandlerFamily>;
    using signal_type = SigH<void(Registry &, const Entity)>;
    using traits_type = entt_traits<Entity>;

    template<typename... Component>
    static void creating(Registry &registry, const Entity entity) {
        if(registry.has<Component...>(entity)) {
            registry.handlers[handler_family::type<Component...>()]->construct(entity);
        }
    }

    template<typename... Component>
    static void destroying(Registry &registry, const Entity entity) {
        auto &handler = *registry.handlers[handler_family::type<Component...>()];
        return handler.has(entity) ? handler.destroy(entity) : void();
    }

    struct Attachee {
        Attachee(const Entity entity): entity{entity} {}
        virtual ~Attachee() = default;
        Entity entity;
    };

    template<typename Tag>
    struct Attaching: Attachee {
        // requirements for aggregates are relaxed only since C++17
        template<typename... Args>
        Attaching(const Entity entity, Args &&... args)
            : Attachee{entity}, tag{std::forward<Args>(args)...}
        {}

        Tag tag;
    };

    template<typename Component>
    bool managed() const ENTT_NOEXCEPT {
        const auto ctype = component_family::type<Component>();
        return ctype < pools.size() && std::get<0>(pools[ctype]);
    }

    template<typename Component>
    const SparseSet<Entity, Component> & pool() const ENTT_NOEXCEPT {
        assert(managed<Component>());
        const auto ctype = component_family::type<Component>();
        return static_cast<SparseSet<Entity, Component> &>(*std::get<0>(pools[ctype]));
    }

    template<typename Component>
    inline SparseSet<Entity, Component> & pool() ENTT_NOEXCEPT {
        return const_cast<SparseSet<Entity, Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    void assure() {
        const auto ctype = component_family::type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        auto &cpool = std::get<0>(pools[ctype]);

        if(!cpool) {
            cpool = std::make_unique<SparseSet<Entity, Component>>();
        }
    }

    template<typename Tag>
    void assure(tag_t) {
        const auto ttype = tag_family::type<Tag>();

        if(!(ttype < tags.size())) {
            tags.resize(ttype + 1);
        }
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
    /*! @brief Type of sink for the given component. */
    using sink_type = typename signal_type::sink_type;

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
    static tag_type type(tag_t) ENTT_NOEXCEPT {
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
    static component_type type() ENTT_NOEXCEPT {
        return component_family::type<Component>();
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        return managed<Component>() ? pool<Component>().size() : size_type{};
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return Number of entities still in use.
     */
    size_type size() const ENTT_NOEXCEPT {
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
    void reserve(const size_type cap) {
        assure<Component>();
        pool<Component>().reserve(cap);
    }

    /**
     * @brief Increases the capacity of a registry in terms of entities.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        entities.reserve(cap);
    }

    /**
     * @brief Returns the number of entities ever created.
     * @return Number of entities ever created.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Checks whether the pool for the given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool for the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        return !managed<Component>() || pool<Component>().empty();
    }

    /**
     * @brief Checks if there exists at least an entity still in use.
     * @return True if at least an entity is still in use, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return entities.size() == available;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    const Component * raw() const ENTT_NOEXCEPT {
        return managed<Component>() ? pool<Component>().raw() : nullptr;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    inline Component * raw() ENTT_NOEXCEPT {
        return const_cast<Component *>(const_cast<const Registry *>(this)->raw<Component>());
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        return managed<Component>() ? pool<Component>().data() : nullptr;
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    bool valid(const entity_type entity) const ENTT_NOEXCEPT {
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
    bool fast(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        assert(pos < entities.size());
        return (entities[pos] == entity);
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return Version stored along with the given entity identifier.
     */
    version_type version(const entity_type entity) const ENTT_NOEXCEPT {
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
    version_type current(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        assert(pos < entities.size());
        return version_type((entities[pos] >> traits_type::entity_shift) & traits_type::version_mask);
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * Users should not care about the type of the returned entity identifier.
     * In case entity identifers are stored around, the `valid` member
     * function can be used to know if they are still valid or the entity has
     * been destroyed and potentially recycled.
     *
     * The returned entity has no components assigned.
     *
     * @return A valid entity identifier.
     */
    entity_type create() ENTT_NOEXCEPT {
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
            entities.push_back(entity);
            // traits_type::entity_mask is reserved to allow for null identifiers
            assert(entity < traits_type::entity_mask);
        }

        return entity;
    }

    /**
     * @brief Destroys an entity and lets the registry recycle the identifier.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time. In case entity identifers are stored around,
     * the `valid` member function can be used to know if they are still valid
     * or the entity has been destroyed and potentially recycled.
     *
     * @warning
     * In case there are listeners that observe the destruction of components
     * and assign other components to the entity in their bodies, the result of
     * invoking this function may not be as expected. In the worst case, it
     * could lead to undefined behavior. An assertion will abort the execution
     * at runtime in debug mode if a violation is detected.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param entity A valid entity identifier
     */
    void destroy(const entity_type entity) {
        assert(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            auto &tup = pools[pos-1];
            auto &cpool = std::get<0>(tup);

            if(cpool && cpool->has(entity)) {
                std::get<2>(tup).publish(*this, entity);
                cpool->destroy(entity);
            }
        };

        for(auto pos = tags.size(); pos; --pos) {
            auto &tup = tags[pos-1];
            auto &tag = std::get<0>(tup);

            if(tag && tag->entity == entity) {
                std::get<2>(tup).publish(*this, entity);
                tag.reset();
            }
        };

        // just a way to protect users from listeners that attach components
        assert(orphan(entity));

        const auto entt = entity & traits_type::entity_mask;
        const auto version = (((entity >> traits_type::entity_shift) + 1) & traits_type::version_mask) << traits_type::entity_shift;
        const auto node = (available ? next : ((entt + 1) & traits_type::entity_mask)) | version;

        entities[entt] = node;
        next = entt;
        ++available;
    }

    /**
     * @brief Attaches the given tag to an entity.
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
    Tag & assign(tag_t, const entity_type entity, Args &&... args) {
        assert(valid(entity));
        assert(!has<Tag>());
        assure<Tag>(tag_t{});
        auto &tup = tags[tag_family::type<Tag>()];
        std::get<0>(tup).reset(new Attaching<Tag>{entity, std::forward<Args>(args)...});
        std::get<1>(tup).publish(*this, entity);
        return get<Tag>();
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
    Component & assign(const entity_type entity, Args &&... args) {
        assert(valid(entity));
        assure<Component>();
        pool<Component>().construct(entity, std::forward<Args>(args)...);
        std::get<1>(pools[component_family::type<Component>()]).publish(*this, entity);
        return pool<Component>().get(entity);
    }

    /**
     * @brief Removes the given tag from its owner, if any.
     * @tparam Tag Type of tag to remove.
     */
    template<typename Tag>
    void remove() {
        if(has<Tag>()) {
            auto &tup = tags[tag_family::type<Tag>()];
            auto &tag = std::get<0>(tup);
            std::get<2>(tup).publish(*this, tag->entity);
            tag.reset();
        }
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
    void remove(const entity_type entity) {
        assert(valid(entity));
        assert(managed<Component>());
        const auto ctype = component_family::type<Component>();
        std::get<2>(pools[ctype]).publish(*this, entity);
        pool<Component>().destroy(entity);
    }

    /**
     * @brief Checks if the given tag has an owner.
     * @tparam Tag Type of tag for which to perform the check.
     * @return True if the tag already has an owner, false otherwise.
     */
    template<typename Tag>
    bool has() const ENTT_NOEXCEPT {
        const auto ttype = tag_family::type<Tag>();
        bool found = false;

        if(ttype < tags.size()) {
            auto &tag = std::get<0>(tags[ttype]);
            // it's a valid tag and the associated entity hasn't been destroyed in the meantime
            found = tag && (tag->entity == (entities[tag->entity & traits_type::entity_mask]));
        }

        return found;
    }

    /**
     * @brief Checks if an entity owns the given tag.
     *
     * Syntactic sugar for the following snippet:
     *
     * @code{.cpp}
     * registry.has<Tag>() && registry.attachee<Tag>() == entity
     * @endcode
     *
     * @tparam Tag Type of tag for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity owns the tag, false otherwise.
     */
    template<typename Tag>
    bool has(tag_t, const entity_type entity) const ENTT_NOEXCEPT {
        return has<Tag>() && attachee<Tag>() == entity;
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
    bool has(const entity_type entity) const ENTT_NOEXCEPT {
        assert(valid(entity));
        bool all = true;
        using accumulator_type = bool[];
        accumulator_type accumulator = { all, (all = all && managed<Component>() && pool<Component>().has(entity))... };
        (void)accumulator;
        return all;
    }

    /**
     * @brief Returns a reference to the given tag.
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
    const Tag & get() const ENTT_NOEXCEPT {
        assert(has<Tag>());
        return static_cast<Attaching<Tag> *>(std::get<0>(tags[tag_family::type<Tag>()]).get())->tag;
    }

    /**
     * @brief Returns a reference to the given tag.
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
    inline Tag & get() ENTT_NOEXCEPT {
        return const_cast<Tag &>(const_cast<const Registry *>(this)->get<Tag>());
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
    const Component & get(const entity_type entity) const ENTT_NOEXCEPT {
        assert(valid(entity));
        assert(managed<Component>());
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
    inline Component & get(const entity_type entity) ENTT_NOEXCEPT {
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
    inline std::enable_if_t<(sizeof...(Component) > 1), std::tuple<const Component &...>>
    get(const entity_type entity) const ENTT_NOEXCEPT {
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
    inline std::enable_if_t<(sizeof...(Component) > 1), std::tuple<Component &...>>
    get(const entity_type entity) ENTT_NOEXCEPT {
        return std::tuple<Component &...>{get<Component>(entity)...};
    }

    /**
     * @brief Replaces the given tag.
     *
     * A new instance of the given tag is created and initialized with the
     * arguments provided (the tag must have a proper constructor or be of
     * aggregate type).
     *
     * @warning
     * Attempting to replace a tag that hasn't an owner results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * tag hasn't been previously attached to an entity.
     *
     * @tparam Tag Type of tag to replace.
     * @tparam Args Types of arguments to use to construct the tag.
     * @param args Parameters to use to initialize the tag.
     * @return A reference to the tag.
     */
    template<typename Tag, typename... Args>
    Tag & replace(tag_t, Args &&... args) {
        return (get<Tag>() = Tag{std::forward<Args>(args)...});
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
    Component & replace(const entity_type entity, Args &&... args) {
        return (get<Component>(entity) = Component{std::forward<Args>(args)...});
    }

    /**
     * @brief Changes the owner of the given tag.
     *
     * The ownership of the tag is transferred from one entity to another.
     *
     * @warning
     * Attempting to use an invalid entity or to transfer the ownership of a tag
     * that hasn't an owner results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the tag hasn't been previously attached to an
     * entity.
     *
     * @tparam Tag Type of tag of which to transfer the ownership.
     * @param entity A valid entity identifier.
     * @return A valid entity identifier.
     */
    template<typename Tag>
    entity_type move(const entity_type entity) {
        assert(valid(entity));
        assert(has<Tag>());
        auto &tag = std::get<0>(tags[tag_family::type<Tag>()]);
        const auto owner = tag->entity;
        tag->entity = entity;
        return owner;
    }

    /**
     * @brief Gets the owner of the given tag, if any.
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
    entity_type attachee() const ENTT_NOEXCEPT {
        assert(has<Tag>());
        return std::get<0>(tags[tag_family::type<Tag>()])->entity;
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
     * Prefer this function anyway because it has slightly better performance.
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
    Component & accommodate(const entity_type entity, Args &&... args) {
        assure<Component>();
        auto &cpool = pool<Component>();

        return (cpool.has(entity)
                ? cpool.get(entity) = Component{std::forward<Args>(args)...}
                : cpool.construct(entity, std::forward<Args>(args)...));
    }

    /**
     * @brief Returns a sink object for the given tag.
     *
     * A sink is an opaque object used to connect listeners to tags.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given tag is created and assigned to an
     * entity.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(Registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the tag has been assigned to the entity.
     * The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a new tag is created.
     *
     * @sa SigH::Sink
     *
     * @tparam Tag Type of tag of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Tag>
    sink_type construction(tag_t) ENTT_NOEXCEPT {
        assure<Tag>(tag_t{});
        return std::get<1>(tags[tag_family::type<Tag>()]).sink();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given component is created and assigned to
     * an entity.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(Registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been assigned to the
     * entity. The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a new component is created.
     *
     * @sa SigH::Sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    sink_type construction() ENTT_NOEXCEPT {
        assure<Component>();
        return std::get<1>(pools[component_family::type<Component>()]).sink();
    }

    /**
     * @brief Returns a sink object for the given tag.
     *
     * A sink is an opaque object used to connect listeners to tag.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given tag is removed from an entity and thus
     * destroyed.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(Registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** the tag has been removed from the
     * entity. The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a tag is destroyed.
     *
     * @sa SigH::Sink
     *
     * @tparam Tag Type of tag of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Tag>
    sink_type destruction(tag_t) ENTT_NOEXCEPT {
        assure<Tag>(tag_t{});
        return std::get<2>(tags[tag_family::type<Tag>()]).sink();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is removed from an entity and
     * thus destroyed.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(Registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** the component has been removed from the
     * entity. The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a component is destroyed.
     *
     * @sa SigH::Sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    sink_type destruction() ENTT_NOEXCEPT {
        assure<Component>();
        return std::get<2>(pools[component_family::type<Component>()]).sink();
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
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * The comparison funtion object received by the sort function object hasn't
     * necessarily the type of the one passed along with the other parameters to
     * this member function.
     *
     * @tparam Component Type of components to sort.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @param compare A valid comparison function object.
     * @param sort A valid sort function object.
     */
    template<typename Component, typename Compare, typename Sort = StdSort>
    void sort(Compare compare, Sort sort = Sort{}) {
        assure<Component>();
        pool<Component>().sort(std::move(compare), std::move(sort));
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
     *   according to the order they have in `B`.
     * * All the entities in `A` that are not in `B` are returned in no
     *   particular order after all the other entities.
     *
     * Any subsequent change to `B` won't affect the order in `A`.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        assure<To>();
        assure<From>();
        pool<To>().respect(pool<From>());
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
    void reset(const entity_type entity) {
        assert(valid(entity));
        assure<Component>();
        const auto ctype = component_family::type<Component>();
        auto &cpool = *std::get<0>(pools[ctype]);

        if(cpool.has(entity)) {
            std::get<2>(pools[ctype]).publish(*this, entity);
            cpool.destroy(entity);
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
        assure<Component>();
        const auto ctype = component_family::type<Component>();
        auto &cpool = *std::get<0>(pools[ctype]);
        auto &sig = std::get<2>(pools[ctype]);

        for(const auto entity: cpool) {
            sig.publish(*this, entity);
            cpool.destroy(entity);
        }
    }

    /**
     * @brief Resets a whole registry.
     *
     * Destroys all the entities. After a call to `reset`, all the entities
     * still in use are recycled with a new version number. In case entity
     * identifers are stored around, the `valid` member function can be used
     * to know if they are still valid.
     */
    void reset() {
        each([this](const auto entity) {
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
     * void(const entity_type);
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
                const auto entity = entities[curr];
                const auto entt = entity & traits_type::entity_mask;

                if(curr == entt) {
                    func(entity);
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
    bool orphan(const entity_type entity) const {
        assert(valid(entity));
        bool orphan = true;

        for(std::size_t i = 0; i < pools.size() && orphan; ++i) {
            const auto &pool = std::get<0>(pools[i]);
            orphan = !(pool && pool->has(entity));
        }

        for(std::size_t i = 0; i < tags.size() && orphan; ++i) {
            const auto &tag = std::get<0>(tags[i]);
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
     * void(const entity_type);
     * @endcode
     *
     * This function can be very slow and should not be used frequently.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void orphans(Func func) const {
        each([func = std::move(func), this](const auto entity) {
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
     *   of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     *   component and pick up a reference to the smallest set of candidates to
     *   test for the given components.
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
     * @see RawView
     *
     * @tparam Component Type of components used to construct the view.
     * @return A newly created standard view.
     */
    template<typename... Component>
    View<Entity, Component...> view() {
        return View<Entity, Component...>{(assure<Component>(), pool<Component>())...};
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
        static_assert(sizeof...(Component) > 1, "!");
        const auto htype = handler_family::type<Component...>();

        if(!(htype < handlers.size())) {
            handlers.resize(htype + 1);
        }

        if(!handlers[htype]) {
            handlers[htype] = std::make_unique<SparseSet<entity_type>>();
            auto &handler = handlers[htype];

            for(auto entity: view<Component...>()) {
                handler->construct(entity);
            }

            auto connect = [this](const auto ctype) {
                auto &cpool = pools[ctype];
                std::get<1>(cpool).sink().template connect<&Registry::creating<Component...>>();
                std::get<2>(cpool).sink().template connect<&Registry::destroying<Component...>>();
            };

            using accumulator_type = int[];
            accumulator_type accumulator = { (assure<Component>(), connect(component_family::type<Component>()), 0)... };
            (void)accumulator;
        }
    }

    /**
     * @brief Discards all the data structures used for a given persitent view.
     *
     * Persistent views occupy memory, no matter if they are in use or not.<br/>
     * This function can be used to discard all the internal data structures
     * dedicated to a specific persistent view, with the goal of reducing the
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
            const auto htype = handler_family::type<Component...>();

            auto disconnect = [this](const auto ctype) {
                auto &cpool = pools[ctype];
                std::get<1>(cpool).sink().template disconnect<&Registry::creating<Component...>>();
                std::get<2>(cpool).sink().template disconnect<&Registry::destroying<Component...>>();
            };

            // if a set exists, pools have already been created for it
            using accumulator_type = int[];
            accumulator_type accumulator = { (disconnect(component_family::type<Component>()), 0)... };
            handlers[htype].reset();
            (void)accumulator;
        }
    }

    /**
     * @brief Checks if a persistent view has already been prepared.
     * @tparam Component Types of components of the persistent view.
     * @return True if the view has already been prepared, false otherwise.
     */
    template<typename... Component>
    bool contains() const ENTT_NOEXCEPT {
        static_assert(sizeof...(Component) > 1, "!");
        const auto htype = handler_family::type<Component...>();
        return (htype < handlers.size() && handlers[htype]);
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
     *   is allocated within the registry and it increases memory pressure.
     * * Internal data structures used to construct persistent views must be
     *   kept updated and it affects slightly construction and destruction of
     *   entities and components.
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
     * @see RawView
     *
     * @tparam Component Types of components used to construct the view.
     * @return A newly created persistent view.
     */
    template<typename... Component>
    PersistentView<Entity, Component...> view(persistent_t) {
        prepare<Component...>();
        const auto htype = handler_family::type<Component...>();
        return PersistentView<Entity, Component...>{*handlers[htype], (assure<Component>(), pool<Component>())...};
    }

    /**
     * @brief Returns a raw view for the given component.
     *
     * This kind of views are created on the fly and share with the registry its
     * internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Raw views are incredibly fast and must be considered the best tool to
     * iterate components whenever knowing the entities to which they belong
     * isn't required.
     *
     * @see View
     * @see View<Entity, Component>
     * @see PersistentView
     * @see RawView
     *
     * @tparam Component Type of component used to construct the view.
     * @return A newly created raw view.
     */
    template<typename Component>
    RawView<Entity, Component> view(raw_t) {
        assure<Component>();
        return RawView<Entity, Component>{pool<Component>()};
    }

    /**
     * @brief Returns a temporary object to use to create snapshots.
     *
     * A snapshot is either a full or a partial dump of a registry.<br/>
     * It can be used to save and restore its internal state or to keep two or
     * more instances of this class in sync, as an example in a client-server
     * architecture.
     *
     * @return A temporary object to use to take snasphosts.
     */
    Snapshot<Entity> snapshot() const {
        using follow_fn_type = entity_type(*)(const Registry &, const entity_type);
        const entity_type seed = available ? (next | (entities[next] & ~traits_type::entity_mask)) : next;

        follow_fn_type follow = [](const Registry &registry, const entity_type entity) -> entity_type {
            const auto &entities = registry.entities;
            const auto entt = entity & traits_type::entity_mask;
            const auto next = entities[entt] & traits_type::entity_mask;
            return (next | (entities[next] & ~traits_type::entity_mask));
        };

        return { *this, seed, follow };
    }

    /**
     * @brief Returns a temporary object to use to load snapshots.
     *
     * A snapshot is either a full or a partial dump of a registry.<br/>
     * It can be used to save and restore its internal state or to keep two or
     * more instances of this class in sync, as an example in a client-server
     * architecture.
     *
     * @warning
     * The loader returned by this function requires that the registry be empty.
     * In case it isn't, all the data will be automatically deleted before to
     * return.
     *
     * @return A temporary object to use to load snasphosts.
     */
    SnapshotLoader<Entity> restore() {
        using assure_fn_type = void(*)(Registry &, const entity_type, const bool);

        assure_fn_type assure = [](Registry &registry, const entity_type entity, const bool destroyed) {
            using promotion_type = std::conditional_t<sizeof(size_type) >= sizeof(entity_type), size_type, entity_type>;
            // explicit promotion to avoid warnings with std::uint16_t
            const auto entt = promotion_type{entity} & traits_type::entity_mask;
            auto &entities = registry.entities;

            if(!(entt < entities.size())) {
                auto curr = entities.size();
                entities.resize(entt + 1);
                std::iota(entities.data() + curr, entities.data() + entt, entity_type(curr));
            }

            entities[entt] = entity;

            if(destroyed) {
                registry.destroy(entity);
                const auto version = (entity & (~traits_type::entity_mask));
                entities[entt] = ((entities[entt] & traits_type::entity_mask) | version);
            }
        };

        return { (*this = {}), assure };
    }

private:
    std::vector<std::unique_ptr<SparseSet<Entity>>> handlers;
    std::vector<std::tuple<std::unique_ptr<SparseSet<Entity>>, signal_type, signal_type>> pools;
    std::vector<std::tuple<std::unique_ptr<Attachee>, signal_type, signal_type>> tags;
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
