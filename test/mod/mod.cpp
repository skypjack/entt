#include <gtest/gtest.h>
#include <cassert>
#include <map>
#include <string>
#include <duktape.h>
#include <entt/entity/registry.hpp>

template<typename Type>
struct tag { using type = Type; };

struct Position {
    double x;
    double y;
};

struct Renderable {};

struct DuktapeRuntime {
    std::map<duk_uint_t, std::string> components;
};

template<typename Comp>
duk_ret_t set(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    registry.accommodate<Comp>(entity);
    return 0;
}

template<>
duk_ret_t set<Position>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    const auto x = duk_require_number(ctx, 2);
    const auto y = duk_require_number(ctx, 3);
    registry.accommodate<Position>(entity, x, y);
    return 0;
}

template<>
duk_ret_t set<DuktapeRuntime>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    const auto type = duk_require_uint(ctx, 1);

    duk_dup(ctx, 2);

    if(!registry.has<DuktapeRuntime>(entity)) {
        registry.assign<DuktapeRuntime>(entity).components[type] = duk_json_encode(ctx, -1);
    } else {
        registry.get<DuktapeRuntime>(entity).components[type] = duk_json_encode(ctx, -1);
    }

    duk_pop(ctx);

    return 0;
}

template<typename Comp>
duk_ret_t unset(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    registry.remove<Comp>(entity);
    return 0;
}

template<>
duk_ret_t unset<DuktapeRuntime>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    const auto type = duk_require_uint(ctx, 1);

    auto &components = registry.get<DuktapeRuntime>(entity).components;
    assert(components.find(type) != components.cend());
    components.erase(type);

    if(components.empty()) {
        registry.remove<DuktapeRuntime>(entity);
    }

    return 0;
}

template<typename Comp>
duk_ret_t has(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    duk_push_boolean(ctx, registry.has<Comp>(entity));
    return 1;
}

template<>
duk_ret_t has<DuktapeRuntime>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    duk_push_boolean(ctx, registry.has<DuktapeRuntime>(entity));

    if(registry.has<DuktapeRuntime>(entity)) {
        const auto type = duk_require_uint(ctx, 1);
        const auto &components = registry.get<DuktapeRuntime>(entity).components;
        duk_push_boolean(ctx, components.find(type) != components.cend());
    } else {
        duk_push_false(ctx);
    }

    return 1;
}

template<typename Comp>
duk_ret_t get(duk_context *ctx, entt::DefaultRegistry &registry) {
    assert(registry.has<Comp>(duk_require_uint(ctx, 0)));
    duk_push_object(ctx);
    return 1;
}

template<>
duk_ret_t get<Position>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    const auto &position = registry.get<Position>(entity);

    const auto idx = duk_push_object(ctx);

    duk_push_string(ctx, "x");
    duk_push_number(ctx, position.x);
    duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE);

    duk_push_string(ctx, "y");
    duk_push_number(ctx, position.y);
    duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE);

    return 1;
}

template<>
duk_ret_t get<DuktapeRuntime>(duk_context *ctx, entt::DefaultRegistry &registry) {
    const auto entity = duk_require_uint(ctx, 0);
    const auto type = duk_require_uint(ctx, 1);

    auto &runtime = registry.get<DuktapeRuntime>(entity);
    assert(runtime.components.find(type) != runtime.components.cend());

    duk_push_string(ctx, runtime.components[type].c_str());
    duk_json_decode(ctx, -1);


    return 1;
}

class DuktapeRegistry {
    // I'm pretty sure I won't have more than 99 components in the example
    static constexpr entt::DefaultRegistry::component_type udef = 100;

    struct Func {
        using func_type = duk_ret_t(*)(duk_context *, entt::DefaultRegistry &);
        using test_type = bool(entt::DefaultRegistry:: *)(entt::DefaultRegistry::entity_type) const;

        func_type set;
        func_type unset;
        func_type has;
        func_type get;
        test_type test;
    };

    template<typename... Comp>
    void reg() {
        using accumulator_type = int[];
        accumulator_type acc = { (func[registry.component<Comp>()] = {
                                     &::set<Comp>,
                                     &::unset<Comp>,
                                     &::has<Comp>,
                                     &::get<Comp>,
                                     &entt::DefaultRegistry::has<Comp>
                                 }, 0)... };
        (void)acc;
    }

