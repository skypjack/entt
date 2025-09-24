#ifndef ENTT_META_FWD_HPP
#define ENTT_META_FWD_HPP

#include <cstddef>
#include <limits>

namespace entt {

class meta_ctx;

class meta_sequence_container;

class meta_associative_container;

class meta_any;

class meta_handle;

struct meta_custom;

class meta_data;

class meta_func;

class meta_type;

template<typename>
class meta_factory;

/*! @brief Used to identicate that a sequence container has not a fixed size. */
inline constexpr std::size_t meta_dynamic_extent = (std::numeric_limits<std::size_t>::max)();

} // namespace entt

#endif
