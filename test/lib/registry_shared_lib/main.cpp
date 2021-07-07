
#ifndef ENTT_API_IMPORT
#error "ENTT_API_IMPORT must be defined when using entt as a shared library"
#endif

#ifndef PLUGIN_PATH
#error "PLUGIN_PATH must be defined for this test"
#endif

#include "api.h"

#include <gtest/gtest.h>

#if _WIN32
#include <Windows.h>
#else
#include <dlfcn.h> //dlopen
#endif

//#include <filesystem>

struct zero_component {};

/**
 * This test will fail if entt::internal::type_seq::next() was not linked
 * (imported/exported from shared registry). Two different versions will be used
 * by template specializations of entt::type_seq.
 *
 * This test will not fail (for a wrong reason) if linker has not imported or
 * optimized out imported entt symbols (will fail without optimization). Thus
 * both type ids will use ent::internal::type_seq::next::value generated within
 * the executable.
 */
TEST(shared_registry_lib, compare_ids_within_core) {
  EXPECT_EQ(entt::type_seq<zero_component>(), 0);
  ASSERT_EQ(entt::type_seq<test_increment>(), 1);
}

/**
 * This test will instantiate 1 internal component within the core and 2
 * components within a plugin. Returned id is expected to be equal to 2 if 3
 * instantiated components will indirectly use imported
 * entt::internal::type_seq.
 */
TEST(shared_registry_lib, compare_ids_across_modules) {
  EXPECT_EQ(entt::type_seq<zero_component>(), 0);

  //  auto pluginPath = std::filesystem::path(PLUGIN_PATH);
  //  ASSERT_TRUE(std::filesystem::exists(pluginPath));
  int (*pGetId)() = nullptr;

#if _WIN32
  //  auto handle = LoadLibrary(pluginPath.string().c_str());
  auto handle = LoadLibrary(PLUGIN_PATH);
  ASSERT_TRUE(handle);
  reinterpret_cast<void *&>(pGetId) = (void *)GetProcAddress(handle, "get_id");
#else
  //  auto handle = dlopen(pluginPath.string().c_str(), RTLD_LAZY);
  auto handle = dlopen(PLUGIN_PATH, RTLD_LAZY);
  ASSERT_TRUE(handle);
  reinterpret_cast<void *&>(pGetId) = (void *)dlsym(handle, "get_id");
#endif

  ASSERT_TRUE(pGetId);
  EXPECT_EQ(pGetId(), 2);

#if _WIN32
  FreeLibrary(handle);
#else
  dlclose(handle);
#endif
}

/**
 * This test *is supposed to* fail if symbols were not imported (including
 * linker optimization). In that case dynamically loaded plugin will fail to
 * find an entt with the registered test component: executable and plugin will
 * have their own versions for entt:internal::type_seq::next() symbols.
 */
TEST(shared_registry_lib, test_increment) {

  ASSERT_EQ(entt::type_seq<zero_component>(), 0);

  //  auto pluginPath = std::filesystem::path(PLUGIN_PATH);
  //  ASSERT_TRUE(std::filesystem::exists(pluginPath));

  entt::registry registry{};
  auto testEntt = registry.create();
  auto &component = registry.emplace<test_increment>(testEntt);

  void (*pIncrement)(entt::registry &) = nullptr;

#if _WIN32
  //  auto handle = LoadLibrary(pluginPath.string().c_str());
  auto handle = LoadLibrary(PLUGIN_PATH);
  ASSERT_TRUE(handle);
  reinterpret_cast<void *&>(pIncrement) =
      (void *)GetProcAddress(handle, "increment");
#else
  //  auto handle = dlopen(pluginPath.string().c_str(), RTLD_LAZY);
  auto handle = dlopen(PLUGIN_PATH, RTLD_LAZY);
  ASSERT_TRUE(handle);
  reinterpret_cast<void *&>(pIncrement) = (void *)dlsym(handle, "increment");
#endif

  ASSERT_TRUE(pIncrement);
  pIncrement(registry);
  EXPECT_EQ(component.i, 1);

#if _WIN32
  FreeLibrary(handle);
#else
  dlclose(handle);
#endif
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}