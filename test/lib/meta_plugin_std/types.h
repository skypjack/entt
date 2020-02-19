#ifndef ENTT_LIB_META_PLUGIN_TYPES_STD_H
#define ENTT_LIB_META_PLUGIN_TYPES_STD_H

#include <type_traits>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/meta/meta.hpp>

template<typename>
struct type_id;

#define ASSIGN_TYPE_ID(clazz)\
    template<>\
    struct type_id<clazz>\
        : std::integral_constant<ENTT_ID_TYPE, entt::basic_hashed_string<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(#clazz)>>>>{#clazz}>\
    {}

template<typename Type>
struct entt::type_info<Type> {
    static constexpr ENTT_ID_TYPE id() ENTT_NOEXCEPT {
        return type_id<Type>::value;
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

struct userdata {
    entt::meta_ctx ctx;
    entt::meta_any any;
};

ASSIGN_TYPE_ID(void);
ASSIGN_TYPE_ID(std::size_t);
ASSIGN_TYPE_ID(position);
ASSIGN_TYPE_ID(velocity);
ASSIGN_TYPE_ID(double);
ASSIGN_TYPE_ID(int);

#endif
