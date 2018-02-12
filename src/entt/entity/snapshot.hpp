#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <unordered_map>
#include <algorithm>
#include <cstddef>
#include <utility>
#include <cassert>
#include <iterator>
#include <type_traits>
#include "entt_traits.hpp"


namespace entt {


/**
 * @brief TODO
 *
 * TODO
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Registry;


/**
 * @brief TODO
 *
 * TODO
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Snapshot final {
    friend class Registry<Entity>;

    Snapshot(Registry<Entity> &registry, const Entity *available, std::size_t size) noexcept
        : registry{registry},
          available{available},
          size{size}
    {}

    Snapshot(const Snapshot &) = default;
    Snapshot(Snapshot &&) = default;

    Snapshot & operator=(const Snapshot &) = default;
    Snapshot & operator=(Snapshot &&) = default;

    template<typename View, typename Archive>
    void get(Archive &archive, const View &view) {
        archive(static_cast<Entity>(view.size()));

        for(typename View::size_type i{}, sz = view.size(); i < sz; ++i) {
            archive(view.data()[i]);
            archive(view.raw()[i]);
        };
    }

    template<typename Tag, typename Archive>
    void get(Archive &archive) {
        const bool has = registry.template has<Tag>();

        // numerical length is forced for tags to facilitate loading
        archive(has ? Entity(1): Entity{});

        if(has) {
            archive(registry.template attachee<Tag>());
            archive(registry.template get<Tag>());
        }
    }

public:
    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    Snapshot entities(Archive archive) && {
        archive(static_cast<Entity>(registry.size()));
        registry.each([&archive, this](auto entity) { archive(entity); });
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    Snapshot destroyed(Archive archive) && {
        archive(static_cast<Entity>(size));
        std::for_each(available, available+size, [&archive, this](auto entity) { archive(entity); });
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component Types of components to serialize.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Component, typename Archive>
    Snapshot component(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (get(archive, registry.template view<Component>()), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Tag Types of tags to serialize.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Tag, typename Archive>
    Snapshot tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (get<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

private:
    Registry<Entity> &registry;
    const Entity *available;
    std::size_t size;
};


/**
 * @brief TODO
 *
 * TODO
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class SnapshotLoader final {
    friend class Registry<Entity>;

    using func_type = void(*)(Registry<Entity> &, Entity, bool);

    SnapshotLoader(Registry<Entity> &registry, func_type force_fn) noexcept
        : registry{registry},
          force_fn{force_fn}
    {
        // restore a snapshot as a whole requires a clean registry
        assert(!registry.capacity());
    }

    SnapshotLoader(const SnapshotLoader &) = default;
    SnapshotLoader(SnapshotLoader &&) = default;

    SnapshotLoader & operator=(const SnapshotLoader &) = default;
    SnapshotLoader & operator=(SnapshotLoader &&) = default;

    template<typename Archive, typename Func>
    void each(Archive &archive, Func func) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            func(entity);
            --length;
        }
    }

    template<typename Component, typename Archive>
    void assign(Archive &archive) {
        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            force_fn(registry, entity, destroyed);
            archive(registry.template assign<Component>(entity));
        });
    }

    template<typename Tag, typename Archive>
    void attach(Archive &archive) {
        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            force_fn(registry, entity, destroyed);
            archive(registry.template attach<Tag>(entity));
        });
    }

public:
    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    SnapshotLoader entities(Archive &archive) && {
        each(archive, [this](auto entity) {
            const bool destroyed = false;
            force_fn(registry, entity, destroyed);
        });

        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    SnapshotLoader destroyed(Archive &archive) && {
        each(archive, [this](auto entity) {
            const bool destroyed = true;
            force_fn(registry, entity, destroyed);
        });

        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component Types of components to restore.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Component, typename Archive>
    SnapshotLoader component(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Tag Types of tags to restore.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Tag, typename Archive>
    SnapshotLoader tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }


    /**
     * @brief TODO
     *
     * TODO
     *
     * @return TODO
     */
    SnapshotLoader orphans() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });

        return *this;
    }

private:
    Registry<Entity> &registry;
    func_type force_fn;
};


