#include <gtest/gtest.h>
#include <entt/signal/delegate.hpp>

int f(int i) {
    return i*i;
}

struct S {
    int f(int i) {
        return i+i;
    }
};

TEST(Delegate, Functionalities) {
    entt::Delegate<int(int)> ffdel;
    entt::Delegate<int(int)> mfdel;
    S test;

    ASSERT_EQ(ffdel(42), int{});
    ASSERT_EQ(mfdel(42), int{});

    ffdel.connect<&f>();
    mfdel.connect<S, &S::f>(&test);

    ASSERT_EQ(ffdel(3), 9);
    ASSERT_EQ(mfdel(3), 6);

    ffdel.reset();
    mfdel.reset();

    ASSERT_EQ(ffdel(42), int{});
    ASSERT_EQ(mfdel(42), int{});
}

TEST(Delegate, Comparison) {
    entt::Delegate<int(int)> delegate;
    entt::Delegate<int(int)> def;
    delegate.connect<&f>();

    ASSERT_EQ(def, entt::Delegate<int(int)>{});
    ASSERT_NE(def, delegate);

    ASSERT_TRUE(def == entt::Delegate<int(int)>{});
    ASSERT_TRUE (def != delegate);
}
