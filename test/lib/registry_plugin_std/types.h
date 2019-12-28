#ifndef ENTT_LIB_REGISTRY_PLUGIN_STD_TYPES_H
#define ENTT_LIB_REGISTRY_PLUGIN_STD_TYPES_H

struct position {
    int x;
    int y;
};

struct velocity {
    double dx;
    double dy;
};

struct registry_proxy {
    virtual ~registry_proxy() = default;
    virtual void for_each(void(*)(position &, velocity &)) = 0;
    virtual void assign(velocity) = 0;
};

#endif
