#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP


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
#include "view.hpp"
#include "entt.hpp"


namespace entt {


template<typename Entity>
class Registry {
    /* >>> WIP */
    /*
    template<typename Component>
    struct Pool: SigH<void(Entity, bool)>, SparseSet<Entity, Component> {
        template<typename... Args>
        Component & construct(Entity entity, Args&&... args) {
            SparseSet<Entity, Component>::construct(entity, std::forward<Args>(args)...);
            SigH<void(Entity, bool)>::publish(entity, false);
        }

        virtual void destroy(Entity entity) {
            SparseSet<Entity, Component>::destroy(entity);
            SigH<void(Entity, bool)>::publish(entity, true);
        }
    };

    template<typename... Component>
    struct Persistent: SparseSet<Entity> {
        void update(Entity entity, bool removed) {
            if(removed) {
                SparseSet<Entity>::destroy(entity);
            } else {
                // TODO
            }
        }
    };
    */
    /* <<< WIP */

    using component_family = Family<struct InternalRegistryComponentFamily>;
    using view_family = Family<struct InternalRegistryViewFamily>;

    using traits_type = entt_traits<Entity>;
    using base_pool_type = SparseSet<Entity>;

    template<typename Component>
    using pool_type = SparseSet<Entity, Component>;

    template<typename Component>
    bool managed() const noexcept {
        const auto ctype = component_family::type<Component>();
        return ctype < pools.size() && pools[ctype];
    }

    template<typename Component>
    const pool_type<Component> & pool() const noexcept {
        assert(managed<Component>());
        return static_cast<pool_type<Component> &>(*pools[component_family::type<Component>()]);
    }

    template<typename Component>
    pool_type<Component> & pool() noexcept {
        assert(managed<Component>());
        return const_cast<pool_type<Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    pool_type<Component> & ensure() {
        const auto ctype = component_family::type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        if(!pools[ctype]) {
            pools[ctype] = std::make_unique<pool_type<Component>>();
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

    template<typename Component>
    size_type capacity() const noexcept {
        return managed<Component>() ? pool<Component>().capacity() : size_type{};
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

    entity_type entity(entity_type entity) const noexcept {
        return entity_type(entity & traits_type::entity_mask);
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
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { all, (all = all && managed<Component>() && pool<Component>().has(entity))... };
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
        ensure<To>().respect(ensure<From>());
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
    auto view() {
        return View<Entity, Component...>{ensure<Component>()...};
    }

    template<typename... Component>
    void prepare() {
        static_assert(sizeof...(Component) > 1, "!");
        // TODO
    }

    template<typename... Component>
    Group<Entity, Component...> group() {
        static_assert(sizeof...(Component) > 1, "!");
        // TODO
    }

private:
    std::vector<std::unique_ptr<base_pool_type>> pools;
    std::vector<entity_type> available;
    std::vector<entity_type> entities;
};


using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_ENTITY_REGISTRY_HPP
