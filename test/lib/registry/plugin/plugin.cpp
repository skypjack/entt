#include <common/boxed_type.h>
#include <common/empty.h>
#include <cr.h>
#include <entt/entity/registry.hpp>

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    constexpr auto count = 3;

    switch(operation) {
    case CR_STEP: {
        // forces things to break
        auto &registry = *static_cast<entt::registry *>(ctx->userdata);

        // forces the creation of the pool for the empty component
        static_cast<void>(registry.storage<test::empty>());

        const auto view = registry.view<test::boxed_int>();
        registry.insert(view.begin(), view.end(), test::empty{});

        registry.view<test::boxed_int, test::empty>().each([cnt = count](test::boxed_int &elem) {
            elem.value += cnt;
        });
    } break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}
