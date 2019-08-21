_msvc_copts = ["/std:c++17"]
_gcc_copts = ["-std=c++17"]

entt_copts = select({
    # Windows
    "@bazel_tools//src/conditions:windows": _msvc_copts,
    "@bazel_tools//src/conditions:windows_msvc": _msvc_copts,
    "@bazel_tools//src/conditions:windows_msys": _msvc_copts,
    "//conditions:default": _gcc_copts,
})
