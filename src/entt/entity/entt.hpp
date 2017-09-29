#ifndef ENTT_ENTITY_ENTT_HPP
#define ENTT_ENTITY_ENTT_HPP


#include <cstdint>


namespace entt {


template<typename>
struct entt_traits;


template<>
struct entt_traits<std::uint32_t> {
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;

    static constexpr auto entity_mask = 0xFFFFFF;
    static constexpr auto version_mask = 0xFF;
    static constexpr auto version_shift = 24;
};


template<>
struct entt_traits<std::uint64_t> {
    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;

    static constexpr auto entity_mask = 0xFFFFFFFFFF;
    static constexpr auto version_mask = 0xFFFFFF;
    static constexpr auto version_shift = 40;
};


}


#endif // ENTT_ENTITY_ENTT_HPP
