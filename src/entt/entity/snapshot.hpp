#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <array>
#include <cstddef>
#include <utility>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include "../config/config.h"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


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
class basic_snapshot {
    /*! @brief A registry is allowed to create snapshots. */
    friend class basic_registry<Entity>;

    using follow_fn_type = Entity(const basic_registry<Entity> &, const Entity);
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    basic_snapshot(const basic_registry<Entity> *source, Entity init, follow_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          seed{init},
          follow{fn}
    {}

    template<typename Component, typename Archive, typename It>
    void get(Archive &archive, std::size_t sz, It first, It last) const {
        archive(typename traits_type::entity_type(sz));

        while(first != last) {
            const auto entt = *(first++);

            if(reg->template has<Component>(entt)) {
                if constexpr(std::is_empty_v<Component>) {
                    archive(entt);
                } else {
                    archive(entt, reg->template get<Component>(entt));
                }
            }
        }
    }

    template<typename... Component, typename Archive, typename It, std::size_t... Indexes>
    void component(Archive &archive, It first, It last, std::index_sequence<Indexes...>) const {
        std::array<std::size_t, sizeof...(Indexes)> size{};
        auto begin = first;

        while(begin != last) {
            const auto entt = *(begin++);
            ((reg->template has<Component>(entt) ? ++size[Indexes] : size[Indexes]), ...);
        }

        (get<Component>(archive, size[Indexes], first, last), ...);
    }

public:
    /*! @brief Default move constructor. */
    basic_snapshot(basic_snapshot &&) = default;

    /*! @brief Default move assignment operator. @return This snapshot. */
    basic_snapshot & operator=(basic_snapshot &&) = default;

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
    const basic_snapshot & entities(Archive &archive) const {
        archive(typename traits_type::entity_type(reg->alive()));
        reg->each([&archive](const auto entt) { archive(entt); });
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
    const basic_snapshot & destroyed(Archive &archive) const {
        auto size = reg->size() - reg->alive();
        archive(typename traits_type::entity_type(size));

        if(size) {
            auto curr = seed;
            archive(curr);

            for(--size; size; --size) {
                curr = follow(*reg, curr);
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
    const basic_snapshot & component(Archive &archive) const {
        if constexpr(sizeof...(Component) == 1) {
            const auto sz = reg->template size<Component...>();
            const auto *entities = reg->template data<Component...>();

            archive(typename traits_type::entity_type(sz));

            for(std::remove_const_t<decltype(sz)> pos{}; pos < sz; ++pos) {
                const auto entt = entities[pos];

                if constexpr(std::is_empty_v<Component...>) {
                    archive(entt);
                } else {
                    archive(entt, reg->template get<Component...>(entt));
                }
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
    const basic_snapshot & component(Archive &archive, It first, It last) const {
        component<Component...>(archive, first, last, std::make_index_sequence<sizeof...(Component)>{});
        return *this;
    }

private:
    const basic_registry<Entity> *reg;
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
class basic_snapshot_loader {
    /*! @brief A registry is allowed to create snapshot loaders. */
    friend class basic_registry<Entity>;

    using force_fn_type = void(basic_registry<Entity> &, const Entity, const bool);
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    basic_snapshot_loader(basic_registry<Entity> *source, force_fn_type *fn) ENTT_NOEXCEPT
        : reg{source},
          force{fn}
    {
        // to restore a snapshot as a whole requires a clean registry
        ENTT_ASSERT(reg->empty());
    }

    template<typename Archive>
    void assure(Archive &archive, bool discard) const {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            Entity entt{};
            archive(entt);
            force(*reg, entt, discard);
        }
    }

    template<typename Type, typename Archive, typename... Args>
    void assign(Archive &archive, Args... args) const {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            static constexpr auto discard = false;
            Entity entt{};

            if constexpr(std::is_empty_v<Type>) {
                archive(entt);
                force(*reg, entt, discard);
                reg->template assign<Type>(args..., entt);
            } else {
                Type instance{};
                archive(entt, instance);
                force(*reg, entt, discard);
                reg->template assign<Type>(args..., entt, std::as_const(instance));
            }
        }
    }

public:
    /*! @brief Default move constructor. */
    basic_snapshot_loader(basic_snapshot_loader &&) = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_snapshot_loader & operator=(basic_snapshot_loader &&) = default;

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
    const basic_snapshot_loader & entities(Archive &archive) const {
        static constexpr auto discard = false;
        assure(archive, discard);
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
    const basic_snapshot_loader & destroyed(Archive &archive) const {
        static constexpr auto discard = true;
        assure(archive, discard);
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
    const basic_snapshot_loader & component(Archive &archive) const {
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
    const basic_snapshot_loader & orphans() const {
        reg->orphans([this](const auto entt) {
            reg->destroy(entt);
        });

        return *this;
    }

private:
    basic_registry<Entity> *reg;
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
class basic_continuous_loader {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    void destroy(Entity entt) {
        const auto it = remloc.find(entt);

        if(it == remloc.cend()) {
            const auto local = reg->create();
            remloc.emplace(entt, std::make_pair(local, true));
            reg->destroy(local);
        }
    }

    void restore(Entity entt) {
        const auto it = remloc.find(entt);

        if(it == remloc.cend()) {
            const auto local = reg->create();
            remloc.emplace(entt, std::make_pair(local, true));
        } else {
            remloc[entt].first = reg->valid(remloc[entt].first) ? remloc[entt].first : reg->create();
            // set the dirty flag
            remloc[entt].second = true;
        }
    }

    template<typename T, typename U = void>
    struct has_key_value_impl : std::false_type { };
    template<typename T>
    struct has_key_value_impl<T, std::void_t<typename T::key_type,
                                             typename T::mapped_type,
                                             decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>>
        : std::true_type {};
    template<typename T>
    struct has_key_value : has_key_value_impl<T>::type { };

    template<typename Other, typename Type, typename Member>
    void update(Other &instance, Member Type:: *member) {
        if constexpr(!std::is_same_v<Other, Type>) {
            return;
        } else if constexpr(std::is_same_v<Member, Entity>) {
            instance.*member = map(instance.*member);
        } else if constexpr( has_key_value<Member>::value ){
            // it's some kind of map
            if constexpr(std::is_same_v<typename Member::key_type, Entity> && std::is_same_v<typename Member::mapped_type, Entity>){
                Member newMap;
                for(auto &pair : instance.*member) {
                    newMap.emplace(map(pair.first), map(pair.second));
                }
                instance.*member = std::move(newMap);
            } else if constexpr(std::is_same_v<typename Member::key_type, Entity>){
                Member newMap;
                for(auto &pair : instance.*member) {
                    newMap.emplace(map(pair.first), std::move(pair.second));
                }
                instance.*member = std::move(newMap);
            } else if constexpr(std::is_same_v<typename Member::mapped_type, Entity>) {
                for(auto &pair : instance.*member) {
                    pair.second = map(pair.second);
                }
            }

        } else {
            // maybe a container? let's try...
            for(auto &entt : instance.*member) {
                    entt = map(entt);
            }
        }
    }

    template<typename Archive>
    void assure(Archive &archive, void(basic_continuous_loader:: *member)(Entity)) {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            Entity entt{};
            archive(entt);
            (this->*member)(entt);
        }
    }

    template<typename Component>
    void reset() {
        for(auto &&ref: remloc) {
            const auto local = ref.second.first;

            if(reg->valid(local)) {
                reg->template reset<Component>(local);
            }
        }
    }

    template<typename Other, typename Archive, typename... Type, typename... Member>
    void assign(Archive &archive, [[maybe_unused]] Member Type:: *... member) {
        typename traits_type::entity_type length{};
        archive(length);

        while(length--) {
            Entity entt{};

            if constexpr(std::is_empty_v<Other>) {
                archive(entt);
                restore(entt);
                reg->template assign_or_replace<Other>(map(entt));
            } else {
                Other instance{};
                archive(entt, instance);
                (update(instance, member), ...);
                restore(entt);
                reg->template assign_or_replace<Other>(map(entt), std::as_const(instance));
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs a loader that is bound to a given registry.
     * @param source A valid reference to a registry.
     */
    basic_continuous_loader(basic_registry<entity_type> &source) ENTT_NOEXCEPT
        : reg{&source}
    {}

    /*! @brief Default move constructor. */
    basic_continuous_loader(basic_continuous_loader &&) = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_continuous_loader & operator=(basic_continuous_loader &&) = default;

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
    basic_continuous_loader & entities(Archive &archive) {
        assure(archive, &basic_continuous_loader::restore);
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
    basic_continuous_loader & destroyed(Archive &archive) {
        assure(archive, &basic_continuous_loader::destroy);
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
    basic_continuous_loader & component(Archive &archive, Member Type:: *... member) {
        (reset<Component>(), ...);
        (assign<Component>(archive, member...), ...);
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
    basic_continuous_loader & shrink() {
        auto it = remloc.begin();

        while(it != remloc.cend()) {
            const auto local = it->second.first;
            bool &dirty = it->second.second;

            if(dirty) {
                dirty = false;
                ++it;
            } else {
                if(reg->valid(local)) {
                    reg->destroy(local);
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
    basic_continuous_loader & orphans() {
        reg->orphans([this](const auto entt) {
            reg->destroy(entt);
        });

        return *this;
    }

    /**
     * @brief Tests if a loader knows about a given entity.
     * @param entt An entity identifier.
     * @return True if `entity` is managed by the loader, false otherwise.
     */
    bool has(entity_type entt) const ENTT_NOEXCEPT {
        return (remloc.find(entt) != remloc.cend());
    }

    /**
     * @brief Returns the identifier to which an entity refers.
     * @param entt An entity identifier.
     * @return The local identifier if any, the null entity otherwise.
     */
    entity_type map(entity_type entt) const ENTT_NOEXCEPT {
        const auto it = remloc.find(entt);
        entity_type other = null;

        if(it != remloc.cend()) {
            other = it->second.first;
        }

        return other;
    }

private:
    std::unordered_map<entity_type, std::pair<entity_type, bool>> remloc;
    basic_registry<entity_type> *reg;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
