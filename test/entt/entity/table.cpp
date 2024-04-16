#include <gtest/gtest.h>
#include <entt/entity/table.hpp>

TEST(Table, Placeholder) {
    [[maybe_unused]] entt::table<int, char> table{};
}
