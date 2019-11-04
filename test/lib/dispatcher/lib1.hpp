#ifndef TEST_LIB_DISPATCHER_LIB1_HPP
#define TEST_LIB_DISPATCHER_LIB1_HPP


#include <entt/lib/attribute.h>
#include <entt/signal/dispatcher.hpp>

#include "common.h"


#ifdef EXPORT_LIB1
#define LIB1_EXPORT_ATTRIBUTE ENTT_EXPORT
#else
#define LIB1_EXPORT_ATTRIBUTE ENTT_IMPORT
#endif


struct lib1_empty_event {};

struct lib1_payload_event {
	int payload;
};


LIB1_EXPORT_ATTRIBUTE void
ntrigger_common_empty_event(entt::dispatcher &dispatcher);

LIB1_EXPORT_ATTRIBUTE void
trigger_lib1_empty_event(entt::dispatcher &dispatcher);

LIB1_EXPORT_ATTRIBUTE void
trigger_lib1_payload_plus_1_event(int payload, entt::dispatcher &dispatcher);

LIB1_EXPORT_ATTRIBUTE void
trigger_lib1_payload_plus_3_event(int payload, entt::dispatcher &dispatcher);


ENTT_DISPATCHER_TYPE_EXPORT(lib1_empty_event, LIB1_EXPORT_ATTRIBUTE);
ENTT_DISPATCHER_TYPE_EXPORT(lib1_payload_event, LIB1_EXPORT_ATTRIBUTE);

#endif /* TEST_LIB_DISPATCHER_LIB1_HPP */
