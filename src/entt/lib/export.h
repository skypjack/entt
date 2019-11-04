#ifndef ENTT_LIB_LIB_H
#define ENTT_LIB_LIB_H


#include "../config/config.h"
#include "../core/family.hpp"


#define ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(family_tag, attribute, extern_attr)	\
	namespace entt {\
        extern_attr template attribute ENTT_ID_TYPE family<family_tag>::generate_identifier();\
    }\
    static_assert(true)


#define ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(family_tag, clazz, attribute, extern_attr) \
	namespace entt {\
        extern_attr template attribute ENTT_ID_TYPE family<family_tag>::generate_type_id<clazz>();\
    }\
    static_assert(true)


/**
   @brief Exports dispatcher family

   @details Required for ensuring a single instantiation of
   family::generate_identifier function. This macro is separated because only
   one explicit specialisation of the template function is allowed by GCC and
   Clang.

   @param attribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#define ENTT_DISPATCHER_EXPORT(attribute, extern_attr)\
    ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(struct internal_dispatcher_event_family, attribute, extern_attr)


/**
   @brief Exports dispatcher for the given type

   @param clazz The type for which to export dispatcher.
   @param attribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#define ENTT_DISPATCHER_TYPE_EXPORT(clazz, attribute, extern_attr)\
	ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(struct internal_dispatcher_event_family, clazz, attribute, extern_attr)


#endif // ENTT_LIB_LIB_H
