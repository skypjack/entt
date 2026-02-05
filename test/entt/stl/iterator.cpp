#include <gtest/gtest.h>
#include <entt/entity/sparse_set.hpp>
#include <entt/stl/iterator.hpp>

TEST(Iterator, Concepts) {
    using iterator = typename entt::sparse_set::iterator;

    ASSERT_TRUE(entt::stl::bidirectional_iterator<iterator>);
    ASSERT_TRUE(entt::stl::forward_iterator<iterator>);
    ASSERT_TRUE(entt::stl::input_iterator<iterator>);
    ASSERT_TRUE(entt::stl::input_or_output_iterator<iterator>);
    ASSERT_FALSE((entt::stl::output_iterator<iterator, entt::entity>));
    ASSERT_TRUE(entt::stl::random_access_iterator<iterator>);
    ASSERT_TRUE((entt::stl::sentinel_for<iterator, iterator>));
}
