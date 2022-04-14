#include <iostream>
#include <vector>
#include <entt/entt.hpp>
#include <entt/entity/polymorphic.hpp>


struct shape { virtual void draw() = 0; };
template<>
struct entt::poly_parent_types<shape> {
    using parent_types = type_list<>;
};
struct circle: public entt::inherit<shape> { void draw() override { std::cout << "circle"; } };
struct rectangle: public entt::inherit<shape> { void draw() override { std::cout << "rectangle"; } };


struct animal { virtual void name() const = 0; };
struct cat: public animal { void name() const override { std::cout << "cat"; } };
struct dog: public animal { void name() const override { std::cout << "dog"; } };

template<>
struct entt::poly_parent_types<animal*> {
    using parent_types = type_list<>;
};
template<>
struct entt::poly_parent_types<cat*> {
    using parent_types = type_list<animal*>;
};
template<>
struct entt::poly_parent_types<dog*> {
    using parent_types = type_list<animal*>;
};

void on_construct(entt::registry&, entt::entity e) {
    std::cout << "constructed " << entt::to_entity(e) << "\n";
}

void on_update(entt::registry&, entt::entity e) {
    std::cout << "updated " << entt::to_entity(e) << "\n";
}

void on_destroy(entt::registry&, entt::entity e) {
    std::cout << "destroyed " << entt::to_entity(e) << "\n";
}

int main() {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

//    registry.on_construct<shape>().connect<on_construct>();
//    registry.on_update<shape>().connect<on_update>();
//    registry.on_destroy<shape>().connect<on_destroy>();

    registry.emplace<circle>(entity);
    registry.emplace<rectangle>(entity);
    registry.emplace<cat*>(entity, new cat);

    registry.emplace<circle>(other);
    registry.emplace<dog*>(other, new dog);

    std::cout << "\nall shapes\n";
    entt::algorithm::each_poly<shape>(registry, [](auto ent, auto& s) {
        std::cout << entt::to_entity(ent) << " -> ";
        s.draw();
        std::cout << '\n';
    });

    std::cout << "\nall shapes for entity " << entt::to_entity(entity) << "\n";
    for ( shape& s : entt::algorithm::poly_get_all< shape>(registry, entity)) {
        s.draw();
        std::cout << '\n';
    }

    std::cout << "any shape for entity " << entt::to_entity(entity) << " ";
    entt::algorithm::poly_get_any<shape>(registry, entity)->draw();
    std::cout << '\n';

    std::cout << "\nall animals\n";
    entt::algorithm::each_poly<const animal*>(registry, [](auto ent, const animal* a) {
        std::cout << entt::to_entity(ent) << " -> ";
        a->name();
        std::cout << '\n';
    });

    std::cout << "\nall animals for entity " << entt::to_entity(entity) << "\n";
    for (const animal* a : entt::algorithm::poly_get_all<const animal*>(registry, entity)) {
        a->name();
        std::cout << '\n';
    }

    registry.patch<circle>(entity);
    entt::algorithm::poly_remove<shape>(registry, entity);
}