#ifndef ENTT_LIB_LIB_H
#define ENTT_LIB_LIB_H


#include "../config/config.h"
#include "../core/family.hpp"


/**
   @brief Exports the type id of the given class in the family with the given
   tag

   @details For unknown reasons exporting the type id within the family seems to
   be required for GCC and Clang. MSVC doesn't allow importing member variables
   so exporting it makes no sense.

   @param family_tag The tag of the family for which to export the type id.
   @param clazz The type for which to export its type id withic the family.
   @param annribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#define ENTT_EXPORT_FAMILY_TYPE_ID(family_tag, clazz, attribute) static_assert(true)
#else
#define ENTT_EXPORT_FAMILY_TYPE_ID(family_tag, clazz, attribute)			\
	static_assert(true)
//	template attribute const ENTT_ID_TYPE family<family_tag>::type<clazz>
#endif


#define ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(family_tag, attribute)	\
	namespace entt {\
        template attribute ENTT_ID_TYPE family<family_tag>::generate_identifier();\
    }\
    static_assert(true)


#define ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(family_tag, clazz, attribute) \
	namespace entt {\
        template attribute ENTT_ID_TYPE family<family_tag>::generate_type_id<clazz>();\
		ENTT_EXPORT_FAMILY_TYPE_ID(family_tag, clazz, attribute); \
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
#define ENTT_DISPATCHER_EXPORT(attribute)\
    ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(struct internal_dispatcher_event_family, attribute)


/**
   @brief Exports dispatcher for the given type

   @param clazz The type for which to export dispatcher.
   @param attribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#define ENTT_DISPATCHER_TYPE_EXPORT(clazz, attribute)\
	ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(struct internal_dispatcher_event_family, clazz, attribute)


#endif // ENTT_LIB_LIB_H