/**
 * @brief TODO
 *
 * TODO
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class ProgressiveLoader final {
    friend class Registry<Entity>;

    using traits_type = entt_traits<Entity>;

    Entity prepare(Entity entity, bool destroyed) {
        const auto it = remloc.find(entity);

        if(it == remloc.cend()) {
            remloc.emplace(entity, std::make_pair(registry.create(), true));

            if(destroyed) {
                registry.destroy(remloc[entity].first);
            }
        } else {
            const auto local = remloc[entity].first;

            // set the dirty flag
            remloc[entity].second = true;

            if(destroyed && registry.valid(local)) {
                registry.destroy(local);
            } else if(!destroyed && !registry.valid(local)) {
                remloc[entity].first = registry.create();
            }
        }

        return remloc[entity].first;
    }

    template<typename Instance, typename Type>
    std::enable_if_t<std::is_same<Type, Entity>::value>
    update(Instance &instance, Type Instance::*member) {
        instance.*member = map(instance.*member);
    }

    template<typename Instance, typename Type>
    std::enable_if_t<std::is_same<typename std::iterator_traits<typename Type::iterator>::value_type, Entity>::value>
    update(Instance &instance, Type Instance::*member) {
        for(auto &entity: (instance.*member)) {
            entity = map(entity);
        }
    }

    template<typename Archive, typename Func>
    void each(Archive &archive, Func func) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            func(entity);
            --length;
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

    template<typename Component, typename Archive>
    void assign(Archive &archive) {
        reset<Component>();

        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            archive(registry.template accomodate<Component>(entity));
        });
    }

    template<typename Component, typename Archive, typename... Type>
    void assign(Archive &archive, Type Component::*... member) {
        reset<Component>();

        each(archive, [&archive, member..., this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            auto &component = registry.template accomodate<Component>(entity);
            archive(component);

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (update(component, member), 0)... };
            (void)accumulator;
        });
    }

    template<typename Tag, typename Archive>
    void attach(Archive &archive) {
        registry.template remove<Tag>();

        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            archive(registry.template attach<Tag>(entity));
        });
    }

    template<typename Tag, typename Archive, typename... Type>
    void attach(Archive &archive, Type Tag::*... member) {
        registry.template remove<Tag>();

        each(archive, [&archive, member..., this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            auto &tag = registry.template attach<Tag>(entity);
            archive(tag);

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (update(tag, member), 0)... };
            (void)accumulator;
        });
    }

public:
    /*! @brief TODO */
    using entity_type = Entity;

    /**
     * @brief TODO
     *
     * TODO
     *
     * @param registry TODO
     */
    ProgressiveLoader(Registry<entity_type> &registry) noexcept
        : registry{registry}
    {}

    /*! @brief Default copy constructor. */
    ProgressiveLoader(const ProgressiveLoader &) = default;
    /*! @brief Default move constructor. */
    ProgressiveLoader(ProgressiveLoader &&) = default;

    /*! @brief Default copy assignment operator. @return This loader. */
    ProgressiveLoader & operator=(const ProgressiveLoader &) = default;
    /*! @brief Default move assignment operator. @return This loader. */
    ProgressiveLoader & operator=(ProgressiveLoader &&) = default;

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    ProgressiveLoader & entities(Archive &archive) {
        each(archive, [this](auto entity) {
            const bool destroyed = false;
            prepare(entity, destroyed);
        });

        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename Archive>
    ProgressiveLoader & destroyed(Archive &archive) {
        each(archive, [this](auto entity) {
            const bool destroyed = true;
            prepare(entity, destroyed);
        });

        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component Types of components to restore.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Component, typename Archive>
    ProgressiveLoader & component(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Component Type of component to restore.
     * @tparam Archive TODO
     * @tparam Type TODO
     * @param archive TODO
     * @param member TODO
     * @return TODO
     */
    template<typename Component, typename Archive, typename... Type>
    ProgressiveLoader & component(Archive &archive, Type Component::*... member) {
        assign(archive, member...);
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Tag Types of tags to restore.
     * @tparam Archive TODO
     * @param archive TODO
     * @return TODO
     */
    template<typename... Tag, typename Archive>
    ProgressiveLoader & tag(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Tag Type of tag to restore.
     * @tparam Archive TODO
     * @tparam Type TODO
     * @param archive TODO
     * @param member TODO
     * @return TODO
     */
    template<typename Tag, typename Archive, typename... Type>
    ProgressiveLoader & tag(Archive &archive, Type Tag::*... member) {
        attach<Tag>(archive, member...);
        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @return TODO
     */
    ProgressiveLoader & shrink() {
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
     * @brief TODO
     *
     * TODO
     *
     * @return TODO
     */
    ProgressiveLoader & orphans() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });

        return *this;
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @param entity TODO
     * @return TODO
     */
    bool has(entity_type entity) {
        return !(remloc.find(entity) == remloc.cend());
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @param entity TODO
     * @return TODO
     */
    entity_type map(entity_type entity) {
        assert(has(entity));
        return remloc[entity].first;
    }

private:
    std::unordered_map<Entity, std::pair<Entity, bool>> remloc;
    Registry<Entity> &registry;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
