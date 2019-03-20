#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/family.hpp"
#include "../core/hashed_string.hpp"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "entt_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "snapshot.hpp"
#include "sparse_set.hpp"
#include "view.hpp"


namespace entt {


/**
 * @brief Alias for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
struct exclude_t: type_list<Type...> {};


/**
 * @brief Variable template for exclusion lists.
 * @tparam Type List of types.
 */
template<typename... Type>
constexpr exclude_t<Type...> exclude{};


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
class basic_registry {
    using component_family = family<struct internal_registry_component_family>;
    using signal_type = sigh<void(basic_registry &, const Entity)>;
    using traits_type = entt_traits<Entity>;

    template<typename Component>
    struct pool_wrapper: sparse_set<Entity, Component> {
        template<typename... Args>
        Component & construct(Entity entt, Args &&... args) {
            auto &component = sparse_set<Entity, Component>::construct(entt, std::forward<Args>(args)...);
            construction.publish(*owner, entt);
            return component;
        }

        template<typename It>
        Component * batch(It first, It last, typename sparse_set<Entity>::size_type hint) {
            auto *component = sparse_set<Entity, Component>::batch(first, last, hint);

            if(!construction.empty()) {
                std::for_each(first, last, [this](const auto entt) {
                    construction.publish(*owner, entt);
                });
            }

            return component;
        }

        void destroy(Entity entt) override {
            destruction.publish(*owner, entt);
            sparse_set<Entity, Component>::destroy(entt);
        }

        signal_type construction;
        signal_type destruction;
        basic_registry *owner;
    };

    template<typename Component>
    using pool_type = pool_wrapper<std::decay_t<Component>>;

    template<typename, typename>
    struct non_owning_group;

    template<typename... Get, typename... Exclude>
    struct non_owning_group<type_list<Exclude...>, type_list<Get...>>: sparse_set<Entity> {
        template<auto Accepted>
        void construct_if(basic_registry &reg, const Entity entt) {
            if(reg.has<Get...>(entt) && (0 + ... + reg.has<Exclude>(entt)) == Accepted) {
                this->construct(entt);
            }
        }

        void destroy_if(basic_registry &, const Entity entt) {
            if(this->has(entt)) {
                this->destroy(entt);
            }
        }
    };

    struct boxed_owned {
        std::size_t owned;
    };

    template<typename...>
    struct owning_group;

    template<typename... Owned, typename... Get, typename... Exclude>
    struct owning_group<type_list<Exclude...>, type_list<Get...>, Owned...>: boxed_owned {
        template<auto Accepted>
        void induce_if(basic_registry &reg, const Entity entt) {
            if(reg.has<Owned..., Get...>(entt) && (0 + ... + reg.has<Exclude>(entt)) == Accepted) {
                const auto curr = this->owned++;
                const auto cpools = std::make_tuple(reg.pool<Owned>()...);
                (std::swap(std::get<pool_type<Owned> *>(cpools)->get(entt), std::get<pool_type<Owned> *>(cpools)->raw()[curr]), ...);
                (std::get<pool_type<Owned> *>(cpools)->swap(std::get<pool_type<Owned> *>(cpools)->sparse_set<Entity>::get(entt), curr), ...);
            }
        }

        void discard_if(basic_registry &reg, const Entity entt) {
            const auto cpools = std::make_tuple(reg.pool<Owned>()...);

            if(std::get<0>(cpools)->has(entt) && std::get<0>(cpools)->sparse_set<Entity>::get(entt) < this->owned) {
                const auto curr = --this->owned;
                (std::swap(std::get<pool_type<Owned> *>(cpools)->get(entt), std::get<pool_type<Owned> *>(cpools)->raw()[curr]), ...);
                (std::get<pool_type<Owned> *>(cpools)->swap(std::get<pool_type<Owned> *>(cpools)->sparse_set<Entity>::get(entt), curr), ...);
            }
        }
    };

    struct pool_data {
        std::unique_ptr<sparse_set<Entity>> pool;
        ENTT_ID_TYPE runtime_type;
    };

    struct owning_group_data {
        using match_fn_type = bool(ENTT_ID_TYPE);
        std::unique_ptr<boxed_owned> data;
        match_fn_type *exclude;
        match_fn_type *get;
        match_fn_type *owned;
        std::size_t extent;
    };

