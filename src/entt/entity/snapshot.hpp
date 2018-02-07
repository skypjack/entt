#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <unordered_map>
#include <algorithm>
#include <cstddef>
#include <utility>
#include <cassert>


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
    void component(const View &view, Archive &archive) {
        archive(static_cast<Entity>(view.size()));

        for(typename View::size_type i{}; i < view.size(); ++i) {
            archive(view.data()[i]);
            archive(view.raw()[i]);
        };
    }

    template<typename Tag, typename Archive>
    void tag(bool has, Archive &archive) {
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
        accumulator_type accumulator = { 0, (component(registry.template view<Component>(), archive), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename... Tag, typename Archive>
    Snapshot tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (tag<Tag>(registry.template has<Tag>(), archive), 0)... };
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
    void restore(bool destroyed, Archive &archive) {
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
        restore(false, archive);
        return *this;
    }

    template<typename Archive>
    SnapshotRestore destroyed(Archive &archive) && {
        restore(true, archive);
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


//template<typename Entity>
//class SnapshotAppend {
//
//public:
//    SnapshotAppend(Registry<Entity> &reg)
//        : reg{reg}
//    {}
//
//    const Registry<Entity> & registry() const noexcept {
//        return reg;
//    }
//
//    Registry<Entity> & registry() noexcept {
//        return const_cast<Registry<Entity> &>(const_cast<const SnapshotAppend *>(this)->registry());
//    }
//
//    template<typename Archive>
//    SnapshotAppend entities(Archive &archive) {
//        // TODO
//    }
//
//    template<typename Archive>
//    SnapshotAppend destroyed(Archive &archive) {
//        // TODO
//    }
//
//    template<typename... Component, typename Archive>
//    SnapshotAppend component(Archive &archive) {
//        // TODO
//    }
//
//    template<typename... Tag, typename Archive>
//    SnapshotAppend tag(Archive &archive) {
//        // TODO
//    }
//
//private:
//    std::unordered_map<Entity, Entity> remloc;
//    Registry<Entity> &reg;
//};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
