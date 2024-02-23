#ifndef ENTT_COMMON_LISTENER_H
#define ENTT_COMMON_LISTENER_H

namespace test {

template<typename Type>
struct listener {
    void on(Type elem) {
        value = elem;
    }

    int value{};
};

} // namespace test

#endif
