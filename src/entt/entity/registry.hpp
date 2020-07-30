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
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "group.hpp"
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

    template<typename Component>
    struct pool_handler final: storage<Entity, Component> {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Invalid component type");

        [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
            return sink{construction};
        }

        [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
            return sink{update};
        }

        [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
            return sink{destruction};
        }

        template<typename... Args>
        decltype(auto) emplace(basic_registry &owner, const Entity entt, Args &&... args) {
            storage<entity_type, Component>::emplace(entt, std::forward<Args>(args)...);
            construction.publish(owner, entt);

            if constexpr(!ENTT_IS_EMPTY(Component)) {
                return this->get(entt);
            }
        }

        template<typename It, typename... Args>
        void insert(basic_registry &owner, It first, It last, Args &&... args) {
            storage<entity_type, Component>::insert(first, last, std::forward<Args>(args)...);

            if(!construction.empty()) {
                while(first != last) { construction.publish(owner, *(first++)); }
            }
        }

        void remove(basic_registry &owner, const Entity entt) {
            destruction.publish(owner, entt);
            this->erase(entt);
        }

        template<typename It>
        void remove(basic_registry &owner, It first, It last) {
            if(std::distance(first, last) == std::distance(this->begin(), this->end())) {
                if(!destruction.empty()) {
                    while(first != last) { destruction.publish(owner, *(first++)); }
                }

                this->clear();
            } else {
                while(first != last) { this->remove(owner, *(first++)); }
            }
        }

        template<typename... Func>
        decltype(auto) patch(basic_registry &owner, const Entity entt, [[maybe_unused]] Func &&... func) {
            if constexpr(ENTT_IS_EMPTY(Component)) {
                update.publish(owner, entt);
            } else {
                (std::forward<Func>(func)(this->get(entt)), ...);
                update.publish(owner, entt);
                return this->get(entt);
            }
        }

        decltype(auto) replace(basic_registry &owner, const Entity entt, Component component) {
            return patch(owner, entt, [&component](auto &&curr) { curr = std::move(component); });
        }

    private:
        sigh<void(basic_registry &, const Entity)> construction{};
        sigh<void(basic_registry &, const Entity)> destruction{};
        sigh<void(basic_registry &, const Entity)> update{};
    };

    struct pool_data {
        id_type type_id{};
        std::unique_ptr<sparse_set<Entity>> pool{};
        void(* remove)(sparse_set<Entity> &, basic_registry &, const Entity){};
    };

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...> {
        static_assert(std::conjunction_v<std::is_same<Owned, std::decay_t<Owned>>..., std::is_same<Get, std::decay_t<Get>>..., std::is_same<Exclude, std::decay_t<Exclude>>...>, "One or more component types are invalid");
        std::conditional_t<sizeof...(Owned) == 0, sparse_set<Entity>, std::size_t> current{};

        template<typename Component>
        void maybe_valid_if(basic_registry &owner, const Entity entt) {
            [[maybe_unused]] const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...);

            const auto is_valid = ((std::is_same_v<Component, Owned> || std::get<pool_handler<Owned> &>(cpools).contains(entt)) && ...)
                    && ((std::is_same_v<Component, Get> || owner.assure<Get>().contains(entt)) && ...)
                    && ((std::is_same_v<Component, Exclude> || !owner.assure<Exclude>().contains(entt)) && ...);

            if constexpr(sizeof...(Owned) == 0) {
                if(is_valid && !current.contains(entt)) {
                    current.emplace(entt);
                }
            } else {
                if(is_valid && !(std::get<0>(cpools).index(entt) < current)) {
                    const auto pos = current++;
                    (std::get<pool_handler<Owned> &>(cpools).swap(std::get<pool_handler<Owned> &>(cpools).data()[pos], entt), ...);
                }
            }
        }

        void discard_if([[maybe_unused]] basic_registry &owner, const Entity entt) {
            if constexpr(sizeof...(Owned) == 0) {
                if(current.contains(entt)) {
                    current.erase(entt);
                }
            } else {
                if(const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...); std::get<0>(cpools).contains(entt) && (std::get<0>(cpools).index(entt) < current)) {
                    const auto pos = --current;
                    (std::get<pool_handler<Owned> &>(cpools).swap(std::get<pool_handler<Owned> &>(cpools).data()[pos], entt), ...);
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

    struct variable_data {
        id_type type_id;
        std::unique_ptr<void, void(*)(void *)> value;
    };

    template<typename Component>
    [[nodiscard]] const pool_handler<Component> & assure() const {
        const sparse_set<entity_type> *cpool;

        if constexpr(ENTT_FAST_PATH(has_type_index_v<Component>)) {
            const auto index = type_index<Component>::value();

            if(!(index < pools.size())) {
                pools.resize(index+1);
            }

            if(auto &&pdata = pools[index]; !pdata.pool) {
                pdata.type_id = type_info<Component>::id();
                pdata.pool.reset(new pool_handler<Component>());
                pdata.remove = [](sparse_set<entity_type> &target, basic_registry &owner, const entity_type entt) {
                    static_cast<pool_handler<Component> &>(target).remove(owner, entt);
                };
            }

            cpool = pools[index].pool.get();
        } else {
            if(const auto it = std::find_if(pools.cbegin(), pools.cend(), [id = type_info<Component>::id()](const auto &pdata) { return id == pdata.type_id; }); it == pools.cend()) {
                cpool = pools.emplace_back(pool_data{
                    type_info<Component>::id(),
                    std::unique_ptr<sparse_set<entity_type>>{new pool_handler<Component>()},
                    [](sparse_set<entity_type> &target, basic_registry &owner, const entity_type entt) {
                        static_cast<pool_handler<Component> &>(target).remove(owner, entt);
                    }
                }).pool.get();
            } else {
                cpool = it->pool.get();
            }
        }

        return *static_cast<const pool_handler<Component> *>(cpool);
    }

    template<typename Component>
    [[nodiscard]] pool_handler<Component> & assure() {
        return const_cast<pool_handler<Component> &>(std::as_const(*this).template assure<Component>());
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

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
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    [[nodiscard]] size_type size() const {
        return assure<Component>().size();
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
        auto curr = destroyed;

        for(; curr != null; --sz) {
            curr = entities[to_integral(curr) & traits_type::entity_mask];
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
            (assure<Component>().reserve(cap), ...);
        }
    }

    /**
     * @brief Returns the capacity of the pool for the given component.
     * @tparam Component Type of component in which one is interested.
     * @return Capacity of the pool of the given component.
     */
    template<typename Component>
    [[nodiscard]] size_type capacity() const {
        return assure<Component>().capacity();
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
        (assure<Component>().shrink_to_fit(), ...);
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
            return (assure<Component>().empty() && ...);
        }
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * Components are in the reverse order as imposed by the sorting
     * functionalities.
     *
     * @note
     * Empty components aren't explicitly instantiated. Therefore, this function
     * isn't available for them. A compilation error will occur if invoked.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    [[nodiscard]] const Component * raw() const {
        return assure<Component>().raw();
    }

    /*! @copydoc raw */
    template<typename Component>
    [[nodiscard]] Component * raw() {
        return const_cast<Component *>(std::as_const(*this).template raw<Component>());
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * Entities are in the reverse order as imposed by the sorting
     * functionalities.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    [[nodiscard]] const entity_type * data() const {
        return assure<Component>().data();
    }

    /**
     * @brief Direct access to the list of entities of a registry.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
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
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    [[nodiscard]] bool valid(const entity_type entity) const {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    [[nodiscard]] static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integral(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    [[nodiscard]] static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(to_integral(entity) >> traits_type::entity_shift);
    }

    /**
     * @brief Returns the actual version for an entity identifier.
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
    [[nodiscard]] version_type current(const entity_type entity) const {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        ENTT_ASSERT(pos < entities.size());
        return version_type(to_integral(entities[pos]) >> traits_type::entity_shift);
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
    entity_type create() {
        entity_type entt;

        if(destroyed == null) {
            entt = entities.emplace_back(entity_type{static_cast<typename traits_type::entity_type>(entities.size())});
            // traits_type::entity_mask is reserved to allow for null identifiers
            ENTT_ASSERT(to_integral(entt) < traits_type::entity_mask);
        } else {
            const auto curr = to_integral(destroyed);
            const auto version = to_integral(entities[curr]) & (traits_type::version_mask << traits_type::entity_shift);
            destroyed = entity_type{to_integral(entities[curr]) & traits_type::entity_mask};
            entt = entities[curr] = entity_type{curr | version};
        }

        return entt;
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * @sa create
     *
     * If the requested entity isn't in use, the suggested identifier is created
     * and returned. Otherwise, a new one will be generated for this purpose.
     *
     * @param hint A desired entity identifier.
     * @return A valid entity identifier.
     */
    [[nodiscard]] entity_type create(const entity_type hint) {
        ENTT_ASSERT(hint != null);
        entity_type entt;

        if(const auto req = (to_integral(hint) & traits_type::entity_mask); !(req < entities.size())) {
            entities.reserve(req + 1);

            for(auto pos = entities.size(); pos < req; ++pos) {
                entities.emplace_back(destroyed);
                destroyed = entity_type{static_cast<typename traits_type::entity_type>(pos)};
            }

            entt = entities.emplace_back(hint);
        } else if(const auto curr = (to_integral(entities[req]) & traits_type::entity_mask); req == curr) {
            entt = create();
        } else {
            auto *it = &destroyed;
            for(; (to_integral(*it) & traits_type::entity_mask) != req; it = &entities[to_integral(*it) & traits_type::entity_mask]);
            *it = entity_type{curr | (to_integral(*it) & (traits_type::version_mask << traits_type::entity_shift))};
            entt = entities[req] = hint;
        }

        return entt;
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
        std::generate(first, last, [this]() { return create(); });
    }

    /**
     * @brief Assigns entities to an empty registry.
     *
     * This function is intended for use in conjunction with `raw`.<br/>
     * Don't try to inject ranges of randomly generated entities. There is no
     * guarantee that a registry will continue to work properly in this case.
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode if all
     * pools aren't empty.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void assign(It first, It last) {
        ENTT_ASSERT(std::all_of(pools.cbegin(), pools.cend(), [](auto &&pdata) { return !pdata.pool || pdata.pool->empty(); }));
        entities.assign(first, last);
        destroyed = null;

        for(std::size_t pos{}, end = entities.size(); pos < end; ++pos) {
            if((to_integral(entities[pos]) & traits_type::entity_mask) != pos) {
                const auto version = to_integral(entities[pos]) & (traits_type::version_mask << traits_type::entity_shift);
                entities[pos] = entity_type{to_integral(destroyed) | version};
                destroyed = entity_type{static_cast<typename traits_type::entity_type>(pos)};
            }
        }
    }

    /**
     * @brief Destroys an entity.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        destroy(entity, version_type((to_integral(entity) >> traits_type::entity_shift) + 1));
    }

    /**
     * @brief Destroys an entity.
     *
     * If the entity isn't already destroyed, the suggested version is used
     * instead of the implicitly generated one.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     * @param version A desired version upon destruction.
     */
    void destroy(const entity_type entity, const version_type version) {
        remove_all(entity);
        // lengthens the implicit list of destroyed entities
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        entities[entt] = entity_type{to_integral(destroyed) | (typename traits_type::entity_type{version} << traits_type::entity_shift)};
        destroyed = entity_type{entt};
    }

    /**
     * @brief Destroys all the entities in a range.
     *
     * @sa destroy
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void destroy(It first, It last) {
        while(first != last) { destroy(*(first++)); }
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
    decltype(auto) emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>().emplace(*this, entity, std::forward<Args>(args)...);
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
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().insert(*this, first, last, value);
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
     * @param to An iterator past the last element of the range of components.
     */
    template<typename Component, typename EIt, typename CIt>
    void insert(EIt first, EIt last, CIt from, CIt to) {
        static_assert(std::is_constructible_v<Component, typename std::iterator_traits<CIt>::value_type>, "Invalid value type");
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().insert(*this, first, last, from, to);
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.emplace<Component>(entity, args...);
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
    decltype(auto) emplace_or_replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();

        return cpool.contains(entity)
                ? cpool.replace(*this, entity, Component{std::forward<Args>(args)...})
                : cpool.emplace(*this, entity, std::forward<Args>(args)...);
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
     * that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Type of component to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched component.
     */
    template<typename Component, typename... Func>
    decltype(auto) patch(const entity_type entity, Func &&... func) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>().patch(*this, entity, std::forward<Func>(func)...);
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
     * @return A reference to the component being replaced.
     */
    template<typename Component, typename... Args>
    decltype(auto) replace(const entity_type entity, Args &&... args) {
        return assure<Component>().replace(*this, entity, Component{std::forward<Args>(args)...});
    }

    /**
     * @brief Removes the given components from an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to remove a component from an
     * entity that doesn't own it results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity or if the entity doesn't own an instance of the given
     * component.
     *
     * @tparam Component Types of components to remove.
     * @param entity A valid entity identifier.
     */
    template<typename... Component>
    void remove(const entity_type entity) {
        ENTT_ASSERT(valid(entity));
        (assure<Component>().remove(*this, entity), ...);
    }

    /**
     * @brief Removes the given components from all the entities in a range.
     *
     * @see remove
     *
     * @tparam Component Types of components to remove.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename... Component, typename It>
    void remove(It first, It last) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        (assure<Component>().remove(*this, first, last), ...);
    }

    /**
     * @brief Removes the given components from an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * if(registry.has<Component>(entity)) { registry.remove<Component>(entity) }
     * @endcode
     *
     * Prefer this function anyway because it has slightly better performance.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Types of components to remove.
     * @param entity A valid entity identifier.
     * @return The number of components actually removed.
     */
    template<typename... Component>
    size_type remove_if_exists(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        return ([this, entity](auto &&cpool) {
            return cpool.contains(entity) ? (cpool.remove(*this, entity), true) : false;
        }(assure<Component>()) + ... + size_type{});
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
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param entity A valid entity identifier.
     */
    void remove_all(const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool && pdata.pool->contains(entity)) {
                pdata.remove(*pdata.pool, *this, entity);
            }
        }
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
    [[nodiscard]] bool has(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().contains(entity) && ...);
    }

    /**
     * @brief Checks if an entity has at least one of the given components.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @tparam Component Components for which to perform the check.
     * @param entity A valid entity identifier.
     * @return True if the entity has at least one of the given components,
     * false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool any(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().contains(entity) || ...);
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
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().get(entity), ...);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().get(entity), ...);
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
     * auto &component = registry.has<Component>(entity) ? registry.get<Component>(entity) : registry.emplace<Component>(entity, args...);
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
    [[nodiscard]] decltype(auto) get_or_emplace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();
        return cpool.contains(entity) ? cpool.get(entity) : cpool.emplace(*this, entity, std::forward<Args>(args)...);
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
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().try_get(entity), ...);
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entity) {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().try_get(entity), ...);
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
            // useless this-> used to suppress a warning with clang
            each([this](const auto entity) { this->destroy(entity); });
        } else {
            ([this](auto &&cpool) {
                cpool.remove(*this, cpool.sparse_set<entity_type>::begin(), cpool.sparse_set<entity_type>::end());
            }(assure<Component>()), ...);
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
        if(destroyed == null) {
            for(auto pos = entities.size(); pos; --pos) {
                func(entities[pos-1]);
            }
        } else {
            for(auto pos = entities.size(); pos; --pos) {
                if(const auto entt = entities[pos - 1]; (to_integral(entt) & traits_type::entity_mask) == (pos - 1)) {
                    func(entt);
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
        ENTT_ASSERT(valid(entity));
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
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance of the given component is created and assigned to
     * an entity.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
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
        return assure<Component>().on_construct();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is explicitly updated.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
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
        return assure<Component>().on_update();
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
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity);
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
        return assure<Component>().on_destroy();
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
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) const {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        return { assure<std::decay_t<Component>>()..., assure<Exclude>()... };
    }

    /*! @copydoc view */
    template<typename... Component, typename... Exclude>
    [[nodiscard]] basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) > 0, "Exclusion-only views are not supported");
        return { assure<std::decay_t<Component>>()..., assure<Exclude>()... };
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
        std::vector<const sparse_set<Entity> *> component(std::distance(first, last));
        std::vector<const sparse_set<Entity> *> exclude(std::distance(from, to));

        std::transform(first, last, component.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.pool && pdata.type_id == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        std::transform(from, to, exclude.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.pool && pdata.type_id == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        return { std::move(component), std::move(exclude) };
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
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0, "Exclusion-only views are not supported");
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1, "Single component groups are not allowed");

        using handler_type = group_handler<exclude_t<Exclude...>, get_t<std::decay_t<Get>...>, std::decay_t<Owned>...>;

        const auto cpools = std::forward_as_tuple(assure<std::decay_t<Owned>>()..., assure<std::decay_t<Get>>()...);
        constexpr auto size = sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude);
        handler_type *handler = nullptr;

        if(auto it = std::find_if(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
            return gdata.size == size
                && (gdata.owned(type_info<std::decay_t<Owned>>::id()) && ...)
                && (gdata.get(type_info<std::decay_t<Get>>::id()) && ...)
                && (gdata.exclude(type_info<Exclude>::id()) && ...);
        }); it != groups.cend())
        {
            handler = static_cast<handler_type *>(it->group.get());
        }

        if(!handler) {
            group_data candidate = {
                size,
                { new handler_type{}, [](void *instance) { delete static_cast<handler_type *>(instance); } },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Owned>>::id()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Get>>::id()) || ...); },
                []([[maybe_unused]] const id_type ctype) ENTT_NOEXCEPT { return ((ctype == type_info<Exclude>::id()) || ...); },
            };

            handler = static_cast<handler_type *>(candidate.group.get());

            const void *maybe_valid_if = nullptr;
            const void *discard_if = nullptr;

            if constexpr(sizeof...(Owned) == 0) {
                groups.push_back(std::move(candidate));
            } else {
                ENTT_ASSERT(std::all_of(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    const auto overlapping = (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id()));
                    const auto sz = overlapping + (0u + ... + gdata.get(type_info<std::decay_t<Get>>::id())) + (0u + ... + gdata.exclude(type_info<Exclude>::id()));
                    return !overlapping || ((sz == size) || (sz == gdata.size));
                }));

                const auto next = std::find_if_not(groups.cbegin(), groups.cend(), [size](const auto &gdata) {
                    return !(0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id())) || (size > gdata.size);
                });

                const auto prev = std::find_if(std::make_reverse_iterator(next), groups.crend(), [](const auto &gdata) {
                    return (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id()));
                });

                maybe_valid_if = (next == groups.cend() ? maybe_valid_if : next->group.get());
                discard_if = (prev == groups.crend() ? discard_if : prev->group.get());
                groups.insert(next, std::move(candidate));
            }

            (on_construct<std::decay_t<Owned>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Owned>>>(*handler), ...);
            (on_construct<std::decay_t<Get>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Get>>>(*handler), ...);
            (on_destroy<Exclude>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<Exclude>>(*handler), ...);

            (on_destroy<std::decay_t<Owned>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_destroy<std::decay_t<Get>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_construct<Exclude>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);

            if constexpr(sizeof...(Owned) == 0) {
                for(const auto entity: view<Owned..., Get...>(exclude<Exclude...>)) {
                    handler->current.emplace(entity);
                }
            } else {
                // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
                for(auto *first = std::get<0>(cpools).data(), *last = first + std::get<0>(cpools).size(); first != last; ++first) {
                    handler->template maybe_valid_if<std::tuple_element_t<0, std::tuple<std::decay_t<Owned>...>>>(*this, *first);
                }
            }
        }

        if constexpr(sizeof...(Owned) == 0) {
            return { handler->current, std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
        } else {
            return { handler->current, std::get<pool_handler<std::decay_t<Owned>> &>(cpools)... , std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
        }
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
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>..., std::is_const<Get>...>, "Invalid non-const type");
        return const_cast<basic_registry *>(this)->group<Owned...>(get_t<Get...>{}, exclude<Exclude...>);
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
     * @sa group
     *
     * @tparam Owned Types of components owned by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Exclude>
    [[nodiscard]] basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>...>, "Invalid non-const type");
        return const_cast<basic_registry *>(this)->group<Owned...>(exclude<Exclude...>);
    }

    /**
     * @brief Checks whether the given components belong to any group.
     * @tparam Component Types of components in which one is interested.
     * @return True if the pools of the given components are sortable, false
     * otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool sortable() const {
        return std::none_of(groups.cbegin(), groups.cend(), [](auto &&gdata) { return (gdata.owned(type_info<std::decay_t<Component>>::id()) || ...); });
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
            return (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id())) && (size < gdata.size);
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
     * Pools of components owned by a group cannot be sorted.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
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
        ENTT_ASSERT(sortable<Component>());
        auto &cpool = assure<Component>();
        cpool.sort(cpool.begin(), cpool.end(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
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
     * Pools of components owned by a group cannot be sorted.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the pool is owned by a group.
     *
     * @tparam To Type of components to sort.
     * @tparam From Type of components to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ENTT_ASSERT(sortable<To>());
        assure<To>().respect(assure<From>());
    }

    /**
     * @brief Visits an entity and returns the types for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
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
                func(pdata.type_id);
            }
        }
    }

    /**
     * @brief Visits a registry and returns the types for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
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
                func(pdata.type_id);
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
        vars.push_back(variable_data{type_info<Type>::id(), { new Type{std::forward<Args>(args)...}, [](void *instance) { delete static_cast<Type *>(instance); } }});
        return *static_cast<Type *>(vars.back().value.get());
    }

    /**
     * @brief Unsets a context variable if it exists.
     * @tparam Type Type of object to set.
     */
    template<typename Type>
    void unset() {
        vars.erase(std::remove_if(vars.begin(), vars.end(), [](auto &&var) {
            return var.type_id == type_info<Type>::id();
        }), vars.end());
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
    [[nodiscard]] const Type * try_ctx() const {
        auto it = std::find_if(vars.cbegin(), vars.cend(), [](auto &&var) { return var.type_id == type_info<Type>::id(); });
        return it == vars.cend() ? nullptr : static_cast<const Type *>(it->value.get());
    }

    /*! @copydoc try_ctx */
    template<typename Type>
    [[nodiscard]] Type * try_ctx() {
        return const_cast<Type *>(std::as_const(*this).template try_ctx<Type>());
    }

    /**
     * @brief Returns a reference to an object in the context of the registry.
     *
     * @warning
     * Attempting to get a context variable that doesn't exist results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid requests.
     *
     * @tparam Type Type of object to get.
     * @return A valid reference to the object in the context of the registry.
     */
    template<typename Type>
    [[nodiscard]] const Type & ctx() const {
        const auto *instance = try_ctx<Type>();
        ENTT_ASSERT(instance);
        return *instance;
    }

    /*! @copydoc ctx */
    template<typename Type>
    [[nodiscard]] Type & ctx() {
        return const_cast<Type &>(std::as_const(*this).template ctx<Type>());
    }

    /**
     * @brief Visits a registry and returns the types for its context variables.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const id_type);
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
            func(vars[pos-1].type_id);
        }
    }

private:
    std::vector<group_data> groups{};
    mutable std::vector<pool_data> pools{};
    std::vector<entity_type> entities{};
    std::vector<variable_data> vars{};
    entity_type destroyed{null};
};


}


#endif
