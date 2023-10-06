#ifndef ENTT_COMMON_BASIC_TEST_ALLOCATOR_HPP
#define ENTT_COMMON_BASIC_TEST_ALLOCATOR_HPP

#include <memory>
#include <type_traits>

namespace test {

template<typename Type, typename Pocs = std::true_type>
struct basic_test_allocator: std::allocator<Type> {
    // basic pocca/pocma/pocs allocator

    using base = std::allocator<Type>;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_swap = Pocs;

    using std::allocator<Type>::allocator;

    // necessary to avoid a warning by clang-cl :)
    basic_test_allocator(const basic_test_allocator &other)
        : base{other} {
    }

    basic_test_allocator &operator=(const basic_test_allocator &other) {
        // necessary to avoid call suppression
        base::operator=(other);
        return *this;
    }

    bool operator==(const basic_test_allocator &other) const {
        return (this == &other);
    }
};

} // namespace test

#endif
