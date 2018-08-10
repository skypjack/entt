#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

struct SigHListener {
    static void f(int &v) { v = 42; }

    bool g(int) { k = !k; return true; }
    bool h(int) { return k; }

    void i() {}
    void l() {}

    bool k{false};
};

template<typename Ret>
struct TestCollectAll {
    std::vector<Ret> vec{};
    static int f() { return 42; }
    static int g() { return 42; }
    bool operator()(Ret r) noexcept {
        vec.push_back(r);
        return true;
    }
};

template<>
struct TestCollectAll<void> {
    std::vector<int> vec{};
    static void h() {}
    bool operator()() noexcept {
        return true;
    }
};

template<typename Ret>
struct TestCollectFirst {
    std::vector<Ret> vec{};
    static int f() { return 42; }
    bool operator()(Ret r) noexcept {
        vec.push_back(r);
        return false;
    }
};

struct ConstNonConstNoExcept {
    void f() { ++cnt; }
    void g() noexcept { ++cnt; }
    void h() const { ++cnt; }
    void i() const noexcept { ++cnt; }
    mutable int cnt{0};
};

TEST(SigH, Lifetime) {
    using signal = entt::SigH<void(void)>;

    ASSERT_NO_THROW(signal{});

    signal src{}, other{};

    ASSERT_NO_THROW(signal{src});
    ASSERT_NO_THROW(signal{std::move(other)});
    ASSERT_NO_THROW(src = other);
    ASSERT_NO_THROW(src = std::move(other));

    ASSERT_NO_THROW(delete new signal{});
}

TEST(SigH, Comparison) {
    entt::SigH<void()> sig1;
    entt::SigH<void()> sig2;

    SigHListener s1;
    SigHListener s2;

    sig1.sink().connect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::i>(&s2);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().disconnect<SigHListener, &SigHListener::i>(&s2);

    sig1.sink().connect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::l>(&s1);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.sink().disconnect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().disconnect<SigHListener, &SigHListener::l>(&s1);

    ASSERT_TRUE(sig1 == sig2);
    ASSERT_FALSE(sig1 != sig2);

    sig1.sink().connect<SigHListener, &SigHListener::i>(&s1);
    sig1.sink().connect<SigHListener, &SigHListener::l>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::l>(&s1);

    ASSERT_TRUE(sig1 == sig2);

    sig1.sink().disconnect<SigHListener, &SigHListener::i>(&s1);
    sig1.sink().disconnect<SigHListener, &SigHListener::l>(&s1);
    sig2.sink().disconnect<SigHListener, &SigHListener::i>(&s1);
    sig2.sink().disconnect<SigHListener, &SigHListener::l>(&s1);

    sig1.sink().connect<SigHListener, &SigHListener::i>(&s1);
    sig1.sink().connect<SigHListener, &SigHListener::l>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::l>(&s1);
    sig2.sink().connect<SigHListener, &SigHListener::i>(&s1);

    ASSERT_FALSE(sig1 == sig2);
}

TEST(SigH, Clear) {
    entt::SigH<void(int &)> sigh;
    sigh.sink().connect<&SigHListener::f>();

    ASSERT_FALSE(sigh.empty());

    sigh.sink().disconnect();

    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, Swap) {
    entt::SigH<void(int &)> sigh1;
    entt::SigH<void(int &)> sigh2;

    sigh1.sink().connect<&SigHListener::f>();

    ASSERT_FALSE(sigh1.empty());
    ASSERT_TRUE(sigh2.empty());

    std::swap(sigh1, sigh2);

    ASSERT_TRUE(sigh1.empty());
    ASSERT_FALSE(sigh2.empty());
}

TEST(SigH, Functions) {
    entt::SigH<void(int &)> sigh;
    int v = 0;

    sigh.sink().connect<&SigHListener::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(1), sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sigh.sink().disconnect<&SigHListener::f>();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(0), sigh.size());
    ASSERT_EQ(0, v);

    sigh.sink().connect<&SigHListener::f>();
}

TEST(SigH, Members) {
    SigHListener s;
    SigHListener *ptr = &s;
    entt::SigH<bool(int)> sigh;

    sigh.sink().connect<SigHListener, &SigHListener::g>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.k);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(1), sigh.size());

    sigh.sink().disconnect<SigHListener, &SigHListener::g>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.k);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(0), sigh.size());

    sigh.sink().connect<SigHListener, &SigHListener::g>(ptr);
    sigh.sink().connect<SigHListener, &SigHListener::h>(ptr);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(2), sigh.size());

    sigh.sink().disconnect(ptr);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(static_cast<entt::SigH<bool(int)>::size_type>(0), sigh.size());
}

TEST(SigH, Collector) {
    entt::SigH<void(), TestCollectAll<void>> sigh_void;

    sigh_void.sink().connect<&TestCollectAll<void>::h>();
    auto collector_void = sigh_void.collect();

    ASSERT_FALSE(sigh_void.empty());
    ASSERT_TRUE(collector_void.vec.empty());

    entt::SigH<int(), TestCollectAll<int>> sigh_all;

    sigh_all.sink().connect<&TestCollectAll<int>::f>();
    sigh_all.sink().connect<&TestCollectAll<int>::f>();
    sigh_all.sink().connect<&TestCollectAll<int>::g>();
    auto collector_all = sigh_all.collect();

    ASSERT_FALSE(sigh_all.empty());
    ASSERT_FALSE(collector_all.vec.empty());
    ASSERT_EQ(static_cast<std::vector<int>::size_type>(2), collector_all.vec.size());
    ASSERT_EQ(42, collector_all.vec[0]);
    ASSERT_EQ(42, collector_all.vec[1]);

    entt::SigH<int(), TestCollectFirst<int>> sigh_first;

    sigh_first.sink().connect<&TestCollectFirst<int>::f>();
    sigh_first.sink().connect<&TestCollectFirst<int>::f>();
    auto collector_first = sigh_first.collect();

    ASSERT_FALSE(sigh_first.empty());
    ASSERT_FALSE(collector_first.vec.empty());
    ASSERT_EQ(static_cast<std::vector<int>::size_type>(1), collector_first.vec.size());
    ASSERT_EQ(42, collector_first.vec[0]);
}

TEST(SigH, ConstNonConstNoExcept) {
    entt::SigH<void()> sigh;
    ConstNonConstNoExcept functor;

    sigh.sink().connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::f>(&functor);
    sigh.sink().connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::g>(&functor);
    sigh.sink().connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::h>(&functor);
    sigh.sink().connect<ConstNonConstNoExcept, &ConstNonConstNoExcept::i>(&functor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 4);

    sigh.sink().disconnect<ConstNonConstNoExcept, &ConstNonConstNoExcept::f>(&functor);
    sigh.sink().disconnect<ConstNonConstNoExcept, &ConstNonConstNoExcept::g>(&functor);
    sigh.sink().disconnect<ConstNonConstNoExcept, &ConstNonConstNoExcept::h>(&functor);
    sigh.sink().disconnect<ConstNonConstNoExcept, &ConstNonConstNoExcept::i>(&functor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 4);
}
