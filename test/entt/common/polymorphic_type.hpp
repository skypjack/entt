#ifndef ENTT_COMMON_POLYMORPHIC_TYPE_HPP
#define ENTT_COMMON_POLYMORPHIC_TYPE_HPP

#include <entt/entity/polymorphic.hpp>


struct animal : public entt::inherit<> { virtual std::string name() = 0; int animal_payload{}; };
struct dog : public entt::inherit<animal> { std::string name() override { return "dog"; }; };
struct cat : public entt::inherit<animal> { std::string name() override { return "cat"; }; };

struct shape { virtual std::string draw() = 0; int shape_payload{}; };
struct sphere : public shape { std::string draw() override { return "sphere"; } };
struct cube : public shape { std::string draw() override { return "cube"; } };

template<>
struct entt::poly_direct_parent_types<shape> {
    using parent_types = type_list<>;
};

template<>
struct entt::poly_direct_parent_types<sphere> {
    using parent_types = type_list<shape>;
};

template<>
struct entt::poly_direct_parent_types<cube> {
    using parent_types = type_list<shape>;
};

struct fat_cat : public entt::inherit<cat, sphere> {};


template<>
struct entt::poly_direct_parent_types<animal*> {
    using parent_types = type_list<>;
};

template<>
struct entt::poly_direct_parent_types<cat*> {
    using parent_types = type_list<animal*>;
};

template<>
struct entt::poly_direct_parent_types<dog*> {
    using parent_types = type_list<animal*>;
};

template<>
struct entt::poly_direct_parent_types<shape*> {
    using parent_types = type_list<>;
};

template<>
struct entt::poly_direct_parent_types<sphere*> {
    using parent_types = type_list<shape*>;
};

template<>
struct entt::poly_direct_parent_types<cube*> {
    using parent_types = type_list<shape*>;
};

template<>
struct entt::poly_direct_parent_types<fat_cat*> {
    using parent_types = type_list<cat*, sphere*>;
};


struct not_poly_type_base {};
struct not_poly_type : public not_poly_type_base {};

#endif
