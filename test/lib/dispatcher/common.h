#include <entt/lib/attribute.h>
#include <entt/lib/export.h>
#include "types.h"

#ifdef EXPORT_COMMON
#define EXPORT_ATTRIBUTE ENTT_EXPORT
#else
#define EXPORT_ATTRIBUTE ENTT_IMPORT
#endif

ENTT_DISPATCHER_EXPORT(EXPORT_ATTRIBUTE);

ENTT_DISPATCHER_TYPE_EXPORT(an_event, EXPORT_ATTRIBUTE);
ENTT_DISPATCHER_TYPE_EXPORT(another_event, EXPORT_ATTRIBUTE);
