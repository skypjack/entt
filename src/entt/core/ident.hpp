#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP


#include <tuple>
#include <cstddef>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Types identifiers.
 *
 * Variable template used to generate identifiers at compile-time for the given
 * types. Use the `get` member function to know what's the identifier associated
 * to the specific type.
 *
 * @note
 * Identifiers are constant expression and can be used in any context where such
 * an expression is required. As an example:
 * @code{.cpp}
 * using id = entt::identifier<a_type, another_type>;
 *
 * switch(a_type_identifier) {
 * case id::type<a_type>:
 *     // ...
 *     break;
 * case id::type<another_type>:
 *     // ...
 *     break;
 * default:
 *     // ...
 * }
 * @endcode
 *
 * @tparam Types List of types for which to generate identifiers.
 */
template<typename... Types>
class identifier {
    using tuple_type = std::tuple<std::decay_t<Types>...>;

    template<typename Type, std::size_t... Indexes>
    [[nodiscard]] static constexpr id_type get(std::index_sequence<Indexes...>) {
        static_assert(std::disjunction_v<std::is_same<Type, Types>...>, "Invalid type");
        return (0 + ... + (std::is_same_v<Type, std::tuple_element_t<Indexes, tuple_type>> ? id_type(Indexes) : id_type{}));
    }

public:
    /*! @brief Unsigned integer type. */
    using identifier_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename Type>
    static constexpr identifier_type type = get<std::decay_t<Type>>(std::index_sequence_for<Types...>{});
};


}


#endif
