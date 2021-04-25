#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/poly/poly.hpp>

struct Deduced: entt::type_list<> {
    template<typename Base>
    struct type: Base {
        void incr() { entt::poly_call<0>(*this); }
        void set(int v) { entt::poly_call<1>(*this, v); }
        int get() const { return entt::poly_call<2>(*this); }
        void decr() { entt::poly_call<3>(*this); }
        int mul(int v) const { return static_cast<int>(entt::poly_call<4>(*this, v)); }
    };

    template<typename Type>
    struct members {
        static void decr(Type &self) { self.set(self.get()-1); }
        static double mul(const Type &self, double v) { return v * self.get(); }
    };

    template<typename Type>
    using impl = entt::value_list<
        &Type::incr,
        &Type::set,
        &Type::get,
        &members<Type>::decr,
        &members<Type>::mul
    >;
};

struct impl {
    impl() = default;
    impl(int v): value{v} {}
    void incr() { ++value; }
    void set(int v) { value = v; }
    int get() const { return value; }
    void decrement() { --value; }
    double multiply(double v) const { return v * value; }
    int value{};
};

struct alignas(64u) over_aligned: impl {};

TEST(PolyDeduced, Functionalities) {
    impl instance{};

    entt::poly<Deduced> empty{};
    entt::poly<Deduced> in_place{std::in_place_type<impl>, 3};
    entt::poly<Deduced> alias{std::in_place_type<impl &>, instance};
    entt::poly<Deduced> value{impl{}};

    ASSERT_FALSE(empty);
    ASSERT_TRUE(in_place);
    ASSERT_TRUE(alias);
    ASSERT_TRUE(value);

    ASSERT_EQ(empty.type(), entt::type_info{});
    ASSERT_EQ(in_place.type(), entt::type_id<impl>());
    ASSERT_EQ(alias.type(), entt::type_id<impl>());
    ASSERT_EQ(value.type(), entt::type_id<impl>());

    ASSERT_EQ(alias.data(), &instance);
    ASSERT_EQ(std::as_const(alias).data(), &instance);

    empty = impl{};

    ASSERT_TRUE(empty);
    ASSERT_NE(empty.data(), nullptr);
    ASSERT_NE(std::as_const(empty).data(), nullptr);
    ASSERT_EQ(empty.type(), entt::type_id<impl>());
    ASSERT_EQ(empty->get(), 0);

    empty.template emplace<impl>(3);

    ASSERT_TRUE(empty);
    ASSERT_EQ(empty->get(), 3);

    entt::poly<Deduced> ref = in_place.as_ref();

    ASSERT_TRUE(ref);
    ASSERT_NE(ref.data(), nullptr);
    ASSERT_EQ(ref.data(), in_place.data());
    ASSERT_EQ(std::as_const(ref).data(), std::as_const(in_place).data());
    ASSERT_EQ(ref.type(), entt::type_id<impl>());
    ASSERT_EQ(ref->get(), 3);

    entt::poly<Deduced> null{};
    std::swap(empty, null);

    ASSERT_FALSE(empty);

    entt::poly<Deduced> copy = in_place;

    ASSERT_TRUE(copy);
    ASSERT_EQ(copy->get(), 3);

    entt::poly<Deduced> move = std::move(copy);

    ASSERT_TRUE(move);
    ASSERT_FALSE(copy);
    ASSERT_EQ(move->get(), 3);

    move.reset();

    ASSERT_FALSE(move);
    ASSERT_EQ(move.type(), entt::type_info{});
}

TEST(PolyDeduced, Owned) {
    entt::poly<Deduced> poly{impl{}};
    auto *ptr = static_cast<impl *>(poly.data());

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(ptr->value, 0);
    ASSERT_EQ(poly->get(), 0);

    poly->set(1);
    poly->incr();

    ASSERT_EQ(ptr->value, 2);
    ASSERT_EQ(poly->get(), 2);
    ASSERT_EQ(poly->mul(3), 6);

    poly->decr();

    ASSERT_EQ(ptr->value, 1);
    ASSERT_EQ(poly->get(), 1);
    ASSERT_EQ(poly->mul(3), 3);
}

