#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "runtime_view.hpp"
#include "sparse_set.hpp"
#include "snapshot.hpp"
#include "storage.hpp"
#include "entity.hpp"
#include "group.hpp"
#include "view.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components and thus
 * create views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_registry {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    template<typename Component>
    struct pool_handler: storage<Entity, Component> {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>);
        std::size_t super{};

        auto on_construct() ENTT_NOEXCEPT {
            return sink{construction};
        }

        auto on_replace() ENTT_NOEXCEPT {
            return sink{update};
        }

        auto on_destroy() ENTT_NOEXCEPT {
            return sink{destruction};
        }

        template<typename... Args>
        decltype(auto) assign(basic_registry &owner, const Entity entt, Args &&... args) {
            this->construct(entt, std::forward<Args>(args)...);
            construction.publish(owner, entt);
            return this->get(entt);
        }

        template<typename It, typename... Value>
        std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, Entity>, void>
        assign(basic_registry &owner, It first, It last, Value &&... value) {
            this->construct(first, last, std::forward<Value>(value)...);

            if(!construction.empty()) {
                std::for_each(first, last, [this, &owner](const auto entt) { construction.publish(owner, entt); });
            }
        }

        void remove(basic_registry &owner, const Entity entt) {
            destruction.publish(owner, entt);
            this->destroy(entt);
        }

        template<typename It>
        void remove(basic_registry &owner, It first, It last) {
            if(std::distance(first, last) == std::distance(this->begin(), this->end())) {
                if(!destruction.empty()) {
                    std::for_each(first, last, [this, &owner](const auto entt) { destruction.publish(owner, entt); });
                }

                this->clear();
            } else {
                // useless this-> used to suppress a warning with clang
                std::for_each(first, last, [this, &owner](const auto entt) { this->remove(owner, entt); });
            }
        }

        template<typename... Func>
        void replace(basic_registry &owner, const Entity entt, Func &&... func) {
            (std::forward<Func>(func)(this->get(entt)), ...);
            update.publish(owner, entt);
        }

    private:
        sigh<void(basic_registry &, const Entity)> construction{};
        sigh<void(basic_registry &, const Entity)> destruction{};
        sigh<void(basic_registry &, const Entity)> update{};
    };

    struct pool_data {
        ENTT_ID_TYPE type_id{};
        std::unique_ptr<sparse_set<Entity>> pool{};
        void(* assure)(basic_registry &, const sparse_set<Entity> &){};
        void(* remove)(sparse_set<Entity> &, basic_registry &, const Entity){};
        void(* stamp)(basic_registry &, const Entity, const sparse_set<Entity> &, const Entity){};
    };

    template<typename...>
    struct group_handler;

    template<typename... Exclude, typename... Get, typename... Owned>
    struct group_handler<exclude_t<Exclude...>, get_t<Get...>, Owned...> {
        static_assert(std::conjunction_v<std::is_same<Owned, std::decay_t<Owned>>..., std::is_same<Get, std::decay_t<Get>>..., std::is_same<Exclude, std::decay_t<Exclude>>...>);
        std::conditional_t<sizeof...(Owned) == 0, sparse_set<Entity>, std::size_t> current{};

        template<typename Component>
        void maybe_valid_if(basic_registry &owner, const Entity entt) {
            static_assert(std::disjunction_v<std::is_same<Owned, std::decay_t<Owned>>..., std::is_same<Get, std::decay_t<Get>>..., std::is_same<Exclude, std::decay_t<Exclude>>...>);
            [[maybe_unused]] const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...);

            const auto is_valid = ((std::is_same_v<Component, Owned> || std::get<pool_handler<Owned> &>(cpools).has(entt)) && ...)
                    && ((std::is_same_v<Component, Get> || owner.assure<Get>().has(entt)) && ...)
                    && ((std::is_same_v<Component, Exclude> || !owner.assure<Exclude>().has(entt)) && ...);

            if constexpr(sizeof...(Owned) == 0) {
                if(is_valid && !current.has(entt)) {
                    current.construct(entt);
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
                if(current.has(entt)) {
                    current.destroy(entt);
                }
            } else {
                if(const auto cpools = std::forward_as_tuple(owner.assure<Owned>()...); std::get<0>(cpools).has(entt) && (std::get<0>(cpools).index(entt) < current)) {
                    const auto pos = --current;
                    (std::get<pool_handler<Owned> &>(cpools).swap(std::get<pool_handler<Owned> &>(cpools).data()[pos], entt), ...);
                }
            }
        }
    };

    struct group_data {
        std::size_t size;
        std::unique_ptr<void, void(*)(void *)> group;
        bool (* owned)(const ENTT_ID_TYPE) ENTT_NOEXCEPT;
        bool (* get)(const ENTT_ID_TYPE) ENTT_NOEXCEPT;
        bool (* exclude)(const ENTT_ID_TYPE) ENTT_NOEXCEPT;
    };

    struct basic_variable {
        virtual ~basic_variable() = default;
        virtual ENTT_ID_TYPE type_id() const ENTT_NOEXCEPT = 0;
    };

    template<typename Type>
    struct variable_handler: basic_variable {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>);
        Type value;

        template<typename... Args>
        variable_handler(Args &&... args)
            : value{std::forward<Args>(args)...}
        {}

        ENTT_ID_TYPE type_id() const ENTT_NOEXCEPT override {
            return type_info<Type>::id();
        }
    };

    template<typename Component>
    const pool_handler<Component> & assure() const {
        static std::size_t index{pools.size()};

        if(const auto length = pools.size(); !(index < length) || pools[index].type_id != type_info<Component>::id()) {
            for(index = {}; index < length && pools[index].type_id != type_info<Component>::id(); ++index);

            if(index == pools.size()) {
                auto &&pdata = pools.emplace_back();

                pdata.type_id = type_info<Component>::id();
                pdata.pool.reset(new pool_handler<Component>());

                pdata.remove = [](sparse_set<entity_type> &cpool, basic_registry &owner, const entity_type entt) {
                    static_cast<pool_handler<Component> &>(cpool).remove(owner, entt);
                };

                if constexpr(std::is_copy_constructible_v<std::decay_t<Component>>) {
                    pdata.assure = [](basic_registry &other, const sparse_set<entity_type> &cpool) {
                        if constexpr(ENTT_ENABLE_ETO(Component)) {
                            other.assure<Component>().assign(other, cpool.begin(), cpool.end());
                        } else {
                            other.assure<Component>().assign(other, cpool.begin(), cpool.end(), static_cast<const pool_handler<Component> &>(cpool).cbegin());
                        }
                    };

                    pdata.stamp = [](basic_registry &other, const entity_type dst, const sparse_set<entity_type> &cpool, const entity_type src) {
                        other.assign_or_replace<Component>(dst, static_cast<const pool_handler<Component> &>(cpool).get(src));
                    };
                }
            }
        }

        return static_cast<pool_handler<Component> &>(*pools[index].pool);
    }

    template<typename Component>
    pool_handler<Component> & assure() {
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
        ENTT_ASSERT(std::none_of(pools.cbegin(), pools.cend(), [](auto &&pdata) { return pdata.type_id == type_info<Component>::id(); }));
        assure<Component>();
    }

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const {
        return assure<Component>().size();
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
    size_type alive() const {
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
    size_type capacity() const {
        return assure<Component>().capacity();
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
    bool empty() const {
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
     * There are no guarantees on the order of the components. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @note
     * Empty components aren't explicitly instantiated. Therefore, this function
     * isn't available for them. A compilation error will occur if invoked.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components of the given type.
     */
    template<typename Component>
    const Component * raw() const {
        return assure<Component>().raw();
    }

    /*! @copydoc raw */
    template<typename Component>
    Component * raw() {
        return const_cast<Component *>(std::as_const(*this).template raw<Component>());
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * There are no guarantees on the order of the entities. Use a view if you
     * want to iterate entities and components in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const {
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
    const entity_type * data() const ENTT_NOEXCEPT {
        return entities.data();
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    bool valid(const entity_type entity) const {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integral(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    static version_type version(const entity_type entity) ENTT_NOEXCEPT {
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
    version_type current(const entity_type entity) const {
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
            entt = entities.emplace_back(entity_type(entities.size()));
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
    entity_type create(const entity_type hint) {
        ENTT_ASSERT(hint != null);
        entity_type entt;

        if(const auto req = (to_integral(hint) & traits_type::entity_mask); !(req < entities.size())) {
            entities.reserve(req + 1);

            for(auto pos = entities.size(); pos < req; ++pos) {
                entities.emplace_back(destroyed);
                destroyed = entity_type(pos);
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
     * @brief Destroys an entity and lets the registry recycle the identifier.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time.
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
        ENTT_ASSERT(valid(entity));

        for(auto pos = pools.size(); pos; --pos) {
            if(auto &pdata = pools[pos-1]; pdata.pool->has(entity)) {
                pdata.remove(*pdata.pool, *this, entity);
            }
        }

        // just a way to protect users from listeners that attach components
        ENTT_ASSERT(orphan(entity));

        // lengthens the implicit list of destroyed entities
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        const auto version = ((to_integral(entity) >> traits_type::entity_shift) + 1) << traits_type::entity_shift;
        entities[entt] = entity_type{to_integral(destroyed) | version};
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
        // useless this-> used to suppress a warning with clang
        std::for_each(first, last, [this](const auto entity) { this->destroy(entity); });
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
    decltype(auto) assign(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        return assure<Component>().assign(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns each entity in a range the given component.
     *
     * @sa assign
     *
     * @tparam Component Type of component to create.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the component to assign.
     */
    template<typename Component, typename It>
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    assign(It first, It last, const Component &value = {}) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().assign(*this, first, last, value);
    }

    /**
     * @brief Assigns each entity in a range the given components.
     *
     * @sa assign
     *
     * @tparam Component Type of component to create.
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An iterator to the first element of the range of components.
     */
    template<typename Component, typename EIt, typename CIt>
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<EIt>::value_type, entity_type>, void>
    assign(EIt first, EIt last, CIt value) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entity) { return valid(entity); }));
        assure<Component>().assign(*this, first, last, value);
    }

    /**
     * @brief Assigns entities to an empty registry.
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode if all
     * pools aren't empty. Groups and context variables are ignored.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void assign(It first, It last) {
        ENTT_ASSERT(std::all_of(pools.cbegin(), pools.cend(), [](auto &&cpool) { return cpool.pool->empty(); }));
        entities.assign(first, last);
        destroyed = null;

        for(std::size_t pos{}, end = entities.size(); pos < end; ++pos) {
            if((to_integral(entities[pos]) & traits_type::entity_mask) != pos) {
                const auto version = to_integral(entities[pos]) & (traits_type::version_mask << traits_type::entity_shift);
                entities[pos] = entity_type{to_integral(destroyed) | version};
                destroyed = entity_type(pos);
            }
        }
    }

    /**
     * @brief Assigns or replaces the given component for an entity.
     *
     * Equivalent to the following snippet (pseudocode):
     *
     * @code{.cpp}
     * auto &component = registry.has<Component>(entity) ? registry.replace<Component>(entity, args...) : registry.assign<Component>(entity, args...);
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
    decltype(auto) assign_or_replace(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();

        return cpool.has(entity)
                ? (cpool.replace(*this, entity, [&args...](auto &&component) { component = Component{std::forward<Args>(args)...}; }), cpool.get(entity))
                : cpool.assign(*this, entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces the given component for an entity.
     *
     * The signature of the functions should be equivalent to the following:
     *
     * @code{.cpp}
     * void(Component &);
     * @endcode
     *
     * Temporary objects are returned for empty types though. Capture them by
     * copy or by const reference if needed.
     *
     * @tparam Component Type of component to replace.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     */
    template<typename Component, typename... Func>
    auto replace(const entity_type entity, Func &&... func)
    -> decltype(std::enable_if_t<sizeof...(Func) != 0>(), (func(std::declval<Component &>()), ...), void()) {
        ENTT_ASSERT(valid(entity));
        assure<Component>().replace(*this, entity, std::forward<Func>(func)...);
    }

    /**
     * @copybrief replace
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
    [[deprecated("use in-place replace instead")]]
    auto replace(const entity_type entity, Args &&... args)
    -> decltype(Component{std::forward<Args>(args)...}, assure<Component>().get(entity)) {
        replace<Component>(entity, [args = std::forward_as_tuple(std::forward<Args>(args)...)](auto &&component) {
            component = std::make_from_tuple<Component>(std::move(args));
        });

        return assure<Component>().get(entity);
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
     */
    template<typename... Component>
    void remove_if_exists(const entity_type entity) {
        ENTT_ASSERT(valid(entity));
        ([this, entity](auto &&cpool) { if(cpool.has(entity)) { cpool.remove(*this, entity); } }(assure<Component>()), ...);
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
    bool has(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().has(entity) && ...);
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
    bool any(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return (assure<Component>().has(entity) || ...);
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
    decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().get(entity), ...);
        } else {
            return std::forward_as_tuple(get<Component>(entity)...);
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entity) {
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
     * auto &component = registry.has<Component>(entity) ? registry.get<Component>(entity) : registry.assign<Component>(entity, args...);
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
    decltype(auto) get_or_assign(const entity_type entity, Args &&... args) {
        ENTT_ASSERT(valid(entity));
        auto &cpool = assure<Component>();
        return cpool.has(entity) ? cpool.get(entity) : cpool.assign(*this, entity, std::forward<Args>(args)...);
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
    auto try_get([[maybe_unused]] const entity_type entity) const {
        ENTT_ASSERT(valid(entity));

        if constexpr(sizeof...(Component) == 1) {
            return (assure<Component>().try_get(entity), ...);
        } else {
            return std::make_tuple(try_get<Component>(entity)...);
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    auto try_get([[maybe_unused]] const entity_type entity) {
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
            ([this](auto &&cpool) { cpool.remove(*this, cpool.sparse_set<entity_type>::begin(), cpool.sparse_set<entity_type>::end()); }(assure<Component>()), ...);
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
        static_assert(std::is_invocable_v<Func, entity_type>);

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
    bool orphan(const entity_type entity) const {
        ENTT_ASSERT(valid(entity));
        return std::none_of(pools.cbegin(), pools.cend(), [entity](auto &&pdata) { return pdata.pool->has(entity); });
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
        static_assert(std::is_invocable_v<Func, entity_type>);

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
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_construct() {
        return assure<Component>().on_construct();
    }

    /**
     * @brief Returns a sink object for the given component.
     *
     * A sink is an opaque object used to connect listeners to components.<br/>
     * The sink returned by this function can be used to receive notifications
     * whenever an instance of the given component is explicitly replaced.
     *
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(registry<Entity> &, Entity, Component &);
     * @endcode
     *
     * Listeners are invoked **before** the component has been replaced.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_replace() {
        return assure<Component>().on_replace();
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
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned through signals. They can be caught only by copy or with
     * const references.
     *
     * @sa sink
     *
     * @tparam Component Type of component of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Component>
    auto on_destroy() {
        return assure<Component>().on_destroy();
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
        auto &cpool = assure<Component>();
        ENTT_ASSERT(!cpool.super);
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
        auto &cpool = assure<To>();
        ENTT_ASSERT(!cpool.super);
        cpool.respect(assure<From>());
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
    entt::basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Component) > 0);
        return { assure<std::decay_t<Component>>()..., assure<Exclude>()... };
    }

    /*! @copydoc view */
    template<typename... Component, typename... Exclude>
    entt::basic_view<Entity, exclude_t<Exclude...>, Component...> view(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Component>...>);
        return const_cast<basic_registry *>(this)->view<Component...>(exclude<Exclude...>);
    }

    /**
     * @brief Checks whether the given components belong to any group.
     * @tparam Component Types of components in which one is interested.
     * @return True if the pools of the given components are sortable, false
     * otherwise.
     */
    template<typename... Component>
    bool sortable() const {
        return !(assure<Component>().super || ...);
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) {
        static_assert(sizeof...(Owned) + sizeof...(Get) > 0);
        static_assert(sizeof...(Owned) + sizeof...(Get) + sizeof...(Exclude) > 1);

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
                []([[maybe_unused]] const ENTT_ID_TYPE ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Owned>>::id()) || ...); },
                []([[maybe_unused]] const ENTT_ID_TYPE ctype) ENTT_NOEXCEPT { return ((ctype == type_info<std::decay_t<Get>>::id()) || ...); },
                []([[maybe_unused]] const ENTT_ID_TYPE ctype) ENTT_NOEXCEPT { return ((ctype == type_info<Exclude>::id()) || ...); },
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
                    return !(0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id())) || (size > (gdata.size));
                });

                const auto prev = std::find_if(std::make_reverse_iterator(next), groups.crend(), [](const auto &gdata) {
                    return (0u + ... + gdata.owned(type_info<std::decay_t<Owned>>::id()));
                });

                maybe_valid_if = (next == groups.cend() ? maybe_valid_if : next->group.get());
                discard_if = (prev == groups.crend() ? discard_if : prev->group.get());
                groups.insert(next, std::move(candidate));
            }

            ((std::get<pool_handler<std::decay_t<Owned>> &>(cpools).super = std::max(std::get<pool_handler<std::decay_t<Owned>> &>(cpools).super, size)), ...);

            (on_construct<std::decay_t<Owned>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Owned>>>(*handler), ...);
            (on_construct<std::decay_t<Get>>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<std::decay_t<Get>>>(*handler), ...);
            (on_destroy<Exclude>().before(maybe_valid_if).template connect<&handler_type::template maybe_valid_if<Exclude>>(*handler), ...);

            (on_destroy<std::decay_t<Owned>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_destroy<std::decay_t<Get>>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);
            (on_construct<Exclude>().before(discard_if).template connect<&handler_type::discard_if>(*handler), ...);

            if constexpr(sizeof...(Owned) == 0) {
                for(const auto entity: view<Owned..., Get...>(entt::exclude<Exclude...>)) {
                    handler->current.construct(entity);
                }
            } else {
                // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
                std::for_each(std::get<0>(cpools).data(), std::get<0>(cpools).data() + std::get<0>(cpools).size(), [this, handler](const auto entity) {
                    handler->template maybe_valid_if<std::tuple_element_t<0, std::tuple<std::decay_t<Owned>...>>>(*this, entity);
                });
            }
        }

        if constexpr(sizeof...(Owned) == 0) {
            return { handler->current, std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
        } else {
            return { std::get<0>(cpools).super, handler->current, std::get<pool_handler<std::decay_t<Owned>> &>(cpools)... , std::get<pool_handler<std::decay_t<Get>> &>(cpools)... };
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> group(get_t<Get...>, exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>..., std::is_const<Get>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(entt::get<Get...>, exclude<Exclude...>);
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) {
        return group<Owned...>(entt::get<>, exclude<Exclude...>);
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
    entt::basic_group<Entity, exclude_t<Exclude...>, get_t<>, Owned...> group(exclude_t<Exclude...> = {}) const {
        static_assert(std::conjunction_v<std::is_const<Owned>...>);
        return const_cast<basic_registry *>(this)->group<Owned...>(exclude<Exclude...>);
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
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of components.
     * @param last An iterator past the last element of the range of components.
     * @return A newly created runtime view.
     */
    template<typename It>
    entt::basic_runtime_view<Entity> runtime_view(It first, It last) const {
        std::vector<const sparse_set<Entity> *> selected(std::distance(first, last));

        std::transform(first, last, selected.begin(), [this](const auto ctype) {
            const auto it = std::find_if(pools.cbegin(), pools.cend(), [ctype](auto &&pdata) { return pdata.type_id == ctype; });
            return it == pools.cend() ? nullptr : it->pool.get();
        });

        return { std::move(selected) };
    }

    /**
     * @brief Returns a full or partial copy of a registry.
     *
     * The components must be copyable for obvious reasons. The entities
     * maintain their versions once copied.<br/>
     * If no components are provided, the registry will try to clone all the
     * existing pools. The ones for non-copyable types won't be cloned.
     *
     * This feature supports exclusion lists. The excluded types have higher
     * priority than those indicated for cloning. An excluded type will never be
     * cloned.
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
     * Attempting to clone components that aren't copyable results in unexpected
     * behaviors.<br/>
     * A static assertion will abort the compilation when the components
     * provided aren't copy constructible. Otherwise, an assertion will abort
     * the execution at runtime in debug mode in case one or more pools cannot
     * be cloned.
     *
     * @tparam Component Types of components to clone.
     * @tparam Exclude Types of components not to be cloned.
     * @return A fresh copy of the registry.
     */
    template<typename... Component, typename... Exclude>
    [[deprecated("use ::visit and custom (eventually erased) functions instead")]]
    basic_registry clone(exclude_t<Exclude...> = {}) const {
        basic_registry other;

        other.destroyed = destroyed;
        other.entities = entities;

        std::for_each(pools.cbegin(), pools.cend(), [&other](auto &&pdata) {
            if constexpr(sizeof...(Component) == 0) {
                if(pdata.assure && ((pdata.type_id != type_info<Exclude>::id()) && ...)) {
                    pdata.assure(other, *pdata.pool);
                }
            } else {
                static_assert(sizeof...(Exclude) == 0 && std::conjunction_v<std::is_copy_constructible<Component>...>);
                if(pdata.assure && ((pdata.type_id == type_info<Component>::id()) || ...)) {
                    pdata.assure(other, *pdata.pool);
                }
            }
        });

        return other;
    }

    /**
     * @brief Stamps an entity onto another entity.
     *
     * This function supports exclusion lists. An excluded type will never be
     * copied.
     *
     * @warning
     * Attempting to copy components that aren't copyable results in unexpected
     * behaviors.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * one or more types cannot be copied.
     *
     * @warning
     * Attempting to use invalid entities results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entities.
     *
     * @tparam Exclude Types of components not to be copied.
     * @param dst A valid entity identifier to copy to.
     * @param other The registry that owns the source entity.
     * @param src A valid entity identifier to be copied.
     */
    template<typename... Exclude>
    [[deprecated("use ::visit and custom (eventually erased) functions instead")]]
    void stamp(const entity_type dst, const basic_registry &other, const entity_type src, exclude_t<Exclude...> = {}) {
        std::for_each(other.pools.cbegin(), other.pools.cend(), [this, dst, src](auto &&pdata) {
            if(((pdata.type_id != type_info<Exclude>::id()) && ...) && pdata.pool->has(src)) {
                ENTT_ASSERT(pdata.stamp);
                pdata.stamp(*this, dst, *pdata.pool, src);
            }
        });
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
    entt::basic_snapshot<Entity> snapshot() const {
        using follow_fn_type = entity_type(const basic_registry &, const entity_type);

        const auto head = to_integral(destroyed);
        const entity_type seed = (destroyed == null) ? destroyed : entity_type{head | (to_integral(entities[head]) & (traits_type::version_mask << traits_type::entity_shift))};

        follow_fn_type *follow = [](const basic_registry &reg, const entity_type entity) -> entity_type {
            const auto &others = reg.entities;
            const auto entt = to_integral(entity) & traits_type::entity_mask;
            const auto curr = to_integral(others[entt]) & traits_type::entity_mask;
            return entity_type{curr | (to_integral(others[curr]) & (traits_type::version_mask << traits_type::entity_shift))};
        };

        return { this, seed, follow };
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
    basic_snapshot_loader<Entity> loader() {
        using force_fn_type = void(basic_registry &, const entity_type, const bool);

        force_fn_type *force = [](basic_registry &reg, const entity_type entity, const bool drop) {
            const auto entt = to_integral(entity) & traits_type::entity_mask;
            auto &others = reg.entities;

            if(!(entt < others.size())) {
                auto curr = others.size();
                others.resize(entt + 1);
                std::generate(others.data() + curr, others.data() + entt, [&curr]() { return entity_type(curr++); });
            }

            others[entt] = entity;

            if(drop) {
                reg.destroy(entity);
                const auto version = to_integral(entity) & (traits_type::version_mask << traits_type::entity_shift);
                others[entt] = entity_type{(to_integral(others[entt]) & traits_type::entity_mask) | version};
            }
        };

        clear();
        entities.clear();
        destroyed = null;

        return { this, force };
    }

    /**
     * @brief Visits an entity and returns the types for its components.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const ENTT_ID_TYPE);
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
            if(auto &pdata = pools[pos-1]; pdata.pool->has(entity)) {
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
     * void(const ENTT_ID_TYPE);
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
            func(pools[pos-1].type_id);
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
        vars.emplace_back(new variable_handler<Type>{std::forward<Args>(args)...});
        return static_cast<variable_handler<Type> &>(*vars.back()).value;
    }

    /**
     * @brief Unsets a context variable if it exists.
     * @tparam Type Type of object to set.
     */
    template<typename Type>
    void unset() {
        vars.erase(std::remove_if(vars.begin(), vars.end(), [](auto &&handler) {
            return handler->type_id() == type_info<Type>::id();
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
    Type & ctx_or_set(Args &&... args) {
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
    const Type * try_ctx() const {
        auto it = std::find_if(vars.cbegin(), vars.cend(), [](auto &&handler) { return handler->type_id() == type_info<Type>::id(); });
        return it == vars.cend() ? nullptr : &static_cast<const variable_handler<Type> &>(*it->get()).value;
    }

    /*! @copydoc try_ctx */
    template<typename Type>
    Type * try_ctx() {
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
    const Type & ctx() const {
        const auto *instance = try_ctx<Type>();
        ENTT_ASSERT(instance);
        return *instance;
    }

    /*! @copydoc ctx */
    template<typename Type>
    Type & ctx() {
        return const_cast<Type &>(std::as_const(*this).template ctx<Type>());
    }

    /**
     * @brief Visits a registry and returns the types for its context variables.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const ENTT_ID_TYPE);
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
            func(vars[pos-1]->type_id());
        }
    }

private:
    std::vector<group_data> groups{};
    mutable std::vector<pool_data> pools{};
    std::vector<std::unique_ptr<basic_variable>> vars{};
    std::vector<entity_type> entities{};
    entity_type destroyed{null};
};


}


#endif
