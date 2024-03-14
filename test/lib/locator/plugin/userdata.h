#ifndef ENTT_LIB_LOCATOR_PLUGIN_USERDATA_H
#define ENTT_LIB_LOCATOR_PLUGIN_USERDATA_H

#include "common/boxed_type.h"
#include <entt/locator/locator.hpp>

struct userdata {
    typename entt::locator<test::boxed_int>::node_type handle{};
    int value{};
};

#endif
