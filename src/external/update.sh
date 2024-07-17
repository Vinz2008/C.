#!/bin/bash
set -ex

LLVM_VERSION=18.x

DOWNLOAD_PROGRAM=curl

$DOWNLOAD_PROGRAM https://raw.githubusercontent.com/llvm/llvm-project/release/$LLVM_VERSION/clang/tools/driver/cc1_main.cpp -o cc1_main.cpp

$DOWNLOAD_PROGRAM https://raw.githubusercontent.com/llvm/llvm-project/release/$LLVM_VERSION/clang/tools/driver/cc1as_main.cpp -o cc1as_main.cpp

$DOWNLOAD_PROGRAM https://raw.githubusercontent.com/llvm/llvm-project/release/$LLVM_VERSION/clang/tools/driver/cc1gen_reproducer_main.cpp -o cc1gen_reproducer_main.cpp

$DOWNLOAD_PROGRAM https://raw.githubusercontent.com/llvm/llvm-project/release/$LLVM_VERSION/clang/tools/driver/driver.cpp -o driver.cpp

$DOWNLOAD_PROGRAM https://raw.githubusercontent.com/llvm/llvm-project/release/$LLVM_VERSION/llvm/tools/llvm-ar/llvm-ar.cpp -o llvm-ar.cpp

git apply driver.patch