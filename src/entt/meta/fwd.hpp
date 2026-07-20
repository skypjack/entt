#ifndef ENTT_META_FWD_HPP
#define ENTT_META_FWD_HPP

#include "../stl/cstddef.hpp"
#include "../stl/limits.hpp"

namespace entt {

struct meta_ctx;

class meta_sequence_container;

class meta_associative_container;

class meta_any;

class meta_handle;

struct meta_custom;

struct meta_data;

struct meta_func;

struct meta_base;

class meta_type;

template<typename>
class meta_factory;

/*! @brief Used to identicate that a sequence container has not a fixed size. */
inline constexpr stl::size_t meta_dynamic_extent = (stl::numeric_limits<stl::size_t>::max)();

/*! @brief Disambiguation tag for constructors and the like. */
struct meta_ctx_arg_t final {};

/*! @brief Constant of type meta_context_arg_t used to disambiguate calls. */
inline constexpr meta_ctx_arg_t meta_ctx_arg{};

} // namespace entt

#endif
