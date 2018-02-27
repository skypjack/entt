#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

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
    struct S {
        void f() {}
        void g() {}
    };

    entt::SigH<void()> sig1;
    entt::SigH<void()> sig2;

    S s1;
    S s2;

    sig1.connect<S, &S::f>(&s1);
    sig2.connect<S, &S::f>(&s2);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.disconnect<S, &S::f>(&s1);
    sig2.disconnect<S, &S::f>(&s2);

    sig1.connect<S, &S::f>(&s1);
    sig2.connect<S, &S::g>(&s1);

    ASSERT_FALSE(sig1 == sig2);
    ASSERT_TRUE(sig1 != sig2);

    sig1.disconnect<S, &S::f>(&s1);
    sig2.disconnect<S, &S::g>(&s1);

    ASSERT_TRUE(sig1 == sig2);
    ASSERT_FALSE(sig1 != sig2);

    sig1.connect<S, &S::f>(&s1);
    sig1.connect<S, &S::g>(&s1);
    sig2.connect<S, &S::f>(&s1);
    sig2.connect<S, &S::g>(&s1);

    ASSERT_TRUE(sig1 == sig2);

    sig1.disconnect<S, &S::f>(&s1);
    sig1.disconnect<S, &S::g>(&s1);
    sig2.disconnect<S, &S::f>(&s1);
    sig2.disconnect<S, &S::g>(&s1);

    sig1.connect<S, &S::f>(&s1);
    sig1.connect<S, &S::g>(&s1);
    sig2.connect<S, &S::g>(&s1);
    sig2.connect<S, &S::f>(&s1);

    ASSERT_FALSE(sig1 == sig2);
}

struct S {
    static void f(int &v) { v = 42; }
};

TEST(SigH, Clear) {
    entt::SigH<void(int &)> sigh;
    sigh.connect<&S::f>();

    ASSERT_FALSE(sigh.empty());

    sigh.clear();

    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, Swap) {
    entt::SigH<void(int &)> sigh1;
    entt::SigH<void(int &)> sigh2;

    sigh1.connect<&S::f>();

    ASSERT_FALSE(sigh1.empty());
    ASSERT_TRUE(sigh2.empty());

    std::swap(sigh1, sigh2);

    ASSERT_TRUE(sigh1.empty());
    ASSERT_FALSE(sigh2.empty());
}

TEST(SigH, Functions) {
    entt::SigH<void(int &)> sigh;
    int v = 0;

    sigh.connect<&S::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)1, sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sigh.disconnect<&S::f>();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)0, sigh.size());
    ASSERT_EQ(0, v);

    sigh.connect<&S::f>();
}

TEST(SigH, Members) {
    struct S {
        bool f(int) { b = !b; return true; }
        bool g(int) { return b; }
        bool b{false};
    };

    S s;
    S *ptr = &s;
    entt::SigH<bool(int)> sigh;

    sigh.connect<S, &S::f>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.b);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)1, sigh.size());

    sigh.disconnect<S, &S::f>(ptr);
    sigh.publish(42);

    ASSERT_TRUE(s.b);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)0, sigh.size());

    sigh.connect<S, &S::f>(ptr);
    sigh.connect<S, &S::g>(ptr);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)2, sigh.size());

    sigh.disconnect(ptr);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ((entt::SigH<bool(int)>::size_type)0, sigh.size());
}

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

TEST(SigH, Collector) {
    entt::SigH<void(), TestCollectAll<void>> sigh_void;

    sigh_void.connect<&TestCollectAll<void>::h>();
    auto collector_void = sigh_void.collect();

    ASSERT_FALSE(sigh_void.empty());
    ASSERT_TRUE(collector_void.vec.empty());

    entt::SigH<int(), TestCollectAll<int>> sigh_all;

    sigh_all.connect<&TestCollectAll<int>::f>();
    sigh_all.connect<&TestCollectAll<int>::f>();
    sigh_all.connect<&TestCollectAll<int>::g>();
    auto collector_all = sigh_all.collect();

    ASSERT_FALSE(sigh_all.empty());
    ASSERT_FALSE(collector_all.vec.empty());
    ASSERT_EQ((std::vector<int>::size_type)2, collector_all.vec.size());
    ASSERT_EQ(42, collector_all.vec[0]);
    ASSERT_EQ(42, collector_all.vec[1]);

    entt::SigH<int(), TestCollectFirst<int>> sigh_first;

    sigh_first.connect<&TestCollectFirst<int>::f>();
    sigh_first.connect<&TestCollectFirst<int>::f>();
    auto collector_first = sigh_first.collect();

    ASSERT_FALSE(sigh_first.empty());
    ASSERT_FALSE(collector_first.vec.empty());
    ASSERT_EQ((std::vector<int>::size_type)1, collector_first.vec.size());
    ASSERT_EQ(42, collector_first.vec[0]);
}
