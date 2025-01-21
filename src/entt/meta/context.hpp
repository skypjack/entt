#ifndef ENTT_META_CTX_HPP
#define ENTT_META_CTX_HPP

#include "../container/dense_map.hpp"
#include "../core/fwd.hpp"
#include "../core/utility.hpp"

namespace entt {

class meta_ctx;

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

struct meta_type_node;

struct meta_context {
    dense_map<id_type, meta_type_node, identity> value;

    [[nodiscard]] inline static meta_context &from(meta_ctx &ctx);
    [[nodiscard]] inline static const meta_context &from(const meta_ctx &ctx);
};

} // namespace internal
/*! @endcond */

/*! @brief Disambiguation tag for constructors and the like. */
class meta_ctx_arg_t final {};

/*! @brief Constant of type meta_context_arg_t used to disambiguate calls. */
inline constexpr meta_ctx_arg_t meta_ctx_arg{};

/*! @brief Opaque meta context type. */
class meta_ctx: private internal::meta_context {
    // attorney idiom like model to access the base class
    friend struct internal::meta_context;
};

/*! @cond TURN_OFF_DOXYGEN */
[[nodiscard]] inline internal::meta_context &internal::meta_context::from(meta_ctx &ctx) {
    return ctx;
}

[[nodiscard]] inline const internal::meta_context &internal::meta_context::from(const meta_ctx &ctx) {
    return ctx;
}
/*! @endcond */

} // namespace entt

#endif
