#include <entt/core/attribute.h>
#include "../types.h"

ENTT_API const void *filter(const view_type &view) {
    // forces the creation of all symbols for the view type
    [[maybe_unused]] const view_type other{};

    // unset filter fallback should not be accessible across boundaries
    return view.storage<1u>();
}
