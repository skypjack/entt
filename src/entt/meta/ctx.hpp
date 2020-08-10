#ifndef ENTT_META_CTX_HPP
#define ENTT_META_CTX_HPP


#include "../core/attribute.h"
#include "../config/config.h"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_type_node;


struct ENTT_API meta_context {
    // we could use the lines below but VS2017 returns with an ICE if combined with ENTT_API despite the code being valid C++
    //     inline static meta_type_node *local = nullptr;
    //     inline static meta_type_node **global = &local;

    [[nodiscard]] static meta_type_node * & local() ENTT_NOEXCEPT {
        static meta_type_node *chain = nullptr;
        return chain;
    }

    [[nodiscard]] static meta_type_node ** & global() ENTT_NOEXCEPT {
        static meta_type_node **chain = &local();
        return chain;
    }
};


}


/**
 * Internal details not to be documented.
 * @endcond
 */


/*! @brief Opaque container for a meta context. */
struct meta_ctx {
    /**
     * @brief Binds the meta system to a given context.
     * @param other A valid context to which to bind.
     */
    static void bind(meta_ctx other) ENTT_NOEXCEPT {
        internal::meta_context::global() = other.ctx;
    }

private:
    internal::meta_type_node **ctx{&internal::meta_context::local()};
};


}


#endif
