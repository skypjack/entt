#ifndef ENTT_COMMON_THROWING_ALLOCATOR_HPP
#define ENTT_COMMON_THROWING_ALLOCATOR_HPP

#include <cstddef>
#include <memory>
#include <type_traits>

namespace test {

template<typename Type>
class throwing_allocator: std::allocator<Type> {
    template<typename Other>
    friend class throwing_allocator;

    using base = std::allocator<Type>;
    struct test_exception {};

public:
    using value_type = Type;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using void_pointer = void *;
    using const_void_pointer = const void *;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using exception_type = test_exception;

    template<class Other>
    struct rebind {
        using other = throwing_allocator<Other>;
    };

    throwing_allocator() = default;

    template<class Other>
    throwing_allocator(const throwing_allocator<Other> &other)
        : base{other} {}

    pointer allocate(std::size_t length) {
        if(trigger_on_allocate) {
            trigger_on_allocate = false;
            throw test_exception{};
        }

        trigger_on_allocate = trigger_after_allocate;
        trigger_after_allocate = false;

        return base::allocate(length);
    }

    void deallocate(pointer mem, std::size_t length) {
        base::deallocate(mem, length);
    }

    bool operator==(const throwing_allocator<Type> &) const {
        return true;
    }

    bool operator!=(const throwing_allocator<Type> &other) const {
        return !(*this == other);
    }

    static inline bool trigger_on_allocate{};
    static inline bool trigger_after_allocate{};
};

} // namespace test

#endif
