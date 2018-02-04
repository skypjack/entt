#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP


#include <utility>
#include "registry.hpp"


namespace entt {


template<typename Entity>
class Snapshot final {
    friend class Registry<Entity>;

    using registry_type = Registry<Entity>;
    using entity_type = registry_type::entity_type;
    using basic_fn_type = void(registry_type::*)(entity_type);

    Snapshot(Registry<Entity> &registry, basic_fn_type ensure, basic_fn_type drop)
        : registry{registry}, ensure{ensure}, drop{drop}
    {}

    Snapshot(const Snapshot &) = delete;
    Snapshot(Snapshot &&) = delete;

    Snapshot & operator=(const Snapshot &) = delete;
    Snapshot & operator=(Snapshot &&) = delete;

public:
    template<typename... Component, typename Archive>
    void component(Archive &archive) && {
        auto save = [](const auto &view, auto &archive) {
            const std::uint32_t length = view.size();

            archive(length);

            for(std::uint32_t i{}; i < length; ++i) {
                archive(view.data()[i]);
                archive(view.raw()[i]);
            }
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (save(registry.template view<Component>(), archive), 0)... };
        (void)accumulator;
    }

    template<typename... Tag, typename Archive>
    void tag(Archive &archive) && {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (archive(registry.template attachee<Tag>()), archive(registry.template get<Tag>()), 0)... };
        (void)accumulator;
    }

    template<typename... Component, typename Archive>
    void restore(Archive &archive) {
        auto load = [this](basic_fn_type accomodate, auto &archive) {
            std::uint32_t length;

            archive(length);

            for(std::uint32_t i = {}; i < length; ++i) {
                Entity entity;
                archive(entity);
                (registry.*ensure)(entity);
                archive((registry.*accomodate)(entity));
            }
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (load(&Registry::accomodate<Component>, archive), 0)... };
        (void)accumulator;
    }

    template<typename... Tag, typename Archive>
    void attach(Archive &archive) {
        auto load = [this](basic_fn_type get, auto &archive) {
            Entity entity;
            archive(entity);
            (registry.*ensure)(entity);
            archive((registry.*get)(entity));
        };

        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (load(&Registry::attach<Component>, archive), 0)... };
        (void)accumulator;
    }

    template<typename Archive>
    void dump(Archive &archive) {
        // TODO
    }

    template<typename Archive>
    void import(Archive &archive) {
        // TODO
    }

    void finalize() {
        registry.orphans([this](auto entity) { (registry.*discard)(entity); });
    }

private:
    Registry<Entity> &registry;
    basic_fn_type ensure;
    basic_fn_type drop;
};


}


#endif // ENTT_ENTITY_SNAPSHOT_HPP
