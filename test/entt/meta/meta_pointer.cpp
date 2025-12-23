#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/meta/adl_pointer.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>
#include <entt/meta/type_traits.hpp>
#include "../../common/config.h"

struct MetaPointer: ::testing::Test {
    template<typename Type>
    struct wrapped_shared_ptr {
        wrapped_shared_ptr(Type init)
            : ptr{new Type{init}} {}

        [[nodiscard]] Type &deref() const {
            return *ptr;
        }

    private:
        std::shared_ptr<Type> ptr;
    };

    struct self_ptr {
        using element_type = self_ptr;

        self_ptr(int val)
            : value{val} {}

        const self_ptr &operator*() const {
            return *this;
        }

        int value;
    };

    struct proxy_ptr {
        using element_type = proxy_ptr;

        proxy_ptr(int &val)
            : value{&val} {}

        proxy_ptr operator*() const {
            return *this;
        }

        int *value;
    };

    template<typename Type>
    struct adl_wrapped_shared_ptr: wrapped_shared_ptr<Type> {
        using is_meta_pointer_like = void;
    };

    template<typename Type>
    struct spec_wrapped_shared_ptr: wrapped_shared_ptr<Type> {
        using is_meta_pointer_like = void;
    };

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaPointerDeathTest = MetaPointer;

template<>
struct entt::is_meta_pointer_like<MetaPointer::self_ptr>: std::true_type {};

template<>
struct entt::is_meta_pointer_like<MetaPointer::proxy_ptr>: std::true_type {};

template<typename Type>
struct entt::adl_meta_pointer_like<MetaPointer::spec_wrapped_shared_ptr<Type>> {
    static decltype(auto) dereference(const MetaPointer::spec_wrapped_shared_ptr<Type> &ptr) {
        return ptr.deref();
    }
};

template<typename Type>
Type &dereference_meta_pointer_like(const MetaPointer::adl_wrapped_shared_ptr<Type> &ptr) {
    return ptr.deref();
}

int test_function() {
    return 3;
}

TEST_F(MetaPointer, DereferenceOperatorInvalidType) {
    const int value = 0;
    const entt::meta_any any{value};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_FALSE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int>());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST_F(MetaPointer, DereferenceOperatorConstType) {
    const int value = 3;
    const entt::meta_any any{&value};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<const int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    ASSERT_EQ(deref.try_cast<int>(), nullptr);
    ASSERT_EQ(deref.try_cast<const int>(), &value);
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

ENTT_DEBUG_TEST_F(MetaPointerDeathTest, DereferenceOperatorConstType) {
    const int value = 3;
    const entt::meta_any any{&value};
    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_DEATH(deref.cast<int &>() = 0, "");
}

TEST_F(MetaPointer, DereferenceOperatorConstAnyNonConstType) {
    int value = 3;
    const entt::meta_any any{&value};
    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    ASSERT_NE(deref.try_cast<int>(), nullptr);
    ASSERT_NE(deref.try_cast<const int>(), nullptr);
    ASSERT_EQ(deref.cast<int &>(), 3);
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

TEST_F(MetaPointer, DereferenceOperatorConstAnyConstType) {
    const int value = 3;
    const entt::meta_any any{&value};
    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    ASSERT_EQ(deref.try_cast<int>(), nullptr);
    ASSERT_NE(deref.try_cast<const int>(), nullptr);
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

ENTT_DEBUG_TEST_F(MetaPointerDeathTest, DereferenceOperatorConstAnyConstType) {
    const int value = 3;
    const entt::meta_any any{&value};
    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_DEATH(deref.cast<int &>() = 0, "");
}

TEST_F(MetaPointer, DereferenceOperatorRawPointer) {
    int value = 0;
    entt::meta_any any{&value};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int &>() = 3;

    ASSERT_EQ(*any.cast<int *>(), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(MetaPointer, DereferenceOperatorSmartPointer) {
    auto value = std::make_shared<int>(0);
    entt::meta_any any{value};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<std::shared_ptr<int>>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int &>() = 3;

    ASSERT_EQ(*any.cast<std::shared_ptr<int>>(), 3);
    ASSERT_EQ(*value, 3);
}

TEST_F(MetaPointer, PointerToConstMoveOnlyType) {
    const std::unique_ptr<int> instance;
    const entt::meta_any any{&instance};
    auto deref = *any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(deref);

    ASSERT_EQ(deref.try_cast<std::unique_ptr<int>>(), nullptr);
    ASSERT_NE(deref.try_cast<const std::unique_ptr<int>>(), nullptr);
    ASSERT_EQ(&deref.cast<const std::unique_ptr<int> &>(), &instance);
}

TEST_F(MetaPointer, AsRef) {
    int value = 0;
    int *ptr = &value;
    entt::meta_any any{entt::forward_as_meta(ptr)};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int &>() = 3;

    ASSERT_EQ(*any.cast<int *>(), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(MetaPointer, AsConstRef) {
    int value = 3;
    int *const ptr = &value;
    entt::meta_any any{entt::forward_as_meta(ptr)};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int &>() = 3;

    ASSERT_EQ(*any.cast<int *>(), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(MetaPointer, DereferenceOverloadAdl) {
    const entt::meta_any any{adl_wrapped_shared_ptr<int>{3}};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    ASSERT_EQ(deref.cast<int &>(), 3);
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

TEST_F(MetaPointer, DereferenceOverloadSpec) {
    const entt::meta_any any{spec_wrapped_shared_ptr<int>{3}};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    ASSERT_EQ(deref.cast<int &>(), 3);
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

TEST_F(MetaPointer, DereferencePointerToConstOverloadAdl) {
    const entt::meta_any any{adl_wrapped_shared_ptr<const int>{3}};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

TEST_F(MetaPointer, DereferencePointerToConstOverloadSpec) {
    const entt::meta_any any{spec_wrapped_shared_ptr<const int>{3}};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());
    ASSERT_EQ(deref.cast<const int &>(), 3);
}

ENTT_DEBUG_TEST_F(MetaPointerDeathTest, DereferencePointerToConstOverloadAdl) {
    const entt::meta_any any{adl_wrapped_shared_ptr<const int>{3}};

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_DEATH(deref.cast<int &>() = 3, "");
}

ENTT_DEBUG_TEST_F(MetaPointerDeathTest, DereferencePointerToConstOverloadSpec) {
    const entt::meta_any any{spec_wrapped_shared_ptr<const int>{3}};

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_DEATH(deref.cast<int &>() = 3, "");
}

TEST_F(MetaPointer, DereferencePointerToVoid) {
    const entt::meta_any any{static_cast<void *>(nullptr)};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST_F(MetaPointer, DereferencePointerToConstVoid) {
    const entt::meta_any any{static_cast<const void *>(nullptr)};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST_F(MetaPointer, DereferenceSharedPointerToVoid) {
    const entt::meta_any any{std::shared_ptr<void>{}};

    ASSERT_TRUE(any.type().is_class());
    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST_F(MetaPointer, DereferenceUniquePointerToVoid) {
    const entt::meta_any any{std::unique_ptr<void, void (*)(void *)>{nullptr, nullptr}};

    ASSERT_TRUE(any.type().is_class());
    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST_F(MetaPointer, DereferencePointerToFunction) {
    entt::meta_any any{&test_function};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE((*std::as_const(any)).type().is_pointer_like());
    ASSERT_NE((**any).try_cast<int (*)()>(), nullptr);
    ASSERT_EQ((***std::as_const(any)).cast<int (*)()>()(), 3);
}

TEST_F(MetaPointer, DereferenceSelfPointer) {
    self_ptr obj{3};
    const entt::meta_any any{entt::forward_as_meta(obj)};
    entt::meta_any deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(deref.cast<const self_ptr &>().value, obj.value);
    ASSERT_FALSE(deref.try_cast<self_ptr>());
}

TEST_F(MetaPointer, DereferenceProxyPointer) {
    int value = 3;
    const proxy_ptr obj{value};
    const entt::meta_any any{obj};
    entt::meta_any deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(*deref.cast<const proxy_ptr &>().value, value);
    ASSERT_TRUE(deref.try_cast<proxy_ptr>());

    *deref.cast<proxy_ptr &>().value = 3;

    ASSERT_EQ(value, 3);
}

TEST_F(MetaPointer, DereferenceArray) {
    // NOLINTBEGIN(*-avoid-c-arrays)
    const entt::meta_any array{std::in_place_type<int[3]>};
    const entt::meta_any array_of_array{std::in_place_type<int[3][3]>};
    // NOLINTEND(*-avoid-c-arrays)

    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_EQ(array.type(), entt::resolve<int[3]>());
    ASSERT_EQ(array_of_array.type(), entt::resolve<int[3][3]>());
    // NOLINTEND(*-avoid-c-arrays)

    ASSERT_FALSE(*array);
    ASSERT_FALSE(*array_of_array);
}

TEST_F(MetaPointer, DereferencePlainNullPointer) {
    const entt::meta_any any{static_cast<int *>(nullptr)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(*any);
}

TEST_F(MetaPointer, DereferenceSharedNullPointer) {
    const entt::meta_any any{std::shared_ptr<int>{}};

    ASSERT_TRUE(any);
    ASSERT_FALSE(*any);
}

TEST_F(MetaPointer, DereferenceUniqueNullPointer) {
    const entt::meta_any any{std::unique_ptr<int>{}};

    ASSERT_TRUE(any);
    ASSERT_FALSE(*any);
}