TEST(PolyDeduced, Reference) {
    impl instance{};
    entt::poly<Deduced> poly{std::in_place_type<impl &>, instance};

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);

    poly->set(1);
    poly->incr();

    ASSERT_EQ(instance.value, 2);
    ASSERT_EQ(poly->get(), 2);
    ASSERT_EQ(poly->mul(3), 6);

    poly->decr();

    ASSERT_EQ(instance.value, 1);
    ASSERT_EQ(poly->get(), 1);
    ASSERT_EQ(poly->mul(3), 3);
}

TEST(PolyDeduced, ConstReference) {
    impl instance{};
    entt::poly<Deduced> poly{std::in_place_type<const impl &>, instance};

    ASSERT_TRUE(poly);
    ASSERT_EQ(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);

    ASSERT_DEATH(poly->set(1), "");
    ASSERT_DEATH(poly->incr(), "");

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);
    ASSERT_EQ(poly->mul(3), 0);

    ASSERT_DEATH(poly->decr(), "");

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);
    ASSERT_EQ(poly->mul(3), 0);
}

TEST(PolyDeduced, AsRef) {
    entt::poly<Deduced> poly{impl{}};
    auto ref = poly.as_ref();
    auto cref = std::as_const(poly).as_ref();

    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(ref.data(), nullptr);
    ASSERT_EQ(cref.data(), nullptr);
    ASSERT_NE(std::as_const(cref).data(), nullptr);

    std::swap(ref, cref);

    ASSERT_EQ(ref.data(), nullptr);
    ASSERT_NE(std::as_const(ref).data(), nullptr);
    ASSERT_NE(cref.data(), nullptr);

    ref = ref.as_ref();
    cref = std::as_const(cref).as_ref();

    ASSERT_EQ(ref.data(), nullptr);
    ASSERT_NE(std::as_const(ref).data(), nullptr);
    ASSERT_EQ(cref.data(), nullptr);
    ASSERT_NE(std::as_const(cref).data(), nullptr);

    ref = impl{};
    cref = impl{};

    ASSERT_NE(ref.data(), nullptr);
    ASSERT_NE(cref.data(), nullptr);
}

TEST(PolyDeduced, SBOVsZeroedSBOSize) {
    entt::poly<Deduced> sbo{impl{}};
    const auto broken = sbo.data();
    entt::poly<Deduced> other = std::move(sbo);

    ASSERT_NE(broken, other.data());

    entt::basic_poly<Deduced, 0u> dyn{impl{}};
    const auto valid = dyn.data();
    entt::basic_poly<Deduced, 0u> same = std::move(dyn);

    ASSERT_EQ(valid, same.data());

    // everything works as expected
    same->incr();

    ASSERT_EQ(same->get(), 1);
}

TEST(PolyDeduced, Alignment) {
    static constexpr auto alignment = alignof(over_aligned);

    auto test = [](auto *target, auto cb) {
        const auto *data = target[0].data();

        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[0u].data()) % alignment) == 0u);
        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[1u].data()) % alignment) == 0u);

        std::swap(target[0], target[1]);

        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[0u].data()) % alignment) == 0u);
        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[1u].data()) % alignment) == 0u);

        cb(data, target[1].data());
    };

    entt::basic_poly<Deduced, alignment> nosbo[2] = { over_aligned{}, over_aligned{} };
    test(nosbo, [](auto *pre, auto *post) { ASSERT_EQ(pre, post); });

    entt::basic_poly<Deduced, alignment, alignment> sbo[2] = { over_aligned{}, over_aligned{} };
    test(sbo, [](auto *pre, auto *post) { ASSERT_NE(pre, post); });
}
