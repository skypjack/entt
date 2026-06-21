module;

#define ENTT_MODULE
#define ENTT_MODULE_EXPORT export
#define ENTT_MODULE_EXPORT_BEGIN export {
#define ENTT_MODULE_EXPORT_END }

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <unordered_set>
#include <utility>

#ifdef ENTT_USER_CONFIG
#   include ENTT_USER_CONFIG
#endif // ENTT_USER_CONFIG

#include "config/config.h"
#include "config/macro.h"
#include "config/version.h"
#include "core/attribute.h"

export module entt;

#include "core/fwd.hpp"
#include "core/utility.hpp"
#include "core/algorithm.hpp"
#include "core/hashed_string.hpp"
#include "core/type_info.hpp"
#include "core/type_traits.hpp"
#include "core/any.hpp"
#include "core/bit.hpp"
#include "core/compressed_pair.hpp"
#include "core/enum.hpp"
#include "core/family.hpp"
#include "core/ident.hpp"
#include "core/iterator.hpp"
#include "core/memory.hpp"
#include "core/monostate.hpp"
#include "core/ranges.hpp"
#include "core/tuple.hpp"

#include "container/fwd.hpp"
#include "container/dense_map.hpp"
#include "container/dense_set.hpp"
#include "container/table.hpp"

#include "signal/fwd.hpp"
#include "signal/delegate.hpp"
#include "signal/dispatcher.hpp"
#include "signal/emitter.hpp"
#include "signal/sigh.hpp"

#include "graph/fwd.hpp"
#include "graph/adjacency_matrix.hpp"
#include "graph/dot.hpp"
#include "graph/flow.hpp"

#include "entity/fwd.hpp"
#include "entity/component.hpp"
#include "entity/entity.hpp"
#include "entity/group.hpp"
#include "entity/handle.hpp"
#include "entity/helper.hpp"
#include "entity/mixin.hpp"
#include "entity/organizer.hpp"
#include "entity/ranges.hpp"
#include "entity/registry.hpp"
#include "entity/runtime_view.hpp"
#include "entity/snapshot.hpp"
#include "entity/sparse_set.hpp"
#include "entity/storage.hpp"
#include "entity/view.hpp"

#include "locator/locator.hpp"

#include "meta/fwd.hpp"
#include "meta/adl_pointer.hpp"
#include "meta/context.hpp"
#include "meta/type_traits.hpp"
#include "meta/node.hpp"
#include "meta/range.hpp"
#include "meta/meta.hpp"
#include "meta/container.hpp"
#include "meta/resolve.hpp"
#include "meta/policy.hpp"
#include "meta/utility.hpp"
#include "meta/factory.hpp"
#include "meta/pointer.hpp"
#include "meta/template.hpp"

#include "poly/fwd.hpp"
#include "poly/poly.hpp"

#include "process/fwd.hpp"
#include "process/process.hpp"
#include "process/scheduler.hpp"

#include "resource/fwd.hpp"
#include "resource/cache.hpp"
#include "resource/loader.hpp"
#include "resource/resource.hpp"