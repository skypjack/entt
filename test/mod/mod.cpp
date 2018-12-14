#include <gtest/gtest.h>
#include <type_traits>
#include <cassert>
#include <map>
#include <string>
#include <duktape.h>
#include <entt/entity/registry.hpp>

template<typename Type>
struct tag { using type = Type; };

struct position {
    double x;
    double y;
};

struct renderable {};

struct duktape_runtime {
    std::map<duk_uint_t, std::string> components;
};

template<typename Comp>
duk_ret_t set(duk_context *ctx, entt::registry<> &registry) {
    const auto entity = duk_require_uint(ctx, 0);

    if constexpr(std::is_same_v<Comp, position>) {
        const auto x = duk_require_number(ctx, 2);
        const auto y = duk_require_number(ctx, 3);
        registry.assign_or_replace<position>(entity, x, y);
    } else if constexpr(std::is_same_v<Comp, duktape_runtime>) {
        const auto type = duk_require_uint(ctx, 1);

        duk_dup(ctx, 2);

        if(!registry.has<duktape_runtime>(entity)) {
            registry.assign<duktape_runtime>(entity).components[type] = duk_json_encode(ctx, -1);
        } else {
            registry.get<duktape_runtime>(entity).components[type] = duk_json_encode(ctx, -1);
        }

        duk_pop(ctx);
    } else {
        registry.assign_or_replace<Comp>(entity);
    }

    return 0;
}

template<typename Comp>
duk_ret_t unset(duk_context *ctx, entt::registry<> &registry) {
    const auto entity = duk_require_uint(ctx, 0);

    if constexpr(std::is_same_v<Comp, duktape_runtime>) {
        const auto type = duk_require_uint(ctx, 1);

        auto &components = registry.get<duktape_runtime>(entity).components;
        assert(components.find(type) != components.cend());
        components.erase(type);

        if(components.empty()) {
            registry.remove<duktape_runtime>(entity);
        }
    } else {
        registry.remove<Comp>(entity);
    }

    return 0;
}

template<typename Comp>
duk_ret_t has(duk_context *ctx, entt::registry<> &registry) {
    const auto entity = duk_require_uint(ctx, 0);

    if constexpr(std::is_same_v<Comp, duktape_runtime>) {
        duk_push_boolean(ctx, registry.has<duktape_runtime>(entity));

        if(registry.has<duktape_runtime>(entity)) {
            const auto type = duk_require_uint(ctx, 1);
            const auto &components = registry.get<duktape_runtime>(entity).components;
            duk_push_boolean(ctx, components.find(type) != components.cend());
        } else {
            duk_push_false(ctx);
        }
    } else {
        duk_push_boolean(ctx, registry.has<Comp>(entity));
    }

    return 1;
}

template<typename Comp>
duk_ret_t get(duk_context *ctx, entt::registry<> &registry) {
    [[maybe_unused]] const auto entity = duk_require_uint(ctx, 0);

    if constexpr(std::is_same_v<Comp, position>) {
        const auto &pos = registry.get<position>(entity);

        const auto idx = duk_push_object(ctx);

        duk_push_string(ctx, "x");
        duk_push_number(ctx, pos.x);
        duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE);

        duk_push_string(ctx, "y");
        duk_push_number(ctx, pos.y);
        duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE);
    } if constexpr(std::is_same_v<Comp, duktape_runtime>) {
        const auto type = duk_require_uint(ctx, 1);

        auto &runtime = registry.get<duktape_runtime>(entity);
        assert(runtime.components.find(type) != runtime.components.cend());

        duk_push_string(ctx, runtime.components[type].c_str());
        duk_json_decode(ctx, -1);
    } else {
        assert(registry.has<Comp>(entity));
        duk_push_object(ctx);
    }

    return 1;
}

class duktape_registry {
    // I'm pretty sure I won't have more than 99 components in the example
    static constexpr entt::registry<>::component_type udef = 100;

    struct func_map {
        using func_type = duk_ret_t(*)(duk_context *, entt::registry<> &);

        func_type set;
        func_type unset;
        func_type has;
        func_type get;
    };

    template<typename... Comp>
    void reg() {
        ((func[registry.type<Comp>()] = {
                &::set<Comp>,
                &::unset<Comp>,
                &::has<Comp>,
                &::get<Comp>
        }), ...);
    }

