#include <gtest/gtest.h>
#include <entt/signal/delegate.hpp>

int delegateFunction(int i) {
    return i*i;
}

struct DelegateFunctor {
    int operator()(int i) {
        return i+i;
    }
};

struct ConstNonConstNoExcept {
    void f() { ++cnt; }
    void g() noexcept { ++cnt; }
    void h() const { ++cnt; }
    void i() const noexcept { ++cnt; }
    mutable int cnt{0};
};

TEST(Delegate, Functionalities) {
    entt::Delegate<int(int)> ffdel;
    entt::Delegate<int(int)> mfdel;
    DelegateFunctor functor;

    ASSERT_TRUE(ffdel.empty());
    ASSERT_TRUE(mfdel.empty());

    ffdel.connect<&delegateFunction>();
    mfdel.connect<DelegateFunctor, &DelegateFunctor::operator()>(&functor);

    ASSERT_FALSE(ffdel.empty());
    ASSERT_FALSE(mfdel.empty());

    ASSERT_EQ(ffdel(3), 9);
    ASSERT_EQ(mfdel(3), 6);

    ffdel.reset();
    mfdel.reset();

    ASSERT_TRUE(ffdel.empty());
    ASSERT_TRUE(mfdel.empty());
}

TEST(Delegate, Comparison) {
    entt::Delegate<int(int)> delegate;
    entt::Delegate<int(int)> def;
    delegate.connect<&delegateFunction>();

    ASSERT_EQ(def, entt::Delegate<int(int)>{});
    ASSERT_NE(def, delegate);

    ASSERT_TRUE(def == entt::Delegate<int(int)>{});
    ASSERT_TRUE (def != delegate);
}

TEST(Delegate, ConstNonConstNoExcept) {
    entt::Delegate<void()> delegate;
    ConstNonConstNoExcept functor;

    delegate.connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::f>(&functor);
    delegate();

    delegate.connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::g>(&functor);
    delegate();

    delegate.connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::h>(&functor);
    delegate();

    delegate.connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::i>(&functor);
    delegate();

    ASSERT_EQ(functor.cnt, 4);
}
