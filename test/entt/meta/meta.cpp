#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <iostream>


void print(unsigned int n, entt::MetaType *meta) {
    std::cout << std::string(n, ' ') << "class: " << meta->name() << std::endl;

    meta->properties([n=n+2](auto *prop) {
        std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
    });

    meta->data([n=n+2](auto *data) {
        std::cout << std::string(n, ' ') << "data: " << data->name() << std::endl;

        data->properties([n=n+2](auto *prop) {
            std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
        });

        if(data->type()) {
            print(n, data->type());
        }
    });

    meta->func([n=n+2](auto *func) {
        std::cout << std::string(n, ' ') << "func: " << func->name() << std::endl;

        func->properties([n=n+2](auto *prop) {
            std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
        });

        if(func->ret()) {
            print(n+2, func->ret());
        }

        for(unsigned int i = 0; i < func->size(); ++i) {
            if(func->arg(i)) {
                print(n+2, func->arg(i));
            }
        }
    });
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


void destroy(A &a) {
    std::cout << "destroy A: " << a.i << "/" << a.c << std::endl;
    a.~A();
}


void serialize(const S &) {
    std::cout << "serializing S" << std::endl;
}


void serialize(T &) {
    std::cout << "serializing T" << std::endl;
}


TEST(Meta, TODO) {
    entt::Meta::reflect<S>("foo", entt::property(entt::HashedString{"x"}, 42), entt::property(entt::HashedString{"y"}, 100))
            .ctor<>()
            .ctor<int, int>()
            .ctor<const S &>()
            .data<int, &S::i>("i")
            .data<int, &S::j>("j")
            .data<const int * const, &S::k>("k")
            .func<void(), &S::f>("f")
            .func<S *(int), &S::g>("g")
            .func<const S *(int) const, &S::g>("cg")
            .func<int(int) const, &S::h>("h")
            .func<void(const S &), &serialize>("serialize", entt::property(entt::HashedString{"3"}, 3))
            ;

    entt::Meta::reflect<T>("bar")
            .data<S, &T::s1>("s1")
            .data<const S, &T::s2>("s2")
            .func<void(const S &), &T::f>("f")
            .func<void(T &), &serialize>("serialize")
            ;

    ASSERT_NE(entt::Meta::resolve<S>(), nullptr);
    ASSERT_NE(entt::Meta::resolve<T>(), nullptr);
    ASSERT_NE(entt::Meta::resolve<int>(), nullptr);

    auto *sMeta = entt::Meta::resolve<S>();
    auto *tMeta = entt::Meta::resolve("bar");

    print(0u, sMeta);
    print(0u, tMeta);

    ASSERT_EQ(sMeta->property(entt::HashedString{"x"})->value().to<int>(), 42);
    ASSERT_EQ(sMeta->property(entt::HashedString{"y"})->value().to<int>(), 100);

    ASSERT_STREQ(sMeta->name(), "foo");
    ASSERT_NE(sMeta, tMeta);

    S s{0, 0};
    sMeta->data("i")->set(&s, 3);
    sMeta->data("j")->set(&s, 42);

    ASSERT_EQ(s.i, 3);
    ASSERT_EQ(s.j, 42);
    ASSERT_EQ(sMeta->data("i")->get(&s).to<int>(), 3);
    ASSERT_EQ(sMeta->data("j")->get(&s).to<int>(), 42);

    sMeta->data([&s](auto *data) {
        if(!data->constant()) {
            data->set(&s, 0);
        }
    });

    ASSERT_EQ(s.i, 0);
    ASSERT_EQ(s.j, 0);
    ASSERT_EQ(sMeta->data("i")->get(&s).to<int>(), 0);
    ASSERT_EQ(sMeta->data("j")->get(&s).to<int>(), 0);

    T t{S{0, 0}, S{0, 0}};

    auto *s1Data = tMeta->data("s1");
    auto *s1DataMeta = s1Data->type();
    auto instance = s1DataMeta->ctor<int, int>()->invoke(99, 100);

    ASSERT_EQ(instance.meta(), entt::Meta::resolve<S>());
    ASSERT_TRUE(instance);

    tMeta->data("s1")->set(&t, instance.to<S>());
    s1DataMeta->destroy(instance);

    ASSERT_EQ(t.s1.i, 99);
    ASSERT_EQ(t.s1.j, 100);

    auto res = sMeta->func("h")->invoke(&s, 41);

    ASSERT_EQ(res.meta(), entt::Meta::resolve<int>());
    ASSERT_EQ(res.to<int>(), 42);

    sMeta->func("serialize")->invoke(static_cast<const void *>(&s));
    tMeta->func("serialize")->invoke(&t);

    ASSERT_EQ(sMeta->func("serialize")->size(), 0);
    ASSERT_EQ(tMeta->func("serialize")->size(), 0);

    entt::Meta::reflect<A>("A").ctor<int, char>().dtor<&destroy>();
    auto any = entt::Meta::resolve<A>()->construct(42, 'c');

    ASSERT_EQ(any.to<A>().i, 42);
    ASSERT_EQ(any.to<A>().c, 'c');

    entt::Meta::resolve<A>()->dtor()->invoke(any);
    entt::Meta::resolve<A>()->destroy(entt::Meta::resolve<A>()->construct(42, 'c'));

    print(0, entt::Meta::resolve("A"));
}
