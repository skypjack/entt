#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <gtest/gtest.h>
#include "common/config.h"
#include "common/linter.hpp"
#include <entt/core/type_info.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/poly/poly.hpp>

template<typename Base>
struct common_type: Base {
    void incr() {
        constexpr auto member_index = 0u;
        entt::poly_call<member_index>(*this);
    }

    void set(int v) {
        constexpr auto member_index = 1u;
        entt::poly_call<member_index>(*this, v);
    }

    [[nodiscard]] int get() const {
        constexpr auto member_index = 2u;
        return static_cast<int>(entt::poly_call<member_index>(*this));
    }

    void decr() {
        constexpr auto member_index = 3u;
        entt::poly_call<member_index>(*this);
    }

    [[nodiscard]] int mul(int v) const {
        constexpr auto member_index = 4u;
        return static_cast<int>(entt::poly_call<member_index>(*this, v));
    }

    [[nodiscard]] int rand() const {
        constexpr auto member_index = 5u;
        return static_cast<int>(entt::poly_call<member_index>(*this));
    }
};

template<typename Type>
struct common_members {
    static void decr(Type &self) {
        self.set(self.get() - 1);
    }

    [[nodiscard]] static double mul(const Type &self, double v) {
        return v * self.get();
    }
};

namespace {

[[nodiscard]] int absolutely_random() {
    return 4;
}

} // namespace

template<typename Type>
using common_impl = entt::value_list<
    &Type::incr,
    &Type::set,
    &Type::get,
    &common_members<Type>::decr,
    &common_members<Type>::mul,
    &absolutely_random>;

struct Deduced
    : entt::type_list<> {
    template<typename Base>
    using type = common_type<Base>;

    template<typename Type>
    using members = common_members<Type>;

    template<typename Type>
    using impl = common_impl<Type>;
};

struct Defined
    : entt::type_list<
          void(),
          void(int),
          int() const,
          void(),
          int(int) const,
          int() const> {
    template<typename Base>
    using type = common_type<Base>;

    template<typename Type>
    using members = common_members<Type>;

    template<typename Type>
    using impl = common_impl<Type>;
};

struct DeducedEmbedded
    : entt::type_list<> {
    template<typename Base>
    struct type: Base {
        [[nodiscard]] int get() const {
            return entt::poly_call<0>(*this);
        }
    };

    template<typename Type>
    using impl = entt::value_list<&Type::get>;
};

struct DefinedEmbedded
    : entt::type_list<int()> {
    template<typename Base>
    struct type: Base {
        // non-const get on purpose
        [[nodiscard]] int get() {
            return entt::poly_call<0>(*this);
        }
    };

    template<typename Type>
    using impl = entt::value_list<&Type::get>;
};

struct impl {
    impl() = default;

    impl(int v)
        : value{v} {}

    void incr() {
        ++value;
    }

    void set(int v) {
        value = v;
    }

    [[nodiscard]] int get() const {
        return value;
    }

    int value{};
};

struct alignas(64u) over_aligned: impl {};

template<typename Type>
struct Poly: testing::Test {
    template<std::size_t... Args>
    using type = entt::basic_poly<Type, Args...>;
};

template<typename Type>
using PolyDeathTest = Poly<Type>;

using PolyTypes = ::testing::Types<Deduced, Defined>;

TYPED_TEST_SUITE(Poly, PolyTypes, );
TYPED_TEST_SUITE(PolyDeathTest, PolyTypes, );

template<typename Type>
struct PolyEmbedded: testing::Test {
    using type = entt::basic_poly<Type>;
};

using PolyEmbeddedTypes = ::testing::Types<DeducedEmbedded, DefinedEmbedded>;

TYPED_TEST_SUITE(PolyEmbedded, PolyEmbeddedTypes, );

TYPED_TEST(Poly, Functionalities) {
    using poly_type = typename TestFixture::template type<>;

    impl instance{};

    poly_type empty{};
    poly_type in_place{std::in_place_type<impl>, 3};
    poly_type alias{std::in_place_type<impl &>, instance};
    poly_type value{impl{}};

    ASSERT_FALSE(empty);
    ASSERT_TRUE(in_place);
    ASSERT_TRUE(alias);
    ASSERT_TRUE(value);

    ASSERT_EQ(empty.type(), entt::type_id<void>());
    ASSERT_EQ(in_place.type(), entt::type_id<impl>());
    ASSERT_EQ(alias.type(), entt::type_id<impl>());
    ASSERT_EQ(value.type(), entt::type_id<impl>());

    ASSERT_EQ(alias.data(), &instance);
    ASSERT_EQ(std::as_const(alias).data(), &instance);

    ASSERT_EQ(value->rand(), 4);

    empty = impl{};

    ASSERT_TRUE(empty);
    ASSERT_NE(empty.data(), nullptr);
    ASSERT_NE(std::as_const(empty).data(), nullptr);
    ASSERT_EQ(empty.type(), entt::type_id<impl>());
    ASSERT_EQ(empty->get(), 0);

    empty.template emplace<impl>(3);

    ASSERT_TRUE(empty);
    ASSERT_EQ(std::as_const(empty)->get(), 3);

    poly_type ref = in_place.as_ref();

    ASSERT_TRUE(ref);
    ASSERT_NE(ref.data(), nullptr);
    ASSERT_EQ(ref.data(), in_place.data());
    ASSERT_EQ(std::as_const(ref).data(), std::as_const(in_place).data());
    ASSERT_EQ(ref.type(), entt::type_id<impl>());
    ASSERT_EQ(ref->get(), 3);

    poly_type null{};
    std::swap(empty, null);

    ASSERT_FALSE(empty);

    poly_type copy = in_place;

    ASSERT_TRUE(copy);
    ASSERT_EQ(copy->get(), 3);

    poly_type move = std::move(copy);
    test::is_initialized(copy);

    ASSERT_TRUE(move);
    ASSERT_TRUE(copy);
    ASSERT_EQ(move->get(), 3);

    move.reset();

    ASSERT_FALSE(move);
    ASSERT_EQ(move.type(), entt::type_id<void>());
}

