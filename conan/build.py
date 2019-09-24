#!/usr/bin/env python
# -*- coding: utf-8 -*-
from cpt.packager import ConanMultiPackager
import os

if __name__ == "__main__":
    username = os.getenv("GITHUB_ACTOR")
    tag_version = os.getenv("GITHUB_REF")
    tag_package = os.getenv("GITHUB_REPOSITORY")
    login_username = os.getenv("CONAN_LOGIN_USERNAME")
    package_version = tag_version.replace("refs/tags/v", "")
    package_name = tag_package.replace("skypjack/", "")
    reference = "{}/{}".format(package_name, package_version)
    channel = os.getenv("CONAN_CHANNEL", "stable")
    upload = os.getenv("CONAN_UPLOAD")
    stable_branch_pattern = os.getenv("CONAN_STABLE_BRANCH_PATTERN", r"v\d+\.\d+\.\d+.*")
    test_folder = os.getenv("CPT_TEST_FOLDER", os.path.join("conan", "test_package"))
    upload_only_when_stable = os.getenv("CONAN_UPLOAD_ONLY_WHEN_STABLE", True)
    disable_shared = os.getenv("CONAN_DISABLE_SHARED_BUILD", "False")

    builder = ConanMultiPackager(username=username,
                                 reference=reference,
                                 channel=channel,
                                 login_username=login_username,
                                 upload=upload,
                                 stable_branch_pattern=stable_branch_pattern,
                                 upload_only_when_stable=upload_only_when_stable,
                                 test_folder=test_folder)
    builder.add()

    filtered_builds = []
    for settings, options, env_vars, build_requires, reference in builder.items:
        if disable_shared == "False" or not options["{}:shared".format(package_name)]:
             filtered_builds.append([settings, options, env_vars, build_requires])
    builder.builds = filtered_builds

    builder.run()
