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

int main() {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<circle>(entity);
    registry.emplace<rectangle>(entity);
    registry.emplace<cat*>(entity, new cat);

    registry.emplace<circle>(other);
    registry.emplace<dog*>(other, new dog);

    std::cout << "\nall shapes\n";
    registry.each_poly<shape>([](auto ent, auto& s) {
        std::cout << entt::to_entity(ent) << " -> ";
        s.draw();
        std::cout << '\n';
    });

    std::cout << "\nall shapes for entity " << entt::to_entity(entity) << "\n";
    for (shape& s : registry.poly_get_all<shape>(entity)) {
        s.draw();
        std::cout << '\n';
    }

    std::cout << "any shape for entity " << entt::to_entity(entity) << " ";
    registry.poly_get_any<shape>(entity)->draw();
    std::cout << '\n';

    std::cout << "\nall animals\n";
    registry.each_poly<animal*>([](auto ent, auto* a) {
        std::cout << entt::to_entity(ent) << " -> ";
        a->name();
        std::cout << '\n';
    });

    std::cout << "\nall animals for entity " << entt::to_entity(entity) << "\n";
    for (animal* a : registry.poly_get_all<animal*>(entity)) {
        a->name();
        std::cout << '\n';
    }
}