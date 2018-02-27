#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/signal.hpp>

struct S {
    static void f(const int &j) { i = j; }
    void g(const int &j) { i = j; }
    void h(const int &) {}
    static int i;
};

int S::i = 0;

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
    struct S {
        void f() {}
        void g() {}
    };

    entt::Signal<void()> sig1;
    entt::Signal<void()> sig2;

    auto s1 = std::make_shared<S>();
    auto s2 = std::make_shared<S>();

    sig1.connect<S, &S::f>(s1);
    sig2.connect<S, &S::f>(s2);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.disconnect<S, &S::f>(s1);
    sig2.disconnect<S, &S::f>(s2);

    sig1.connect<S, &S::f>(s1);
    sig2.connect<S, &S::g>(s1);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.disconnect<S, &S::f>(s1);
    sig2.disconnect<S, &S::g>(s1);

    ASSERT_TRUE(sig1 == sig2);
    ASSERT_FALSE(sig1 != sig2);

    sig1.connect<S, &S::f>(s1);
    sig1.connect<S, &S::g>(s1);
    sig2.connect<S, &S::f>(s1);
    sig2.connect<S, &S::g>(s1);

    ASSERT_TRUE(sig1 == sig2);

    sig1.disconnect<S, &S::f>(s1);
    sig1.disconnect<S, &S::g>(s1);
    sig2.disconnect<S, &S::f>(s1);
    sig2.disconnect<S, &S::g>(s1);

    sig1.connect<S, &S::f>(s1);
    sig1.connect<S, &S::g>(s1);
    sig2.connect<S, &S::g>(s1);
    sig2.connect<S, &S::f>(s1);

    ASSERT_FALSE(sig1 == sig2);
}

TEST(Signal, Clear) {
    entt::Signal<void(const int &)> signal;
    signal.connect<&S::f>();

    ASSERT_FALSE(signal.empty());

    signal.clear();

    ASSERT_TRUE(signal.empty());
}

TEST(Signal, Swap) {
    entt::Signal<void(const int &)> sig1;
    entt::Signal<void(const int &)> sig2;

    sig1.connect<&S::f>();

    ASSERT_FALSE(sig1.empty());
    ASSERT_TRUE(sig2.empty());

    std::swap(sig1, sig2);

    ASSERT_TRUE(sig1.empty());
    ASSERT_FALSE(sig2.empty());
}

TEST(Signal, Functions) {
    entt::Signal<void(const int &)> signal;
    auto val = S::i + 1;

    signal.connect<&S::f>();
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{1}, signal.size());
    ASSERT_EQ(S::i, val);

    signal.disconnect<&S::f>();
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::i, val);
}

TEST(Signal, Members) {
    entt::Signal<void(const int &)> signal;
    auto ptr = std::make_shared<S>();
    auto val = S::i + 1;

    signal.connect<S, &S::g>(ptr);
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{1}, signal.size());
    ASSERT_EQ(S::i, val);

    signal.disconnect<S, &S::g>(ptr);
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::i, val);

    ++val;

    signal.connect<S, &S::g>(ptr);
    signal.connect<S, &S::h>(ptr);
    signal.publish(val);

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{2}, signal.size());
    ASSERT_EQ(S::i, val);

    signal.disconnect(ptr);
    signal.publish(val+1);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(entt::Signal<void(const int &)>::size_type{0}, signal.size());
    ASSERT_EQ(S::i, val);
}

TEST(Signal, Cleanup) {
    entt::Signal<void(const int &)> signal;
    auto ptr = std::make_shared<S>();
    signal.connect<S, &S::g>(ptr);
    auto val = S::i;
    ptr = nullptr;

    ASSERT_FALSE(signal.empty());
    ASSERT_EQ(S::i, val);

    signal.publish(val);

    ASSERT_TRUE(signal.empty());
    ASSERT_EQ(S::i, val);
}
