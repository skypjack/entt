#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "component.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "poly_storage.hpp"
#include "runtime_view.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "utility.hpp"
#include "view.hpp"


namespace entt {


/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components, then create
 * views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_registry {
    using traits_type = entt_traits<Entity>;
    using poly_storage_type = typename poly_storage_traits<Entity>::storage_type;
    using basic_common_type = basic_sparse_set<Entity>;

    template<typename Component>
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;

    struct pool_data {
        poly_storage_type poly;
        std::unique_ptr<basic_common_type> pool{};
    };

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...> {
        static_assert(!std::disjunction_v<typename component_traits<Owned>::in_place_delete...>, "Groups do not support in-place delete");
        static_assert(std::conjunction_v<std::is_same<Owned, std::remove_const_t<Owned>>..., std::is_same<Get, std::remove_const_t<Get>>..., std::is_same<Exclude, std::remove_const_t<Exclude>>...>, "One or more component types are invalid");
        std::conditional_t<sizeof...(Owned) == 0, basic_common_type, std::size_t> current{};

        template<typename Component>
        void maybe_valid_if(basic_registry &owner, const Entity entt) {
            [[maybe_unused]] const auto cpools = std::make_tuple(owner.assure<Owned>()...);

            const auto is_valid = ((std::is_same_v<Component, Owned> || std::get<storage_type<Owned> *>(cpools)->contains(entt)) && ...)
                    && ((std::is_same_v<Component, Get> || owner.assure<Get>()->contains(entt)) && ...)
                    && ((std::is_same_v<Component, Exclude> || !owner.assure<Exclude>()->contains(entt)) && ...);

            if constexpr(sizeof...(Owned) == 0) {
                if(is_valid && !current.contains(entt)) {
                    current.emplace(entt);
                }
            } else {
                if(is_valid && !(std::get<0>(cpools)->index(entt) < current)) {
                    const auto pos = current++;
                    (std::get<storage_type<Owned> *>(cpools)->swap(std::get<storage_type<Owned> *>(cpools)->data()[pos], entt), ...);
                }
            }
        }

        void discard_if([[maybe_unused]] basic_registry &owner, const Entity entt) {
            if constexpr(sizeof...(Owned) == 0) {
                current.remove(entt);
            } else {
                if(const auto cpools = std::make_tuple(owner.assure<Owned>()...); std::get<0>(cpools)->contains(entt) && (std::get<0>(cpools)->index(entt) < current)) {
                    const auto pos = --current;
                    (std::get<storage_type<Owned> *>(cpools)->swap(std::get<storage_type<Owned> *>(cpools)->data()[pos], entt), ...);
                }
            }
        }
    };

    struct group_data {
        std::size_t size;
        std::unique_ptr<void, void(*)(void *)> group;
        bool (* owned)(const id_type) ENTT_NOEXCEPT;
        bool (* get)(const id_type) ENTT_NOEXCEPT;
        bool (* exclude)(const id_type) ENTT_NOEXCEPT;
    };

    template<typename Component>
    [[nodiscard]] storage_type<Component> * assure() const {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Non-decayed types not allowed");
        const auto index = type_seq<Component>::value();

        if(!(index < pools.size())) {
            pools.resize(size_type(index)+1u);
        }

        if(auto &&pdata = pools[index]; !pdata.pool) {
            pdata.pool.reset(new storage_type<Component>());
            pdata.poly.template emplace<storage_type<Component> &>(*static_cast<storage_type<Component> *>(pdata.pool.get()));
        }

        return static_cast<storage_type<Component> *>(pools[index].pool.get());
    }

    template<typename Component>
    [[nodiscard]] const storage_type<Component> * pool_if_exists() const ENTT_NOEXCEPT {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Non-decayed types not allowed");
        const auto index = type_seq<Component>::value();
        return (!(index < pools.size()) || !pools[index].pool) ? nullptr : static_cast<const storage_type<Component> *>(pools[index].pool.get());
    }

    auto generate_identifier(const std::size_t pos) ENTT_NOEXCEPT {
        ENTT_ASSERT(pos < traits_type::to_integral(null), "No entities available");
        return traits_type::construct(static_cast<typename traits_type::entity_type>(pos), {});
    }

    auto recycle_identifier() ENTT_NOEXCEPT {
        ENTT_ASSERT(free_list != null, "No entities available");
        const auto curr = traits_type::to_entity(free_list);
        free_list = (tombstone | entities[curr]);
        return (entities[curr] = traits_type::construct(curr, traits_type::to_version(entities[curr])));
    }

    auto release_entity(const Entity entity, const typename traits_type::version_type version) {
        const typename traits_type::version_type vers = version + (version == traits_type::to_version(tombstone));
        entities[traits_type::to_entity(entity)] = traits_type::construct(traits_type::to_entity(free_list), vers);
        free_list = (tombstone | entity);
        return vers;
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Poly storage type. */
    using poly_storage = typename poly_storage_traits<Entity>::storage_type;

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    [[nodiscard]] static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return traits_type::construct(traits_type::to_entity(entity), {});
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    [[nodiscard]] static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return traits_type::to_version(entity);
    }

    /*! @brief Default constructor. */
    basic_registry() = default;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;

    /**
     * @brief Prepares a pool for the given type if required.
     * @tparam Component Type of component for which to prepare a pool.
     */
    template<typename Component>
    void prepare() {
        // suppress the warning due to the [[nodiscard]] attribute
        static_cast<void>(assure<Component>());
    }

    /**
     * @brief Returns a poly storage for a given type.
     * @param info The type for which to return a poly storage.
     * @return A valid poly storage if a pool for the given type exists, an
     * empty and thus invalid element otherwise.
     */
    poly_storage & storage(const type_info info) {
        ENTT_ASSERT(info.seq() < pools.size() && pools[info.seq()].poly, "Storage not available");
        return pools[info.seq()].poly;
    }

    /*! @copydoc storage */
    const poly_storage & storage(const type_info info) const {
        ENTT_ASSERT(info.seq() < pools.size() && pools[info.seq()].poly, "Storage not available");
        return pools[info.seq()].poly;
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    [[nodiscard]] size_type size() const {
        const auto *cpool = pool_if_exists<Component>();
        return cpool ? cpool->size() : size_type{};
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Returns the number of entities still in use.
     * @return Number of entities still in use.
     */
    [[nodiscard]] size_type alive() const {
        auto sz = entities.size();

        for(auto curr = free_list; curr != null; --sz) {
            curr = entities[traits_type::to_entity(curr)];
        }

        return sz;
    }

    /**
     * @brief Increases the capacity of the registry or of the pools for the
     * given components.
     *
     * If no components are specified, the capacity of the registry is
     * increased, that is the number of entities it contains. Otherwise the
     * capacity of the pools for the given components is increased.<br/>
     * In both cases, if the new capacity is greater than the current capacity,
     * new storage is allocated, otherwise the method does nothing.
     *
     * @tparam Component Types of components for which to reserve storage.
     * @param cap Desired capacity.
     */
    template<typename... Component>
    void reserve(const size_type cap) {
        if constexpr(sizeof...(Component) == 0) {
            entities.reserve(cap);
        } else {
            (assure<Component>()->reserve(cap), ...);
        }
    }

    /**
     * @brief Reserves enough space to store `count` pools.
     * @param count Number of pools to reserve space for.
     */
    [[deprecated("No longer supported")]]
    void reserve_pools(const size_t count) {
        pools.reserve(count);
    }

    /**
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    [[nodiscard]] size_type capacity() const {
        const auto *cpool = pool_if_exists<Component>();
        return cpool ? cpool->capacity() : size_type{};
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return entities.capacity();
    }

    /**
     * @brief Requests the removal of unused capacity for the given components.
     * @tparam Component Types of components for which to reclaim unused
     * capacity.
     */
    template<typename... Component>
    void shrink_to_fit() {
        (assure<Component>()->shrink_to_fit(), ...);
    }

    /**
     * @brief Checks whether the registry or the pools of the given components
     * are empty.
     *
     * A registry is considered empty when it doesn't contain entities that are
     * still in use.
     *
     * @tparam Component Types of components in which one is interested.
     * @return True if the registry or the pools of the given components are
     * empty, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool empty() const {
        if constexpr(sizeof...(Component) == 0) {
            return !alive();
        } else {
            return [](const auto *... cpool) { return ((!cpool || cpool->empty()) && ...); }(pool_if_exists<Component>()...);
        }
    }

    /**
     * @brief Direct access to the list of entities of a registry.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @warning
     * This list contains both valid and destroyed entities and isn't suitable
     * for direct use.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return entities.data();
    }

    /**
     * @brief Returns the head of the list of released entities.
     *
     * This function is intended for use in conjunction with `assign`.<br/>
     * The returned entity has an invalid identifier in all cases.
     *
     * @return The head of the list of released entities.
     */
    [[nodiscard]] entity_type released() const ENTT_NOEXCEPT {
        return free_list;
    }

    /*! @copydoc released */
    [[deprecated("Use ::released instead")]]
    [[nodiscard]] entity_type destroyed() const ENTT_NOEXCEPT {
        return released();
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    [[nodiscard]] bool valid(const entity_type entity) const {
        const auto pos = size_type(traits_type::to_entity(entity));
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the actual version for an entity identifier.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the registry results
     * in undefined behavior. An entity belongs to the registry even if it has
     * been previously destroyed and/or recycled.
     *
     * @param entity A valid entity identifier.
     * @return Actual version for the given entity identifier.
     */
    [[nodiscard]] version_type current(const entity_type entity) const {
        const auto pos = size_type(traits_type::to_entity(entity));
        ENTT_ASSERT(pos < entities.size(), "Entity does not exist");
        return version(entities[pos]);
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of possible entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * @return A valid entity identifier.
     */
    [[nodiscard]] entity_type create() {
        return (free_list == null) ? entities.emplace_back(generate_identifier(entities.size())) : recycle_identifier();
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * @sa create
     *
     * If the requested entity isn't in use, the suggested identifier is created
     * and returned. Otherwise, a new identifier is generated.
     *
     * @param hint Required entity identifier.
     * @return A valid entity identifier.
     */
    [[nodiscard]] entity_type create(const entity_type hint) {
        const auto length = entities.size();

        if(hint == null || hint == tombstone) {
            return create();
        } else if(const auto req = traits_type::to_entity(hint); !(req < length)) {
            entities.resize(size_type(req) + 1u, null);

            for(auto pos = length; pos < req; ++pos) {
                release_entity(generate_identifier(pos), {});
            }

            return (entities[req] = hint);
        } else if(const auto curr = traits_type::to_entity(entities[req]); req == curr) {
            return create();
        } else {
            auto *it = &free_list;
            for(; traits_type::to_entity(*it) != req; it = &entities[traits_type::to_entity(*it)]);
            *it = traits_type::construct(curr, traits_type::to_version(*it));
            return (entities[req] = hint);
        }
    }

    /**
     * @brief Assigns each element in a range an entity.
     *
     * @sa create
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void create(It first, It last) {
        for(; free_list != null && first != last; ++first) {
            *first = recycle_identifier();
        }

        const auto length = entities.size();
        entities.resize(length + std::distance(first, last), null);

        for(auto pos = length; first != last; ++first, ++pos) {
            *first = entities[pos] = generate_identifier(pos);
        }
    }

    /**
     * @brief Assigns entities to an empty registry.
     *
     * This function is intended for use in conjunction with `data`, `size` and
     * `destroyed`.<br/>
     * Don't try to inject ranges of randomly generated entities nor the _wrong_
     * head for the list of destroyed entities. There is no guarantee that a
     * registry will continue to work properly in this case.
     *
     * @warning
     * There must be no entities still alive for this to work properly.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param destroyed The head of the list of destroyed entities.
     */
    template<typename It>
    void assign(It first, It last, const entity_type destroyed) {
        ENTT_ASSERT(!alive(), "Entities still alive");
        entities.assign(first, last);
        free_list = destroyed;
    }

    /**
     * @brief Releases an entity identifier.
     *
     * The version is updated and the identifier can be recycled at any time.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     * @return The version of the recycled entity.
     */
    version_type release(const entity_type entity) {
        return release(entity, version(entity) + 1u);
    }

    /**
     * @brief Releases an entity identifier.
     *
     * The suggested version or the valid version closest to the suggested one
     * is used instead of the implicitly generated version.
     *
     * @sa release
     *
     * @param entity A valid entity identifier.
     * @param version A desired version upon destruction.
     * @return The version actually assigned to the entity.
     */
    version_type release(const entity_type entity, const version_type version) {
        ENTT_ASSERT(orphan(entity), "Non-orphan entity");
        return release_entity(entity, version);
    }

    /**
     * @brief Releases all entity identifiers in a range.
     *
     * @sa release
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void release(It first, It last) {
        for(; first != last; ++first) {
            release(*first, version(*first) + 1u);
        }
    }

    /**
     * @brief Destroys an entity and releases its identifier.
     *
     * The version is updated and the identifier can be recycled at any time.
     *
     * @warning
     * Adding or removing components to an entity that is being destroyed can
     * result in undefined behavior.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     * @return The version of the recycled entity.
     */
    version_type destroy(const entity_type entity) {
        return destroy(entity, version(entity) + 1u);
    }

    /**
     * @brief Destroys an entity and releases its identifier.
     *
     * The suggested version or the valid version closest to the suggested one
     * is used instead of the implicitly generated version.
     *
     * @sa destroy
     *
     * @param entity A valid entity identifier.
     * @param version A desired version upon destruction.
     * @return The version actually assigned to the entity.
     */
    version_type destroy(const entity_type entity, const version_type version) {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        for(auto &&pdata: pools) {
            pdata.pool && pdata.pool->remove(entity, this);
        }

        return release_entity(entity, version);
    }

    /**
     * @brief Destroys all entities in a range and releases their identifiers.
     *
     * @sa destroy
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void destroy(It first, It last) {
        if constexpr(is_iterator_type_v<typename basic_common_type::iterator, It>) {
            for(; first != last; ++first) {
                destroy(*first, version(*first) + 1u);
            }
        } else {
            for(auto &&pdata: pools) {
                pdata.pool && pdata.pool->remove(first, last, this);
            }

            release(first, last);
        }
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
     * that already owns it results in undefined behavior.
     *
     * @tparam Component Type of component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        return assure<Component>()->emplace(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns each entity in a range the given component.
     *
     * @sa emplace
     *
     * @tparam Component Type of component to create.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the component to assign.
     */
    template<typename Component, typename It>
    void insert(It first, It last, const Component &value = {}) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }), "Invalid entity");
        assure<Component>()->insert(*this, first, last, value);
    }

    /**
     * @brief Assigns each entity in a range the given components.
     *
     * @sa emplace
     *
     * @tparam Component Type of component to create.
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of components.
     */
    template<typename Component, typename EIt, typename CIt, typename = std::enable_if_t<std::is_same_v<std::decay_t<typename std::iterator_traits<CIt>::value_type>, Component>>>
    void insert(EIt first, EIt last, CIt from) {
        static_assert(std::is_constructible_v<Component, typename std::iterator_traits<CIt>::value_type>, "Invalid value type");
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }), "Invalid entity");
        assure<Component>()->insert(*this, first, last, from);
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.all_of<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.emplace<Component>(entity, args...);
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) emplace_or_replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        auto *cpool = assure<Component>();

