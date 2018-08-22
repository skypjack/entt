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
#include "utility.hpp"


namespace entt {


/**
 * @brief Forward declaration of the registry class.
 */
template<typename>
class Registry;


/**
 * @brief Utility class to create snapshots from a registry.
 *
 * A _snapshot_ can be either a dump of the entire registry or a narrower
 * selection of components and tags of interest.<br/>
 * This type can be used in both cases if provided with a correctly configured
 * output archive.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Snapshot final {
    /*! @brief A registry is allowed to create snapshots. */
    friend class Registry<Entity>;

    using follow_fn_type = Entity(const Registry<Entity> &, const Entity);

    Snapshot(const Registry<Entity> &registry, Entity seed, follow_fn_type *follow) ENTT_NOEXCEPT
        : registry{registry},
          seed{seed},
          follow{follow}
    {}

    template<typename Component, typename Archive, typename It>
    void get(Archive &archive, std::size_t sz, It first, It last) const {
        archive(static_cast<Entity>(sz));

        while(first != last) {
            const auto entity = *(first++);

            if(registry.template has<Component>(entity)) {
                archive(entity, registry.template get<Component>(entity));
            }
        }
    }

    template<typename... Component, typename Archive, typename It, std::size_t... Indexes>
    void component(Archive &archive, It first, It last, std::index_sequence<Indexes...>) const {
        std::array<std::size_t, sizeof...(Indexes)> size{};
        auto begin = first;

        while(begin != last) {
            const auto entity = *(begin++);
            using accumulator_type = std::size_t[];
            accumulator_type accumulator = { (registry.template has<Component>(entity) ? ++size[Indexes] : size[Indexes])... };
            (void)accumulator;
        }

        using accumulator_type = int[];
        accumulator_type accumulator = { (get<Component>(archive, size[Indexes], first, last), 0)... };
        (void)accumulator;
    }

public:
    /*! @brief Copying a snapshot isn't allowed. */
    Snapshot(const Snapshot &) = delete;
    /*! @brief Default move constructor. */
    Snapshot(Snapshot &&) = default;

