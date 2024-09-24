#include <gtest/gtest.h>
template<typename Type>
struct ReactiveMixin: testing::Test {
    using type = Type;
};

template<typename Type>
using ReactiveMixinDeathTest = ReactiveMixin<Type>;

using ReactiveMixinTypes = ::testing::Types<void, bool>;

TYPED_TEST_SUITE(ReactiveMixin, ReactiveMixinTypes, );
TYPED_TEST_SUITE(ReactiveMixinDeathTest, ReactiveMixinTypes, );

