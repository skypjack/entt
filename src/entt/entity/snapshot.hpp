#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <utility>


namespace entt {


template<typename Entity>
class Registry;


template<typename Entity, typename Archive>
class Snapshot final {
    friend class Registry<Entity>;

    using func_type = void(*)(const Registry<Entity> &, Snapshot &);

    Snapshot(Registry<Entity> &registry, Archive &archive, func_type destroyed_fn)
        : registry{registry},
          archive{archive},
          destroyed_fn{destroyed_fn}
    {}

    Snapshot(const Snapshot &) = default;
    Snapshot(Snapshot &&) = default;

    Snapshot & operator=(const Snapshot &) = default;
    Snapshot & operator=(Snapshot &&) = default;

    template<typename View>
    void component(const View &view) {
        archive(static_cast<Entity>(view.size()));

        for(typename View::size_type i{}; i < view.size(); ++i) {
            archive(view.data()[i]);
            archive(view.raw()[i]);
        };
    }

    template<typename Tag>
    void tag(bool has) {
        archive(has);

        if(has) {
            archive(registry.template attachee<Tag>());
            archive(registry.template get<Tag>());
        }
    }

    template<typename It>
    void destroyed(It begin, It end) {
        archive(static_cast<Entity>(end - begin));

        while(begin != end) {
            archive(*begin++);
        }
    }

public:
    Snapshot entities() && {
        archive(static_cast<Entity>(registry.size()));
        registry.each([this](auto entity) { archive(entity); });
        return *this;
    }

    Snapshot destroyed() && {
        (destroyed_fn)(registry, *this);
        return *this;
    }

    template<typename... Component>
    Snapshot component() && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (component(registry.template view<Component>()), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename... Tag>
    Snapshot tag() && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (tag<Tag>(registry.template has<Tag>()), 0)... };
        (void)accumulator;
        return *this;
    }

private:
    Registry<Entity> &registry;
    Archive &archive;
    func_type destroyed_fn;
};


template<typename Entity, typename Archive>
class Loader final {
    friend class Registry<Entity>;

    using func_type = void(*)(Registry<Entity> &, Entity);

    Loader(Registry<Entity> &registry, Archive &archive, func_type ensure_fn, func_type destroyed_fn)
        : registry{registry},
          archive{archive},
          ensure_fn{ensure_fn},
          destroyed_fn{destroyed_fn}
    {}

    Loader(const Loader &) = default;
    Loader(Loader &&) = default;

    Loader & operator=(const Loader &) = default;
    Loader & operator=(Loader &&) = default;

    void push(func_type func) {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            func(registry, entity);
            --length;
        }
    }

    template<typename Component>
    void restore() {
        Entity length{};
        archive(length);

        while(length) {
            Entity entity{};
            archive(entity);
            ensure_fn(registry, entity);
            archive(registry.template assign<Component>(entity));
            --length;
        }
    }

    template<typename Tag>
    void attach() {
        bool has{};
        archive(has);

        if(has) {
            Entity entity{};
            archive(entity);
            ensure_fn(registry, entity);
            archive(registry.template attach<Tag>(entity));
        }
    }

public:
    ~Loader() {
        registry.orphans([this](auto entity) {
            registry.destroy(entity);
        });
    }

    Loader entities() && {
        push(ensure_fn);
        return *this;
    }

    Loader destroyed() && {
        push(destroyed_fn);
        return *this;
    }

    template<typename... Component>
    Loader component() && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (restore<Component>(), 0)... };
        (void)accumulator;
        return *this;
    }

    template<typename... Tag>
    Loader tag() && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (attach<Tag>(), 0)... };
        (void)accumulator;
        return *this;
    }

private:
    Registry<Entity> &registry;
    Archive &archive;
    func_type ensure_fn;
    func_type destroyed_fn;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
