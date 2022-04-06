#include <type_traits>
#include <gtest/gtest.h>
#include <entt/resource/loader.hpp>

TEST(ResourceLoader, Functionalities) {
    using loader_type = entt::resource_loader<int>;
    const auto resource = loader_type{}(42);

    static_assert(std::is_same_v<typename loader_type::result_type, std::shared_ptr<int>>);

    ASSERT_TRUE(resource);
    ASSERT_EQ(*resource, 42);
}
