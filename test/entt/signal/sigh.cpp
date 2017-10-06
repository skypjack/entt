#include <vector>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

TEST(SigH, Functionalities) {
    struct S {
        void f() {}
    };

    ASSERT_NO_THROW(entt::SigH<void(void)>{});

    entt::SigH<void(void)> src{}, other{};

    ASSERT_NO_THROW(entt::SigH<void(void)>{src});
    ASSERT_NO_THROW(entt::SigH<void(void)>{std::move(other)});
    ASSERT_NO_THROW(src = other);
    ASSERT_NO_THROW(src = std::move(other));
    ASSERT_NO_THROW(std::swap(src, other));

    ASSERT_EQ(src.size(), entt::SigH<void(void)>::size_type{0});
    ASSERT_TRUE(src == other);
    ASSERT_FALSE(src != other);
    ASSERT_TRUE(src.empty());

    S s;
    src.connect<S, &S::f>(&s);

    ASSERT_EQ(src.size(), entt::SigH<void(void)>::size_type{1});
    ASSERT_FALSE(src == other);
    ASSERT_TRUE(src != other);
    ASSERT_FALSE(src.empty());

    src.clear();

    ASSERT_EQ(src.size(), entt::SigH<void(void)>::size_type{0});
    ASSERT_TRUE(src == other);
    ASSERT_FALSE(src != other);
    ASSERT_TRUE(src.empty());

    ASSERT_NO_THROW(delete new entt::SigH<void(void)>{});
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
