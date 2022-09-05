#ifndef ENTT_POLY_FWD_HPP
#define ENTT_POLY_FWD_HPP

#include <cstddef>

namespace entt {

template<typename, std::size_t Len = sizeof(double[2]), std::size_t = alignof(double[2])>
class basic_poly;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Concept Concept descriptor.
 */
template<typename Concept>
using poly = basic_poly<Concept>;

} // namespace entt

#endif
