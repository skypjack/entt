#ifndef ENTT_IDENT_HPP
#define ENTT_IDENT_HPP


#include<type_traits>
#include<cstddef>
#include<utility>


namespace entt {


namespace details {


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


template<typename... Types>
constexpr auto ident = details::Identifier<std::decay_t<Types>...>{std::make_index_sequence<sizeof...(Types)>{}};


}


#endif // ENTT_IDENT_HPP
