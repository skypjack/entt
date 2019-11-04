#ifndef TEST_LIB_DISPATCHER_LIB2_HPP
#define TEST_LIB_DISPATCHER_LIB2_HPP


#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "common.h"


#ifdef EXPORT_LIB2
#define LIB2_EXPORT_ATTRIBUTE ENTT_EXPORT
#define LIB2_EXTERN_ATTRIBUTE
#else
#define LIB2_EXPORT_ATTRIBUTE ENTT_IMPORT
#define LIB2_EXTERN_ATTRIBUTE extern
#endif


struct lib2_empty_event {};

struct lib2_payload_event {
	int payload;
};


LIB2_EXPORT_ATTRIBUTE
void trigger_common_payload_event(int payload, entt::dispatcher &dispatcher);


LIB2_EXPORT_ATTRIBUTE void
trigger_common_empty_event(entt::dispatcher &dispatcher);

LIB2_EXPORT_ATTRIBUTE void
trigger_lib2_payload_plus_2_event(int payload, entt::dispatcher &dispatcher);

LIB2_EXPORT_ATTRIBUTE void
trigger_lib2_empty_event(entt::dispatcher &dispatcher);


ENTT_DISPATCHER_TYPE_EXPORT(lib2_empty_event, LIB2_EXPORT_ATTRIBUTE, LIB2_EXTERN_ATTRIBUTE);
ENTT_DISPATCHER_TYPE_EXPORT(lib2_payload_event, LIB2_EXPORT_ATTRIBUTE, LIB2_EXTERN_ATTRIBUTE);


#endif /* TEST_LIB_DISPATCHER_LIB2_HPP */
