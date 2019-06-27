#!/usr/bin/env python
# -*- coding: utf-8 -*-
from conans import ConanFile


class EnttConan(ConanFile):
    name = "entt"
    description = "Gaming meets modern C++ - a fast and reliable entity-component system (ECS) and much more "
    topics = ("conan," "entt", "gaming", "entity", "ecs")
    url = "https://github.com/skypjack/entt"
    homepage = url
    author = "Michele Caini <michele.caini@gmail.com>"
    license = "MIT"
    exports = ["LICENSE"]
    exports_sources = ["src/*"]
    no_copy_source = True

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses")
        self.copy(pattern="*", dst="include", src="src", keep_path=True)

    def package_info(self):
        if not self.in_local_cache:
            self.cpp_info.includedirs = ["src"]

    def package_id(self):
        self.info.header_only()