    static duktape_registry & instance(duk_context *ctx) {
        duk_push_this(ctx);

        duk_push_string(ctx, DUK_HIDDEN_SYMBOL("dreg"));
        duk_get_prop(ctx, -2);
        auto &dreg = *static_cast<duktape_registry *>(duk_require_pointer(ctx, -1));
        duk_pop_2(ctx);

        return dreg;
    }

    template<func_map::func_type func_map::*Op>
    static duk_ret_t invoke(duk_context *ctx) {
        auto &dreg = instance(ctx);
        auto &func = dreg.func;
        auto &registry = dreg.registry;
        auto type = duk_require_uint(ctx, 1);

        if(type >= udef) {
            type = registry.type<duktape_runtime>();
        }

        assert(func.find(type) != func.cend());

        return (func[type].*Op)(ctx, registry);
    }

public:
    duktape_registry(entt::registry<> &registry)
        : registry{registry}
    {
        reg<position, renderable, duktape_runtime>();
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
        return invoke<&func_map::set>(ctx);
    }

    static duk_ret_t unset(duk_context *ctx) {
        return invoke<&func_map::unset>(ctx);
    }

    static duk_ret_t has(duk_context *ctx) {
        return invoke<&func_map::has>(ctx);
    }

    static duk_ret_t get(duk_context *ctx) {
        return invoke<&func_map::get>(ctx);
    }

    static duk_ret_t entities(duk_context *ctx) {
        const duk_idx_t nargs = duk_get_top(ctx);
        auto &dreg = instance(ctx);
        duk_uarridx_t pos = 0;

        duk_push_array(ctx);

        std::vector<typename entt::registry<>::component_type> components;
        std::vector<typename entt::registry<>::component_type> runtime;

        for(duk_idx_t arg = 0; arg < nargs; arg++) {
            auto type = duk_require_uint(ctx, arg);

            if(type < udef) {
                components.push_back(type);
            } else {
                if(runtime.empty()) {
                    components.push_back(dreg.registry.type<duktape_runtime>());
                }

                runtime.push_back(type);
            }
        }

        auto view = dreg.registry.runtime_view(components.cbegin(), components.cend());

        for(const auto entity: view) {
            if(runtime.empty()) {
                duk_push_uint(ctx, entity);
                duk_put_prop_index(ctx, -2, pos++);
            } else {
                const auto &components = dreg.registry.get<duktape_runtime>(entity).components;
                const auto match = std::all_of(runtime.cbegin(), runtime.cend(), [&components](const auto type) {
                    return components.find(type) != components.cend();
                });

                if(match) {
                    duk_push_uint(ctx, entity);
                    duk_put_prop_index(ctx, -2, pos++);
                }
            }
        }

        return 1;
    }

private:
    std::map<duk_uint_t, func_map> func;
    entt::registry<> &registry;
};

const duk_function_list_entry js_duktape_registry_methods[] = {
    { "identifier", &duktape_registry::identifier, 0 },
    { "create", &duktape_registry::create, 0 },
    { "set", &duktape_registry::set, DUK_VARARGS },
    { "unset", &duktape_registry::unset, 2 },
    { "has", &duktape_registry::has, 2 },
    { "get", &duktape_registry::get, 2 },
    { "entities", &duktape_registry::entities, DUK_VARARGS },
    { nullptr, nullptr, 0 }
};

void export_types(duk_context *ctx, entt::registry<> &registry) {
    auto export_type = [](auto *ctx, auto &registry, auto idx, auto type, const auto *name) {
        duk_push_string(ctx, name);
        duk_push_uint(ctx, registry.template type<typename decltype(type)::type>());
        duk_def_prop(ctx, idx, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_WRITABLE);
    };

    auto idx = duk_push_object(ctx);

    export_type(ctx, registry, idx, tag<position>{}, "position");
    export_type(ctx, registry, idx, tag<renderable>{}, "renderable");

    duk_put_global_string(ctx, "Types");
}

void export_duktape_registry(duk_context *ctx, duktape_registry &dreg) {
    auto idx = duk_push_object(ctx);

    duk_push_string(ctx, DUK_HIDDEN_SYMBOL("dreg"));
    duk_push_pointer(ctx, &dreg);
    duk_put_prop(ctx, idx);

    duk_put_function_list(ctx, idx, js_duktape_registry_methods);
    duk_put_global_string(ctx, "Registry");
}

