#ifndef ENTT_POLY_FWD_HPP
#define ENTT_POLY_FWD_HPP


namespace entt {


template<typename, std::size_t>
class basic_poly;


/**
 * @brief Alias declaration for the most common use case.
 * @tparam Concept Concept descriptor.
 */
template<typename Concept>
using poly = basic_poly<Concept, sizeof(double[2])>;


}


#endif
