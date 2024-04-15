#include <gtest/gtest.h>
#include <entt/entity/table.hpp>

TEST(Table, Placeholder) {
    entt::table<int, char> table{};
}
