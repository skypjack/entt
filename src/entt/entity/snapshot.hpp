#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <unordered_map>
#include <algorithm>
#include <cstddef>
#include <utility>
#include <cassert>
#include "entt_traits.hpp"


namespace entt {


template<typename Entity>
class Registry;


template<typename Entity>
class Snapshot final {
    friend class Registry<Entity>;

    Snapshot(Registry<Entity> &registry, const Entity *available, std::size_t size)
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

        archive(has);

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
class SnapshotRestore final {
    friend class Registry<Entity>;

    using func_type = void(Registry<Entity>::*)(Entity, bool);

    SnapshotRestore(Registry<Entity> &registry, func_type force_fn)
        : registry{registry},
          force_fn{force_fn}
    {
        // restore a snapshot as a whole requires a clean registry
        assert(!registry.capacity());
    }

    SnapshotRestore(const SnapshotRestore &) = default;
    SnapshotRestore(SnapshotRestore &&) = default;

    SnapshotRestore & operator=(const SnapshotRestore &) = default;
    SnapshotRestore & operator=(SnapshotRestore &&) = default;

    template<typename Archive>
    void restore(Archive &archive, bool destroyed) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            (registry.*force_fn)(entity, destroyed);
            --length;
        }
    }

    template<typename Component, typename Archive>
    void assign(Archive &archive) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            (registry.*force_fn)(entity, false);
            archive(registry.template assign<Component>(entity));
            --length;
        }
    }

    template<typename Tag, typename Archive>
    void attach(Archive &archive) {
        bool has{};
        archive(has);

        if(has) {
            Entity entity{};
            archive(entity);
            (registry.*force_fn)(entity, false);
            archive(registry.template attach<Tag>(entity));
        }
    }

public:
    ~SnapshotRestore() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });
    }

    template<typename Archive>
    SnapshotRestore entities(Archive &archive) && {
        restore(archive, false);
        return *this;
    }

    template<typename Archive>
    SnapshotRestore destroyed(Archive &archive) && {
        restore(archive, true);
        return *this;
    }

    template<typename... Component, typename Archive>
    SnapshotRestore component(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename... Tag, typename Archive>
    SnapshotRestore tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

private:
    Registry<Entity> &registry;
    func_type force_fn;
};


template<typename Entity>
class SnapshotAppend {
    using traits_type = entt_traits<Entity>;

    template<typename Archive>
    void prepare(Archive &archive, bool destroyed) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);

            const auto entt = entity & traits_type::entity_mask;
            auto it = remloc.find(entt);

            if(destroyed) {
                if(it != remloc.cend()) {
                    registry.destroy(it->second.second);
                    remloc.erase(it);
                }
            } else {
                if(it == remloc.cend()) {
                    remloc.emplace(entt, std::make_pair(entity, registry.create()));
                } else if(entity != it->second.first) {
                    registry.destroy(it->second.second);
                    remloc[entt] = { entity, registry.create() };
                }
            }

            --length;
        }
    }

    template<typename Component, typename Archive>
    void assign(Archive &archive) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);

            // TODO

            --length;
        }
    }

    template<typename Component, typename Archive, typename... Type>
    void assign(Archive &archive, Type Component::*... member) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);

            // TODO

            --length;
        }
    }

    template<typename Tag, typename Archive>
    void attach(Archive &archive) {
        bool has{};
        archive(has);

        if(has) {
            Entity entity{};
            archive(entity);

            // TODO
        }
    }

    template<typename Tag, typename Archive, typename... Type>
    void attach(Archive &archive, Type Tag::*... member) {
        bool has{};
        archive(has);

        if(has) {
            Entity entity{};
            archive(entity);

            // TODO
        }
    }

public:
    using registry_type = Registry<Entity>;

    SnapshotAppend(registry_type &registry)
        : registry{registry}
    {}

    template<typename Archive>
    SnapshotAppend & entities(Archive &archive) {
        prepare(archive, false);
        return *this;
    }

    template<typename Archive>
    SnapshotAppend & destroyed(Archive &archive) {
        prepare(archive, true);
        return *this;
    }

    template<typename... Component, typename Archive>
    SnapshotAppend & component(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (assign<Component>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename Component, typename Archive, typename... Type>
    SnapshotAppend & component(Archive &archive, Type Component::*... member) {
        assign(archive, member...);
        return *this;
    }

    template<typename... Tag, typename Archive>
    SnapshotAppend & tag(Archive &archive) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename Tag, typename Archive, typename... Type>
    SnapshotAppend & tag(Archive &archive, Type Tag::*... member) {
        attach<Tag>(archive, member...);
        return *this;
    }

private:
    std::unordered_map<Entity, std::pair<Entity, Entity>> remloc;
    Registry<Entity> &registry;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
