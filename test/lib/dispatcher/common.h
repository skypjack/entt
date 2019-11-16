#ifndef TEST_LIB_DISPATCHER_COMMON_H
#define TEST_LIB_DISPATCHER_COMMON_H

#include <entt/lib/attribute.h>
#include <entt/lib/export.h>

#ifdef EXPORT_COMMON
#define EXPORT_ATTRIBUTE ENTT_EXPORT
#define EXPORT_ANY_ATTRIBUTE ENTT_EXPORT
#else
#define EXPORT_ATTRIBUTE ENTT_IMPORT
#define EXPORT_ANY_ATTRIBUTE ENTT_SELECT_ANY_ATTRIBUTE
#endif

struct common_empty_event {};

struct common_payload_event {
  int payload;
};

ENTT_DISPATCHER_EXPORT(EXPORT_ATTRIBUTE);

ENTT_DISPATCHER_TYPE_EXPORT(common_empty_event, EXPORT_ATTRIBUTE);
ENTT_DISPATCHER_TYPE_EXPORT(common_payload_event, EXPORT_ATTRIBUTE);

#endif /* TEST_LIB_DISPATCHER_COMMON_H */
