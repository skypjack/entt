#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/signal.hpp>

struct S {
    static void f(const int &j) { k = j; }

    void g() {}
    void h() {}

    void i(const int &j) { k = j; }
    void l(const int &) {}

    static int k;
};

int S::k = 0;

TEST(Signal, Lifetime) {
    using signal = entt::Signal<void(void)>;

    ASSERT_NO_THROW(signal{});

    signal src{}, other{};

    ASSERT_NO_THROW(signal{src});
    ASSERT_NO_THROW(signal{std::move(other)});
    ASSERT_NO_THROW(src = other);
    ASSERT_NO_THROW(src = std::move(other));

    ASSERT_NO_THROW(delete new signal{});
}

TEST(Signal, Comparison) {
    entt::Signal<void()> sig1;
    entt::Signal<void()> sig2;

    auto s1 = std::make_shared<S>();
    auto s2 = std::make_shared<S>();

    sig1.sink().connect<S, &S::g>(s1);
    sig2.sink().connect<S, &S::g>(s2);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<S, &S::g>(s1);
    sig2.sink().disconnect<S, &S::g>(s2);

    sig1.sink().connect<S, &S::g>(s1);
    sig2.sink().connect<S, &S::h>(s1);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<S, &S::g>(s1);
    sig2.sink().disconnect<S, &S::h>(s1);

    ASSERT_TRUE(sig1 == sig2);
    ASSERT_FALSE(sig1 != sig2);

    sig1.sink().connect<S, &S::g>(s1);
    sig1.sink().connect<S, &S::h>(s1);
    sig2.sink().connect<S, &S::g>(s1);
    sig2.sink().connect<S, &S::h>(s1);

    ASSERT_TRUE(sig1 == sig2);

    sig1.sink().disconnect<S, &S::g>(s1);
    sig1.sink().disconnect<S, &S::h>(s1);
    sig2.sink().disconnect<S, &S::g>(s1);
    sig2.sink().disconnect<S, &S::h>(s1);

    sig1.sink().connect<S, &S::g>(s1);
    sig1.sink().connect<S, &S::h>(s1);
    sig2.sink().connect<S, &S::h>(s1);
    sig2.sink().connect<S, &S::g>(s1);

    ASSERT_FALSE(sig1 == sig2);
}

TEST(Signal, Clear) {
    entt::Signal<void(const int &)> signal;
    signal.sink().connect<&S::f>();

    ASSERT_FALSE(signal.empty());

    signal.sink().disconnect();

    ASSERT_TRUE(signal.empty());
}

TEST(Signal, Swap) {
    entt::Signal<void(const int &)> sig1;
    entt::Signal<void(const int &)> sig2;

    sig1.sink().connect<&S::f>();

    ASSERT_FALSE(sig1.empty());
    ASSERT_TRUE(sig2.empty());

    std::swap(sig1, sig2);

    ASSERT_TRUE(sig1.empty());
    ASSERT_FALSE(sig2.empty());
}

TEST(Signal, Functions) {
    entt::Signal<void(const int &)> signal;
    auto val = (S::k = 0) + 1;

    signal.sink().connect<&S::f>();
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{1}, signal.size());
    ASSERT_EQ(S::k, val);

    signal.sink().disconnect<&S::f>();
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::k, val);
}

TEST(Signal, Members) {
    entt::Signal<void(const int &)> signal;
    auto ptr = std::make_shared<S>();
    auto val = (S::k = 0) + 1;

    signal.sink().connect<S, &S::i>(ptr);
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{1}, signal.size());
    ASSERT_EQ(S::k, val);

    signal.sink().disconnect<S, &S::i>(ptr);
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::k, val);

    ++val;

    signal.sink().connect<S, &S::i>(ptr);
    signal.sink().connect<S, &S::l>(ptr);
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{2}, signal.size());
    ASSERT_EQ(S::k, val);

    signal.sink().disconnect(ptr);
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::k, val);
}

TEST(Signal, Cleanup) {
    entt::Signal<void(const int &)> signal;
    auto ptr = std::make_shared<S>();
    signal.sink().connect<S, &S::i>(ptr);
    auto val = (S::k = 0);
    ptr = nullptr;

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(S::k, val);

    signal.publish(val);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(S::k, val);
}