    struct non_owning_group_data {
        using match_fn_type = bool(ENTT_ID_TYPE);
        std::unique_ptr<sparse_set<Entity>> data;
        match_fn_type *exclude;
        match_fn_type *get;
        std::size_t extent;
    };

    void release(const Entity entity) {
        // lengthens the implicit list of destroyed entities
        const auto entt = entity & traits_type::entity_mask;
        const auto version = ((entity >> traits_type::entity_shift) + 1) << traits_type::entity_shift;
        const auto node = (available ? next : ((entt + 1) & traits_type::entity_mask)) | version;
        entities[entt] = node;
        next = entt;
        ++available;
    }

    template<typename Component>
    inline const auto * pool() const ENTT_NOEXCEPT {
        const auto ctype = type<Component>();

        if constexpr(is_named_type_v<Component>) {
            const auto it = std::find_if(pools.begin(), pools.end(), [ctype](const auto &candidate) {
                return candidate.pool && candidate.runtime_type == ctype;
            });

            return (it != pools.cend() && it->pool)
                    ? static_cast<const pool_type<Component> *>(it->pool.get())
                    : nullptr;
        } else {
            return (ctype < pools.size() && pools[ctype].pool && pools[ctype].runtime_type == ctype)
                    ? static_cast<const pool_type<Component> *>(pools[ctype].pool.get())
                    : nullptr;
        }
    }

    template<typename Component>
    inline auto * pool() ENTT_NOEXCEPT {
        return const_cast<pool_type<Component> *>(std::as_const(*this).template pool<Component>());
    }

    template<typename Component>
    auto * assure() {
        const auto ctype = type<Component>();
        pool_data *pdata = nullptr;

        if constexpr(is_named_type_v<Component>) {
            const auto it = std::find_if(pools.begin(), pools.end(), [ctype](const auto &candidate) {
                return candidate.pool && candidate.runtime_type == ctype;
            });

            pdata = (it == pools.cend() ? &pools.emplace_back() : &(*it));
        } else {
            if(!(ctype < pools.size())) {
                pools.resize(ctype+1);
            }

            pdata = &pools[ctype];

            if(pdata->pool && pdata->runtime_type != ctype) {
                pools.emplace_back();
                std::swap(pools[ctype], pools.back());
                pdata = &pools[ctype];
            }
        }

        if(!pdata->pool) {
            pdata->pool = std::make_unique<pool_type<Component>>();
            static_cast<pool_type<Component> &>(*pdata->pool).owner = this;
            pdata->runtime_type = ctype;
        }

        return static_cast<pool_type<Component> *>(pdata->pool.get());
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::entity_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Unsigned integer type. */
    using component_type = ENTT_ID_TYPE;
    /*! @brief Type of sink for the given component. */
    using sink_type = typename signal_type::sink_type;

    /*! @brief Default constructor. */
    basic_registry() ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;

    /**
     * @brief Returns the numeric identifier of a component.
     *
     * The given component doesn't need to be necessarily in use.<br/>
     * Do not use this functionality to provide numeric identifiers to types at
     * runtime, because they aren't guaranteed to be stable between different
     * runs.
     *
     * @tparam Component Type of component to query.
     * @return Runtime numeric identifier of the given type of component.
     */
    template<typename Component>
    static component_type type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Component>) {
            return named_type_traits<Component>::value;
        } else {
            return component_family::type<Component>;
        }
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->size() : size_type{};
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    size_type size() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return Number of entities still in use.
     */
    size_type alive() const ENTT_NOEXCEPT {
        return entities.size() - available;
    }

