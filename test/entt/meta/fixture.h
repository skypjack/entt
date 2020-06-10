#ifndef ENTT_META_FIXTURE_H
#define ENTT_META_FIXTURE_H

#include <gtest/gtest.h>

template<typename Type>
void set(Type &prop, Type value) {
    prop = value;
}

template<typename Type>
Type get(Type &prop) {
    return prop;
}

enum class props {
    prop_int,
    prop_value,
    prop_bool,
    key_only,
    prop_list
};

struct empty_type {
    virtual ~empty_type() = default;
    static void destroy(empty_type &);
    inline static int counter = 0;
};

struct fat_type: empty_type {
    fat_type() = default;
    fat_type(int *);

    bool operator==(const fat_type &) const;

    int *foo{nullptr};
    int *bar{nullptr};
};

union union_type {
    int i;
    double d;
};

struct base_type {
    virtual ~base_type() = default;
};

struct derived_type: base_type {
    derived_type() = default;
    derived_type(const base_type &, int, char);

    int f() const;
    static char g(const derived_type &);

    const int i{};
    const char c{};
};

derived_type derived_factory(const base_type &, int);

struct data_type {
    int i{0};
    const int j{1};
    inline static int h{2};
    inline static const int k{3};
    empty_type empty{};
    int v{0};
};

struct array_type {
    static inline int global[3];
    int local[3];
};

struct func_type {
    int f(const base_type &, int, int);
    int f(int, int);
    int f(int) const;
    void g(int);

    static int h(int &);
    static void k(int);

    int v(int) const;
    int & a() const;

    inline static int value = 0;
};

struct setter_getter_type {
    int value{};

    int setter(int);
    int getter();

    int setter_with_ref(const int &);
    const int & getter_with_ref();

    static int static_setter(setter_getter_type &, int);
    static int static_getter(const setter_getter_type &);
};

struct not_comparable_type {
    bool operator==(const not_comparable_type &) const = delete;
};

struct unmanageable_type {
    unmanageable_type() = default;
    unmanageable_type(const unmanageable_type &) = delete;
    unmanageable_type(unmanageable_type &&) = delete;
    unmanageable_type & operator=(const unmanageable_type &) = delete;
    unmanageable_type & operator=(unmanageable_type &&) = delete;
};

bool operator!=(const not_comparable_type &, const not_comparable_type &) = delete;

struct an_abstract_type {
    virtual ~an_abstract_type() = default;
    void f(int v);
    virtual void g(int) = 0;
    int i{};
};

struct another_abstract_type {
    virtual ~another_abstract_type() = default;
    virtual void h(char) = 0;
    char j{};
};

struct concrete_type: an_abstract_type, another_abstract_type {
    void f(int v); // hide, it's ok :-)
    void g(int v) override;
    void h(char c) override;
};

struct Meta: ::testing::Test {
    static void SetUpTestCase();
    static void SetUpAfterUnregistration();
    void SetUp() override;
};

#endif
