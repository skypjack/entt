#ifndef ENTT_COMMON_NEW_DELETE_H
#define ENTT_COMMON_NEW_DELETE_H

#include <cstddef>

namespace test {

struct new_delete {
    static void *operator new(std::size_t count) {
        return ::operator new(count);
    }

    static void operator delete(void *ptr) {
        ::operator delete(ptr);
    }

    int value{};
};

} // namespace test

#endif
