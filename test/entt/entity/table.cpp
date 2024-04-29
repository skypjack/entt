#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>
#include <entt/entity/table.hpp>
#include "../../common/linter.hpp"

TEST(Table, Constructors) {
    entt::table<int, char> table;

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = table.get_allocator());

    table = entt::table<int, char>{std::allocator<void>{}};

    ASSERT_NO_THROW([[maybe_unused]] auto alloc = table.get_allocator());
}
