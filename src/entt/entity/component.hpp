#ifndef ENTT_ENTITY_COMPONENT_HPP
#define ENTT_ENTITY_COMPONENT_HPP


#include <type_traits>
#include "../config/config.h"


namespace entt {


/*! @brief Commonly used default traits for all types. */
struct basic_component_traits {
    /*! @brief Pointer stability, default is `std::false_type`. */
    using in_place_delete = std::false_type;
    /*! @brief Empty type optimization, default is `ENTT_IGNORE_IF_EMPTY`. */
    using ignore_if_empty = ENTT_IGNORE_IF_EMPTY;
};


/**
 * @brief Common way to access various properties of components.
 * @tparam Type Type of component.
 */
template<typename Type, typename = void>
struct component_traits: basic_component_traits {
    static_assert(std::is_same_v<std::decay_t<Type>, Type>, "Unsupported type");
};


}


#endif
