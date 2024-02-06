#include <memory>
#include <gtest/gtest.h>
#include <entt/resource/loader.hpp>

TEST(ResourceLoader, Functionalities) {
    using loader_type = entt::resource_loader<int>;
    const auto resource = loader_type{}(4);

    testing::StaticAssertTypeEq<typename loader_type::result_type, std::shared_ptr<int>>();

    ASSERT_TRUE(resource);
    ASSERT_EQ(*resource, 4);
}
