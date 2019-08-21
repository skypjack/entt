workspace(name = "com_github_skypjack_entt")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-release-1.8.1",
    url = "https://github.com/google/googletest/archive/release-1.8.1.tar.gz",
    sha256 = "9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c",
)

http_archive(
    name = "org_duktape",
    url = "https://duktape.org/duktape-2.4.0.tar.xz",
    strip_prefix = "duktape-2.4.0",
    sha256 = "86a89307d1633b5cedb2c6e56dc86e92679fc34b05be551722d8cc69ab0771fc",
    build_file_content = """
cc_library(
    name = "duktape",
    visibility = ["//visibility:public"],
    strip_include_prefix = "src",
    hdrs = ["src/duktape.h", "src/duk_config.h"],
    srcs = ["src/duktape.c", "src/duktape.h", "src/duk_config.h"],
)
    """,
)

http_archive(
    name = "bazelregistry_cereal",
    strip_prefix = "cereal-8629f40d932d57c5337d4557327f6f22436211b7",
    url = "https://github.com/bazelregistry/cereal/archive/8629f40d932d57c5337d4557327f6f22436211b7.zip",
    sha256 = "c983a7a2e16b153c3de022a0818d2f4836e510a3fc3bea9d3703de79f58a90a6",
)

# This is for bazelregistry_cereal
http_archive(
    name = "bazelregistry_rapidjson",
    strip_prefix = "rapidjson-6b980984dacf689be8f65be823203b967c69da04",
    url = "https://github.com/bazelregistry/rapidjson/archive/6b980984dacf689be8f65be823203b967c69da04.zip",
    sha256 = "82187ba8de53bab3b4fb9e56d0bae81a96fa27a115e5a6ce14c70f0ef9338965",
)
