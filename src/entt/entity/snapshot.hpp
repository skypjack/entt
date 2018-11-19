#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <array>
#include <cstddef>
#include <utility>
#include <cassert>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include "../config/config.h"
#include "entt_traits.hpp"


namespace entt {


/**
 * @brief Forward declaration of the registry class.
 */
template<typename>
class registry;


/**
 * @brief Utility class to create snapshots from a registry.
 *
 * A _snapshot_ can be either a dump of the entire registry or a narrower
 * selection of components of interest.<br/>
 * This type can be used in both cases if provided with a correctly configured
 * output archive.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class snapshot final {
    /*! @brief A registry is allowed to create snapshots. */
    friend class registry<Entity>;

    using follow_fn_type = Entity(const registry<Entity> &, const Entity);

    snapshot(const registry<Entity> &reg, Entity seed, follow_fn_type *follow) ENTT_NOEXCEPT
        : reg{reg},
          seed{seed},
          follow{follow}
    {}

    template<typename Component, typename Archive, typename It>
    void get(Archive &archive, std::size_t sz, It first, It last) const {
        archive(static_cast<Entity>(sz));

        while(first != last) {
            const auto entity = *(first++);

            if(reg.template has<Component>(entity)) {
                archive(entity, reg.template get<Component>(entity));
            }
        }
    }

    template<typename... Component, typename Archive, typename It, std::size_t... Indexes>
    void component(Archive &archive, It first, It last, std::index_sequence<Indexes...>) const {
        std::array<std::size_t, sizeof...(Indexes)> size{};
        auto begin = first;

        while(begin != last) {
            const auto entity = *(begin++);
            ((reg.template has<Component>(entity) ? ++size[Indexes] : size[Indexes]), ...);
        }

        (get<Component>(archive, size[Indexes], first, last), ...);
    }

public:
    /*! @brief Copying a snapshot isn't allowed. */
    snapshot(const snapshot &) = delete;
    /*! @brief Default move constructor. */
    snapshot(snapshot &&) = default;

    /*! @brief Copying a snapshot isn't allowed. @return This snapshot. */
    snapshot & operator=(const snapshot &) = delete;
    /*! @brief Default move assignment operator. @return This snapshot. */
    snapshot & operator=(snapshot &&) = default;

    /**
     * @brief Puts aside all the entities that are still in use.
     *
     * Entities are serialized along with their versions. Destroyed entities are
     * not taken in consideration by this function.
     *
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Archive>
    const snapshot & entities(Archive &archive) const {
        archive(static_cast<Entity>(reg.alive()));
        reg.each([&archive](const auto entity) { archive(entity); });
        return *this;
    }

    /**
     * @brief Puts aside destroyed entities.
     *
     * Entities are serialized along with their versions. Entities that are
     * still in use are not taken in consideration by this function.
     *
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Archive>
    const snapshot & destroyed(Archive &archive) const {
        auto size = reg.size() - reg.alive();
        archive(static_cast<Entity>(size));

        if(size) {
            auto curr = seed;
            archive(curr);

            for(--size; size; --size) {
                curr = follow(reg, curr);
                archive(curr);
            }
        }

        return *this;
    }

    /**
     * @brief Puts aside the given components.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Component Types of components to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename... Component, typename Archive>
    const snapshot & component(Archive &archive) const {
        if constexpr(sizeof...(Component) == 1) {
            const auto sz = reg.template size<Component...>();
            const auto *entities = reg.template data<Component...>();

            archive(static_cast<Entity>(sz));

            for(std::remove_const_t<decltype(sz)> i{}; i < sz; ++i) {
                const auto entity = entities[i];
                archive(entity, reg.template get<Component...>(entity));
            };
        } else {
            (component<Component>(archive), ...);
        }

        return *this;
    }

    /**
     * @brief Puts aside the given components for the entities in a range.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Component Types of components to serialize.
     * @tparam Archive Type of output archive.
     * @tparam It Type of input iterator.
     * @param archive A valid reference to an output archive.
     * @param first An iterator to the first element of the range to serialize.
     * @param last An iterator past the last element of the range to serialize.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename... Component, typename Archive, typename It>
    const snapshot & component(Archive &archive, It first, It last) const {
        component<Component...>(archive, first, last, std::make_index_sequence<sizeof...(Component)>{});
        return *this;
    }

private:
    const registry<Entity> &reg;
    const Entity seed;
    follow_fn_type *follow;
};


/**
 * @brief Utility class to restore a snapshot as a whole.
 *
 * A snapshot loader requires that the destination registry be empty and loads
 * all the data at once while keeping intact the identifiers that the entities
 * originally had.<br/>
 * An example of use is the implementation of a save/restore utility.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class snapshot_loader final {
    /*! @brief A registry is allowed to create snapshot loaders. */
    friend class registry<Entity>;

