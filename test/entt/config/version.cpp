#include <regex>
#include <gtest/gtest.h>
#include <entt/config/version.h>

TEST(Version, All) {
    ASSERT_STREQ(ENTT_VERSION, ENTT_XSTR(ENTT_VERSION_MAJOR) "." ENTT_XSTR(ENTT_VERSION_MINOR) "." ENTT_XSTR(ENTT_VERSION_PATCH));
    ASSERT_TRUE(std::regex_match(ENTT_VERSION, std::regex{"^[0-9]+\\.[0-9]+\\.[0-9]+$"}));
}
