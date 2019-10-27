#ifndef ENTT_LIB_LIB_H
#define ENTT_LIB_LIB_H


#include "../config/config.h"
#include "../core/family.hpp"


#define ENTT_DISPATCHER_EXPORT(clazz, attribute)\
    namespace entt {\
        template attribute const ENTT_ID_TYPE family<struct internal_dispatcher_event_family>::inner<clazz>;\
        template attribute const ENTT_ID_TYPE family<struct internal_dispatcher_event_family>::type<clazz>;\
    }\
    static_assert(true)


#endif // ENTT_LIB_LIB_H
