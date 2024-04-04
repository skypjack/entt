#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "view.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Registry>
void orphans(Registry &registry) {
    auto &storage = registry.template storage<typename Registry::entity_type>();

    for(auto entt: storage) {
        if(registry.orphan(entt)) {
            storage.erase(entt);
        }
    }
}

} // namespace internal
/*! @endcond */

/**
 * @brief Utility class to create snapshots from a registry.
 *
 * A _snapshot_ can be either a dump of the entire registry or a narrower
 * selection of elements of interest.<br/>
 * This type can be used in both cases if provided with a correctly configured
 * output archive.
 *
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class basic_snapshot {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

public:
    /*! Basic registry type. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;

    /**
     * @brief Constructs an instance that is bound to a given registry.
     * @param source A valid reference to a registry.
     */
    basic_snapshot(const registry_type &source) noexcept
        : reg{&source} {}

    /*! @brief Default move constructor. */
    basic_snapshot(basic_snapshot &&) noexcept = default;

    /*! @brief Default move assignment operator. @return This snapshot. */
    basic_snapshot &operator=(basic_snapshot &&) noexcept = default;

    /**
     * @brief Serializes all elements of a type with associated identifiers.
     * @tparam Type Type of elements to serialize.
     * @tparam Archive Type of output archive.
     * @param archive A valid reference to an output archive.
     * @param id Optional name used to map the storage within the registry.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Type, typename Archive>
    const basic_snapshot &get(Archive &archive, const id_type id = type_hash<Type>::value()) const {
        if(const auto *storage = reg->template storage<Type>(id); storage) {
            archive(static_cast<typename traits_type::entity_type>(storage->size()));

            if constexpr(std::is_same_v<Type, entity_type>) {
                archive(static_cast<typename traits_type::entity_type>(storage->free_list()));

                for(auto first = storage->data(), last = first + storage->size(); first != last; ++first) {
                    archive(*first);
                }
            } else if constexpr(component_traits<Type>::in_place_delete) {
                const typename registry_type::common_type &base = *storage;

                for(auto it = base.rbegin(), last = base.rend(); it != last; ++it) {
                    if(const auto entt = *it; entt == tombstone) {
                        archive(static_cast<entity_type>(null));
                    } else {
                        archive(entt);
                        std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, storage->get_as_tuple(entt));
                    }
                }
            } else {
                for(auto elem: storage->reach()) {
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, elem);
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

    /**
     * @brief Serializes all elements of a type with associated identifiers for
     * the entities in a range.
     * @tparam Type Type of elements to serialize.
     * @tparam Archive Type of output archive.
     * @tparam It Type of input iterator.
     * @param archive A valid reference to an output archive.
     * @param first An iterator to the first element of the range to serialize.
     * @param last An iterator past the last element of the range to serialize.
     * @param id Optional name used to map the storage within the registry.
     * @return An object of this type to continue creating the snapshot.
     */
    template<typename Type, typename Archive, typename It>
    const basic_snapshot &get(Archive &archive, It first, It last, const id_type id = type_hash<Type>::value()) const {
        static_assert(!std::is_same_v<Type, entity_type>, "Entity types not supported");

        if(const auto *storage = reg->template storage<Type>(id); storage && !storage->empty()) {
            archive(static_cast<typename traits_type::entity_type>(std::distance(first, last)));

            for(; first != last; ++first) {
                if(const auto entt = *first; storage->contains(entt)) {
                    archive(entt);
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, storage->get_as_tuple(entt));
                } else {
                    archive(static_cast<entity_type>(null));
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

private:
    const registry_type *reg;
};

/**
 * @brief Utility class to restore a snapshot as a whole.
 *
 * A snapshot loader requires that the destination registry be empty and loads
 * all the data at once while keeping intact the identifiers that the entities
 * originally had.<br/>
 * An example of use is the implementation of a save/restore utility.
 *
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class basic_snapshot_loader {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

public:
    /*! Basic registry type. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;

    /**
     * @brief Constructs an instance that is bound to a given registry.
     * @param source A valid reference to a registry.
     */
    basic_snapshot_loader(registry_type &source) noexcept
        : reg{&source} {
        // restoring a snapshot as a whole requires a clean registry
        ENTT_ASSERT(reg->template storage<entity_type>().free_list() == 0u, "Registry must be empty");
    }

    /*! @brief Default move constructor. */
    basic_snapshot_loader(basic_snapshot_loader &&) noexcept = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_snapshot_loader &operator=(basic_snapshot_loader &&) noexcept = default;

    /**
     * @brief Restores all elements of a type with associated identifiers.
     * @tparam Type Type of elements to restore.
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @param id Optional name used to map the storage within the registry.
     * @return A valid loader to continue restoring data.
     */
    template<typename Type, typename Archive>
    basic_snapshot_loader &get(Archive &archive, const id_type id = type_hash<Type>::value()) {
        auto &storage = reg->template storage<Type>(id);
        typename traits_type::entity_type length{};

        archive(length);

        if constexpr(std::is_same_v<Type, entity_type>) {
            typename traits_type::entity_type count{};

            storage.reserve(length);
            archive(count);

            for(entity_type entity = null; length; --length) {
                archive(entity);
                storage.emplace(entity);
            }

            storage.free_list(count);
        } else {
            auto &other = reg->template storage<entity_type>();
            entity_type entt{null};

            while(length--) {
                if(archive(entt); entt != null) {
                    const auto entity = other.contains(entt) ? entt : other.emplace(entt);
                    ENTT_ASSERT(entity == entt, "Entity not available for use");

                    if constexpr(std::tuple_size_v<decltype(storage.get_as_tuple({}))> == 0u) {
                        storage.emplace(entity);
                    } else {
                        Type elem{};
                        archive(elem);
                        storage.emplace(entity, std::move(elem));
                    }
                }
            }
        }

        return *this;
    }

    /**
     * @brief Destroys those entities that have no elements.
     *
     * In case all the entities were serialized but only part of the elements
     * was saved, it could happen that some of the entities have no elements
     * once restored.<br/>
     * This function helps to identify and destroy those entities.
     *
     * @return A valid loader to continue restoring data.
     */
    basic_snapshot_loader &orphans() {
        internal::orphans(*reg);
        return *this;
    }

private:
    registry_type *reg;
};

/**
 * @brief Utility class for _continuous loading_.
 *
 * A _continuous loader_ is designed to load data from a source registry to a
 * (possibly) non-empty destination. The loader can accommodate in a registry
 * more than one snapshot in a sort of _continuous loading_ that updates the
 * destination one step at a time.<br/>
 * Identifiers that entities originally had are not transferred to the target.
 * Instead, the loader maps remote identifiers to local ones while restoring a
 * snapshot.<br/>
 * An example of use is the implementation of a client-server application with
 * the requirement of transferring somehow parts of the representation side to
 * side.
 *
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class basic_continuous_loader {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

    void restore(typename Registry::entity_type entt) {
        if(const auto entity = to_entity(entt); remloc.contains(entity) && remloc[entity].first == entt) {
            if(!reg->valid(remloc[entity].second)) {
                remloc[entity].second = reg->create();
            }
        } else {
            remloc.insert_or_assign(entity, std::make_pair(entt, reg->create()));
        }
    }

    template<typename Container>
    auto update(int, Container &container) -> decltype(typename Container::mapped_type{}, void()) {
        // map like container
        Container other;

        for(auto &&pair: container) {
            using first_type = std::remove_const_t<typename std::decay_t<decltype(pair)>::first_type>;
            using second_type = typename std::decay_t<decltype(pair)>::second_type;

            if constexpr(std::is_same_v<first_type, entity_type> && std::is_same_v<second_type, entity_type>) {
                other.emplace(map(pair.first), map(pair.second));
            } else if constexpr(std::is_same_v<first_type, entity_type>) {
                other.emplace(map(pair.first), std::move(pair.second));
            } else {
                static_assert(std::is_same_v<second_type, entity_type>, "Neither the key nor the value are of entity type");
                other.emplace(std::move(pair.first), map(pair.second));
            }
        }

        using std::swap;
        swap(container, other);
    }

    template<typename Container>
    auto update(char, Container &container) -> decltype(typename Container::value_type{}, void()) {
        // vector like container
        static_assert(std::is_same_v<typename Container::value_type, entity_type>, "Invalid value type");

        for(auto &&entt: container) {
            entt = map(entt);
        }
    }

    template<typename Component, typename Other, typename Member>
    void update([[maybe_unused]] Component &instance, [[maybe_unused]] Member Other::*member) {
        if constexpr(!std::is_same_v<Component, Other>) {
            return;
        } else if constexpr(std::is_same_v<Member, entity_type>) {
            instance.*member = map(instance.*member);
        } else {
            // maybe a container? let's try...
            update(0, instance.*member);
        }
    }

public:
    /*! Basic registry type. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;

    /**
     * @brief Constructs an instance that is bound to a given registry.
     * @param source A valid reference to a registry.
     */
    basic_continuous_loader(registry_type &source) noexcept
        : remloc{source.get_allocator()},
          reg{&source} {}

    /*! @brief Default move constructor. */
    basic_continuous_loader(basic_continuous_loader &&) = default;

    /*! @brief Default move assignment operator. @return This loader. */
    basic_continuous_loader &operator=(basic_continuous_loader &&) = default;

    /**
     * @brief Restores all elements of a type with associated identifiers.
     *
     * It creates local counterparts for remote elements as needed.<br/>
     * Members are either data members of type entity_type or containers of
     * entities. In both cases, a loader visits them and replaces entities with
     * their local counterpart.
     *
     * @tparam Type Type of elements to restore.
     * @tparam Archive Type of input archive.
     * @param archive A valid reference to an input archive.
     * @param id Optional name used to map the storage within the registry.
     * @return A valid loader to continue restoring data.
     */
    template<typename Type, typename Archive>
    basic_continuous_loader &get(Archive &archive, const id_type id = type_hash<Type>::value()) {
        auto &storage = reg->template storage<Type>(id);
        typename traits_type::entity_type length{};
        entity_type entt{null};

        archive(length);

        if constexpr(std::is_same_v<Type, entity_type>) {
            typename traits_type::entity_type in_use{};

            storage.reserve(length);
            archive(in_use);

            for(std::size_t pos{}; pos < in_use; ++pos) {
                archive(entt);
                restore(entt);
            }

            for(std::size_t pos = in_use; pos < length; ++pos) {
                archive(entt);

                if(const auto entity = to_entity(entt); remloc.contains(entity)) {
                    if(reg->valid(remloc[entity].second)) {
                        reg->destroy(remloc[entity].second);
                    }

                    remloc.erase(entity);
                }
            }
        } else {
            for(auto &&ref: remloc) {
                storage.remove(ref.second.second);
            }

            while(length--) {
                if(archive(entt); entt != null) {
                    restore(entt);

                    if constexpr(std::tuple_size_v<decltype(storage.get_as_tuple({}))> == 0u) {
                        storage.emplace(map(entt));
                    } else {
                        Type elem{};
                        archive(elem);
                        storage.emplace(map(entt), std::move(elem));
                    }
                }
            }
        }

        return *this;
    }

    /**
     * @brief Destroys those entities that have no elements.
     *
     * In case all the entities were serialized but only part of the elements
     * was saved, it could happen that some of the entities have no elements
     * once restored.<br/>
     * This function helps to identify and destroy those entities.
     *
     * @return A non-const reference to this loader.
     */
    basic_continuous_loader &orphans() {
        internal::orphans(*reg);
        return *this;
    }

    /**
     * @brief Tests if a loader knows about a given entity.
     * @param entt A valid identifier.
     * @return True if `entity` is managed by the loader, false otherwise.
     */
    [[nodiscard]] bool contains(entity_type entt) const noexcept {
        const auto it = remloc.find(to_entity(entt));
        return it != remloc.cend() && it->second.first == entt;
    }

    /**
     * @brief Returns the identifier to which an entity refers.
     * @param entt A valid identifier.
     * @return The local identifier if any, the null entity otherwise.
     */
    [[nodiscard]] entity_type map(entity_type entt) const noexcept {
        if(const auto it = remloc.find(to_entity(entt)); it != remloc.cend() && it->second.first == entt) {
            return it->second.second;
        }

        return null;
    }

private:
    dense_map<typename traits_type::entity_type, std::pair<entity_type, entity_type>> remloc;
    registry_type *reg;
};

} // namespace entt

#endif
