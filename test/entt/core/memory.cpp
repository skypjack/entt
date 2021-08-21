#include <memory>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>

struct test_allocator: std::allocator<int> {
    using base = std::allocator<int>;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    using std::allocator<int>::allocator;

    test_allocator & operator=(const test_allocator &other) {
        // necessary to avoid call suppression
        base::operator=(other);
        return *this;
    }
};

TEST(Memory, ToAddress) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

TEST(Memory, PoccaPocmaAndPocs) {
    test_allocator lhs, rhs;
    // honestly, I don't even know how one is supposed to test such a thing :)
    entt::propagate_on_container_copy_assignment(lhs, rhs);
    entt::propagate_on_container_move_assignment(lhs, rhs);
    entt::propagate_on_container_swap(lhs, rhs);
}
