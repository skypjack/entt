#ifndef ENTT_COMMON_CONFIG_H
#define ENTT_COMMON_CONFIG_H

namespace test {

#ifdef NDEBUG
#    define ENTT_DEBUG_TEST(Case, Test) TEST(Case, DISABLED_##Test)
#    define ENTT_DEBUG_TEST_P(Case, Test) TEST_P(Case, DISABLED_##Test)
#    define ENTT_DEBUG_TEST_F(Case, Test) TEST_F(Case, DISABLED_##Test)
#    define ENTT_DEBUG_TYPED_TEST(Case, Test) TYPED_TEST(Case, DISABLED_##Test)
#else
#    define ENTT_DEBUG_TEST(Case, Test) TEST(Case, Test)
#    define ENTT_DEBUG_TEST_P(Case, Test) TEST_P(Case, Test)
#    define ENTT_DEBUG_TEST_F(Case, Test) TEST_F(Case, Test)
#    define ENTT_DEBUG_TYPED_TEST(Case, Test) TYPED_TEST(Case, Test)
#endif

} // namespace test

#endif
