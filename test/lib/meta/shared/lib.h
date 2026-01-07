#include <entt/config/config.h>
#include <entt/locator/locator.hpp>
#include <entt/meta/fwd.hpp>

ENTT_API void share(const entt::locator<entt::meta_ctx>::node_type &);
ENTT_API void set_up();
ENTT_API void tear_down();
ENTT_API entt::meta_any wrap_int(int);
