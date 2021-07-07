
#ifndef ENTT_API_IMPORT
#error "ENTT_API_IMPORT must be defined when using entt as a shared library"
#endif

#include "api.h"

#include <gtest/gtest.h>

struct zero_component {};

/**
 * This test will fail if entt::internal::type_seq::next() was not linked
 * (imported/exported from shared registry). Two different versions will be used
 * by template specializations of entt::type_seq.
 *
 * This test will not fail (for a wrong reason) if linker has not imported or
 * optimized out imported entt symbols (will fail without optimization). Thus
 * both type ids will use ent::internal::type_seq::next::value generated within
 * the executable.
 */
TEST(shared_registry_lib, compare_ids_within_core) {
  EXPECT_EQ(entt::type_seq<zero_component>(), 0);
  ASSERT_EQ(entt::type_seq<test_increment>(), 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}