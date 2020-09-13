#ifndef ENTT_LIB_EMITTER_PLUGIN_TYPE_CONTEXT_H
#define ENTT_LIB_EMITTER_PLUGIN_TYPE_CONTEXT_H

#include <unordered_map>
#include <entt/core/fwd.hpp>

class type_context {
    type_context() = default;

public:
    inline entt::id_type value(const entt::id_type name) {
        if(name_to_index.find(name) == name_to_index.cend()) {
            name_to_index[name] = entt::id_type(name_to_index.size());
        }

        return name_to_index[name];
    }

    static type_context * instance() {
        static type_context self{};
        return &self;
    }

private:
    std::unordered_map<entt::id_type, entt::id_type> name_to_index{};
};

#endif
