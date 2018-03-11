#ifndef ENTT_ENTITY_SPACE_HPP
#define ENTT_ENTITY_SPACE_HPP


#include <utility>
#include "sparse_set.hpp"
#include "registry.hpp"


namespace entt {


/**
 * @brief A space is a sort of partition of a registry.
 *
 * Spaces can be used to create partitions of a registry. They can be useful for
 * logically separating menus, world and any other type of scene, while still
 * using only one registry.<br/>
 * Similar results are obtained either using multiple registries or using
 * dedicated components, even though in both cases the memory usage isn't the
 * same. On the other side, spaces can introduce performance hits that are
 * sometimes unacceptable (mainly if you are working on AAA games or kind of).
 *
 * For more details about spaces and their use, take a look at this article:
 * https://gamedevelopment.tutsplus.com/tutorials/spaces-useful-game-object-containers--gamedev-14091
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Space: private SparseSet<Entity> {
    using view_type = SparseSet<Entity>;

    template<typename View, typename Func>
    inline void each(View view, Func func) {
        // use the view to iterate so as to respect order of components if any
        view.each([func = std::move(func), this](auto entity, auto &&... components) {
            if(this->has(entity)) {
                if(this->data()[this->get(entity)] == entity) {
                    func(entity, std::forward<decltype(components)>(components)...);
                } else {
                    // lazy destroy to avoid keeping a space in sync
                    this->destroy(entity);
                }
            }
        });
    }

public:
    /*! @brief Type of registry to which the space refers. */
    using registry_type = Registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename view_type::iterator_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;

    /**
     * @brief Constructs a space by using the given registry.
     * @param registry An entity-component system properly initialized.
     */
    Space(registry_type &registry)
        : registry{registry}
    {}

    /**
     * @brief Returns the number of entities tracked by a space.
     * @return Number of entities tracked by the space.
     */
    size_type size() const noexcept {
        return SparseSet<Entity>::size();
    }

    /**
     * @brief Checks if there exists at least an entity tracked by a space.
     * @return True if the space tracks at least an entity, false otherwise.
     */
    bool empty() const noexcept {
        return SparseSet<Entity>::empty();
    }

    /**
     * @brief Returns an iterator to the first entity tracked by a space.
     *
     * The returned iterator points to the first entity tracked by the space. If
     * the space is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity tracked by a space.
     */
    iterator_type begin() const noexcept {
        return SparseSet<Entity>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity tracked by a
     * space.
     *
     * The returned iterator points to the entity following the last entity
     * tracked by the space. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity tracked by a
     * space.
     */
    iterator_type end() const noexcept {
        return SparseSet<Entity>::end();
    }

    /**
     * @brief Checks if a space contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the space contains the given entity, false otherwise.
     */
    bool contains(entity_type entity) const noexcept {
        return this->has(entity) && this->data()[this->get(entity)] == entity;
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * The space creates an entity from the underlying registry and registers it
     * immediately before to return the identifier. Use the `assign` member
     * function to register an already existent entity created at a different
     * time.
     *
     * The returned entity has no components assigned.
     *
     * @return A valid entity identifier.
     */
    entity_type create() {
        const auto entity = registry.create();
        assign(entity);
        return entity;
    }

    /**
     * @brief Assigns an entity to a space.
     *
     * The space starts tracking the given entity and will return it during
     * iterations whenever required.<br/>
     * Entities can be assigned to more than one space at the same time.
     *
     * @warning
     * Attempting to use an invalid entity or to assign an entity that doesn't
     * belong to the underlying registry results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the registry doesn't own the entity.
     *
     * @param entity A valid entity identifier.
     */
    void assign(entity_type entity) {
        assert(registry.valid(entity));

        if(this->has(entity)) {
            this->destroy(entity);
        }

        this->construct(entity);
    }

    /**
     * @brief Removes an entity from a space.
     *
     * The space stops tracking the given entity and won't return it anymore
     * during iterations.<br/>
     * In case the entity belongs to more than one space, it won't be removed
     * automatically from all the other ones as a consequence of invoking this
     * function.
     *
     * @param entity A valid entity identifier.
     */
    void remove(entity_type entity) {
        if(this->has(entity)) {
            this->destroy(entity);
        }
    }

    /**
     * @brief Iterates entities using a standard view under the hood.
     *
     * A space does not return directly views to iterate entities because it
     * requires to apply a filter to those sets. Instead, it uses a view
     * internally and returns only those entities that are tracked by the space
     * itself.<br/>
     * This member function can be used to iterate a space by means of a
     * standard view. Naming the function the same as the type of view used to
     * perform the task proved to be a good choice so as not to tricky users.
     *
     * @note
     * Performance tend to degenerate when the number of components to iterate
     * grows up and the most of the entities have all the given components.<br/>
     * To get a performance boost, consider using the `persistent` member
     * function instead.
     *
     * @tparam Component Type of components used to construct the view.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename... Component, typename Func>
    void view(Func func) {
        each(registry.template view<Component...>(), std::move(func));
    }

    /**
     * @brief Iterates entities using a persistent view under the hood.
     *
     * A space does not return directly views to iterate entities because it
     * requires to apply a filter to those sets. Instead, it uses a view
     * internally and returns only those entities that are tracked by the space
     * itself.<br/>
     * This member function can be used to iterate a space by means of a
     * persistent view. Naming the function the same as the type of view used to
     * perform the task proved to be a good choice so as not to tricky users.
     *
     * @tparam Component Type of components used to construct the view.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename... Component, typename Func>
    void persistent(Func func) {
        each(registry.template persistent<Component...>(), std::move(func));
    }

    /**
     * @brief Performs a clean up step.
     *
     * Spaces do a lazy cleanup during iterations to avoid introducing
     * performance hits when entities are destroyed.<br/>
     * This function can be used to force a clean up step and to get rid of all
     * those entities that are still tracked by a space but have been destroyed
     * in the underlying registry.
     */
    void shrink() {
        for(auto entity: *this) {
            if(!registry.fast(entity)) {
                this->destroy(entity);
            }
        }
    }

    /**
     * @brief Resets a whole space.
     *
     * The space stops tracking all the entities assigned to it so far. After
     * calling this function, iterations won't return any entity.
     */
    void reset() {
        SparseSet<Entity>::reset();
    }

private:
    Registry<Entity> &registry;
};


/**
 * @brief Default space class.
 *
 * The default space is the best choice for almost all the applications.<br/>
 * Users should have a really good reason to choose something different.
 */
using DefaultSpace = Space<DefaultRegistry::entity_type>;


}


#endif // ENTT_ENTITY_SPACE_HPP
