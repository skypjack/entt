#ifndef ENTT_LIB_VIEW_TYPE_H
#define ENTT_LIB_VIEW_TYPE_H

#include <entt/entity/storage.hpp>
#include <entt/entity/view.hpp>
#include "../../common/empty.h"

using view_type = entt::basic_view<entt::get_t<entt::storage<test::empty>>, entt::exclude_t<entt::storage<test::empty>>>;

#endif
