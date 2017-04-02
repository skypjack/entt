#!/usr/bin/env bash

USER_PATH_DIR=~/bin
TEMP_DIR=~/.cmake
mkdir -p ${USER_PATH_DIR}
mkdir -p ${TEMP_DIR}

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    curl -s https://cmake.org/files/v3.6/cmake-3.6.2-Linux-x86_64.tar.gz | tar -xz -C ${TEMP_DIR}
    ln -s ${TEMP_DIR}/cmake-3.6.2-Linux-x86_64/bin/cmake ${USER_PATH_DIR}
fi

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
    curl -s https://cmake.org/files/v3.6/cmake-3.6.2-Darwin-x86_64.tar.gz | tar -xz -C ${TEMP_DIR} 
    "${TEMP_DIR}/cmake-3.6.2-Darwin-x86_64/CMake.app/Contents/bin/cmake-gui" --install=${USER_PATH_DIR}
fi
