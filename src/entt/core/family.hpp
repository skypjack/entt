#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
#include "../config/config.h"


namespace entt {


namespace family_internal {
    template<typename Family>
    auto generate_identifier () {
        static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier{};

        return identifier++;
    }

    template<typename Family, typename... Type>
    auto type_id () {
        static ENTT_ID_TYPE type = generate_identifier<Family> ();

        return type;
    }
}


/**
 * @brief Dynamic identifier generator.
 *
 * Utility class template that can be used to assign unique identifiers to types
 * at runtime. Use different specializations to create separate sets of
 * identifiers.
 */
template<typename... Tags>
class family {
public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const family_type type =
         family_internal::type_id<family<Tags...>, std::decay_t<Type>...>();
};


}


#endif // ENTT_CORE_FAMILY_HPP
