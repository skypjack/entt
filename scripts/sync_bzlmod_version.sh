#!/usr/bin/env bash

set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
VERSION_HEADER=$(realpath "$SCRIPT_DIR/../src/entt/config/version.h" --relative-to=$(pwd))
BAZEL_MODULE=$(realpath "$SCRIPT_DIR/../MODULE.bazel" --relative-to=$(pwd))

if [[ -z "${VERSION_HEADER}" ]]; then
	echo "Cannot find version header"
	exit 1
fi

echo "Getting version from $VERSION_HEADER ..."

ENTT_MAJOR_VERSION=$(sed -nr 's/#define ENTT_VERSION_MAJOR ([0-9]+)/\1/p' $VERSION_HEADER)
ENTT_MINOR_VERSION=$(sed -nr 's/#define ENTT_VERSION_MINOR ([0-9]+)/\1/p' $VERSION_HEADER)
ENTT_PATCH_VERSION=$(sed -nr 's/#define ENTT_VERSION_PATCH ([0-9]+)/\1/p' $VERSION_HEADER)

VERSION="$ENTT_MAJOR_VERSION.$ENTT_MINOR_VERSION.$ENTT_PATCH_VERSION"

echo "Found $VERSION"

buildozer "set version $VERSION" //MODULE.bazel:%module

# a commit is needed for 'git archive'
git add $BAZEL_MODULE
git commit -m "chore: update MODULE.bazel version to $VERSION"
