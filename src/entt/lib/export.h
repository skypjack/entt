#ifndef ENTT_LIB_LIB_H
#define ENTT_LIB_LIB_H

#include "../config/config.h"
#include "../core/family.hpp"
#include "../signal/dispatcher.hpp"

#define ENTT_FAMILY_IDENTIFIER_GENERATOR_EXPORT(family_tag, attribute)         \
  namespace entt {                                                             \
  namespace family_internal {                                                  \
  template <> attribute auto generate_identifier<family<family_tag>>() {       \
    static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier{};                       \
                                                                               \
    return identifier++;                                                       \
  };                                                                           \
  }                                                                            \
  }                                                                            \
  static_assert(true)

#define ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(family_tag, clazz, attribute)       \
  namespace entt {                                                             \
  namespace family_internal {                                                  \
  template <> attribute auto type_id<family<family_tag>, clazz>() {            \
    static ENTT_ID_TYPE type = generate_identifier<family<family_tag>>();      \
                                                                               \
    return type;                                                               \
  }                                                                            \
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

/**
   @brief Exports dispatcher for the given type

   @param clazz The type for which to export dispatcher.
   @param attribute The attribute macro that should expand into either export or
   import clause depending on whether the exported library is build or consumed
   respectively.
*/
#define ENTT_DISPATCHER_TYPE_EXPORT(clazz, attribute)                          \
  ENTT_FAMILY_TYPE_IDENTIFIER_EXPORT(                                          \
      struct internal_dispatcher_event_family, clazz, attribute)

#endif // ENTT_LIB_LIB_H
