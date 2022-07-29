#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/delegate.hpp>
#include "../common/config.h"

int delegate_function(const int &i) {
    return i * i;
}

int curried_by_ref(const int &i, int j) {
    return i + j;
}

int curried_by_ptr(const int *i, int j) {
    return (*i) * j;
}

int non_const_reference(int &i) {
    return i *= i;
}

int move_only_type(std::unique_ptr<int> ptr) {
    return *ptr;
}

struct delegate_functor {
    int operator()(int i) {
        return i + i;
    }

    int identity(int i) const {
        return i;
    }

    static const int static_value = 3;
    const int data_member = 42;
};

struct const_nonconst_noexcept {
    void f() {
        ++cnt;
    }

    void g() noexcept {
        ++cnt;
    }

    void h() const {
        ++cnt;
    }

    void i() const noexcept {
        ++cnt;
    }

    int u{};
    const int v{};
    mutable int cnt{0};
};

TEST(Delegate, Functionalities) {
    entt::delegate<int(int)> ff_del;
    entt::delegate<int(int)> mf_del;
    entt::delegate<int(int)> lf_del;
    delegate_functor functor;

    ASSERT_FALSE(ff_del);
    ASSERT_FALSE(mf_del);
    ASSERT_EQ(ff_del, mf_del);

    ff_del.connect<&delegate_function>();
    mf_del.connect<&delegate_functor::operator()>(functor);
    lf_del.connect([](const void *ptr, int value) { return static_cast<const delegate_functor *>(ptr)->identity(value); }, &functor);

    ASSERT_TRUE(ff_del);
    ASSERT_TRUE(mf_del);
    ASSERT_TRUE(lf_del);

    ASSERT_EQ(ff_del(3), 9);
    ASSERT_EQ(mf_del(3), 6);
    ASSERT_EQ(lf_del(3), 3);

    ff_del.reset();

    ASSERT_FALSE(ff_del);

    ASSERT_EQ(ff_del, entt::delegate<int(int)>{});
    ASSERT_NE(mf_del, entt::delegate<int(int)>{});
    ASSERT_NE(lf_del, entt::delegate<int(int)>{});

    ASSERT_NE(ff_del, mf_del);
    ASSERT_NE(ff_del, lf_del);
    ASSERT_NE(mf_del, lf_del);

    mf_del.reset();

    ASSERT_FALSE(ff_del);
    ASSERT_FALSE(mf_del);
    ASSERT_TRUE(lf_del);

    ASSERT_EQ(ff_del, entt::delegate<int(int)>{});
    ASSERT_EQ(mf_del, entt::delegate<int(int)>{});
    ASSERT_NE(lf_del, entt::delegate<int(int)>{});

    ASSERT_EQ(ff_del, mf_del);
    ASSERT_NE(ff_del, lf_del);
    ASSERT_NE(mf_del, lf_del);
}

ENTT_DEBUG_TEST(DelegateDeathTest, InvokeEmpty) {
    entt::delegate<int(int)> del;

    ASSERT_FALSE(del);
    ASSERT_DEATH(del(42), "");
    ASSERT_DEATH(std::as_const(del)(42), "");
}

TEST(Delegate, DataMembers) {
    entt::delegate<double()> delegate;
    delegate_functor functor;

    delegate.connect<&delegate_functor::data_member>(functor);

    ASSERT_EQ(delegate(), 42);
}

