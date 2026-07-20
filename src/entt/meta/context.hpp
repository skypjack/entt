#ifndef ENTT_META_CTX_HPP
#define ENTT_META_CTX_HPP

#include "../container/dense_map.hpp"
#include "../core/fwd.hpp"
#include "../stl/functional.hpp"
#include "../stl/memory.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

struct meta_type_node;

struct meta_context {
    using bucket_type = dense_map<id_type, stl::unique_ptr<meta_type_node>, stl::identity>;

    bucket_type bucket;

    [[nodiscard]] inline static meta_context &from(meta_ctx &);
    [[nodiscard]] inline static const meta_context &from(const meta_ctx &);
};

} // namespace internal
/*! @endcond */

/*! @brief Opaque meta context type. */
struct meta_ctx: private internal::meta_context {
    // attorney idiom like model to access the base class
    friend struct internal::meta_context;
};

/*! @cond ENTT_INTERNAL */
[[nodiscard]] inline internal::meta_context &internal::meta_context::from(meta_ctx &ctx) {
    return ctx;
}

[[nodiscard]] inline const internal::meta_context &internal::meta_context::from(const meta_ctx &ctx) {
    return ctx;
}
/*! @endcond */

} // namespace entt

#endif
