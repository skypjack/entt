#ifndef ENTT_POLY_FWD_HPP
#define ENTT_POLY_FWD_HPP

#include <type_traits>

namespace entt {

template<typename, std::size_t Len, std::size_t = alignof(typename std::aligned_storage_t<Len + !Len>)>
class basic_poly;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Concept Concept descriptor.
 */
template<typename Concept>
using poly = basic_poly<Concept, sizeof(double[2])>;

} // namespace entt

#endif