    static DuktapeRegistry & instance(duk_context *ctx) {
        duk_push_this(ctx);

        duk_push_string(ctx, DUK_HIDDEN_SYMBOL("dreg"));
        duk_get_prop(ctx, -2);
        auto &dreg = *static_cast<DuktapeRegistry *>(duk_require_pointer(ctx, -1));
        duk_pop_2(ctx);

        return dreg;
    }

    template<Func::func_type Func::*Op>
    static duk_ret_t invoke(duk_context *ctx) {
        auto &dreg = instance(ctx);
        auto &func = dreg.func;
        auto &registry = dreg.registry;
        auto type = duk_require_uint(ctx, 1);

        if(type >= udef) {
            type = registry.component<DuktapeRuntime>();
        }

        assert(func.find(type) != func.cend());

        return (func[type].*Op)(ctx, registry);
    }

public:
    DuktapeRegistry(entt::DefaultRegistry &registry)
        : registry{registry}
    {
        reg<Position, Renderable, DuktapeRuntime>();
    }

    static duk_ret_t identifier(duk_context *ctx) {
        static auto next = udef;
        duk_push_uint(ctx, next++);
        return 1;
    }

    static duk_ret_t create(duk_context *ctx) {
        auto &dreg = instance(ctx);
        duk_push_uint(ctx, dreg.registry.create());
        return 1;
    }

    static duk_ret_t set(duk_context *ctx) {
        return invoke<&Func::set>(ctx);
    }

    static duk_ret_t unset(duk_context *ctx) {
        return invoke<&Func::unset>(ctx);
    }

    static duk_ret_t has(duk_context *ctx) {
        return invoke<&Func::has>(ctx);
    }

    static duk_ret_t get(duk_context *ctx) {
        return invoke<&Func::get>(ctx);
    }

    static duk_ret_t entities(duk_context *ctx) {
        const duk_idx_t nargs = duk_get_top(ctx);
        auto &dreg = instance(ctx);
        duk_uarridx_t pos = 0;

        duk_push_array(ctx);

        dreg.registry.each([ctx, nargs, &pos, &dreg](auto entity) {
            auto &registry = dreg.registry;
            auto &func = dreg.func;
            bool match = true;

            for (duk_idx_t arg = 0; match && arg < nargs; arg++) {
                auto type = duk_require_uint(ctx, arg);

                if(type < udef) {
                    assert(func.find(type) != func.cend());
                    match = (registry.*func[type].test)(entity);
                } else {
                    const auto ctype = registry.component<DuktapeRuntime>();
                    assert(func.find(ctype) != func.cend());
                    match = (registry.*func[ctype].test)(entity);

                    if(match) {
                        auto &components = registry.get<DuktapeRuntime>(entity).components;
                        match = (components.find(type) != components.cend());
                    }
                }
            }

            if(match) {
                duk_push_uint(ctx, entity);
                duk_put_prop_index(ctx, -2, pos++);
            }
        });

        return 1;
    }

private:
    std::map<duk_uint_t, Func> func;
    entt::DefaultRegistry &registry;
};

const duk_function_list_entry js_DuktapeRegistry_methods[] = {
    { "identifier", &DuktapeRegistry::identifier, 0 },
    { "create", &DuktapeRegistry::create, 0 },
    { "set", &DuktapeRegistry::set, DUK_VARARGS },
    { "unset", &DuktapeRegistry::unset, 2 },
    { "has", &DuktapeRegistry::has, 2 },
    { "get", &DuktapeRegistry::get, 2 },
    { "entities", &DuktapeRegistry::entities, DUK_VARARGS },
    { nullptr, nullptr, 0 }
};

void exportTypes(duk_context *ctx, entt::DefaultRegistry &registry) {
    auto exportType = [](auto *ctx, auto &registry, auto idx, auto type, const auto *name) {
        duk_push_string(ctx, name);
        duk_push_uint(ctx, registry.template component<typename decltype(type)::type>());
        duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE);
    };

    auto idx = duk_push_object(ctx);

    exportType(ctx, registry, idx, tag<Position>{}, "POSITION");
    exportType(ctx, registry, idx, tag<Renderable>{}, "RENDERABLE");

    duk_put_global_string(ctx, "Types");
}

