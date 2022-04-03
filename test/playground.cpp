#include <iostream>
#include <vector>
#include <entt/entt.hpp>
#include <entt/entity/polymorphic.hpp>


struct shape { virtual void draw() const = 0; };
template<>
struct entt::poly_parent_types<shape> {
    using parent_types = type_list<>;
};

struct circle: public entt::inherit<shape> { void draw() const override { std::cout << "circle"; } };
struct rectangle: public entt::inherit<shape> { void draw() const override { std::cout << "rectangle"; } };

struct animal: public entt::inherit<> { virtual void name() const = 0; };
struct cat: public entt::inherit<animal> { void name() const override { std::cout << "cat"; } };
struct dog: public entt::inherit<animal> { void name() const override { std::cout << "dog"; } };

template<typename Type, typename Entity, typename Func>
void for_each_poly(entt::basic_registry<Entity>& registry, Func func) {
    auto& sets = registry.poly_data().template assure<Type>().child_pools();
    for (auto set_holder : sets) {
        for (const auto ent : set_holder.pool()) {
            auto* comp = set_holder.template try_get<Type>(ent);
            func(ent, *comp);
        }
    }
}

template<typename Type, typename Entity, typename Func>
void for_each_poly(entt::basic_registry<Entity>& registry, Entity ent, Func func) {
    auto& sets = registry.poly_data().template assure<Type>().child_pools();
    for (auto set_holder : sets) {
        auto* comp = set_holder.template get<Type>(ent);
        func(ent, *comp);
    }
}


int main() {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<circle>(entity);
    registry.emplace<rectangle>(entity);
    registry.emplace<cat>(entity);

    registry.emplace<circle>(other);
    registry.emplace<dog>(other);

    registry.poly_get<shape>(entity)->draw();
    std::cout << '\n';

    for_each_poly<shape>(registry, []( const entt::entity& ent, shape& comp ) -> void {
        std::cout << entt::to_entity(ent) << " -> ";
        comp.draw();
        std::cout << '\n';
    });

//    print_shapes(registry);
//    print_animals(registry);
}