TEST(Delegate, Comparison) {
    entt::delegate<int(int)> lhs;
    entt::delegate<int(int)> rhs;
    delegate_functor functor;
    delegate_functor other;
    const int value = 0;

    ASSERT_EQ(lhs, entt::delegate<int(int)>{});
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&delegate_function>();

    ASSERT_EQ(lhs, entt::delegate<int(int)>{entt::connect_arg<&delegate_function>});
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&delegate_function>();

    ASSERT_EQ(rhs, entt::delegate<int(int)>{entt::connect_arg<&delegate_function>});
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&curried_by_ref>(value);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_by_ref>, value}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&curried_by_ref>(value);

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_by_ref>, value}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&curried_by_ptr>(&value);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_by_ptr>, &value}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&curried_by_ptr>(&value);

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_by_ptr>, &value}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&delegate_functor::operator()>(functor);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, functor}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&delegate_functor::operator()>(functor);

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, functor}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&delegate_functor::operator()>(other);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, other}));
    ASSERT_NE(lhs.data(), rhs.data());
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    lhs.connect([](const void *ptr, int val) { return static_cast<const delegate_functor *>(ptr)->identity(val) * val; }, &functor);

    ASSERT_NE(lhs, (entt::delegate<int(int)>{[](const void *, int val) { return val + val; }, &functor}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect([](const void *ptr, int val) { return static_cast<const delegate_functor *>(ptr)->identity(val) + val; }, &functor);

    ASSERT_NE(rhs, (entt::delegate<int(int)>{[](const void *, int val) { return val * val; }, &functor}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    lhs.reset();

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.reset();

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);
}

TEST(Delegate, ConstNonConstNoExcept) {
    entt::delegate<void()> delegate;
    const_nonconst_noexcept functor;

    delegate.connect<&const_nonconst_noexcept::f>(functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::g>(functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::h>(functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::i>(functor);
    delegate();

    ASSERT_EQ(functor.cnt, 4);
}

TEST(Delegate, DeductionGuide) {
    const_nonconst_noexcept functor;
    int value = 0;

    entt::delegate func{entt::connect_arg<&delegate_function>};
    entt::delegate curried_func_with_ref{entt::connect_arg<&curried_by_ref>, value};
    entt::delegate curried_func_with_const_ref{entt::connect_arg<&curried_by_ref>, std::as_const(value)};
    entt::delegate curried_func_with_ptr{entt::connect_arg<&curried_by_ptr>, &value};
    entt::delegate curried_func_with_const_ptr{entt::connect_arg<&curried_by_ptr>, &std::as_const(value)};
    entt::delegate member_func_f{entt::connect_arg<&const_nonconst_noexcept::f>, functor};
    entt::delegate member_func_g{entt::connect_arg<&const_nonconst_noexcept::g>, functor};
    entt::delegate member_func_h{entt::connect_arg<&const_nonconst_noexcept::h>, &functor};
    entt::delegate member_func_h_const{entt::connect_arg<&const_nonconst_noexcept::h>, &std::as_const(functor)};
    entt::delegate member_func_i{entt::connect_arg<&const_nonconst_noexcept::i>, functor};
    entt::delegate member_func_i_const{entt::connect_arg<&const_nonconst_noexcept::i>, std::as_const(functor)};
    entt::delegate data_member_u{entt::connect_arg<&const_nonconst_noexcept::u>, functor};
    entt::delegate data_member_v{entt::connect_arg<&const_nonconst_noexcept::v>, &functor};
    entt::delegate data_member_v_const{entt::connect_arg<&const_nonconst_noexcept::v>, &std::as_const(functor)};
    entt::delegate lambda{+[](const void *, int) { return 0; }};

    static_assert(std::is_same_v<typename decltype(func)::type, int(const int &)>);
    static_assert(std::is_same_v<typename decltype(curried_func_with_ref)::type, int(int)>);
    static_assert(std::is_same_v<typename decltype(curried_func_with_const_ref)::type, int(int)>);
    static_assert(std::is_same_v<typename decltype(curried_func_with_ptr)::type, int(int)>);
    static_assert(std::is_same_v<typename decltype(curried_func_with_const_ptr)::type, int(int)>);
    static_assert(std::is_same_v<typename decltype(member_func_f)::type, void()>);
    static_assert(std::is_same_v<typename decltype(member_func_g)::type, void()>);
    static_assert(std::is_same_v<typename decltype(member_func_h)::type, void()>);
    static_assert(std::is_same_v<typename decltype(member_func_h_const)::type, void()>);
    static_assert(std::is_same_v<typename decltype(member_func_i)::type, void()>);
    static_assert(std::is_same_v<typename decltype(member_func_i_const)::type, void()>);
    static_assert(std::is_same_v<typename decltype(data_member_u)::type, int()>);
    static_assert(std::is_same_v<typename decltype(data_member_v)::type, const int()>);
    static_assert(std::is_same_v<typename decltype(data_member_v_const)::type, const int()>);
    static_assert(std::is_same_v<typename decltype(lambda)::type, int(int)>);

    ASSERT_TRUE(func);
    ASSERT_TRUE(curried_func_with_ref);
    ASSERT_TRUE(curried_func_with_const_ref);
    ASSERT_TRUE(curried_func_with_ptr);
    ASSERT_TRUE(curried_func_with_const_ptr);
    ASSERT_TRUE(member_func_f);
    ASSERT_TRUE(member_func_g);
    ASSERT_TRUE(member_func_h);
    ASSERT_TRUE(member_func_h_const);
    ASSERT_TRUE(member_func_i);
    ASSERT_TRUE(member_func_i_const);
    ASSERT_TRUE(data_member_u);
    ASSERT_TRUE(data_member_v);
    ASSERT_TRUE(data_member_v_const);
    ASSERT_TRUE(lambda);
}

TEST(Delegate, ConstInstance) {
    entt::delegate<int(int)> delegate;
    const delegate_functor functor;

    ASSERT_FALSE(delegate);

    delegate.connect<&delegate_functor::identity>(functor);

    ASSERT_TRUE(delegate);
    ASSERT_EQ(delegate(3), 3);

    delegate.reset();

    ASSERT_FALSE(delegate);
    ASSERT_EQ(delegate, entt::delegate<int(int)>{});
}

TEST(Delegate, NonConstReference) {
    entt::delegate<int(int &)> delegate;
    delegate.connect<&non_const_reference>();
    int value = 3;

    ASSERT_EQ(delegate(value), value);
    ASSERT_EQ(value, 9);
}

TEST(Delegate, MoveOnlyType) {
    entt::delegate<int(std::unique_ptr<int>)> delegate;
    auto ptr = std::make_unique<int>(3);
    delegate.connect<&move_only_type>();

    ASSERT_EQ(delegate(std::move(ptr)), 3);
    ASSERT_FALSE(ptr);
}

TEST(Delegate, CurriedFunction) {
    entt::delegate<int(int)> delegate;
    const auto value = 3;

    delegate.connect<&curried_by_ref>(value);

    ASSERT_TRUE(delegate);
    ASSERT_EQ(delegate(1), 4);

    delegate.connect<&curried_by_ptr>(&value);

    ASSERT_TRUE(delegate);
    ASSERT_EQ(delegate(2), 6);
}

TEST(Delegate, Constructors) {
    delegate_functor functor;
    const auto value = 2;

    entt::delegate<int(int)> empty{};
    entt::delegate<int(int)> func{entt::connect_arg<&delegate_function>};
    entt::delegate<int(int)> ref{entt::connect_arg<&curried_by_ref>, value};
    entt::delegate<int(int)> ptr{entt::connect_arg<&curried_by_ptr>, &value};
    entt::delegate<int(int)> member{entt::connect_arg<&delegate_functor::operator()>, functor};

    ASSERT_FALSE(empty);

    ASSERT_TRUE(func);
    ASSERT_EQ(9, func(3));

    ASSERT_TRUE(ref);
    ASSERT_EQ(5, ref(3));

    ASSERT_TRUE(ptr);
    ASSERT_EQ(6, ptr(3));

    ASSERT_TRUE(member);
    ASSERT_EQ(6, member(3));
}

TEST(Delegate, VoidVsNonVoidReturnType) {
    delegate_functor functor;

    entt::delegate<void(int)> func{entt::connect_arg<&delegate_function>};
    entt::delegate<void(int)> member{entt::connect_arg<&delegate_functor::operator()>, &functor};
    entt::delegate<void(int)> cmember{entt::connect_arg<&delegate_functor::identity>, &std::as_const(functor)};

    ASSERT_TRUE(func);
    ASSERT_TRUE(member);
    ASSERT_TRUE(cmember);
}

TEST(Delegate, UnboundDataMember) {
    entt::delegate<int(const delegate_functor &)> delegate;
    delegate.connect<&delegate_functor::data_member>();
    delegate_functor functor;

    ASSERT_EQ(delegate(functor), 42);
}

TEST(Delegate, UnboundMemberFunction) {
    entt::delegate<int(delegate_functor *, const int &i)> delegate;
    delegate.connect<&delegate_functor::operator()>();
    delegate_functor functor;

    ASSERT_EQ(delegate(&functor, 3), 6);
}

TEST(Delegate, TheLessTheBetter) {
    entt::delegate<int(int, char)> bound;
    entt::delegate<int(delegate_functor &, int, char)> unbound;
    delegate_functor functor;

    // int delegate_function(const int &);
    bound.connect<&delegate_function>();

    ASSERT_EQ(bound(3, 'c'), 9);

    // int delegate_functor::operator()(int);
    bound.connect<&delegate_functor::operator()>(functor);

    ASSERT_EQ(bound(3, 'c'), 6);

    // int delegate_functor::operator()(int);
    bound.connect<&delegate_functor::identity>(&functor);

    ASSERT_EQ(bound(3, 'c'), 3);

    // int delegate_functor::operator()(int);
    unbound.connect<&delegate_functor::operator()>();

    ASSERT_EQ(unbound(functor, 3, 'c'), 6);
}
