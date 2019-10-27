#ifndef ENTT_LIB_ATTRIBUTE_H
#define ENTT_LIB_ATTRIBUTE_H


#ifndef ENTT_EXPORT_IMPORT_ATTRIBUTE
#    define ENTT_EXPORT_IMPORT_ATTRIBUTE
#
#    if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#        define ENTT_EXPORT __declspec(dllexport)
#        define ENTT_IMPORT __declspec(dllimport)
#    elif defined __GNUC__ && __GNUC__ >= 4
#        define ENTT_EXPORT __attribute__((visibility("default")))
#        define ENTT_IMPORT __attribute__((visibility("default")))
#    else
#        error "Unsupported compoler"
#        define ENTT_EXPORT
#        define ENTT_IMPORT
#    endif
#endif


#endif // ENTT_LIB_ATTRIBUTE_H
