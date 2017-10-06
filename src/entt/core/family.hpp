#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include<type_traits>
#include<cstddef>
#include<utility>


namespace entt {


template<typename...>
class Family {
    static std::size_t identifier() noexcept {
        static std::size_t value = 0;
        return value++;
    }

public:
    template<typename...>
    static std::size_t type() noexcept {
        static const std::size_t value = identifier();
        return value;
    }
};


}


#endif // ENTT_CORE_FAMILY_HPP
