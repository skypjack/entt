#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include "sparse_set.hpp"
#include "view.hpp"


namespace entt {


template<typename Entity>
class Registry {
    using base_pool_type = SparseSet<Entity>;

    template<typename Component>
    using pool_type = SparseSet<Entity, Component>;

    static std::size_t identifier() noexcept {
        static std::size_t value = 0;
        return value++;
    }

    template<typename>
    static std::size_t type() noexcept {
        static const std::size_t value = identifier();
        return value;
    }

    template<typename Component>
    bool managed() const noexcept {
        auto ctype = type<Component>();
        return ctype < pools.size() && pools[ctype];
    }

    template<typename Component>
    const pool_type<Component> & pool() const noexcept {
        assert(managed<Component>());
        return static_cast<pool_type<Component> &>(*pools[type<Component>()]);
    }

    template<typename Component>
    pool_type<Component> & pool() noexcept {
        assert(managed<Component>());
        return const_cast<pool_type<Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    pool_type<Component> & ensure() {
        auto ctype = type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        if(!pools[ctype]) {
            pools[ctype] = std::make_unique<pool_type<Component>>();
        }

        return pool<Component>();
    }

public:
    using entity_type = Entity;
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
        return next - available.size();
    }

    template<typename Component>
    size_type capacity() const noexcept {
        return managed<Component>() ? pool<Component>().capacity() : size_type{};
    }

    size_type capacity() const noexcept {
        return next;
    }

    template<typename Component>
    bool empty() const noexcept {
        return managed<Component>() ? pool<Component>().empty() : true;
    }

    bool empty() const noexcept {
        return next == available.size();
    }

    bool valid(entity_type entity) const noexcept {
        return (entity < next && std::find(available.cbegin(), available.cend(), entity) == available.cend());
    }

    template<typename... Component>
    entity_type create() noexcept {
        using accumulator_type = int[];
        auto entity = create();
        accumulator_type accumulator = { 0, (assign<Component>(entity), 0)... };
        (void)accumulator;
        return entity;
    }

    entity_type create() noexcept {
        entity_type entity;

        if(available.empty()) {
            entity = next++;
        } else {
            entity = available.back();
            available.pop_back();
        }

        return entity;
    }

    void destroy(entity_type entity) {
        assert(valid(entity));

        for(auto &&cpool: pools) {
            if(cpool && cpool->has(entity)) {
                cpool->destroy(entity);
            }
        }

        available.push_back(entity);
    }

    template<typename Component, typename... Args>
    Component & assign(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return ensure<Component>().construct(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    void remove(entity_type entity) {
        assert(valid(entity));
        assert(managed<Component>());
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
        assert(managed<Component>());
        return pool<Component>().get(entity);
    }

    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const Registry *>(this)->get<Component>(entity));
    }

    template<typename Component, typename... Args>
    Component & replace(entity_type entity, Args&&... args) {
        assert(valid(entity));
        assert(managed<Component>());
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

            for(entity_type entity = 0; entity < next; ++entity) {
                if(cpool.has(entity)) {
                    cpool.destroy(entity);
                }
            }
        }
    }

    void reset() {
        for(auto &&cpool: pools) {
            if(cpool) {
                cpool->reset();
            }
        }

        next = entity_type{};
        available.clear();
        pools.clear();
    }

    template<typename... Component>
    auto view() {
        return DynamicView<Entity, Component...>{ensure<Component>()...};
    }

    /*
     * WIP: waiting for the persisten view... :-)
     *
    template<typename... Component>
    std::enable_if_t<(sizeof...(Component) == 1), PersistentView<Entity, Component>>
    view(persistent_view_type = {}) {
        return PersistentView<Entity, Component>{ensure<Component>()};
    }

    template<typename... Component>
    std::enable_if_t<(sizeof...(Component) > 1), PersistentView<Entity, Component>>
    view(persistent_view_type = {}) {
        // TODO
    }

    template<typename... Component>
    DynamicView<Entity, Component...> view(dynamic_view_type) {
        return DynamicView<Entity, Component...>{ensure<Component>()...};
    }
    */

private:
    std::vector<std::unique_ptr<base_pool_type>> pools;
    std::vector<entity_type> available;
    entity_type next{};
};


using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_REGISTRY_HPP