void exportDuktapeRegistry(duk_context *ctx, DuktapeRegistry &dreg) {
    auto idx = duk_push_object(ctx);

    duk_push_string(ctx, DUK_HIDDEN_SYMBOL("dreg"));
    duk_push_pointer(ctx, &dreg);
    duk_put_prop(ctx, idx);

    duk_put_function_list(ctx, idx, js_DuktapeRegistry_methods);
    duk_put_global_string(ctx, "Registry");
}

TEST(Mod, Duktape) {
    entt::DefaultRegistry registry;
    DuktapeRegistry dreg{registry};
    duk_context *ctx = duk_create_heap_default();

    if(!ctx) {
        FAIL();
    }

    exportTypes(ctx, registry);
    exportDuktapeRegistry(ctx, dreg);

    const char *s0 = ""
            "Types[\"PLAYING_CHARACTER\"] = Registry.identifier();"
            "Types[\"VELOCITY\"] = Registry.identifier();"
            "";

    if(duk_peval_string(ctx, s0)) {
        FAIL();
    }

    registry.create(Position{ 0., 0. }, Renderable{});
    registry.create(Position{ 0., 0. });

    const char *s1 = ""
            "Registry.entities(Types.POSITION, Types.RENDERABLE).forEach(function(entity) {"
                "Registry.set(entity, Types.POSITION, 100., 100.);"
            "});"
            "var entity = Registry.create();"
            "Registry.set(entity, Types.POSITION, 100., 100.);"
            "Registry.set(entity, Types.RENDERABLE);"
            "";

    if(duk_peval_string(ctx, s1)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<DuktapeRuntime>().size(), 0u);
    ASSERT_EQ(registry.view<Position>().size(), 3u);
    ASSERT_EQ(registry.view<Renderable>().size(), 2u);

    registry.view<Position>().each([&registry](auto entity, const auto &position) {
        ASSERT_FALSE(registry.has<DuktapeRuntime>(entity));

        if(registry.has<Renderable>(entity)) {
            ASSERT_EQ(position.x, 100.);
            ASSERT_EQ(position.y, 100.);
        } else {
            ASSERT_EQ(position.x, 0.);
            ASSERT_EQ(position.y, 0.);
        }
    });

    const char *s2 = ""
            "Registry.entities(Types.POSITION).forEach(function(entity) {"
                "if(!Registry.has(entity, Types.RENDERABLE)) {"
                    "Registry.set(entity, Types.VELOCITY, { \"dx\": -100., \"dy\": -100. });"
                    "Registry.set(entity, Types.PLAYING_CHARACTER, {});"
                "}"
            "});"
            "";

    if(duk_peval_string(ctx, s2)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<DuktapeRuntime>().size(), 1u);
    ASSERT_EQ(registry.view<Position>().size(), 3u);
    ASSERT_EQ(registry.view<Renderable>().size(), 2u);

    registry.view<DuktapeRuntime>().each([](auto, const DuktapeRuntime &runtime) {
        ASSERT_EQ(runtime.components.size(), 2u);
    });

    const char *s3 = ""
            "Registry.entities(Types.POSITION, Types.RENDERABLE, Types.VELOCITY, Types.PLAYING_CHARACTER).forEach(function(entity) {"
                "var velocity = Registry.get(entity, Types.VELOCITY);"
                "Registry.set(entity, Types.POSITION, velocity.dx, velocity.dy)"
            "});"
            "";

    if(duk_peval_string(ctx, s3)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<DuktapeRuntime>().size(), 1u);
    ASSERT_EQ(registry.view<Position>().size(), 3u);
    ASSERT_EQ(registry.view<Renderable>().size(), 2u);

    registry.view<Position, Renderable, DuktapeRuntime>().each([](auto, const Position &position, const auto &...) {
        ASSERT_EQ(position.x, -100.);
        ASSERT_EQ(position.y, -100.);
    });

    const char *s4 = ""
            "Registry.entities(Types.VELOCITY, Types.PLAYING_CHARACTER).forEach(function(entity) {"
                "Registry.unset(entity, Types.VELOCITY);"
                "Registry.unset(entity, Types.PLAYING_CHARACTER);"
            "});"
            "Registry.entities(Types.POSITION).forEach(function(entity) {"
                "Registry.unset(entity, Types.POSITION);"
            "});"
            "";

    if(duk_peval_string(ctx, s4)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<DuktapeRuntime>().size(), 0u);
    ASSERT_EQ(registry.view<Position>().size(), 0u);
    ASSERT_EQ(registry.view<Renderable>().size(), 2u);

    duk_destroy_heap(ctx);
}
