#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/any.hpp>
#include <iostream>


void print(unsigned int n, entt::MetaClass *meta) {
    std::cout << std::string(n, ' ') << "class: " << static_cast<const char *>(meta->name()) << std::endl;

    for(auto &prop: meta->prop()) {
        std::cout << std::string(n+2, ' ') << "prop: " << static_cast<const char *>(prop.key().value<entt::HashedString>()) << "/" << prop.value().value<int>() << std::endl;
    }

    for(auto &data: meta->data()) {
        std::cout << std::string(n, ' ') << "data: " << static_cast<const char *>(data.name()) << std::endl;

        for(auto &prop: data.prop()) {
            std::cout << std::string(n+2, ' ') << "prop: " << static_cast<const char *>(prop.key().value<entt::HashedString>()) << "/" << prop.value().value<int>() << std::endl;
        }

        if(data.type()) {
            print(n+2, data.type());
        }
    }

    for(auto &func: meta->func()) {
        std::cout << std::string(n, ' ') << "func: " << static_cast<const char *>(func.name()) << std::endl;

        for(auto &prop: func.prop()) {
            std::cout << std::string(n+2, ' ') << "prop: " << static_cast<const char *>(prop.key().value<entt::HashedString>()) << "/" << prop.value().value<int>() << std::endl;
        }

        if(func.ret()) {
            print(n+2, func.ret());
        }

        for(unsigned int i = 0; i < func.size(); ++i) {
            if(func.arg(i)) {
                print(n+2, func.arg(i));
            }
        }
    }
}


struct S {
    S() = default;
    S(int i, int j): i{i}, j{j} {}
    S(const S &other): i{other.i}, j{other.j} {}

    S & operator=(const S &other) {
        i = other.i;
        j = other.j;
        return *this;
    }

    void f() { std::cout << "... ??? !!!" << std::endl; }
    const S * g(int) const { return this; }
    S * g(int) { return this; }
    int h(int v) const noexcept { return v+1; }

    int i;
    int j;
    const int * const k{nullptr};
};


struct T {
    S s1;
    const S s2;
    void f(const S &) {}
};


struct A {
    int i;
    char c;
};


void serialize(const S &) {
    std::cout << "serializing S" << std::endl;
}


void serialize(const T &) {
    std::cout << "serializing T" << std::endl;
}


TEST(Meta, TODO) {
    entt::reflect<S>("foo", entt::property(entt::HashedString{"x"}, 42), entt::property(entt::HashedString{"y"}, 100))
            .ctor<>("def")
            .ctor<int, int>("int.int")
            .ctor<const S &>("const S &")
            .data<int, &S::i>("i")
            .data<int, &S::j>("j")
            .data<const int * const, &S::k>("k")
            .func<void(), &S::f>("f")
            .func<S *(int), &S::g>("g")
            .func<const S *(int) const, &S::g>("g")
            .func<int(int) const, &S::h>("h")
            .func<void(), &serialize>("serialize", entt::property(entt::HashedString{"3"}, 3))
            .func<int(char)>("lambda", [](const S &, auto) { std::cout << "lambda S" << std::endl; return -1; })
            ;

    entt::reflect<T>("bar")
            .data<S, &T::s1>("s1")
            .data<const S, &T::s2>("s2")
            .func<void(const S &), &T::f>("f")
            .func<void(), &serialize>("serialize")
            .func<int(char)>("lambda", [](const T &, auto) { std::cout << "lambda T" << std::endl; return -2; })
            ;

    auto *sMeta = entt::meta<S>();
    auto *tMeta = entt::meta("bar");

    print(0u, sMeta);
    print(0u, tMeta);

    ASSERT_EQ(sMeta->prop(entt::HashedString{"x"})->value().value<int>(), 42);
    ASSERT_EQ(sMeta->prop(entt::HashedString{"y"})->value().value<int>(), 100);

    ASSERT_STREQ(sMeta->name(), "foo");
    ASSERT_NE(sMeta, tMeta);

    S s{0, 0};
    sMeta->data("i")->set(&s, 3);
    sMeta->data("j")->set(&s, 42);

    ASSERT_EQ(s.i, 3);
    ASSERT_EQ(s.j, 42);
    ASSERT_EQ(sMeta->data("i")->get(&s).value<int>(), 3);
    ASSERT_EQ(sMeta->data("j")->get(&s).value<int>(), 42);

    for(auto &data: sMeta->data()) {
        if(!data.constant()) {
            data.set(&s, 0);
        }
    }

    ASSERT_EQ(s.i, 0);
    ASSERT_EQ(s.j, 0);
    ASSERT_EQ(sMeta->data("i")->get(&s).value<int>(), 0);
    ASSERT_EQ(sMeta->data("j")->get(&s).value<int>(), 0);

    T t{S{0, 0}, S{0, 0}};

    auto *s1Data = tMeta->data("s1");
    auto *s1DataMeta = s1Data->type();
    auto instance = s1DataMeta->ctor("int.int")->invoke(99, 100);

    ASSERT_TRUE(instance);

    tMeta->data("s1")->set(&t, instance);
    s1DataMeta->destroy(instance);

    ASSERT_EQ(t.s1.i, 99);
    ASSERT_EQ(t.s1.j, 100);

    auto res = sMeta->func("h")->invoke(&s, 41);

    ASSERT_EQ(res.type(), entt::Any::type<int>());
    ASSERT_EQ(res.value<int>(), 42);

    sMeta->func("serialize")->invoke(&s);
    tMeta->func("serialize")->invoke(static_cast<const void *>(&t));

    ASSERT_EQ(sMeta->func("serialize")->size(), 0);
    ASSERT_EQ(tMeta->func("serialize")->size(), 0);

    ASSERT_EQ(sMeta->func("lambda")->invoke(&s, 'c').value<int>(), -1);
    ASSERT_EQ(tMeta->func("lambda")->invoke(static_cast<const void *>(&t), 'c').value<int>(), -2);

    entt::reflect<A>("A").ctor<int, char>("ic");
    auto any = entt::meta<A>()->construct(42, 'c');
    const auto &a = any.value<A>();

    ASSERT_EQ(a.i, 42);
    ASSERT_EQ(a.c, 'c');

    print(0, entt::meta("A"));
}
