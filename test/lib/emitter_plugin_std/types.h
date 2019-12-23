#ifndef ENTT_LIB_EMITTER_PLUGIN_STD_TYPES_H
#define ENTT_LIB_EMITTER_PLUGIN_STD_TYPES_H

#include <type_traits>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/signal/emitter.hpp>

template<typename>
struct event_id;

#define ASSIGN_TYPE_ID(clazz)\
    template<>\
    struct event_id<clazz>\
        : std::integral_constant<ENTT_ID_TYPE, entt::basic_hashed_string<std::remove_cv_t<std::remove_pointer_t<std::decay_t<decltype(#clazz)>>>>{#clazz}>\
    {}

template<typename Type>
struct entt::type_info<Type> {
    static constexpr ENTT_ID_TYPE id() ENTT_NOEXCEPT {
        return event_id<Type>::value;
    }
};

struct test_emitter
        : entt::emitter<test_emitter>
{};

struct message {
    int payload;
};

struct event {};

ASSIGN_TYPE_ID(message);
ASSIGN_TYPE_ID(event);

#endif
