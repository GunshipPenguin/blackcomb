#!/usr/bin/env bash

set -euo pipefail

readonly BINUTILS_VER=2.40
readonly GCC_VER=12.2.0

readonly SRC_DIR=$(pwd)/cross-src
readonly PREFIX=$(pwd)/cross
readonly TARGET=x86_64-elf

build_binutils() {
    pushd $SRC_DIR
    mkdir -p build-binutils
    pushd build-binutils

    ../binutils-${BINUTILS_VER}/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror

    make -j$(nproc)
    make install -j$(nproc)

    popd
    popd
}

build_gcc() {
    pushd $SRC_DIR
    mkdir -p build-gcc
    pushd build-gcc
    ../gcc-${GCC_VER}/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers

    make -j$(nproc) all-gcc
    make -j$(nproc) all-target-libgcc
    make -j$(nproc) install-gcc
    make -j$(nproc) install-target-libgcc

    popd
    popd
}

download_source() {
    pushd $SRC_DIR
    wget https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.xz
    wget https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.gz
    tar -axvf binutils-${BINUTILS_VER}.tar.xz
    tar -axvf gcc-${GCC_VER}.tar.gz
    popd
}

main() {
    mkdir -p $SRC_DIR
    mkdir -p $PREFIX

    download_source
    build_binutils
    export PATH="$PREFIX/bin:$PATH"
    build_gcc
}

main
