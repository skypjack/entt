#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include "../core/family.hpp"
#include "../signal/sigh.hpp"
#include "sparse_set.hpp"
#include "traits.hpp"
#include "view.hpp"


namespace entt {


template<typename Entity>
class Registry {
    using component_family = Family<struct InternalRegistryComponentFamily>;
    using view_family = Family<struct InternalRegistryViewFamily>;
    using traits_type = entt_traits<Entity>;

    template<typename Component>
    struct Pool: SparseSet<Entity, Component> {
        SigH<void(Entity)> constructed;
        SigH<void(Entity)> destroyed;

        template<typename... Args>
        Component & construct(Entity entity, Args&&... args) {
            auto &component = SparseSet<Entity, Component>::construct(entity, std::forward<Args>(args)...);
            constructed.publish(entity);
            return component;
        }

        void destroy(Entity entity) override {
            SparseSet<Entity, Component>::destroy(entity);
            destroyed.publish(entity);
        }
    };

    template<typename... Component>
    struct PoolHandler: SparseSet<Entity> {
        static_assert(sizeof...(Component) > 1, "!");

        PoolHandler(Pool<Component> &... pools)
            : pools{pools...}
        {}

        void candidate(Entity entity) {
            using accumulator_type = bool[];
            bool match = true;
            accumulator_type accumulator = { (match = match && std::get<Pool<Component> &>(pools).has(entity))... };
            if(match) { SparseSet<Entity>::construct(entity); }
            (void)accumulator;
        }

        void release(Entity entity) {
            if(SparseSet<Entity>::has(entity)) {
                SparseSet<Entity>::destroy(entity);
            }
        }

    private:
        const std::tuple<Pool<Component> &...> pools;
    };

    template<typename Component>
    bool managed() const noexcept {
        const auto ctype = component_family::type<Component>();
        return ctype < pools.size() && pools[ctype];
    }

    template<typename Component>
    const Pool<Component> & pool() const noexcept {
        assert(managed<Component>());
        return static_cast<Pool<Component> &>(*pools[component_family::type<Component>()]);
    }

    template<typename Component>
    Pool<Component> & pool() noexcept {
        assert(managed<Component>());
        return const_cast<Pool<Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    Pool<Component> & ensure() {
        const auto ctype = component_family::type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        if(!pools[ctype]) {
            pools[ctype] = std::make_unique<Pool<Component>>();
        }

        return pool<Component>();
    }

public:
    using entity_type = typename traits_type::entity_type;
    using version_type = typename traits_type::version_type;
    using size_type = std::size_t;

    explicit Registry() = default;
    ~Registry() = default;

    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;

    Registry & operator=(const Registry &) = delete;
    Registry & operator=(Registry &&) = delete;

    template<typename Component>
    size_type size() const noexcept {
        return managed<Component>() ? pool<Component>().size() : size_type{};
    }

    size_type size() const noexcept {
        return entities.size() - available.size();
    }

    size_type capacity() const noexcept {
        return entities.capacity();
    }

    template<typename Component>
    bool empty() const noexcept {
        return managed<Component>() ? pool<Component>().empty() : true;
    }

    bool empty() const noexcept {
        return entities.size() == available.size();
    }

    bool valid(entity_type entity) const noexcept {
        const auto entt = entity & traits_type::entity_mask;
        return (entt < entities.size() && entities[entt] == entity);
    }

    version_type version(entity_type entity) const noexcept {
        return version_type((entity >> traits_type::version_shift) & traits_type::version_mask);
    }

    version_type current(entity_type entity) const noexcept {
        return version_type((entities[entity & traits_type::entity_mask] >> traits_type::version_shift) & traits_type::version_mask);
    }

    template<typename... Component>
    entity_type create() noexcept {
        using accumulator_type = int[];
        const auto entity = create();
        accumulator_type accumulator = { 0, (ensure<Component>().construct(entity), 0)... };
        (void)accumulator;
        return entity;
    }

    entity_type create() noexcept {
        entity_type entity;

        if(available.empty()) {
            entity = entity_type(entities.size());
            assert((entity >> traits_type::version_shift) == entity_type{});
            entities.push_back(entity);
        } else {
            entity = available.back();
            available.pop_back();
        }

        return entity;
    }

    void destroy(entity_type entity) {
        assert(valid(entity));

        const auto entt = entity & traits_type::entity_mask;
        const auto version = 1 + ((entity >> traits_type::version_shift) & traits_type::version_mask);
        entities[entt] = entt | (version << traits_type::version_shift);
        available.push_back(entity);

        for(auto &&cpool: pools) {
            if(cpool && cpool->has(entity)) {
                cpool->destroy(entity);
            }
        }
    }

    template<typename Component, typename... Args>
    Component & assign(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return ensure<Component>().construct(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    void remove(entity_type entity) {
        assert(valid(entity));
        return pool<Component>().destroy(entity);
    }

    template<typename... Component>
    bool has(entity_type entity) const noexcept {
        static_assert(sizeof...(Component) > 0, "!");
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { (all = all && managed<Component>() && pool<Component>().has(entity))... };
        (void)accumulator;
        return all;
    }

    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return pool<Component>().get(entity);
    }

    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const Registry *>(this)->get<Component>(entity));
    }