        return cpool->contains(entity)
            ? cpool->patch(*this, entity, [&args...](auto &... curr) { ((curr = Component{std::forward<Args>(args)...}), ...); })
            : cpool->emplace(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given component for an entity.
     *
     * The signature of the functions should be equivalent to the following:
     *
     * @code{.cpp}
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned. However, this function can be used to trigger an update signal
     * for them.
     *
     * @warning
     * Attempting to use an invalid entity or to patch a component of an entity
     * that doesn't own it results in undefined behavior.
     *
     * @tparam Component Type of component to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched component.
     */
    template<typename Component, typename... Func>
    decltype(auto) patch(const entity_type entity, Func &&... func) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        return assure<Component>()->patch(*this, entity, std::forward<Func>(func)...);
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
     * entity that doesn't own it results in undefined behavior.
     *
     * @tparam Component Type of component to replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the component being replaced.
     */
    template<typename Component, typename... Args>
    decltype(auto) replace(const entity_type entity, Args &&... args) {
        return assure<Component>()->patch(*this, entity, [&args...](auto &... curr) { ((curr = Component{std::forward<Args>(args)...}), ...); });
    }

    /**
     * @brief Removes the given components from an entity.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @tparam Component Types of components to remove.
     * @param entity A valid entity identifier.
     * @return The number of components actually removed.
     */
    template<typename... Component>
    size_type remove(const entity_type entity) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        return (assure<Component>()->remove(entity, this) + ... + size_type{});
    }

