#ifndef ENTT_DAVEY_META_HPP
#define ENTT_DAVEY_META_HPP

namespace entt {

struct davey_data {
    davey_data(const char *value)
        : name{value} {}

    const char *name;
};

} // namespace entt

#endif