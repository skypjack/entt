#include <entt/config/config.h>
#include "../types.h"
#include "lib.h"

ENTT_API const void *filter(const view_type &view) {
    // forces the creation of all symbols for the view type
    [[maybe_unused]] const view_type other{};

    // unset filter fallback should not be accessible across boundaries
    return view.storage<1u>();
}
