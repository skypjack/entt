#ifndef ENTT_COMMON_BASIC_TEST_ALLOCATOR_HPP
#define ENTT_COMMON_BASIC_TEST_ALLOCATOR_HPP

#include <memory>
#include <type_traits>

namespace test {

template<typename Type>
struct basic_test_allocator: std::allocator<Type> {
    // basic pocca/pocma/pocs allocator

    using base = std::allocator<Type>;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    using std::allocator<Type>::allocator;

    basic_test_allocator &operator=(const basic_test_allocator &other) {
        // necessary to avoid call suppression
        base::operator=(other);
        return *this;
    }
};

} // namespace test

#endif
