#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

struct sigh_listener {
    static void f(int &v) { v = 42; }

    bool g(int) { k = !k; return true; }
    bool h(const int &) { return k; }

    void i() {}
    // useless definition just because msvc does weird things if both are empty
    void l() { k = k*k; }

    bool k{false};
};

struct const_nonconst_noexcept {
    void f() { ++cnt; }
    void g() noexcept { ++cnt; }
    void h() const { ++cnt; }
    void i() const noexcept { ++cnt; }
    mutable int cnt{0};
};

TEST(SigH, Lifetime) {
    using signal = entt::sigh<void(void)>;

    ASSERT_NO_THROW(signal{});

    signal src{}, other{};

    ASSERT_NO_THROW(signal{src});
    ASSERT_NO_THROW(signal{std::move(other)});
    ASSERT_NO_THROW(src = other);
    ASSERT_NO_THROW(src = std::move(other));

    ASSERT_NO_THROW(delete new signal{});
}

TEST(SigH, Clear) {
    entt::sigh<void(int &)> sigh;
    sigh.sink().connect<&sigh_listener::f>();

    ASSERT_FALSE(sigh.sink().empty());
    ASSERT_FALSE(sigh.empty());

    sigh.sink().disconnect();

    ASSERT_TRUE(sigh.sink().empty());
    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, Swap) {
    entt::sigh<void(int &)> sigh1;
    entt::sigh<void(int &)> sigh2;

    sigh1.sink().connect<&sigh_listener::f>();

    ASSERT_FALSE(sigh1.sink().empty());
    ASSERT_TRUE(sigh2.sink().empty());

    ASSERT_FALSE(sigh1.empty());
    ASSERT_TRUE(sigh2.empty());

    std::swap(sigh1, sigh2);

    ASSERT_TRUE(sigh1.sink().empty());
    ASSERT_FALSE(sigh2.sink().empty());

    ASSERT_TRUE(sigh1.empty());
    ASSERT_FALSE(sigh2.empty());
}

TEST(SigH, Functions) {
    entt::sigh<void(int &)> sigh;
    int v = 0;

    sigh.sink().connect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<void(int &)>::size_type>(1), sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sigh.sink().disconnect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<void(int &)>::size_type>(0), sigh.size());
    ASSERT_EQ(v, 0);

    sigh.sink().connect<&sigh_listener::f>();

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<void(int &)>::size_type>(1), sigh.size());

    sigh.sink().disconnect(nullptr);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<void(int &)>::size_type>(0), sigh.size());}

TEST(SigH, Members) {
    sigh_listener l1, l2;
    entt::sigh<bool(int)> sigh;

    sigh.sink().connect<&sigh_listener::g>(&l1);
    sigh.publish(42);

    ASSERT_TRUE(l1.k);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(1), sigh.size());

    sigh.sink().disconnect<&sigh_listener::g>(&l1);
    sigh.publish(42);

    ASSERT_TRUE(l1.k);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(0), sigh.size());

    sigh.sink().connect<&sigh_listener::g>(&l1);
    sigh.sink().connect<&sigh_listener::h>(&l2);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(2), sigh.size());

    sigh.sink().disconnect(&l1);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(1), sigh.size());
}

TEST(SigH, Collector) {
    sigh_listener listener;
    entt::sigh<bool(int)> sigh;
    int cnt = 0;

    sigh.sink().connect<&sigh_listener::g>(&listener);
    sigh.sink().connect<&sigh_listener::h>(&listener);

    listener.k = true;
    sigh.collect([&listener, &cnt](bool value) {
        ASSERT_TRUE(value);
        listener.k = true;
        ++cnt;
    }, 42);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    cnt = 0;
    sigh.collect([&cnt](bool value) {
        // gtest and its macro hell are sometimes really annoying...
        [](auto v) { ASSERT_TRUE(v); }(value);
        ++cnt;
        return true;
    }, 42);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, CollectorVoid) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    int cnt = 0;

    sigh.sink().connect<&sigh_listener::g>(&listener);
    sigh.sink().connect<&sigh_listener::h>(&listener);
    sigh.collect([&cnt]() { ++cnt; }, 42);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    cnt = 0;
    sigh.collect([&cnt]() {
        ++cnt;
        return true;
    }, 42);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, ConstNonConstNoExcept) {
    entt::sigh<void()> sigh;
    const_nonconst_noexcept functor;
    const const_nonconst_noexcept cfunctor;

    sigh.sink().connect<&const_nonconst_noexcept::f>(&functor);
    sigh.sink().connect<&const_nonconst_noexcept::g>(&functor);
    sigh.sink().connect<&const_nonconst_noexcept::h>(&cfunctor);
    sigh.sink().connect<&const_nonconst_noexcept::i>(&cfunctor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 2);
    ASSERT_EQ(cfunctor.cnt, 2);

    sigh.sink().disconnect<&const_nonconst_noexcept::f>(&functor);
    sigh.sink().disconnect<&const_nonconst_noexcept::g>(&functor);
    sigh.sink().disconnect<&const_nonconst_noexcept::h>(&cfunctor);
    sigh.sink().disconnect<&const_nonconst_noexcept::i>(&cfunctor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 2);
    ASSERT_EQ(cfunctor.cnt, 2);
}
