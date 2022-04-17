#include <gtest/gtest.h>
#include "../common/polymorphic_type.hpp"


TEST(IsPolyType, Functionalities) {
    ASSERT_TRUE(entt::is_poly_type_v<animal>);
    ASSERT_TRUE(entt::is_poly_type_v<animal*>);
    ASSERT_TRUE(entt::is_poly_type_v<cat>);
    ASSERT_TRUE(entt::is_poly_type_v<cat*>);
    ASSERT_TRUE(entt::is_poly_type_v<dog>);
    ASSERT_TRUE(entt::is_poly_type_v<dog*>);
    ASSERT_TRUE(entt::is_poly_type_v<shape>);
    ASSERT_TRUE(entt::is_poly_type_v<shape*>);
    ASSERT_TRUE(entt::is_poly_type_v<sphere>);
    ASSERT_TRUE(entt::is_poly_type_v<sphere*>);
    ASSERT_TRUE(entt::is_poly_type_v<cube>);
    ASSERT_TRUE(entt::is_poly_type_v<cube*>);
    ASSERT_TRUE(entt::is_poly_type_v<fat_cat>);
    ASSERT_TRUE(entt::is_poly_type_v<fat_cat*>);

    ASSERT_FALSE(entt::is_poly_type_v<int>);
    ASSERT_FALSE(entt::is_poly_type_v<not_poly_type>);
    ASSERT_FALSE(entt::is_poly_type_v<not_poly_type_base>);
}

TEST(ValidatePolyType, Functionalities) {
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<animal>, animal>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<dog>, dog>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<cat>, cat>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<shape>, shape>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<sphere>, sphere>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<cube>, cube>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_validate_t<fat_cat>, fat_cat>));
}

TEST(IsPolyParentOf, Functionalities) {
    ASSERT_TRUE((entt::is_poly_parent_of_v<animal, animal>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<shape, sphere>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<shape, cube>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<animal, dog>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<animal, cat>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<shape, fat_cat>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<animal, fat_cat>));
    ASSERT_TRUE((entt::is_poly_parent_of_v<fat_cat, fat_cat>));

    ASSERT_FALSE((entt::is_poly_parent_of_v<animal, cube>));
    ASSERT_FALSE((entt::is_poly_parent_of_v<cat, animal>));
    ASSERT_FALSE((entt::is_poly_parent_of_v<int, cube>));
    ASSERT_FALSE((entt::is_poly_parent_of_v<int, int>));
    ASSERT_FALSE((entt::is_poly_parent_of_v<not_poly_type_base, not_poly_type>));
    ASSERT_FALSE((entt::is_poly_parent_of_v<not_poly_type, not_poly_type>));
}

TEST(ParentTypeList, Functionalities) {
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<animal>, entt::type_list<>>));
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<cat>, entt::type_list<animal>>));
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<dog>, entt::type_list<animal>>));
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<shape>, entt::type_list<>>));
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<cube>, entt::type_list<shape>>));
    ASSERT_TRUE((std::is_same_v<entt::poly_parent_types_t<sphere>, entt::type_list<shape>>));
}

TEST(SanitizePolyType, Functionalities) {
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<cat>, cat>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<const cat>, const cat>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<cat*>, cat*>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<const cat*>, cat* const>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<cat* const>, cat* const>));
    ASSERT_TRUE((std::is_same_v<entt::poly_type_sanitize_t<const cat* const>, cat* const>));
}
