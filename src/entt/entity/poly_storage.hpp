#ifndef ENTT_ENTITY_POLY_STORAGE_HPP
#define ENTT_ENTITY_POLY_STORAGE_HPP


#include <cstddef>
#include <tuple>
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../poly/poly.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Basic poly storage implementation.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct Storage: type_list<
    type_info() const ENTT_NOEXCEPT,
    void(const Entity *, const Entity *)
> {
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Concept definition.
     * @tparam Base Opaque base class from which to inherit.
     */
    template<typename Base>
    struct type: Base {
        /**
         * @brief Returns a type info for the contained objects.
         * @return The type info for the contained objects.
         */
        type_info value_type() const ENTT_NOEXCEPT {
            return poly_call<0>(*this);
        }

        /**
         * @brief Removes entities from a storage.
         * @param first An iterator to the first element of the range of
         * entities.
         * @param last An iterator past the last element of the range of
         * entities.
         */
        void remove(const entity_type *first, const entity_type *last) {
            poly_call<1>(*this, first, last);
        }
    };

    /**
     * @brief Concept implementation.
     * @tparam Type Type for which to generate an implementation.
     */
    template<typename Type>
    using impl = value_list<
        &type_id<typename Type::value_type>,
        &Type::template remove<const entity_type *>
    >;
};


/**
 * @brief Defines the poly storage type associate with a given entity type.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity, typename = void>
struct poly_storage_traits {
    /*! @brief Poly storage type for the given entity type. */
    using storage_type = poly<Storage<Entity>>;
};


}


#endif
