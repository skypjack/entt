#!/bin/bash

set -e
set -x

conan user
python conan/build.py
