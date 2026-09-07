#ifndef ENTT_POLY_FWD_HPP
#define ENTT_POLY_FWD_HPP

#include "../config/module.h"

#ifndef ENTT_MODULE
#    include "../stl/cstddef.hpp"
#endif // ENTT_MODULE

ENTT_MODULE_EXPORT namespace entt {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    template<typename, stl::size_t Len = sizeof(double[2]), stl::size_t = alignof(double[2])>
    class basic_poly;

    /**
     * @brief Alias declaration for the most common use case.
     * @tparam Concept Concept descriptor.
     */
    template<typename Concept>
    using poly = basic_poly<Concept>;

} // namespace entt

#endif
