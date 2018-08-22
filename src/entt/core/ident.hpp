#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP


#include <type_traits>
#include <cstddef>
#include <utility>
#include <tuple>
#include "../config/config.h"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename...>
struct IsPartOf;

template<typename Type, typename Current, typename... Other>
struct IsPartOf<Type, Current, Other...>: std::conditional_t<std::is_same<Type, Current>::value, std::true_type, IsPartOf<Type, Other...>> {};

template<typename Type>
struct IsPartOf<Type>: std::false_type {};

template <class T, class Tuple>
struct IndexOf;

template <class T, class U, class... Types>
struct IndexOf<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + IndexOf<T, std::tuple<Types...>>::value;
};

template <class T, class... Types>
struct IndexOf<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


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
 * using ID = entt::Identifier<AType, AnotherType>;
 *
 * switch(aTypeIdentifier) {
 * case ID::get<AType>():
 *     // ...
 *     break;
 * case ID::get<AnotherType>():
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
class Identifier final {
    using tuple_type = std::tuple<std::decay_t<Types>...>;

public:
    /*! @brief Unsigned integer type. */
    using identifier_type = std::size_t;

    /**
     * @brief Returns the identifier associated with a given type.
     * @tparam Type of which to return the identifier.
     * @return The identifier associated with the given type.
     */
    template<typename Type>
    static constexpr identifier_type get() ENTT_NOEXCEPT {
        static_assert(internal::IsPartOf<std::decay_t<Type>, Types...>::value, "!");
        return internal::IndexOf<std::decay_t<Type>, tuple_type>::value;
    }
};


}


#endif // ENTT_CORE_IDENT_HPP
