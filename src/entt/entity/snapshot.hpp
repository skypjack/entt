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


template<typename Entity>
class Registry;


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
    template<typename Archive>
    Snapshot entities(Archive archive) && {
        archive(static_cast<Entity>(registry.size()));
        registry.each([&archive, this](auto entity) { archive(entity); });
        return *this;
    }

    template<typename Archive>
    Snapshot destroyed(Archive archive) && {
        archive(static_cast<Entity>(size));
        std::for_each(available, available+size, [&archive, this](auto entity) { archive(entity); });
        return *this;
    }

    template<typename... Component, typename Archive>
    Snapshot component(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (get(archive, registry.template view<Component>()), 0)... };
        (void)accumulator;
        return *this;
    }

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


template<typename Entity>
class SnapshotDumpLoader final {
    friend class Registry<Entity>;

    using func_type = void(*)(Registry<Entity> &, Entity, bool);

    SnapshotDumpLoader(Registry<Entity> &registry, func_type force_fn) noexcept
        : registry{registry},
          force_fn{force_fn}
    {
        // restore a snapshot as a whole requires a clean registry
        assert(!registry.capacity());
    }

    SnapshotDumpLoader(const SnapshotDumpLoader &) = default;
    SnapshotDumpLoader(SnapshotDumpLoader &&) = default;

    SnapshotDumpLoader & operator=(const SnapshotDumpLoader &) = default;
    SnapshotDumpLoader & operator=(SnapshotDumpLoader &&) = default;

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
    template<typename Archive>
    SnapshotDumpLoader entities(Archive &archive) && {
        each(archive, [this](auto entity) {
            const bool destroyed = false;
            force_fn(registry, entity, destroyed);
        });

        return *this;
    }

    template<typename Archive>
    SnapshotDumpLoader destroyed(Archive &archive) && {
        each(archive, [this](auto entity) {
            const bool destroyed = true;
            force_fn(registry, entity, destroyed);
        });

        return *this;
    }

    template<typename... Component, typename Archive>
    SnapshotDumpLoader component(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename... Tag, typename Archive>
    SnapshotDumpLoader tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }


    SnapshotDumpLoader orphans() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });

        return *this;
    }

private:
    Registry<Entity> &registry;
    func_type force_fn;
};


template<typename Entity>
class SnapshotProgressiveLoader final {
    friend class Registry<Entity>;

    using traits_type = entt_traits<Entity>;

    SnapshotProgressiveLoader(Registry<Entity> &registry) noexcept
        : registry{registry}
    {}

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

    void map(Entity &entity) {
        auto it = remloc.find(entity);
        assert(!(it == remloc.cend()));
        entity = it->second.first;
    }

    template<typename Instance, typename Type>
    std::enable_if_t<std::is_same<Type, Entity>::value>
    map(Instance &instance, Type Instance::*member) {
        map(instance.*member);
    }

    template<typename Instance, typename Type>
    std::enable_if_t<std::is_same<typename std::iterator_traits<typename Type::iterator>::value_type, Entity>::value>
    map(Instance &instance, Type Instance::*member) {
        for(auto &&entity: (instance.*member)) {
            map(entity);
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

    template<typename Component, typename Archive>
    void assign(Archive &archive) {
        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            archive(registry.template assign<Component>(entity));
        });
    }

    template<typename Component, typename Archive, typename... Type>
    void assign(Archive &archive, Type Component::*... member) {
        each(archive, [&archive, member..., this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            auto &component = registry.template assign<Component>(entity);
            archive(component);

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (map(component, member), 0)... };
            (void)accumulator;
        });
    }

    template<typename Tag, typename Archive>
    void attach(Archive &archive) {
        each(archive, [&archive, this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            archive(registry.template attach<Tag>(entity));
        });
    }

    template<typename Tag, typename Archive, typename... Type>
    void attach(Archive &archive, Type Tag::*... member) {
        each(archive, [&archive, member..., this](auto entity) {
            const bool destroyed = false;
            entity = prepare(entity, destroyed);
            auto &tag = registry.template attach<Tag>(entity);
            archive(tag);

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (map(tag, member), 0)... };
            (void)accumulator;
        });
    }

public:
    SnapshotProgressiveLoader(const SnapshotProgressiveLoader &) = default;
    SnapshotProgressiveLoader(SnapshotProgressiveLoader &&) = default;

    SnapshotProgressiveLoader & operator=(const SnapshotProgressiveLoader &) = default;
    SnapshotProgressiveLoader & operator=(SnapshotProgressiveLoader &&) = default;

    template<typename Archive>
    SnapshotProgressiveLoader & entities(Archive &archive) {
        each(archive, [this](auto entity) {
            const bool destroyed = false;
            prepare(entity, destroyed);
        });

        return *this;
    }

    template<typename Archive>
    SnapshotProgressiveLoader & destroyed(Archive &archive) {
        each(archive, [this](auto entity) {
            const bool destroyed = true;
            prepare(entity, destroyed);
        });

        return *this;
    }

    template<typename... Component, typename Archive>
    SnapshotProgressiveLoader & component(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename Component, typename Archive, typename... Type>
    SnapshotProgressiveLoader & component(Archive &archive, Type Component::*... member) {
        assign(archive, member...);
        return *this;
    }

    template<typename... Tag, typename Archive>
    SnapshotProgressiveLoader & tag(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename Tag, typename Archive, typename... Type>
    SnapshotProgressiveLoader & tag(Archive &archive, Type Tag::*... member) {
        attach<Tag>(archive, member...);
        return *this;
    }

    SnapshotProgressiveLoader & shrink() {
        auto it = remloc.begin();

        while(it != remloc.cend()) {
            const auto local = it->second.first;
            bool &dirty = it->second.second;

            if(dirty) {
                dirty = false;
            } else {
                if(registry.valid(local)) {
                    registry.destroy(local);
                }

                it = remloc.erase(it);
            }
        }

        return *this;
    }

    SnapshotProgressiveLoader & orphans() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });

        return *this;
    }

private:
    std::unordered_map<Entity, std::pair<Entity, bool>> remloc;
    Registry<Entity> &registry;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