TEST(Mod, Duktape) {
    entt::registry<> registry;
    duktape_registry dreg{registry};
    duk_context *ctx = duk_create_heap_default();

    if(!ctx) {
        FAIL();
    }

    export_types(ctx, registry);
    export_duktape_registry(ctx, dreg);

    const char *s0 = ""
            "Types[\"PLAYING_CHARACTER\"] = Registry.identifier();"
            "Types[\"VELOCITY\"] = Registry.identifier();"
            "";

    if(duk_peval_string(ctx, s0)) {
        FAIL();
    }

    const auto e0 = registry.create();
    registry.assign<position>(e0, 0., 0.);
    registry.assign<renderable>(e0);

    const auto e1 = registry.create();
    registry.assign<position>(e1, 0., 0.);

    const char *s1 = ""
            "Registry.entities(Types.position, Types.renderable).forEach(function(entity) {"
                "Registry.set(entity, Types.position, 100., 100.);"
            "});"
            "var entity = Registry.create();"
            "Registry.set(entity, Types.position, 100., 100.);"
            "Registry.set(entity, Types.renderable);"
            "";

    if(duk_peval_string(ctx, s1)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<duktape_runtime>().size(), 0u);
    ASSERT_EQ(registry.view<position>().size(), 3u);
    ASSERT_EQ(registry.view<renderable>().size(), 2u);

    registry.view<position>().each([&registry](auto entity, const auto &position) {
        ASSERT_FALSE(registry.has<duktape_runtime>(entity));

        if(registry.has<renderable>(entity)) {
            ASSERT_EQ(position.x, 100.);
            ASSERT_EQ(position.y, 100.);
        } else {
            ASSERT_EQ(position.x, 0.);
            ASSERT_EQ(position.y, 0.);
        }
    });

    const char *s2 = ""
            "Registry.entities(Types.position).forEach(function(entity) {"
                "if(!Registry.has(entity, Types.renderable)) {"
                    "Registry.set(entity, Types.VELOCITY, { \"dx\": -100., \"dy\": -100. });"
                    "Registry.set(entity, Types.PLAYING_CHARACTER, {});"
                "}"
            "});"
            "";

    if(duk_peval_string(ctx, s2)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<duktape_runtime>().size(), 1u);
    ASSERT_EQ(registry.view<position>().size(), 3u);
    ASSERT_EQ(registry.view<renderable>().size(), 2u);

    registry.view<duktape_runtime>().each([](auto, const duktape_runtime &runtime) {
        ASSERT_EQ(runtime.components.size(), 2u);
    });

    const char *s3 = ""
            "Registry.entities(Types.position, Types.renderable, Types.VELOCITY, Types.PLAYING_CHARACTER).forEach(function(entity) {"
                "var velocity = Registry.get(entity, Types.VELOCITY);"
                "Registry.set(entity, Types.position, velocity.dx, velocity.dy)"
            "});"
            "";

    if(duk_peval_string(ctx, s3)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<duktape_runtime>().size(), 1u);
    ASSERT_EQ(registry.view<position>().size(), 3u);
    ASSERT_EQ(registry.view<renderable>().size(), 2u);

    registry.view<position, renderable, duktape_runtime>().each([](auto, const position &position, const auto &...) {
        ASSERT_EQ(position.x, -100.);
        ASSERT_EQ(position.y, -100.);
    });

    const char *s4 = ""
            "Registry.entities(Types.VELOCITY, Types.PLAYING_CHARACTER).forEach(function(entity) {"
                "Registry.unset(entity, Types.VELOCITY);"
                "Registry.unset(entity, Types.PLAYING_CHARACTER);"
            "});"
            "Registry.entities(Types.position).forEach(function(entity) {"
                "Registry.unset(entity, Types.position);"
            "});"
            "";

    if(duk_peval_string(ctx, s4)) {
        FAIL();
    }

    ASSERT_EQ(registry.view<duktape_runtime>().size(), 0u);
    ASSERT_EQ(registry.view<position>().size(), 0u);
    ASSERT_EQ(registry.view<renderable>().size(), 2u);

    duk_destroy_heap(ctx);
}
