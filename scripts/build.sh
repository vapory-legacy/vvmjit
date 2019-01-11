#!/bin/sh
set -x -e

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DVVMJIT_EXAMPLES=On -DVVMJIT_TESTS=On ..
cmake --build .
cmake --build . --target vvmjit-standalone
cmake --build . --target example-capi
