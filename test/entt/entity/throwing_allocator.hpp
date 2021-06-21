#ifndef ENTT_ENTITY_THROWING_ALLOCATOR_HPP
#define ENTT_ENTITY_THROWING_ALLOCATOR_HPP


#include <cstddef>
#include <memory>
#include <type_traits>


namespace test {


template<typename Type>
class throwing_allocator {
    template<typename Other>
    friend class throwing_allocator;

    struct test_exception {};

public:
    using value_type = Type;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using void_pointer = void *;
    using const_void_pointer = const void *;
    using propagate_on_container_move_assignment = std::true_type;
    using exception_type = test_exception;

    throwing_allocator() = default;

    template<class Other>
    throwing_allocator(const throwing_allocator<Other> &other)
        : allocator{other.allocator}
    {}

    pointer allocate(std::size_t length) {
        if(trigger_on_allocate) {
            trigger_on_allocate = false;
            throw test_exception{};
        }

        trigger_on_allocate = trigger_after_allocate;
        trigger_after_allocate = false;

        return allocator.allocate(length);
    }

    void deallocate(pointer mem, std::size_t length) {
        allocator.deallocate(mem, length);
    }

    static inline bool trigger_on_allocate{};
    static inline bool trigger_after_allocate{};

private:
    std::allocator<Type> allocator;
};


}


#endif
