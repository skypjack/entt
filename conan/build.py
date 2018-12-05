#!/usr/bin/env python
# -*- coding: utf-8 -*-
from cpt.packager import ConanMultiPackager
import os

if __name__ == "__main__":
    login_username = os.getenv("CONAN_LOGIN_USERNAME")
    username = os.getenv("CONAN_USERNAME")
    tag_version = os.getenv("CONAN_PACKAGE_VERSION", os.getenv("TRAVIS_TAG"))
    package_version = tag_version.replace("v", "")
    package_name_unset = "SET-CONAN_PACKAGE_NAME-OR-CONAN_REFERENCE"
    package_name = os.getenv("CONAN_PACKAGE_NAME", package_name_unset)
    reference = "{}/{}".format(package_name, package_version)
    channel = os.getenv("CONAN_CHANNEL", "stable")
    upload = os.getenv("CONAN_UPLOAD")
    stable_branch_pattern = os.getenv("CONAN_STABLE_BRANCH_PATTERN", r"v\d+\.\d+\.\d+.*")
    test_folder = os.getenv("CPT_TEST_FOLDER", os.path.join("conan", "test_package"))
    upload_only_when_stable = os.getenv("CONAN_UPLOAD_ONLY_WHEN_STABLE", True)
    header_only = os.getenv("CONAN_HEADER_ONLY", False)
    pure_c = os.getenv("CONAN_PURE_C", False)

    disable_shared = os.getenv("CONAN_DISABLE_SHARED_BUILD", "False")
    if disable_shared == "True" and package_name == package_name_unset:
        raise Exception("CONAN_DISABLE_SHARED_BUILD: True is only supported when you define CONAN_PACKAGE_NAME")

    builder = ConanMultiPackager(username=username,
                                 reference=reference,
                                 channel=channel,
                                 login_username=login_username,
                                 upload=upload,
                                 stable_branch_pattern=stable_branch_pattern,
                                 upload_only_when_stable=upload_only_when_stable,
                                 test_folder=test_folder)
    if header_only == "False":
        builder.add_common_builds(pure_c=pure_c)
    else:
        builder.add()

    filtered_builds = []
    for settings, options, env_vars, build_requires, reference in builder.items:
        if disable_shared == "False" or not options["{}:shared".format(package_name)]:
             filtered_builds.append([settings, options, env_vars, build_requires])
    builder.builds = filtered_builds

    builder.run()