    template<typename Component, typename... Args>
    Component & replace(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return (pool<Component>().get(entity) = Component{std::forward<Args>(args)...});
    }

    template<typename Component, typename... Args>
    Component & accomodate(entity_type entity, Args&&... args) {
        assert(valid(entity));
        auto &cpool = ensure<Component>();

        return (cpool.has(entity)
                ? (cpool.get(entity) = Component{std::forward<Args>(args)...})
                : cpool.construct(entity, std::forward<Args>(args)...));
    }

    template<typename Component, typename Compare>
    void sort(Compare compare) {
        auto &cpool = ensure<Component>();

        cpool.sort([&cpool, compare = std::move(compare)](auto lhs, auto rhs) {
            return compare(static_cast<const Component &>(cpool.get(lhs)), static_cast<const Component &>(cpool.get(rhs)));
        });
    }

    template<typename To, typename From>
    void sort() {
        const SparseSet<Entity> &from = ensure<From>();
        ensure<To>().respect(from);
    }

    template<typename Component>
    void reset(entity_type entity) {
        assert(valid(entity));

        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            if(cpool.has(entity)) {
                cpool.destroy(entity);
            }
        }
    }

    template<typename Component>
    void reset() {
        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            for(auto entity: entities) {
                if(cpool.has(entity)) {
                    cpool.destroy(entity);
                }
            }
        }
    }

    void reset() {
        available.clear();
        pools.clear();

        for(auto &&entity: entities) {
            const auto version = 1 + ((entity >> traits_type::version_shift) & traits_type::version_mask);
            entity = (entity & traits_type::entity_mask) | (version << traits_type::version_shift);
            available.push_back(entity);
        }
    }

    template<typename... Component>
    View<Entity, Component...> view() {
        return View<Entity, Component...>{ensure<Component>()...};
    }

    template<typename... Component>
    void prepare() {
        static_assert(sizeof...(Component) > 1, "!");
        const auto vtype = view_family::type<Component...>();

        if(!(vtype < handlers.size())) {
            handlers.resize(vtype + 1);
        }

        if(!handlers[vtype]) {
            using handler_type = PoolHandler<Component...>;
            using accumulator_type = int[];

            auto handler = std::make_unique<handler_type>(ensure<Component>()...);

            for(auto entity: view<Component...>()) {
                handler->construct(entity);
            }

            auto *ptr = handler.get();
            handlers[vtype] = std::move(handler);

            accumulator_type accumulator = {
                (ensure<Component>().constructed.template connect<handler_type, &handler_type::candidate>(ptr), 0)...,
                (ensure<Component>().destroyed.template connect<handler_type, &handler_type::release>(ptr), 0)...
            };

            (void)accumulator;
        }
    }

    template<typename... Component>
    PersistentView<Entity, Component...> persistent() {
        static_assert(sizeof...(Component) > 1, "!");
        prepare<Component...>();
        return PersistentView<Entity, Component...>{*handlers[view_family::type<Component...>()], ensure<Component>()...};
    }

private:
    std::vector<std::unique_ptr<SparseSet<Entity>>> handlers;
    std::vector<std::unique_ptr<SparseSet<Entity>>> pools;
    std::vector<entity_type> available;
    std::vector<entity_type> entities;
};


using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_ENTITY_REGISTRY_HPP