    /**
     * @brief Removes the given components from all the entities in a range.
     *
     * @sa remove
     *
     * @tparam Component Types of components to remove.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return The number of components actually removed.
     */
    template<typename... Component, typename It>
    size_type remove(It first, It last) {
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        const auto cpools = std::make_tuple(assure<Component>()...);
        size_type count{};

        for(; first != last; ++first) {
            const auto entity = *first;
            ENTT_ASSERT(valid(entity), "Invalid entity");
            count += (std::get<storage_type<Component> *>(cpools)->remove(entity, this) + ...);
        }

        return count;
    }

    /**
     * @brief Erases the given components from an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to erase a component from an
     * entity that doesn't own it results in undefined behavior.
     *
     * @tparam Component Types of components to erase.
     * @param entity A valid entity identifier.
     */
    template<typename... Component>
    void erase(const entity_type entity) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        (assure<Component>()->erase(entity, this), ...);
    }

    /**
     * @brief Erases the given components from all the entities in a range.
     *
     * @sa erase
     *
     * @tparam Component Types of components to erase.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename... Component, typename It>
    void erase(It first, It last) {
        static_assert(sizeof...(Component) > 0, "Provide one or more component types");
        const auto cpools = std::make_tuple(assure<Component>()...);

        for(; first != last; ++first) {
            const auto entity = *first;
            ENTT_ASSERT(valid(entity), "Invalid entity");
            (std::get<storage_type<Component> *>(cpools)->erase(entity, this), ...);
        }
    }

    /**
     * @brief Removes all tombstones from a registry or only the pools for the
     * given components.
     * @tparam Component Types of components for which to clear all tombstones.
     */
    template<typename... Component>
    void compact() {
        if constexpr(sizeof...(Component) == 0) {
            for(auto &&pdata: pools) {
                pdata.pool && (pdata.pool->compact(), true);
            }
        } else {
            (assure<Component>()->compact(), ...);
        }
    }

    /*! @copydoc remove */
    template<typename... Component>
    [[deprecated("Use ::remove instead")]]
    size_type remove_if_exists(const entity_type entity) {
        return remove<Component...>(entity);
    }

    /**
     * @brief Removes all the components from an entity and makes it orphaned.
     *
     * @warning
     * In case there are listeners that observe the destruction of components
     * and assign other components to the entity in their bodies, the result of
     * invoking this function may not be as expected. In the worst case, it
     * could lead to undefined behavior.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     */
    [[deprecated("Use ::destroy(entity)/::create(entity) instead")]]
    void remove_all(const entity_type entity) {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        for(auto &&pdata: pools) {
            pdata.pool && pdata.pool->remove(entity, this);
        }
    }

    /**
     * @brief Checks if an entity has all the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has all the components, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool all_of(const entity_type entity) const {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        return [entity](const auto *... cpool) { return ((cpool && cpool->contains(entity)) && ...); }(pool_if_exists<Component>()...);
    }

    /**
     * @brief Checks if an entity has at least one of the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has at least one of the given components,
     * false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool any_of(const entity_type entity) const {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        return [entity](const auto *... cpool) { return !((!cpool || !cpool->contains(entity)) && ...); }(pool_if_exists<Component>()...);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        if constexpr(sizeof...(Component) == 1) {
            const auto *cpool = pool_if_exists<std::remove_const_t<Component>...>();
            ENTT_ASSERT(cpool, "Storage not available");
            return cpool->get(entity);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component &>(assure<std::remove_const_t<Component>>()->get(entity)), ...);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
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
     * auto &component = registry.all_of<Component>(entity) ? registry.get<Component>(entity) : registry.emplace<Component>(entity, args...);
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @tparam Component Type of component to get.
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return Reference to the component owned by the entity.
     */
    template<typename Component, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        auto *cpool = assure<Component>();
        return cpool->contains(entity) ? cpool->get(entity) : cpool->emplace(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.
     *
     * @note
     * The registry retains ownership of the pointed-to components.
     *
     * @tparam Component Types of components to get.
     * @param entity A valid entity identifier.
     * @return Pointers to the components owned by the entity.
     */
    template<typename... Component>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        if constexpr(sizeof...(Component) == 1) {
            const auto *cpool = pool_if_exists<std::remove_const_t<Component>...>();
            return (cpool && cpool->contains(entity)) ? &cpool->get(entity) : nullptr;
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity), "Invalid entity");

        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component *>(std::as_const(*this).template try_get<Component>(entity)), ...);
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /**
     * @brief Clears a whole registry or the pools for the given components.
     * @tparam Component Types of components to remove from their entities.
     */
    template<typename... Component>
    void clear() {
        if constexpr(sizeof...(Component) == 0) {
            for(auto &&pdata: pools) {
                pdata.pool && (pdata.pool->clear(this), true);
            }

            each([this](const auto entity) { release_entity(entity, version(entity) + 1u); });
        } else {
            (assure<Component>()->clear(this), ...);
        }
    }

    /**
     * @brief Iterates all the entities that are still in use.
     *
     * The function object is invoked for each entity that is still in use.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Entity);
     * @endcode
     *
     * This function is fairly slow and should not be used frequently. However,
     * it's useful for iterating all the entities still in use, regardless of
     * their components.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if(free_list == null) {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                if(const auto entity = entities[pos - 1]; traits_type::to_entity(entity) == (pos - 1)) {
                    func(entity);
                }
            }
        }
    }

    /**
     * @brief Checks if an entity has components assigned.
     * @param entity A valid entity identifier.
     * @return True if the entity has no components assigned, false otherwise.
     */
    [[nodiscard]] bool orphan(const entity_type entity) const {
        ENTT_ASSERT(valid(entity), "Invalid entity");
        return std::none_of(pools.cbegin(), pools.cend(), [entity](auto &&pdata) { return pdata.pool && pdata.pool->contains(entity); });
    }

    /**
     * @brief Iterates orphans and applies them the given function object.
     *
     * The function object is invoked for each entity that is still in use and
     * has no components assigned.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Entity);
     * @endcode
     *
     * This function can be very slow and should not be used frequently.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void orphans(Func func) const {
        each([this, &func](const auto entity) {
            if(orphan(entity)) {
                func(entity);
            }
        });
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given component is created and assigned to
     * an entity.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been assigned to the
     * entity.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    [[nodiscard]] auto on_construct() {
        return assure<Component>()->on_construct();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is explicitly updated.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** the component has been updated.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    [[nodiscard]] auto on_update() {
        return assure<Component>()->on_update();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is removed from an entity and
     * thus destroyed.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** the component has been removed from the
     * entity.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    [[nodiscard]] auto on_destroy() {
        return assure<Component>()->on_destroy();
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
     * @tparam Exclude Types of components used to filter the view.
     * @return A newly created view.
     */
    template<typename... Component, typename... Exclude>
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, std::add_const_t<Component>...> view(exclude_t<Exclude...> = {}) const {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        return { *assure<std::remove_const_t<Component>>()..., *assure<Exclude>()... };
    }

    /*! @copydoc view */
    template<typename... Component, typename... Exclude>
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        return { *assure<std::remove_const_t<Component>>()..., *assure<Exclude>()... };
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
     * components.
     *
     * @tparam ItComp Type of input iterator for the components to use to
     * construct the view.
     * @tparam ItExcl Type of input iterator for the components to use to filter
     * the view.
     * @param first An iterator to the first element of the range of components
     * to use to construct the view.
     * @param last An iterator past the last element of the range of components
     * to use to construct the view.
     * @param from An iterator to the first element of the range of components
     * to use to filter the view.
     * @param to An iterator past the last element of the range of components to
     * use to filter the view.
     * @return A newly created runtime view.
     */
    template<typename ItComp, typename ItExcl = id_type *>
    [[nodiscard]] basic_runtime_view<Entity> runtime_view(ItComp first, ItComp last, ItExcl from = {}, ItExcl to = {}) const {
        std::vector<const basic_common_type *> component(std::distance(first, last));
        std::vector<const basic_common_type *> filter(std::distance(from, to));

        std::transform(first, last, component.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.poly && pdata.poly->value_type().hash() == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        std::transform(from, to, filter.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.poly && pdata.poly->value_type().hash() == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        return { std::move(component), std::move(filter) };
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
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0, "Exclusion-only groups are not supported");
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1, "Single component groups are not allowed");

        using handler_type = group_handler<exclude_t<Exclude...>, get_t<std::remove_const_t<Get>...>, std::remove_const_t<Owned>...>;

        const auto cpools = std::make_tuple(assure<std::remove_const_t<Owned>>()..., assure<std::remove_const_t<Get>>()...);
        constexpr auto size = sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude);
        handler_type *handler = nullptr;

        if(auto it = std::find_if(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
            return gdata.size == size
                && (gdata.owned(type_hash<std::remove_const_t<Owned>>::value()) && ...)
                && (gdata.get(type_hash<std::remove_const_t<Get>>::value()) && ...)
                && (gdata.exclude(type_hash<Exclude>::value()) && ...);
        }); it != groups.cend())
        {
            handler = static_cast<handler_type *>(it->group.get());
        }

        if(!handler) {
            group_data candidate = {
                size,
                { new handler_type{}, [](void *instance) { delete static_cast<handler_type *>(instance); } },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_hash<std::remove_const_t<Owned>>::value()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_hash<std::remove_const_t<Get>>::value()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_hash<Exclude>::value()) || ...); },
            };

            handler = static_cast<handler_type *>(candidate.group.get());

            const void *maybe_valid_if = nullptr;
            const void *discard_if = nullptr;

            if constexpr(sizeof...(Owned) == 0) {
                groups.push_back(std::move(candidate));
            } else {
                ENTT_ASSERT(std::all_of(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    const auto overlapping = (0u + ... + gdata.owned(type_hash<std::remove_const_t<Owned>>::value()));
                    const auto sz = overlapping + (0u + ... + gdata.get(type_hash<std::remove_const_t<Get>>::value())) + (0u + ... + gdata.exclude(type_hash<Exclude>::value()));
                    return !overlapping || ((sz == size) || (sz == gdata.size));
                }), "Conflicting groups");

                const auto next = std::find_if_not(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    return !(0u + ... + gdata.owned(type_hash<std::remove_const_t<Owned>>::value())) || (size > gdata.size);
                });

                const auto prev = std::find_if(std::make_reverse_iterator(next), groups.crend(), [](const auto &gdata) {
                    return (0u + ... + gdata.owned(type_hash<std::remove_const_t<Owned>>::value()));
                });

                maybe_valid_if = (next == groups.cend() ? maybe_valid_if : next->group.get());
                discard_if = (prev == groups.crend() ? discard_if : prev->group.get());
                groups.insert(next, std::move(candidate));
            }

            (on_construct<std::remove_const_t<Owned>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::remove_const_t<Owned>>>(*handler), ...);
            (on_construct<std::remove_const_t<Get>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::remove_const_t<Get>>>(*handler), ...);
            (on_destroy<Exclude>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<Exclude>>(*handler), ...);

            (on_destroy<std::remove_const_t<Owned>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_destroy<std::remove_const_t<Get>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_construct<Exclude>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);

            if constexpr(sizeof...(Owned) == 0) {
                for(const auto entity: view<Owned..., Get...>(exclude<Exclude...>)) {
                    handler->current.emplace(entity);
                }
            } else {
                // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
                for(auto *first = std::get<0>(cpools)->data(), *last = first + std::get<0>(cpools)->size(); first != last; ++first) {
                    handler->template maybe_valid_if<type_list_element_t<0, type_list<std::remove_const_t<Owned>...>>>(*this, *first);
                }
            }
        }

        return { handler->current, *std::get<storage_type<std::remove_const_t<Owned>> *>(cpools)..., *std::get<storage_type<std::remove_const_t<Get>> *>(cpools)... };
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<std::add_const_t<Get>...>, std::add_const_t<Owned>...> group_if_exists(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        if(auto it = std::find_if(groups.cbegin(), groups.cend(), [](const auto &gdata) {
            return gdata.size == (sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude))
                && (gdata.owned(type_hash<std::remove_const_t<Owned>>::value()) && ...)
                && (gdata.get(type_hash<std::remove_const_t<Get>>::value()) && ...)
                && (gdata.exclude(type_hash<Exclude>::value()) && ...);
            }); it == groups.cend())
        {
            return {};
        } else {
            using handler_type = group_handler<exclude_t<Exclude...>, get_t<std::remove_const_t<Get>...>, std::remove_const_t<Owned>...>;
            return { static_cast<handler_type *>(it->group.get())->current, *pool_if_exists<std::remove_const_t<Owned>>()... , *pool_if_exists<std::remove_const_t<Get>>()... };
        }
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Exclude>
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) {
        return group<Owned...>(get_t<>{}, exclude<Exclude...>);
    }

    /**
     * @brief Returns a group for the given components.
     *
     * @sa group_if_exists
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Exclude>
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<>, std::add_const_t<Owned>...> group_if_exists(exclude_t<Exclude...> = {}) const {
        return group_if_exists<std::add_const_t<Owned>...>(get_t<>{}, exclude<Exclude...>);
    }

    /**
     * @brief Checks whether the given components belong to any group.
     * @tparam Component Types of components in which one is interested.
     * @return True if the pools of the given components are sortable, false
     * otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool sortable() const {
        return std::none_of(groups.cbegin(), groups.cend(), [](auto &&gdata) { return (gdata.owned(type_hash<std::remove_const_t<Component>>::value()) || ...); });
    }

    /**
     * @brief Checks whether a group can be sorted.
     * @tparam Owned Types of components owned by the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return True if the group can be sorted, false otherwise.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] bool sortable(const basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> &) ENTT_NOEXCEPT {
        constexpr auto size = sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude);
        return std::find_if(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
            return (0u + ... + gdata.owned(type_hash<std::remove_const_t<Owned>>::value())) && (size < gdata.size);
        }) == groups.cend();
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
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
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
     * Pools of components owned by a group cannot be sorted.
     *
     * @tparam Component Type of components to sort.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(sortable<Component>(), "Cannot sort owned storage");
        assure<Component>()->sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
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
     * Pools of components owned by a group cannot be sorted.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ENTT_ASSERT(sortable<To>(), "Cannot sort owned storage");
        assure<To>()->respect(*assure<From>());
    }

    /**
     * @brief Visits an entity and returns the type info for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const type_info);
     * @endcode
     *
     * Returned identifiers are those of the components owned by the entity.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a component attached to or removed from the
     * given entity during the visit is returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param entity A valid entity identifier.
     * @param func A valid function object.
     */
    template<typename Func>
    void visit(entity_type entity, Func func) const {
        for(auto pos = pools.size(); pos; --pos) {
            if(const auto &pdata = pools[pos-1]; pdata.pool && pdata.pool->contains(entity)) {
                func(pdata.poly->value_type());
            }
        }
    }

    /**
     * @brief Visits a registry and returns the type info for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const type_info);
     * @endcode
     *
     * Returned identifiers are those of the components managed by the registry.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a component for which a pool is created during
     * the visit is returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void visit(Func func) const {
        for(auto pos = pools.size(); pos; --pos) {
            if(const auto &pdata = pools[pos-1]; pdata.pool) {
                func(pdata.poly->value_type());
            }
        }
    }

    /**
     * @brief Binds an object to the context of the registry.
     *
     * If the value already exists it is overwritten, otherwise a new instance
     * of the given type is created and initialized with the arguments provided.
     *
     * @tparam Type Type of object to set.
     * @tparam Args Types of arguments to use to construct the object.
     * @param args Parameters to use to initialize the value.
     * @return A reference to the newly created object.
     */
    template<typename Type, typename... Args>
    Type & set(Args &&... args) {
        unset<Type>();
        vars.emplace_back(std::in_place_type<Type>, std::forward<Args>(args)...);
        return any_cast<Type &>(vars.back());
    }

    /**
     * @brief Unsets a context variable if it exists.
     * @tparam Type Type of object to set.
     */
    template<typename Type>
    void unset() {
        vars.erase(std::remove_if(vars.begin(), vars.end(), [type = type_id<Type>()](auto &&var) { return var.type() == type; }), vars.end());
    }

    /**
     * @brief Binds an object to the context of the registry.
     *
     * In case the context doesn't contain the given object, the parameters
     * provided are used to construct it.
     *
     * @tparam Type Type of object to set.
     * @tparam Args Types of arguments to use to construct the object.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the object in the context of the registry.
     */
    template<typename Type, typename... Args>
    [[nodiscard]] Type & ctx_or_set(Args &&... args) {
        auto *value = try_ctx<Type>();
        return value ? *value : set<Type>(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns a pointer to an object in the context of the registry.
     * @tparam Type Type of object to get.
     * @return A pointer to the object if it exists in the context of the
     * registry, a null pointer otherwise.
     */
    template<typename Type>
    [[nodiscard]] std::add_const_t<Type> * try_ctx() const {
        auto it = std::find_if(vars.cbegin(), vars.cend(), [type = type_id<Type>()](auto &&var) { return var.type() == type; });
        return it == vars.cend() ? nullptr : any_cast<std::add_const_t<Type>>(&*it);
    }

    /*! @copydoc try_ctx */
    template<typename Type>
    [[nodiscard]] Type * try_ctx() {
        auto it = std::find_if(vars.begin(), vars.end(), [type = type_id<Type>()](auto &&var) { return var.type() == type; });
        return it == vars.end() ? nullptr : any_cast<Type>(&*it);
    }

    /**
     * @brief Returns a reference to an object in the context of the registry.
     *
     * @warning
     * Attempting to get a context variable that doesn't exist results in
     * undefined behavior.
     *
     * @tparam Type Type of object to get.
     * @return A valid reference to the object in the context of the registry.
     */
    template<typename Type>
    [[nodiscard]] std::add_const_t<Type> & ctx() const {
        auto it = std::find_if(vars.cbegin(), vars.cend(), [type = type_id<Type>()](auto &&var) { return var.type() == type; });
        ENTT_ASSERT(it != vars.cend(), "Invalid instance");
        return any_cast<std::add_const_t<Type> &>(*it);
    }

    /*! @copydoc ctx */
    template<typename Type>
    [[nodiscard]] Type & ctx() {
        auto it = std::find_if(vars.begin(), vars.end(), [type = type_id<Type>()](auto &&var) { return var.type() == type; });
        ENTT_ASSERT(it != vars.end(), "Invalid instance");
        return any_cast<Type &>(*it);
    }

    /**
     * @brief Visits a registry and returns the type info for its context
     * variables.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const type_info);
     * @endcode
     *
     * Returned identifiers are those of the context variables currently set.
     *
     * @sa type_info
     *
     * @warning
     * It's not specified whether a context variable created during the visit is
     * returned or not to the caller.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void ctx(Func func) const {
        for(auto pos = vars.size(); pos; --pos) {
            func(vars[pos-1].type());
        }
    }

private:
    std::vector<basic_any<0u>> vars{};
    mutable std::vector<pool_data> pools{};
    std::vector<group_data> groups{};
    std::vector<entity_type> entities{};
    entity_type free_list{tombstone};
};


}


#endif
