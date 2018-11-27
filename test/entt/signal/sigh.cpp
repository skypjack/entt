#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

struct sigh_listener {
    static void f(int &v) { v = 42; }

    bool g(int) { k = !k; return true; }
    bool h(const int &) { return k; }

    void i() {}
    void l() {}

    bool k{false};
};

template<typename Ret>
struct test_collect_all {
    std::vector<Ret> vec{};
    static int f() { return 42; }
    static int g() { return 42; }
    bool operator()(Ret r) noexcept {
        vec.push_back(r);
        return true;
    }
};

template<>
struct test_collect_all<void> {
    std::vector<int> vec{};
    static void h() {}
    bool operator()() noexcept {
        return true;
    }
};

template<typename Ret>
struct test_collect_first {
    std::vector<Ret> vec{};
    static int f() { return 42; }
    bool operator()(Ret r) noexcept {
        vec.push_back(r);
        return false;
    }
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

TEST(SigH, Comparison) {
    entt::sigh<void()> sig1;
    entt::sigh<void()> sig2;

    sigh_listener s1;
    sigh_listener s2;

    sig1.sink().connect<&sigh_listener::i>(&s1);
    sig2.sink().connect<&sigh_listener::i>(&s2);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<&sigh_listener::i>(&s1);
    sig2.sink().disconnect<&sigh_listener::i>(&s2);

    sig1.sink().connect<&sigh_listener::i>(&s1);
    sig2.sink().connect<&sigh_listener::l>(&s1);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<&sigh_listener::i>(&s1);
    sig2.sink().disconnect<&sigh_listener::l>(&s1);

    ASSERT_TRUE(sig1 == sig2);
    ASSERT_FALSE(sig1 != sig2);

    sig1.sink().connect<&sigh_listener::i>(&s1);
    sig1.sink().connect<&sigh_listener::l>(&s1);
    sig2.sink().connect<&sigh_listener::i>(&s1);
    sig2.sink().connect<&sigh_listener::l>(&s1);

    ASSERT_TRUE(sig1 == sig2);

    sig1.sink().disconnect<&sigh_listener::i>(&s1);
    sig1.sink().disconnect<&sigh_listener::l>(&s1);
    sig2.sink().disconnect<&sigh_listener::i>(&s1);
    sig2.sink().disconnect<&sigh_listener::l>(&s1);

    sig1.sink().connect<&sigh_listener::i>(&s1);
    sig1.sink().connect<&sigh_listener::l>(&s1);
    sig2.sink().connect<&sigh_listener::l>(&s1);
    sig2.sink().connect<&sigh_listener::i>(&s1);

    ASSERT_FALSE(sig1 == sig2);
}

TEST(SigH, Clear) {
    entt::sigh<void(int &)> sigh;
    sigh.sink().connect<&sigh_listener::f>();

    ASSERT_FALSE(sigh.empty());

    sigh.sink().disconnect();

    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, Swap) {
    entt::sigh<void(int &)> sigh1;
    entt::sigh<void(int &)> sigh2;

    sigh1.sink().connect<&sigh_listener::f>();

    ASSERT_FALSE(sigh1.empty());
    ASSERT_TRUE(sigh2.empty());

    std::swap(sigh1, sigh2);

    ASSERT_TRUE(sigh1.empty());
    ASSERT_FALSE(sigh2.empty());
}

TEST(SigH, Functions) {
    entt::sigh<void(int &)> sigh;
    int v = 0;

    sigh.sink().connect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(1), sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sigh.sink().disconnect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(0), sigh.size());
    ASSERT_EQ(0, v);

    sigh.sink().connect<&sigh_listener::f>();
}

TEST(SigH, Members) {
    sigh_listener s;
    sigh_listener *ptr = &s;
    entt::sigh<bool(int)> sigh;

    sigh.sink().connect<&sigh_listener::g>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.k);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(1), sigh.size());

    sigh.sink().disconnect<&sigh_listener::g>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.k);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(0), sigh.size());

    sigh.sink().connect<&sigh_listener::g>(ptr);
    sigh.sink().connect<&sigh_listener::h>(ptr);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(2), sigh.size());

    sigh.sink().disconnect(ptr);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::sigh<bool(int)>::size_type>(0), sigh.size());
}

TEST(SigH, Collector) {
    entt::sigh<void(), test_collect_all<void>> sigh_void;

    sigh_void.sink().connect<&test_collect_all<void>::h>();
    auto collector_void = sigh_void.collect();

    ASSERT_FALSE(sigh_void.empty());
    ASSERT_TRUE(collector_void.vec.empty());

    entt::sigh<int(), test_collect_all<int>> sigh_all;

    sigh_all.sink().connect<&test_collect_all<int>::f>();
    sigh_all.sink().connect<&test_collect_all<int>::f>();
    sigh_all.sink().connect<&test_collect_all<int>::g>();
    auto collector_all = sigh_all.collect();

    ASSERT_FALSE(sigh_all.empty());
    ASSERT_FALSE(collector_all.vec.empty());
    ASSERT_EQ(static_cast<std::vector<int>::size_type>(2), collector_all.vec.size());
    ASSERT_EQ(42, collector_all.vec[0]);
    ASSERT_EQ(42, collector_all.vec[1]);

    entt::sigh<int(), test_collect_first<int>> sigh_first;

    sigh_first.sink().connect<&test_collect_first<int>::f>();
    sigh_first.sink().connect<&test_collect_first<int>::f>();
    auto collector_first = sigh_first.collect();

    ASSERT_FALSE(sigh_first.empty());
    ASSERT_FALSE(collector_first.vec.empty());
    ASSERT_EQ(static_cast<std::vector<int>::size_type>(1), collector_first.vec.size());
    ASSERT_EQ(42, collector_first.vec[0]);
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
