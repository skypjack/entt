#include <gtest/gtest.h>
#include <entt/signal/delegate.hpp>

int delegate_function(const int &i) {
    return i*i;
}

int curried_function(const int *i, int j) {
    return *i+j;
}

struct delegate_functor {
    int operator()(int i) {
        return i+i;
    }

    int identity(int i) const {
        return i;
    }

    static const int static_value = 3;
    const int data_member = 42;
};

struct const_nonconst_noexcept {
    void f() { ++cnt; }
    void g() noexcept { ++cnt; }
    void h() const { ++cnt; }
    void i() const noexcept { ++cnt; }
    mutable int cnt{0};
};

TEST(Delegate, Functionalities) {
    entt::delegate<int(int)> ff_del;
    entt::delegate<int(int)> mf_del;
    delegate_functor functor;

    ASSERT_FALSE(ff_del);
    ASSERT_FALSE(mf_del);
    ASSERT_EQ(ff_del, mf_del);

    ff_del.connect<&delegate_function>();
    mf_del.connect<&delegate_functor::operator()>(&functor);

    ASSERT_TRUE(ff_del);
    ASSERT_TRUE(mf_del);

    ASSERT_EQ(ff_del(3), 9);
    ASSERT_EQ(mf_del(3), 6);

    ff_del.reset();

    ASSERT_FALSE(ff_del);
    ASSERT_TRUE(mf_del);

    ASSERT_EQ(ff_del, entt::delegate<int(int)>{});
    ASSERT_NE(mf_del, entt::delegate<int(int)>{});
    ASSERT_NE(ff_del, mf_del);

    mf_del.reset();

    ASSERT_FALSE(ff_del);
    ASSERT_FALSE(mf_del);

    ASSERT_EQ(ff_del, entt::delegate<int(int)>{});
    ASSERT_EQ(mf_del, entt::delegate<int(int)>{});
    ASSERT_EQ(ff_del, mf_del);
}

TEST(Delegate, DataMembers) {
    entt::delegate<double()> delegate;
    delegate_functor functor;

    delegate.connect<&delegate_functor::data_member>(&functor);

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

    lhs.connect<&curried_function>(&value);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_function>, &value}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&curried_function>(&value);

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{entt::connect_arg<&curried_function>, &value}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&delegate_functor::operator()>(&functor);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, &functor}));
    ASSERT_TRUE(lhs != rhs);
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NE(lhs, rhs);

    rhs.connect<&delegate_functor::operator()>(&functor);

    ASSERT_EQ(rhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, &functor}));
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

    lhs.connect<&delegate_functor::operator()>(&other);

    ASSERT_EQ(lhs, (entt::delegate<int(int)>{entt::connect_arg<&delegate_functor::operator()>, &other}));
    ASSERT_NE(lhs.instance(), rhs.instance());
    ASSERT_FALSE(lhs != rhs);
    ASSERT_TRUE(lhs == rhs);
    ASSERT_EQ(lhs, rhs);

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

    delegate.connect<&const_nonconst_noexcept::f>(&functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::g>(&functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::h>(&functor);
    delegate();

    delegate.connect<&const_nonconst_noexcept::i>(&functor);
    delegate();

    ASSERT_EQ(functor.cnt, 4);
}

TEST(Delegate, DeducedGuidelines) {
    const_nonconst_noexcept functor;
    const int value = 0;

    entt::delegate func_deduced{entt::connect_arg<&delegate_function>};
    entt::delegate curried_func_deduced{entt::connect_arg<&curried_function>, &value};
    entt::delegate member_f_deduced{entt::connect_arg<&const_nonconst_noexcept::f>, &functor};
    entt::delegate member_g_deduced{entt::connect_arg<&const_nonconst_noexcept::g>, &functor};
    entt::delegate member_h_deduced{entt::connect_arg<&const_nonconst_noexcept::h>, &functor};
    entt::delegate member_i_deduced{entt::connect_arg<&const_nonconst_noexcept::i>, &functor};

    static_assert(std::is_same_v<typename decltype(func_deduced)::function_type, int(const int &)>);
    static_assert(std::is_same_v<typename decltype(curried_func_deduced)::function_type, int(int)>);
    static_assert(std::is_same_v<typename decltype(member_f_deduced)::function_type, void()>);
    static_assert(std::is_same_v<typename decltype(member_g_deduced)::function_type, void()>);
    static_assert(std::is_same_v<typename decltype(member_h_deduced)::function_type, void()>);
    static_assert(std::is_same_v<typename decltype(member_i_deduced)::function_type, void()>);

    ASSERT_TRUE(func_deduced);
    ASSERT_TRUE(curried_func_deduced);
    ASSERT_TRUE(member_f_deduced);
    ASSERT_TRUE(member_g_deduced);
    ASSERT_TRUE(member_h_deduced);
    ASSERT_TRUE(member_i_deduced);
}

TEST(Delegate, ConstInstance) {
    entt::delegate<int(int)> delegate;
    const delegate_functor functor;

    ASSERT_FALSE(delegate);

    delegate.connect<&delegate_functor::identity>(&functor);

    ASSERT_TRUE(delegate);
    ASSERT_EQ(delegate(3), 3);

    delegate.reset();

    ASSERT_FALSE(delegate);
    ASSERT_EQ(delegate, entt::delegate<int(int)>{});
}

TEST(Delegate, CurriedFunction) {
    entt::delegate<int(int)> delegate;
    const auto value = 3;

    delegate.connect<&curried_function>(&value);

    ASSERT_TRUE(delegate);
    ASSERT_EQ(delegate(1), 4);
}

TEST(Delegate, Constructors) {
    delegate_functor functor;
    const auto value = 2;

    entt::delegate<int(int)> empty{};
    entt::delegate<int(int)> func{entt::connect_arg<&delegate_function>};
    entt::delegate<int(int)> curr{entt::connect_arg<&curried_function>, &value};
    entt::delegate<int(int)> member{entt::connect_arg<&delegate_functor::operator()>, &functor};

    ASSERT_FALSE(empty);

    ASSERT_TRUE(func);
    ASSERT_EQ(9, func(3));

    ASSERT_TRUE(curr);
    ASSERT_EQ(5, curr(3));

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