    /**
     * @brief Increases the capacity of the pool for the given component.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @tparam Component Type of component for which to reserve storage.
     * @param cap Desired capacity.
     */
    template<typename Component>
    void reserve(const size_type cap) {
        assure<Component>()->reserve(cap);
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
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    size_type capacity() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->capacity() : size_type{};
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return entities.capacity();
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->empty() : true;
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
     * @note
     * Empty components aren't explicitly instantiated. Only one instance of the
     * given type is created. Therefore, this function always returns a pointer
     * to that instance.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    const Component * raw() const ENTT_NOEXCEPT {
        const auto *cpool = pool<Component>();
        return cpool ? cpool->raw() : nullptr;
    }

    /*! @copydoc raw */
    template<typename Component>
    inline Component * raw() ENTT_NOEXCEPT {
        return const_cast<Component *>(std::as_const(*this).template raw<Component>());
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
        const auto *cpool = pool<Component>();
        return cpool ? cpool->data() : nullptr;
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
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    entity_type entity(const entity_type entity) const ENTT_NOEXCEPT {
        return entity & traits_type::entity_mask;
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    version_type version(const entity_type entity) const ENTT_NOEXCEPT {
        return version_type(entity >> traits_type::entity_shift);
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
        return version_type(entities[pos] >> traits_type::entity_shift);
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
     * The returned entity has assigned the given components, if any. The
     * components must be at least default constructible. A compilation error
     * will occur otherwhise.
     *
     * @tparam Component Types of components to assign to the entity.
     * @return A valid entity identifier if the component list is empty, a tuple
     * containing the entity identifier and the references to the components
     * just created otherwise.
     */
    template<typename... Component>
    std::conditional_t<sizeof...(Component) == 0, entity_type, std::tuple<entity_type, Component &...>>
    create() {
        entity_type entity;

        if(available) {
            const auto entt = next;
            const auto version = entities[entt] & (traits_type::version_mask << traits_type::entity_shift);
            next = entities[entt] & traits_type::entity_mask;
            entity = entt | version;
            entities[entt] = entity;
            --available;
        } else {
            entity = entities.emplace_back(entity_type(entities.size()));
            // traits_type::entity_mask is reserved to allow for null identifiers
            assert(entity < traits_type::entity_mask);
        }

        if constexpr(sizeof...(Component) == 0) {
            return entity;
        } else {
            return { entity, assign<Component>(entity)... };
        }
    }

    /**
     * @brief Assigns each element in a range an entity.
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
     * The entities so generated have assigned the given components, if any. The
     * components must be at least default constructible. A compilation error
     * will occur otherwhise.
     *
     * @tparam Component Types of components to assign to the entity.
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     * @return No return value if the component list is empty, a tuple
     * containing the pointers to the arrays of components just created and
     * sorted the same of the entities otherwise.
     */
    template<typename... Component, typename It>
    std::conditional_t<sizeof...(Component) == 0, void, std::tuple<Component *...>>
    create(It first, It last) {
        static_assert(std::is_convertible_v<entity_type, typename std::iterator_traits<It>::value_type>);
        const auto length = size_type(std::distance(first, last));
        const auto sz = std::min(available, length);
        [[maybe_unused]] entity_type candidate{};

        available -= sz;

        const auto tail = std::generate_n(first, sz, [&candidate, this]() mutable {
            if constexpr(sizeof...(Component) > 0) {
                candidate = std::max(candidate, next);
            } else {
                // suppress warnings
                (void)candidate;
            }

            const auto entt = next;
            const auto version = entities[entt] & (traits_type::version_mask << traits_type::entity_shift);
            next = entities[entt] & traits_type::entity_mask;
            return (entities[entt] = entt | version);
        });

        std::generate(tail, last, [this]() {
            return entities.emplace_back(entity_type(entities.size()));
        });

        if constexpr(sizeof...(Component) > 0) {
            const auto hint = size_type(std::max(candidate, *(last-1)))+1;
            return { assure<Component>()->batch(first, last, hint)... };
        }
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
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        assert(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            auto &pdata = pools[pos-1];

            if(pdata.pool && pdata.pool->has(entity)) {
                pdata.pool->destroy(entity);
            }
        };

        // just a way to protect users from listeners that attach components
        assert(orphan(entity));
        release(entity);
    }

    /**
     * @brief Destroys all the entities in a range.
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void destroy(It first, It last) {
        assert(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));

        for(auto pos = pools.size(); pos; --pos) {
            auto &pdata = pools[pos-1];

            if(pdata.pool) {
                std::for_each(first, last, [&pdata](const auto entity) {
                    if(pdata.pool->has(entity)) {
                        pdata.pool->destroy(entity);
                    }
                });
            }
        };

        // just a way to protect users from listeners that attach components
        assert(std::all_of(first, last, [this](const auto entity) { return orphan(entity); }));

        std::for_each(first, last, [this](const auto entity) {
            release(entity);
        });
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
        return assure<Component>()->construct(entity, std::forward<Args>(args)...);
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
        pool<Component>()->destroy(entity);
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
        [[maybe_unused]] const auto cpools = std::make_tuple(pool<Component>()...);
        return ((std::get<const pool_type<Component> *>(cpools)
                 ? std::get<const pool_type<Component> *>(cpools)->has(entity)
                 : false) && ...);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entity) const ENTT_NOEXCEPT {
        assert(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (pool<Component>()->get(entity), ...);
        } else {
            return std::tuple<std::add_const_t<Component> &...>{get<Component>(entity)...};
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    inline decltype(auto) get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component &>(std::as_const(*this).template get<Component>(entity)), ...);
        } else {
            return std::tuple<Component &...>{get<Component>(entity)...};
        }
    }

    /**
     * @brief Returns a reference to the given component for an entity.
     *
     * In case the entity doesn't own the component, the parameters provided are
     * used to construct it.<br/>
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.get<Component>(entity) : registry.assign<Component>(entity, value);
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Type of component to get.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return Reference to the component owned by the entity.
     */
    template<typename Component, typename... Args>
    Component & get_or_assign(const entity_type entity, Args &&... args) ENTT_NOEXCEPT {
        assert(valid(entity));
        auto *cpool = assure<Component>();
        auto *comp = cpool->try_get(entity);
        return comp ? *comp : cpool->construct(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return Pointers to the components owned by the entity.
     */
    template<typename... Component>
    auto try_get([[maybe_unused]] const entity_type entity) const ENTT_NOEXCEPT {
        assert(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            const auto cpools = std::make_tuple(pool<Component>()...);
            return ((std::get<const pool_type<Component> *>(cpools)
                     ? std::get<const pool_type<Component> *>(cpools)->try_get(entity)
                     : nullptr), ...);
        } else {
            return std::tuple<std::add_const_t<Component> *...>{try_get<Component>(entity)...};
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    inline auto try_get([[maybe_unused]] const entity_type entity) ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component *>(std::as_const(*this).template try_get<Component>(entity)), ...);
        } else {
            return std::tuple<Component *...>{try_get<Component>(entity)...};
        }
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
        return (pool<Component>()->get(entity) = std::decay_t<Component>{std::forward<Args>(args)...});
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
    Component & assign_or_replace(const entity_type entity, Args &&... args) {
        auto *cpool = assure<Component>();
        auto *comp = cpool->try_get(entity);

        if(comp) {
            *comp = std::decay_t<Component>{std::forward<Args>(args)...};
        } else {
            comp = &cpool->construct(entity, std::forward<Args>(args)...);
        }

        return *comp;
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given component is created and assigned to
     * an entity.
     *
     * The function type for a listener is equivalent to:
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been assigned to the
     * entity. The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a new component is created.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    sink_type construction() ENTT_NOEXCEPT {
        return assure<Component>()->construction.sink();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is removed from an entity and
     * thus destroyed.
     *
     * The function type for a listener is equivalent to:
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** the component has been removed from the
     * entity. The order of invocation of the listeners isn't guaranteed.<br/>
     * Note also that the greater the number of listeners, the greater the
     * performance hit when a component is destroyed.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    sink_type destruction() ENTT_NOEXCEPT {
        return assure<Component>()->destruction.sink();
    }

    /**
     * @brief Sorts the pool of entities for the given component.
     *
     * The order of the elements in a pool is highly affected by assignments
     * of components to entities and deletions. Components are arranged to
     * maximize the performance during iterations and users should not make any
     * assumption on the order.<br/>
     * This function can be used to impose an order to the elements in the pool
     * of the given component. The order is kept valid until a component of the
     * given type is assigned or removed from an entity.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Component &, const Component &);
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
     * @warning
     * Pools of components that are owned by a group cannot be sorted anymore.
     * The group takes the ownership of the pools and arrange components so as
     * to iterate them as fast as possible.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam Component Type of components to sort.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param sort A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort sort = Sort{}, Args &&... args) {
        assert(!owned<Component>());
        assure<Component>()->sort(std::move(compare), std::move(sort), std::forward<Args>(args)...);
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
     * @warning
     * Pools of components that are owned by a group cannot be sorted anymore.
     * The group takes the ownership of the pools and arrange components so as
     * to iterate them as fast as possible.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        assert(!owned<To>());
        assure<To>()->respect(*assure<From>());
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
        auto *cpool = assure<Component>();

        if(cpool->has(entity)) {
            cpool->destroy(entity);
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
        auto *cpool = assure<Component>();

        if(cpool->destruction.empty()) {
            // no group set, otherwise the signal wouldn't be empty
            cpool->reset();
        } else {
            const sparse_set<entity_type> &base = *cpool;

            for(const auto entity: base) {
                cpool->destroy(entity);
            }
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
            // useless this-> used to suppress a warning with clang
            this->destroy(entity);
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
        static_assert(std::is_invocable_v<Func, entity_type>);

        if(available) {
            for(auto pos = entities.size(); pos; --pos) {
                const auto curr = entity_type(pos - 1);
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
     * An orphan is an entity that has no components assigned.
     *
     * @param entity A valid entity identifier.
     * @return True if the entity is an orphan, false otherwise.
     */
    bool orphan(const entity_type entity) const {
        assert(valid(entity));
        bool orphan = true;

        for(std::size_t i = {}; i < pools.size() && orphan; ++i) {
            const auto &pdata = pools[i];
            orphan = !(pdata.pool && pdata.pool->has(entity));
        }

        return orphan;
    }

    /**
     * @brief Iterates orphans and applies them the given function object.
     *
     * The function object is invoked for each entity that is still in use and
     * has no components assigned.<br/>
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
        static_assert(std::is_invocable_v<Func, entity_type>);

        each([func = std::move(func), this](const auto entity) {
            if(orphan(entity)) {
                func(entity);
            }
        });
    }

    /**
     * @brief Returns a view for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Views do their best to iterate the smallest set of candidate entities.
     * In particular:
     *
     * * Single component views are incredibly fast and iterate a packed array
     *   of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     *   component and pick up a reference to the smallest set of candidates to
     *   test for the given components.
     *
     * Views in no way affect the functionalities of the registry nor those of
     * the underlying pools.
     *
     * @note
     * Multi component views are pretty fast. However their performance tend to
     * degenerate when the number of components to iterate grows up and the most
     * of the entities have all the given components.<br/>
     * To get a performance boost, consider using a group instead.
     *
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename... Component>
    entt::basic_view<Entity, Component...> view() {
        return { assure<Component>()... };
    }

    /*! @copydoc view */
    template<typename... Component>
    inline entt::basic_view<Entity, Component...> view() const {
        static_assert(std::conjunction_v<std::is_const<Component>...>);
        return const_cast<basic_registry *>(this)->view<Component...>();
    }

    /**
     * @brief Checks whether a given component belongs to a group.
     * @tparam Component Type of component in which one is interested.
     * @return True if the component belongs to a group, false otherwise.
     */
    template<typename Component>
    bool owned() const ENTT_NOEXCEPT {
        return std::any_of(inner_groups.cbegin(), inner_groups.cend(), [](auto &&gdata) {
            return gdata.owned(type<Component>());
        });
    }

    /**
     * @brief Returns a group for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a group after the use. Creating and destroying a
     * group is an incredibly cheap operation because they do not require any
     * type of initialization, but for the first time they are requested.<br/>
     * As a rule of thumb, storing a group should never be an option.
     *
     * Groups support exclusion lists and can own types of components. The more
     * types are owned by a group, the faster it is to iterate entities and
     * components.<br/>
     * However, groups also affect some features of the registry such as the
     * creation and destruction of components, which will consequently be
     * slightly slower (nothing that can be noticed in most cases).
     *
     * @note
     * Pools of components that are owned by a group cannot be sorted anymore.
     * The group takes the ownership of the pools and arrange components so as
     * to iterate them as fast as possible.
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    entt::basic_group<Entity, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1);
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0);

        if constexpr(sizeof...(Owned) == 0) {
            auto it = std::find_if(outer_groups.begin(), outer_groups.end(), [](auto &&gdata) {
                return gdata.extent == sizeof...(Get) + sizeof...(Exclude)
                        && (gdata.exclude(type<Exclude>()) && ...)
                        && (gdata.get(type<Get>()) && ...);
            });

            if(it == outer_groups.cend()) {
                using group_type = non_owning_group<type_list<std::decay_t<Exclude>...>, type_list<std::decay_t<Get>...>>;
                auto &gdata = outer_groups.emplace_back();

                gdata.data = std::make_unique<group_type>();
                gdata.exclude = +[](component_type ctype) { return ((ctype == type<Exclude>()) || ...); };
                gdata.get = +[](component_type ctype) { return ((ctype == type<Get>()) || ...); };
                gdata.extent = sizeof...(Get) + sizeof...(Exclude);

                auto *curr = static_cast<group_type *>(gdata.data.get());
                const auto cpools = std::make_tuple(assure<Get>()..., assure<Exclude>()...);

                (std::get<pool_type<Get> *>(cpools)->destruction.sink().template connect<&group_type::destroy_if>(curr), ...);
                (std::get<pool_type<Get> *>(cpools)->construction.sink().template connect<&group_type::template construct_if<0>>(curr), ...);

                (std::get<pool_type<Exclude> *>(cpools)->destruction.sink().template connect<&group_type::template construct_if<1>>(curr), ...);
                (std::get<pool_type<Exclude> *>(cpools)->construction.sink().template connect<&group_type::destroy_if>(curr), ...);

                for(const auto entity: view<Get...>()) {
                    if(!(has<Exclude>(entity) || ...)) {
                        curr->construct(entity);
                    }
                }

                it = std::prev(outer_groups.end());
            }

            return { it->data.get(), pool<Get>()... };
        } else {
            auto it = std::find_if(inner_groups.begin(), inner_groups.end(), [](auto &&gdata) {
                return gdata.extent == sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude)
                        && (gdata.exclude(type<Exclude>()) && ...)
                        && (gdata.get(type<Get>()) && ...)
                        && (gdata.owned(type<Owned>()) && ...);
            });

            if(it == inner_groups.cend()) {
                assert(!(owned<Owned>() || ...));
                using group_type = owning_group<type_list<std::decay_t<Exclude>...>, type_list<std::decay_t<Get>...>, std::decay_t<Owned>...>;
                auto &gdata = inner_groups.emplace_back();

                gdata.data = std::make_unique<group_type>();
                gdata.exclude = +[](component_type ctype) { return ((ctype == type<Exclude>()) || ...); };
                gdata.get = +[](component_type ctype) { return ((ctype == type<Get>()) || ...); };
                gdata.owned = +[](component_type ctype) { return ((ctype == type<Owned>()) || ...); };
                gdata.extent = sizeof...(Get) + sizeof...(Exclude) + sizeof...(Owned);

                auto *curr = static_cast<group_type *>(gdata.data.get());
                const auto cpools = std::make_tuple(assure<Owned>()..., assure<Get>()..., assure<Exclude>()...);

                (std::get<pool_type<Owned> *>(cpools)->construction.sink().template connect<&group_type::template induce_if<0>>(curr), ...);
                (std::get<pool_type<Owned> *>(cpools)->destruction.sink().template connect<&group_type::discard_if>(curr), ...);

                (std::get<pool_type<Get> *>(cpools)->construction.sink().template connect<&group_type::template induce_if<0>>(curr), ...);
                (std::get<pool_type<Get> *>(cpools)->destruction.sink().template connect<&group_type::discard_if>(curr), ...);

                (std::get<pool_type<Exclude> *>(cpools)->destruction.sink().template connect<&group_type::template induce_if<1>>(curr), ...);
                (std::get<pool_type<Exclude> *>(cpools)->construction.sink().template connect<&group_type::discard_if>(curr), ...);

                const auto *cpool = std::min({ static_cast<sparse_set<entity_type> *>(std::get<pool_type<Owned> *>(cpools))... }, [](const auto *lhs, const auto *rhs) {
                    return lhs->size() < rhs->size();
                });

                // we cannot iterate backwards because we want to leave behind valid entities
                std::for_each(cpool->data(), cpool->data() + cpool->size(), [curr, this](const auto entity) {
                    curr->template induce_if<0>(*this, entity);
                });

                it = std::prev(inner_groups.end());
            }

            return { &it->data->owned, pool<Owned>()..., pool<Get>()... };
        }
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Get, typename... Exclude>
    inline entt::basic_group<Entity, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>..., std::is_const<Get>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(entt::get<Get...>, exclude<Exclude...>);
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Exclude>
    inline entt::basic_group<Entity, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) {
        return group<Owned...>(entt::get<>, exclude<Exclude...>);
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Exclude>
    inline entt::basic_group<Entity, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) const {
        return group<Owned...>(entt::get<>, exclude<Exclude...>);
    }

    /**
     * @brief Returns a runtime view for the given components.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Users should throw away the view after use. Fortunately, creating and
     * destroying a runtime view is an incredibly cheap operation because they
     * do not require any type of initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Runtime views are to be used when users want to construct a view from
     * some external inputs and don't know at compile-time what are the required
     * components.<br/>
     * This is particularly well suited to plugin systems and mods in general.
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of components.
     * @param last An iterator past the last element of the range of components.
     * @return A newly created runtime view.
     */
    template<typename It>
    entt::basic_runtime_view<Entity> runtime_view(It first, It last) const {
        static_assert(std::is_convertible_v<typename std::iterator_traits<It>::value_type, component_type>);
        std::vector<const sparse_set<Entity> *> set(std::distance(first, last));

        std::transform(first, last, set.begin(), [this](const component_type ctype) {
            auto it = std::find_if(pools.begin(), pools.end(), [ctype](const auto &pdata) {
                return pdata.pool && pdata.runtime_type == ctype;
            });

            return it != pools.cend() && it->pool ? it->pool.get() : nullptr;
        });

        return { std::move(set) };
    }

    /**
     * @brief Clones the given components and all the entity identifiers.
     *
     * The components must be copiable for obvious reasons. The entities
     * maintain their versions once copied.<br/>
     * If no components are provided, the registry will try to clone all the
     * existing pools.
     *
     * @note
     * There isn't an efficient way to know if all the entities are assigned at
     * least one component once copied. Therefore, there may be orphans. It is
     * up to the caller to clean up the registry if necessary.
     *
     * @note
     * Listeners and groups aren't copied. It is up to the caller to connect the
     * listeners of interest to the new registry and to set up groups.
     *
     * @warning
     * Attempting to clone components that aren't copyable can result in
     * unexpected behaviors.<br/>
     * A static assertion will abort the compilation when the components
     * provided aren't copy constructible. Otherwise, an assertion will abort
     * the execution at runtime in debug mode in case one or more pools cannot
     * be cloned.
     *
     * @tparam Component Types of components to clone.
     * @return A fresh copy of the registry.
     */
    template<typename... Component>
    basic_registry clone() const {
        static_assert(std::conjunction_v<std::is_copy_constructible<Component>...>);
        basic_registry other;

        other.pools.resize(pools.size());

        for(auto pos = pools.size(); pos; --pos) {
            auto &pdata = pools[pos-1];

            if(pdata.pool && (!sizeof...(Component) || ... || (pdata.runtime_type == type<Component>()))) {
                auto &curr = other.pools[pos-1];
                curr.pool = pdata.pool->clone();
                curr.runtime_type = pdata.runtime_type;
                assert(curr.pool);
            }
        }

        other.next = next;
        other.available = available;
        other.entities = entities;

        return other;
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
    entt::basic_snapshot<Entity> snapshot() const ENTT_NOEXCEPT {
        using follow_fn_type = entity_type(const basic_registry &, const entity_type);
        const entity_type seed = available ? (next | (entities[next] & (traits_type::version_mask << traits_type::entity_shift))) : next;

        follow_fn_type *follow = [](const basic_registry &reg, const entity_type entity) -> entity_type {
            const auto &others = reg.entities;
            const auto entt = entity & traits_type::entity_mask;
            const auto curr = others[entt] & traits_type::entity_mask;
            return (curr | (others[curr] & (traits_type::version_mask << traits_type::entity_shift)));
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
     * @note
     * The loader returned by this function requires that the registry be empty.
     * In case it isn't, all the data will be automatically deleted before to
     * return.
     *
     * @return A temporary object to use to load snasphosts.
     */
    basic_snapshot_loader<Entity> loader() ENTT_NOEXCEPT {
        using force_fn_type = void(basic_registry &, const entity_type, const bool);

        force_fn_type *force = [](basic_registry &reg, const entity_type entity, const bool destroyed) {
            using promotion_type = std::conditional_t<sizeof(size_type) >= sizeof(entity_type), size_type, entity_type>;
            // explicit promotion to avoid warnings with std::uint16_t
            const auto entt = promotion_type{entity} & traits_type::entity_mask;
            auto &others = reg.entities;

            if(!(entt < others.size())) {
                auto curr = others.size();
                others.resize(entt + 1);
                std::iota(others.data() + curr, others.data() + entt, entity_type(curr));
            }

            others[entt] = entity;

            if(destroyed) {
                reg.destroy(entity);
                const auto version = entity & (traits_type::version_mask << traits_type::entity_shift);
                others[entt] = ((others[entt] & traits_type::entity_mask) | version);
            }
        };

        return { (*this = {}), force };
    }

private:
    std::vector<pool_data> pools;
    std::vector<owning_group_data> inner_groups;
    std::vector<non_owning_group_data> outer_groups;
    std::vector<entity_type> entities;
    size_type available{};
    entity_type next{};
};


}


#endif // ENTT_ENTITY_REGISTRY_HPP
