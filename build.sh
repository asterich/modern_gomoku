#!/bin/bash

set -e

bash ./install_deps.sh

sdk_dir="$(pwd)/toolchains/llvm"

if [ ! -d "$sdk_dir" ]; then
    echo "Using system Clang toolchain"
    sdk_dir=""
fi

sdk_option=""
if [ -n "$sdk_dir" ]; then
    sdk_option="--sdk=$sdk_dir"
fi

xmake f --mode=release --toolchain=llvm $sdk_option
xmake build