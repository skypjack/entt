#ifndef ENTT_LIB_REGISTRY_PLUGIN_STD_TYPES_H
#define ENTT_LIB_REGISTRY_PLUGIN_STD_TYPES_H

#include <type_traits>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>

template<typename>
struct component_id;

#define ASSIGN_TYPE_ID(clazz)\
    template<>\
    struct component_id<clazz>\
        : std::integral_constant<ENTT_ID_TYPE, entt::basic_hashed_string<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(#clazz)>>>>{#clazz}>\
    {}

template<typename Type>
struct entt::type_info<Type> {
    static constexpr ENTT_ID_TYPE id() ENTT_NOEXCEPT {
        return component_id<Type>::value;
    }
};

struct position {
    int x;
    int y;
};

struct velocity {
    double dx;
    double dy;
};

ASSIGN_TYPE_ID(position);
ASSIGN_TYPE_ID(velocity);

#endif
