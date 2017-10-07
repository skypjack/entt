#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP


#include<type_traits>
#include<cstddef>
#include<utility>


namespace entt {


namespace {


template<typename Type>
struct Wrapper {
    using type = Type;
    constexpr Wrapper(std::size_t index): index{index} {}
    const std::size_t index;
};


template<typename... Types>
struct Identifier final: Wrapper<Types>... {
    template<std::size_t... Indexes>
    constexpr Identifier(std::index_sequence<Indexes...>): Wrapper<Types>{Indexes}... {}

    template<typename Type>
    constexpr std::size_t get() const { return Wrapper<std::decay_t<Type>>::index; }
};


}


/**
 * @brief Types identifers.
 *
 * Variable template used to generate identifiers at compile-time for the given
 * types. Use the `constexpr` `get` member function to know what's the
 * identifier associated to the specific type.
 *
 * @note Identifiers are constant expression and can be used in any context
 * where such an expression is required. As an example:
 * @code{.cpp}
 * constexpr auto identifiers = entt::ident<AType, AnotherType>;
 *
 * switch(aTypeIdentifier) {
 * case identifers.get<AType>():
 *     // ...
 *     break;
 * case identifers.get<AnotherType>():
 *     // ...
 *     break;
 * default:
 *     // ...
 * }
 * @endcode
 *
 * @tparam Types The list of types for which to generate identifiers.
 */
template<typename... Types>
constexpr auto ident = Identifier<std::decay_t<Types>...>{std::make_index_sequence<sizeof...(Types)>{}};


}


#endif // ENTT_CORE_IDENT_HPP
