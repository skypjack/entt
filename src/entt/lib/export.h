#ifndef ENTT_LIB_LIB_H
#define ENTT_LIB_LIB_H


#include "../config/config.h"
#include "../core/family.hpp"

#define ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(family_tag, attribute)         \
  namespace entt {                                                             \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  attribute ENTT_ID_TYPE family<family_tag>::generate_identifier();            \
  }                                                                            \
  static_assert(true)

#define ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(family_tag, clazz, attribute)       \
  namespace entt {                                                             \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  attribute ENTT_ID_TYPE family<family_tag>::generate_type_id<clazz>();        \
  }                                                                            \
  static_assert(true)

#define ENTT_FAMILY_IDENTIFIER_GENERATOR_IMPL(family_tag)                      \
  namespace entt {                                                             \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  ENTT_EXPORT ENTT_ID_TYPE family<family_tag>::generate_identifier() {         \
    static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier{};                       \
                                                                               \
    return identifier++;                                                       \
  }                                                                            \
  }                                                                            \
  static_assert(true)

/*
  GCC is broken with respect to multiple template parameter list at explicit
  specialisation.
*/
#define ENTT_FAMILY_TYPE_IDENTIFIER_IMPL(family_tag, clazz)                    \
  namespace entt {                                                             \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  ENTT_TEMPLATE_SPECIALIZATION_PARAMETER_LIST_FIX_FOR_GCC                      \
  ENTT_EXPORT ENTT_ID_TYPE family<family_tag>::generate_type_id<clazz>() {     \
    static const ENTT_ID_TYPE type_id = generate_identifier();                 \
                                                                               \
    return type_id;                                                            \
  }                                                                            \
  }                                                                            \
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
#define ENTT_DISPATCHER_EXPORT(attribute)                                      \
  ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(                                     \
      struct internal_dispatcher_event_family, attribute)

#define ENTT_DISPATCHER_IMPL                                                   \
  ENTT_FAMILY_IDENTIFIER_GENERATOR_IMPL(struct internal_dispatcher_event_family)

/**
   @brief Exports dispatcher for the given type

   @param clazz The type for which to export dispatcher.
   @param attribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#define ENTT_DISPATCHER_TYPE_EXPORT(clazz, attribute)                          \
  ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(struct internal_dispatcher_event_family,  \
                                     clazz, attribute)

#define ENTT_DISPATCHER_TYPE_IMPL(clazz)                                       \
  ENTT_FAMILY_TYPE_IDENTIFIER_IMPL(struct internal_dispatcher_event_family,    \
                                   clazz)

#endif // ENTT_LIB_LIB_H
