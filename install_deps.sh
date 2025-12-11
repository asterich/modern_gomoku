#!/bin/bash

set -e

install_xmake() {
    if ! command -v xmake &> /dev/null; then
        echo "xmake not found. Installing..."
        curl -fsSL https://xmake.io/shget.text | bash
        source ~/.xmake/profile
    else
        echo "xmake is already installed."
    fi
}

install_clang() {
    echo "Checking Clang version..."
    local install_needed=false
    local target_dir="$(pwd)/toolchains/llvm"
    local clang_bin="$target_dir/bin/clang++"

    if command -v clang++ &> /dev/null; then
        local ver=$(clang++ --version | head -n1 | grep -oP 'clang version \K[0-9]+')
        if [ -z "$ver" ]; then
             ver=$(clang++ --version | head -n1 | awk '{print $3}' | cut -d. -f1)
        fi
        echo "Detected System Clang version: $ver"
        if [ "$ver" -ge 18 ]; then
            echo "System Clang is sufficient."
            return
        fi
    fi

    if [ -f "$clang_bin" ]; then
        local ver=$("$clang_bin" --version | head -n1 | grep -oP 'clang version \K[0-9]+')
        if [ -z "$ver" ]; then
             ver=$("$clang_bin" --version | head -n1 | awk '{print $3}' | cut -d. -f1)
        fi
        echo "Detected Local Clang version: $ver"
        if [ "$ver" -ge 18 ]; then
            echo "Local Clang is sufficient."
            echo "To use it, run: xmake f --toolchain=llvm --sdk=$target_dir"
            return
        fi
    fi

    echo "Installing Clang 20 locally..."
    mkdir -p toolchains
    
    local url="https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.0/LLVM-20.1.0-Linux-X64.tar.xz"
    local tarball="clang.tar.xz"

    echo "Downloading from $url..."
    curl -L "$url" -o "$tarball"

    echo "Extracting..."
    mkdir -p "$target_dir"
    tar -xJf "$tarball" -C "$target_dir" --strip-components=1
    
    rm "$tarball"
    
    echo "Clang installed to $target_dir"
    echo "To configure xmake to use this toolchain, run:"
    echo "  xmake f --toolchain=llvm --sdk=$target_dir"
}

install_xmake
install_clang

echo "Installation check complete."