    using force_fn_type = void(registry<Entity> &, const Entity, const bool);

    snapshot_loader(registry<Entity> &reg, force_fn_type *force) ENTT_NOEXCEPT
        : reg{reg},
          force{force}
    {
        // restore a snapshot as a whole requires a clean registry
        assert(!reg.capacity());
    }

    template<typename Archive>
    void assure(Archive &archive, bool destroyed) const {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entity{};
            archive(entity);
            force(reg, entity, destroyed);
        }
    }

    template<typename Type, typename Archive, typename... Args>
    void assign(Archive &archive, Args... args) const {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entity{};
            Type instance{};
            archive(entity, instance);
            static constexpr auto destroyed = false;
            force(reg, entity, destroyed);
            reg.template assign<Type>(args..., entity, std::as_const(instance));
        }
    }

public:
    /*! @brief Copying a snapshot loader isn't allowed. */
    snapshot_loader(const snapshot_loader &) = delete;
    /*! @brief Default move constructor. */
    snapshot_loader(snapshot_loader &&) = default;

    /*! @brief Copying a snapshot loader isn't allowed. @return This loader. */
    snapshot_loader & operator=(const snapshot_loader &) = delete;
    /*! @brief Default move assignment operator. @return This loader. */
    snapshot_loader & operator=(snapshot_loader &&) = default;

    /**
     * @brief Restores entities that were in use during serialization.
     *
     * This function restores the entities that were in use during serialization
     * and gives them the versions they originally had.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename Archive>
    const snapshot_loader & entities(Archive &archive) const {
        static constexpr auto destroyed = false;
        assure(archive, destroyed);
        return *this;
    }

    /**
     * @brief Restores entities that were destroyed during serialization.
     *
     * This function restores the entities that were destroyed during
     * serialization and gives them the versions they originally had.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename Archive>
    const snapshot_loader & destroyed(Archive &archive) const {
        static constexpr auto destroyed = true;
        assure(archive, destroyed);
        return *this;
    }

    /**
     * @brief Restores components and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the component is
     * assigned doesn't exist yet, the loader will take care to create it with
     * the version it originally had.
     *
     * @tparam Component Types of components to restore.
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename... Component, typename Archive>
    const snapshot_loader & component(Archive &archive) const {
        (assign<Component>(archive), ...);
        return *this;
    }

    /**
     * @brief Destroys those entities that have no components.
     *
     * In case all the entities were serialized but only part of the components
     * was saved, it could happen that some of the entities have no components
     * once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A valid loader to continue restoring data.
     */
    const snapshot_loader & orphans() const {
        reg.orphans([this](const auto entity) {
            reg.destroy(entity);
        });

        return *this;
    }

private:
    registry<Entity> &reg;
    force_fn_type *force;
};