TYPED_TEST(PolyEmbedded, EmbeddedVtable) {
    using poly_type = typename TestFixture::type;

    poly_type poly{impl{}};
    auto *ptr = static_cast<impl *>(poly.data());

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(poly->get(), 0);

    ptr->value = 2;

    ASSERT_EQ(poly->get(), 2);
}

TYPED_TEST(Poly, Owned) {
    using poly_type = typename TestFixture::template type<>;

    poly_type poly{impl{}};
    auto *ptr = static_cast<impl *>(poly.data());

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(ptr->value, 0);
    ASSERT_EQ(poly->get(), 0);

    poly->set(1);
    poly->incr();

    ASSERT_EQ(ptr->value, 2);
    ASSERT_EQ(std::as_const(poly)->get(), 2);
    ASSERT_EQ(poly->mul(3), 6);

    poly->decr();

    ASSERT_EQ(ptr->value, 1);
    ASSERT_EQ(poly->get(), 1);
    ASSERT_EQ(poly->mul(3), 3);
}

TYPED_TEST(Poly, Reference) {
    using poly_type = typename TestFixture::template type<>;

    impl instance{};
    poly_type poly{std::in_place_type<impl &>, instance};

    ASSERT_TRUE(poly);
    ASSERT_NE(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);

    poly->set(1);
    poly->incr();

    ASSERT_EQ(instance.value, 2);
    ASSERT_EQ(std::as_const(poly)->get(), 2);
    ASSERT_EQ(poly->mul(3), 6);

    poly->decr();

    ASSERT_EQ(instance.value, 1);
    ASSERT_EQ(poly->get(), 1);
    ASSERT_EQ(poly->mul(3), 3);
}

TYPED_TEST(Poly, ConstReference) {
    using poly_type = typename TestFixture::template type<>;

    impl instance{};
    poly_type poly{std::in_place_type<const impl &>, instance};

    ASSERT_TRUE(poly);
    ASSERT_EQ(poly.data(), nullptr);
    ASSERT_NE(std::as_const(poly).data(), nullptr);
    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(std::as_const(poly)->get(), 0);
    ASSERT_EQ(poly->mul(3), 0);

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(poly->get(), 0);
    ASSERT_EQ(poly->mul(3), 0);
}

ENTT_DEBUG_TYPED_TEST(PolyDeathTest, ConstReference) {
    using poly_type = typename TestFixture::template type<>;

    impl instance{};
    poly_type poly{std::in_place_type<const impl &>, instance};

    ASSERT_TRUE(poly);
    ASSERT_DEATH(poly->set(1), "");
}

TYPED_TEST(Poly, AsRef) {
    using poly_type = typename TestFixture::template type<>;

    poly_type poly{impl{}};
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

TYPED_TEST(Poly, SBOVsZeroedSBOSize) {
    using poly_type = typename TestFixture::template type<>;
    using zeroed_type = typename TestFixture::template type<0u>;

    poly_type poly{impl{}};
    const auto broken = poly.data();
    poly_type other = std::move(poly);

    ASSERT_NE(broken, other.data());

    zeroed_type dyn{impl{}};
    const auto valid = dyn.data();
    zeroed_type same = std::move(dyn);

    ASSERT_EQ(valid, same.data());

    // everything works as expected
    same->incr();

    ASSERT_EQ(same->get(), 1);
}

TYPED_TEST(Poly, SboAlignment) {
    constexpr auto alignment = alignof(over_aligned);
    using poly_type = typename TestFixture::template type<alignment, alignment>;

    std::array<poly_type, 2u> sbo = {over_aligned{}, over_aligned{}};
    const auto *data = sbo[0].data();

    // NOLINTBEGIN(*-reinterpret-cast)
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[0u].data()) % alignment) == 0u);
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[1u].data()) % alignment) == 0u);
    // NOLINTEND(*-reinterpret-cast)

    std::swap(sbo[0], sbo[1]);

    // NOLINTBEGIN(*-reinterpret-cast)
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[0u].data()) % alignment) == 0u);
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[1u].data()) % alignment) == 0u);
    // NOLINTEND(*-reinterpret-cast)

    ASSERT_NE(data, sbo[1].data());
}

TYPED_TEST(Poly, NoSboAlignment) {
    constexpr auto alignment = alignof(over_aligned);
    using poly_type = typename TestFixture::template type<alignment>;

    std::array<poly_type, 2u> nosbo = {over_aligned{}, over_aligned{}};
    const auto *data = nosbo[0].data();

    // NOLINTBEGIN(*-reinterpret-cast)
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[0u].data()) % alignment) == 0u);
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[1u].data()) % alignment) == 0u);
    // NOLINTEND(*-reinterpret-cast)

    std::swap(nosbo[0], nosbo[1]);

    // NOLINTBEGIN(*-reinterpret-cast)
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[0u].data()) % alignment) == 0u);
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[1u].data()) % alignment) == 0u);
    // NOLINTEND(*-reinterpret-cast)

    ASSERT_EQ(data, nosbo[1].data());
}
