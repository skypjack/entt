#ifndef ENTT_LIB_LOCATOR_PLUGIN_USERDATA_H
#define ENTT_LIB_LOCATOR_PLUGIN_USERDATA_H

#include <entt/locator/locator.hpp>
#include "../../../common/boxed_type.h"

struct userdata {
    entt::locator<test::boxed_int>::node_type handle{};
    int value{};
};

#endif