/**
 * @brief Utility class for _continuous loading_.
 *
 * A _continuous loader_ is designed to load data from a source registry to a
 * (possibly) non-empty destination. The loader can accomodate in a registry
 * more than one snapshot in a sort of _continuous loading_ that updates the
 * destination one step at a time.<br/>
 * Identifiers that entities originally had are not transferred to the target.
 * Instead, the loader maps remote identifiers to local ones while restoring a
 * snapshot.<br/>
 * An example of use is the implementation of a client-server applications with
 * the requirement of transferring somehow parts of the representation side to
 * side.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class continuous_loader final {
    using traits_type = entt_traits<Entity>;

    void destroy(Entity entity) {
        const auto it = remloc.find(entity);

        if(it == remloc.cend()) {
            const auto local = reg.create();
            remloc.emplace(entity, std::make_pair(local, true));
            reg.destroy(local);
        }
    }

    void restore(Entity entity) {
        const auto it = remloc.find(entity);

        if(it == remloc.cend()) {
            const auto local = reg.create();
            remloc.emplace(entity, std::make_pair(local, true));
        } else {
            remloc[entity].first =
                    reg.valid(remloc[entity].first)
                    ? remloc[entity].first
                    : reg.create();

            // set the dirty flag
            remloc[entity].second = true;
        }
    }

    template<typename Other, typename Type, typename Member>
    void update(Other &instance, Member Type:: *member) {
        if constexpr(!std::is_same_v<Other, Type>) {
            return;
        } else if constexpr(std::is_same_v<Member, Entity>) {
            instance.*member = map(instance.*member);
        } else {
            // maybe a container? let's try...
            for(auto &entity: instance.*member) {
                entity = map(entity);
            }
        }
    }

    template<typename Archive>
    void assure(Archive &archive, void(continuous_loader:: *member)(Entity)) {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entity{};
            archive(entity);
            (this->*member)(entity);
        }
    }

    template<typename Component>
    void reset() {
        for(auto &&ref: remloc) {
            const auto local = ref.second.first;

            if(reg.valid(local)) {
                reg.template reset<Component>(local);
            }
        }
    }

    template<typename Other, typename Archive, typename Func, typename... Type, typename... Member>
    void assign(Archive &archive, Func func, Member Type:: *... member) {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entity{};
            Other instance{};
            archive(entity, instance);
            restore(entity);
            (update(instance, member), ...);
            func(map(entity), instance);
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs a loader that is bound to a given registry.
     * @param reg A valid reference to a registry.
     */
    continuous_loader(registry<entity_type> &reg) ENTT_NOEXCEPT
        : reg{reg}
    {}

    /*! @brief Copying a snapshot loader isn't allowed. */
    continuous_loader(const continuous_loader &) = delete;
    /*! @brief Default move constructor. */
    continuous_loader(continuous_loader &&) = default;

    /*! @brief Copying a snapshot loader isn't allowed. @return This loader. */
    continuous_loader & operator=(const continuous_loader &) = delete;
    /*! @brief Default move assignment operator. @return This loader. */
    continuous_loader & operator=(continuous_loader &&) = default;

    /**
     * @brief Restores entities that were in use during serialization.
     *
     * This function restores the entities that were in use during serialization
     * and creates local counterparts for them if required.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A non-const reference to this loader.
     */
    template<typename Archive>
    continuous_loader & entities(Archive &archive) {
        assure(archive, &continuous_loader::restore);
        return *this;
    }

    /**
     * @brief Restores entities that were destroyed during serialization.
     *
     * This function restores the entities that were destroyed during
     * serialization and creates local counterparts for them if required.
     *
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A non-const reference to this loader.
     */
    template<typename Archive>
    continuous_loader & destroyed(Archive &archive) {
        assure(archive, &continuous_loader::destroy);
        return *this;
    }

    /**
     * @brief Restores components and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the component is
     * assigned doesn't exist yet, the loader will take care to create a local
     * counterpart for it.<br/>
     * Members can be either data members of type entity_type or containers of
     * entities. In both cases, the loader will visit them and update the
     * entities by replacing each one with its local counterpart.
     *
     * @tparam Component Type of component to restore.
     * @tparam Archive Type of input archive.
     * @tparam Type Types of components to update with local counterparts.
     * @tparam Member Types of members to update with their local counterparts.
     * @param archive A valid reference to an input archive.
     * @param member Members to update with their local counterparts.
     * @return A non-const reference to this loader.
     */
    template<typename... Component, typename Archive, typename... Type, typename... Member>
    continuous_loader & component(Archive &archive, Member Type:: *... member) {
        auto apply = [this](const auto entity, const auto &component) {
            reg.template assign_or_replace<std::decay_t<decltype(component)>>(entity, component);
        };

        (reset<Component>(), ...);
        (assign<Component>(archive, apply, member...), ...);
        return *this;
    }

    /**
     * @brief Helps to purge entities that no longer have a conterpart.
     *
     * Users should invoke this member function after restoring each snapshot,
     * unless they know exactly what they are doing.
     *
     * @return A non-const reference to this loader.
     */
    continuous_loader & shrink() {
        auto it = remloc.begin();

        while(it != remloc.cend()) {
            const auto local = it->second.first;
            bool &dirty = it->second.second;

            if(dirty) {
                dirty = false;
                ++it;
            } else {
                if(reg.valid(local)) {
                    reg.destroy(local);
                }

                it = remloc.erase(it);
            }
        }

        return *this;
    }

    /**
     * @brief Destroys those entities that have no components.
     *
     * In case all the entities were serialized but only part of the components
     * was saved, it could happen that some of the entities have no components
     * once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A non-const reference to this loader.
     */
    continuous_loader & orphans() {
        reg.orphans([this](const auto entity) {
            reg.destroy(entity);
        });

        return *this;
    }

    /**
     * @brief Tests if a loader knows about a given entity.
     * @param entity An entity identifier.
     * @return True if `entity` is managed by the loader, false otherwise.
     */
    bool has(entity_type entity) const ENTT_NOEXCEPT {
        return (remloc.find(entity) != remloc.cend());
    }

    /**
     * @brief Returns the identifier to which an entity refers.
     *
     * @warning
     * Attempting to use an entity that isn't managed by the loader results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * loader doesn't knows about the entity.
     *
     * @param entity An entity identifier.
     * @return The identifier to which `entity` refers in the target registry.
     */
    entity_type map(entity_type entity) const ENTT_NOEXCEPT {
        assert(has(entity));
        return remloc.find(entity)->second.first;
    }

private:
    std::unordered_map<Entity, std::pair<Entity, bool>> remloc;
    registry<Entity> &reg;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
