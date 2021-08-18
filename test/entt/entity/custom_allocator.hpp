#ifndef ENTT_ENTITY_CUSTOM_ALLOCATOR_HPP
#define ENTT_ENTITY_CUSTOM_ALLOCATOR_HPP


#include <cstddef>
#include <memory>
#include <type_traits>


namespace test {


template<typename Type>
class custom_allocator: std::allocator<Type> {
    template<typename Other>
    friend class custom_allocator;

    using base = std::allocator<Type>;

public:
    using value_type = Type;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using void_pointer = void *;
    using const_void_pointer = const void *;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    custom_allocator() = default;

    template<class Other>
    custom_allocator(const custom_allocator<Other> &other)
        : base{static_cast<const std::allocator<Other> &>(other)}
    {}

    pointer allocate(std::size_t length) {
        return base::allocate(length);
    }

    void deallocate(pointer mem, std::size_t length) {
        base::deallocate(mem, length);
    }

    bool operator==(const custom_allocator<Type> &) const {
        return true;
    }

    bool operator!=(const custom_allocator<Type> &) const {
        return false;
    }
};


}


#endif
