export namespace entt {
using ::entt::meta_function_descriptor_traits;
using ::entt::meta_function_descriptor;
using ::entt::meta_function_helper;
using ::entt::meta_function_helper_t;
using ::entt::meta_dispatch;

// TODO: I cannot export ::entt::meta_args because the function is declared as static. Is this a problem?
// error: using declaration referring to 'meta_arg' with internal linkage cannot be exported
// using ::entt::meta_arg;
using ::entt::meta_setter;
using ::entt::meta_getter;
using ::entt::meta_invoke;
using ::entt::meta_construct;
} // namespace entt