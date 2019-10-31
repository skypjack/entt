#ifndef TEST_LIB_DISPATCHER_LIB2_HPP
#define TEST_LIB_DISPATCHER_LIB2_HPP


#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>


#ifdef EXPORT_LIB2
#define LIB2_EXPORT_ATTRIBUTE ENTT_EXPORT
#else
#define LIB2_EXPORT_ATTRIBUTE ENTT_IMPORT
#endif


LIB2_EXPORT_ATTRIBUTE
void trigger_common_payload_event(int payload, entt::dispatcher &dispatcher);


#endif /* TEST_LIB_DISPATCHER_LIB2_HPP */
