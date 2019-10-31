#ifndef TEST_LIB_DISPATCHER_LIB1_HPP
#define TEST_LIB_DISPATCHER_LIB1_HPP


#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>


#ifdef EXPORT_LIB1
#define LIB1_EXPORT_ATTRIBUTE ENTT_EXPORT
#else
#define LIB1_EXPORT_ATTRIBUTE ENTT_IMPORT
#endif


LIB1_EXPORT_ATTRIBUTE
void trigger_common_empty_event(entt::dispatcher &dispatcher);


#endif /* TEST_LIB_DISPATCHER_LIB1_HPP */
