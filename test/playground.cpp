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


int main() {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<circle>(entity);
    registry.emplace<rectangle>(entity);
    registry.emplace<cat>(entity);

    registry.emplace<circle>(other);
    registry.emplace<dog>(other);

    for (auto& pool : registry.poly_data().assure<shape>().pools())
    {
        for (auto ent : pool.pool()) {
            std::cout << entt::to_entity(ent) << " -> ";
            pool.try_get(ent)->draw();
            std::cout << '\n';
        }
    }

    std::cout << "\nall shapes for entity " << entt::to_entity(entity) << "\n";
    for (shape& shape : registry.poly_get_all<shape>(entity)) {
        shape.draw();
        std::cout << '\n';
    }
    std::cout << "any shape for entity " << entt::to_entity(entity) << " ";
    registry.poly_get_any<shape>(entity)->draw();
    std::cout << '\n';
}