    /*! @brief Copying a snapshot isn't allowed. @return This snapshot. */
    Snapshot & operator=(const Snapshot &) = delete;
    /*! @brief Default move assignment operator. @return This snapshot. */
    Snapshot & operator=(Snapshot &&) = default;

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
    const Snapshot & entities(Archive &archive) const {
        archive(static_cast<Entity>(registry.alive()));
        registry.each([&archive](const auto entity) { archive(entity); });
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
    const Snapshot & destroyed(Archive &archive) const {
        auto size = registry.size() - registry.alive();
        archive(static_cast<Entity>(size));

        if(size) {
            auto curr = seed;
            archive(curr);

            for(--size; size; --size) {
                curr = follow(registry, curr);
                archive(curr);
            }
        }

        return *this;
    }

    /**
     * @brief Puts aside the given component.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Component Type of component to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Component, typename Archive>
    const Snapshot & component(Archive &archive) const {
        const auto sz = registry.template size<Component>();
        const auto *entities = registry.template data<Component>();

        archive(static_cast<Entity>(sz));

        for(std::remove_const_t<decltype(sz)> i{}; i < sz; ++i) {
            const auto entity = entities[i];
            archive(entity, registry.template get<Component>(entity));
        };

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
    std::enable_if_t<(sizeof...(Component) > 1), const Snapshot &>
    component(Archive &archive) const {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (component<Component>(archive), 0)... };
        (void)accumulator;
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
    const Snapshot & component(Archive &archive, It first, It last) const {
        component<Component...>(archive, first, last, std::make_index_sequence<sizeof...(Component)>{});
        return *this;
    }

    /**
     * @brief Puts aside the given tag.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Tag Type of tag to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Tag, typename Archive>
    const Snapshot & tag(Archive &archive) const {
        const bool has = registry.template has<Tag>();

        // numerical length is forced for tags to facilitate loading
        archive(has ? Entity(1): Entity{});

        if(has) {
            archive(registry.template attachee<Tag>(), registry.template get<Tag>());
        }

        return *this;
    }

    /**
     * @brief Puts aside the given tags.
     *
     * Each instance is serialized together with the entity to which it belongs.
     * Entities are serialized along with their versions.
     *
     * @tparam Tag Types of tags to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename... Tag, typename Archive>
    std::enable_if_t<(sizeof...(Tag) > 1), const Snapshot &>
    tag(Archive &archive) const {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (tag<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

private:
    const Registry<Entity> &registry;
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
class SnapshotLoader final {
    /*! @brief A registry is allowed to create snapshot loaders. */
    friend class Registry<Entity>;

    using assure_fn_type = void(Registry<Entity> &, const Entity, const bool);

    SnapshotLoader(Registry<Entity> &registry, assure_fn_type *assure_fn) ENTT_NOEXCEPT
        : registry{registry},
          assure_fn{assure_fn}
    {
        // restore a snapshot as a whole requires a clean registry
        assert(!registry.capacity());
    }

    template<typename Archive>
    void assure(Archive &archive, bool destroyed) const {
        Entity length{};
        archive(length);

        while(length--) {
            Entity entity{};
            archive(entity);
            assure_fn(registry, entity, destroyed);
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
            assure_fn(registry, entity, destroyed);
            registry.template assign<Type>(args..., entity, static_cast<const Type &>(instance));
        }
    }

public:
    /*! @brief Copying a snapshot loader isn't allowed. */
    SnapshotLoader(const SnapshotLoader &) = delete;
    /*! @brief Default move constructor. */
    SnapshotLoader(SnapshotLoader &&) = default;

    /*! @brief Copying a snapshot loader isn't allowed. @return This loader. */
    SnapshotLoader & operator=(const SnapshotLoader &) = delete;
    /*! @brief Default move assignment operator. @return This loader. */
    SnapshotLoader & operator=(SnapshotLoader &&) = default;

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
    const SnapshotLoader & entities(Archive &archive) const {
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
    const SnapshotLoader & destroyed(Archive &archive) const {
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
    const SnapshotLoader & component(Archive &archive) const {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief Restores tags and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the tag is assigned
     * doesn't exist yet, the loader will take care to create it with the
     * version it originally had.
     *
     * @tparam Tag Types of tags to restore.
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @return A valid loader to continue restoring data.
     */
    template<typename... Tag, typename Archive>
    const SnapshotLoader & tag(Archive &archive) const {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Tag>(archive, tag_t{}), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief Destroys those entities that have neither components nor tags.
     *
     * In case all the entities were serialized but only part of the components
     * and tags was saved, it could happen that some of the entities have
     * neither components nor tags once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A valid loader to continue restoring data.
     */
    const SnapshotLoader & orphans() const {
        registry.orphans([this](const auto entity) {
            registry.destroy(entity);
        });

        return *this;
    }

private:
    Registry<Entity> &registry;
    assure_fn_type *assure_fn;
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
class ContinuousLoader final {
    using traits_type = entt_traits<Entity>;

    void destroy(Entity entity) {
        const auto it = remloc.find(entity);

        if(it == remloc.cend()) {
            const auto local = registry.create();
            remloc.emplace(entity, std::make_pair(local, true));
            registry.destroy(local);
        }
    }

    void restore(Entity entity) {
        const auto it = remloc.find(entity);

        if(it == remloc.cend()) {
            const auto local = registry.create();
            remloc.emplace(entity, std::make_pair(local, true));
        } else {
            remloc[entity].first =
                    registry.valid(remloc[entity].first)
                    ? remloc[entity].first
                    : registry.create();

            // set the dirty flag
            remloc[entity].second = true;
        }
    }

    template<typename Type, typename Member>
    std::enable_if_t<std::is_same<Member, Entity>::value>
    update(Type &instance, Member Type:: *member) {
        instance.*member = map(instance.*member);
    }

    template<typename Type, typename Member>
    std::enable_if_t<std::is_same<typename std::iterator_traits<typename Member::iterator>::value_type, Entity>::value>
    update(Type &instance, Member Type:: *member) {
        for(auto &entity: instance.*member) {
            entity = map(entity);
        }
    }

    template<typename Other, typename Type, typename Member>
    std::enable_if_t<!std::is_same<Other, Type>::value>
    update(Other &, Member Type:: *) {}

    template<typename Archive>
    void assure(Archive &archive, void(ContinuousLoader:: *member)(Entity)) {
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

            if(registry.valid(local)) {
                registry.template reset<Component>(local);
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

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (update(instance, member), 0)... };
            (void)accumulator;

            func(map(entity), instance);
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs a loader that is bound to a given registry.
     * @param registry A valid reference to a registry.
     */
    ContinuousLoader(Registry<entity_type> &registry) ENTT_NOEXCEPT
        : registry{registry}
    {}

    /*! @brief Copying a snapshot loader isn't allowed. */
    ContinuousLoader(const ContinuousLoader &) = delete;
    /*! @brief Default move constructor. */
    ContinuousLoader(ContinuousLoader &&) = default;

    /*! @brief Copying a snapshot loader isn't allowed. @return This loader. */
    ContinuousLoader & operator=(const ContinuousLoader &) = delete;
    /*! @brief Default move assignment operator. @return This loader. */
    ContinuousLoader & operator=(ContinuousLoader &&) = default;

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
    ContinuousLoader & entities(Archive &archive) {
        assure(archive, &ContinuousLoader::restore);
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
    ContinuousLoader & destroyed(Archive &archive) {
        assure(archive, &ContinuousLoader::destroy);
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
    ContinuousLoader & component(Archive &archive, Member Type:: *... member) {
        auto apply = [this](const auto entity, const auto &component) {
            registry.template accommodate<std::decay_t<decltype(component)>>(entity, component);
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (reset<Component>(), assign<Component>(archive, apply, member...), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief Restores tags and assigns them to the right entities.
     *
     * The template parameter list must be exactly the same used during
     * serialization. In the event that the entity to which the tag is assigned
     * doesn't exist yet, the loader will take care to create a local
     * counterpart for it.<br/>
     * Members can be either data members of type entity_type or containers of
     * entities. In both cases, the loader will visit them and update the
     * entities by replacing each one with its local counterpart.
     *
     * @tparam Tag Type of tag to restore.
     * @tparam Archive Type of input archive.
     * @tparam Type Types of components to update with local counterparts.
     * @tparam Member Types of members to update with their local counterparts.
     * @param archive A valid reference to an input archive.
     * @param member Members to update with their local counterparts.
     * @return A non-const reference to this loader.
     */
    template<typename... Tag, typename Archive, typename... Type, typename... Member>
    ContinuousLoader & tag(Archive &archive, Member Type:: *... member) {
        auto apply = [this](const auto entity, const auto &tag) {
            registry.template assign<std::decay_t<decltype(tag)>>(tag_t{}, entity, tag);
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (registry.template remove<Tag>(), assign<Tag>(archive, apply, member...), 0)... };
        (void)accumulator;
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
    ContinuousLoader & shrink() {
        auto it = remloc.begin();

        while(it != remloc.cend()) {
            const auto local = it->second.first;
            bool &dirty = it->second.second;

            if(dirty) {
                dirty = false;
                ++it;
            } else {
                if(registry.valid(local)) {
                    registry.destroy(local);
                }

                it = remloc.erase(it);
            }
        }

        return *this;
    }

    /**
     * @brief Destroys those entities that have neither components nor tags.
     *
     * In case all the entities were serialized but only part of the components
     * and tags was saved, it could happen that some of the entities have
     * neither components nor tags once restored.<br/>
     * This functions helps to identify and destroy those entities.
     *
     * @return A non-const reference to this loader.
     */
    ContinuousLoader & orphans() {
        registry.orphans([this](const auto entity) {
            registry.destroy(entity);
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
    Registry<Entity> &registry;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
