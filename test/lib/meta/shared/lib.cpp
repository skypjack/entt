#include <entt/core/attribute.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

test::boxed_int create_boxed_int(int value) {
    return test::boxed_int{value};
}

ENTT_API void share(const entt::locator<entt::meta_ctx>::node_type &handle) {
    entt::locator<entt::meta_ctx>::reset(handle);
}

ENTT_API void set_up() {
    using namespace entt::literals;

    entt::meta_factory<test::boxed_int>{}
        .type("boxed_int"_hs)
        .ctor<&create_boxed_int>()
        .data<&test::boxed_int::value>("value"_hs);

    entt::meta_factory<test::empty>{}
        .type("empty"_hs)
        .ctor<>();

    static_cast<void>(entt::meta_factory<double>{});
}

ENTT_API void tear_down() {
    entt::meta_reset<test::boxed_int>();
    entt::meta_reset<test::empty>();
}

ENTT_API entt::meta_any wrap_int(int value) {
    return value;
}
