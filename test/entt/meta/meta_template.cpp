#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>
#include <entt/meta/type_traits.hpp>

template<typename>
struct function_type;

template<typename Ret, typename... Args>
struct function_type<Ret(Args...)> {};

template<typename Ret, typename... Args>
struct entt::meta_template_traits<function_type<Ret(Args...)>> {
    using class_type = meta_class_template_tag<function_type>;
    using args_type = type_list<Ret, Args...>;
};

TEST(MetaTemplate, Invalid) {
    const auto type = entt::resolve<int>();

    ASSERT_FALSE(type.is_template_specialization());
    ASSERT_EQ(type.template_arity(), 0u);
    ASSERT_EQ(type.template_type(), entt::meta_type{});
    ASSERT_EQ(type.template_arg(0u), entt::meta_type{});
}

TEST(MetaTemplate, Valid) {
    const auto type = entt::resolve<entt::type_list<int, char>>();

    ASSERT_TRUE(type.is_template_specialization());
    ASSERT_EQ(type.template_arity(), 2u);
    ASSERT_EQ(type.template_type(), entt::resolve<entt::meta_class_template_tag<entt::type_list>>());
    ASSERT_EQ(type.template_arg(0u), entt::resolve<int>());
    ASSERT_EQ(type.template_arg(1u), entt::resolve<char>());
    ASSERT_EQ(type.template_arg(2u), entt::meta_type{});
}

TEST(MetaTemplate, CustomTraits) {
    const auto type = entt::resolve<function_type<void(int, const char &)>>();

    ASSERT_TRUE(type.is_template_specialization());
    ASSERT_EQ(type.template_arity(), 3u);
    ASSERT_EQ(type.template_type(), entt::resolve<entt::meta_class_template_tag<function_type>>());
    ASSERT_EQ(type.template_arg(0u), entt::resolve<void>());
    ASSERT_EQ(type.template_arg(1u), entt::resolve<int>());
    ASSERT_EQ(type.template_arg(2u), entt::resolve<char>());
    ASSERT_EQ(type.template_arg(3u), entt::meta_type{});
}
