#ifndef ENTT_LIB_META_PLUGIN_STD_USERDATA_H
#define ENTT_LIB_META_PLUGIN_STD_USERDATA_H

#include <type_traits>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

#define ASSIGN_TYPE_ID(clazz) \
    template<> \
    struct entt::type_hash<clazz> { \
        static constexpr entt::id_type value() noexcept { \
            return entt::basic_hashed_string<std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(#clazz)>>>>{#clazz}; \
        } \
    }

struct userdata {
    entt::locator<entt::meta_ctx>::node_type ctx{};
    entt::meta_any any{};
};

ASSIGN_TYPE_ID(void);
ASSIGN_TYPE_ID(std::size_t);
ASSIGN_TYPE_ID(test::boxed_int);
ASSIGN_TYPE_ID(test::empty);
ASSIGN_TYPE_ID(double);
ASSIGN_TYPE_ID(int);

#endif
