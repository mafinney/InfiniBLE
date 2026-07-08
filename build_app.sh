#!/usr/bin/env bash
set -e

: "${ARM_NONE_EABI_TOOLCHAIN_PATH:?Please set ARM_NONE_EABI_TOOLCHAIN_PATH}"
: "${NRF5_SDK_PATH:?Please set NRF5_SDK_PATH}"

rm -rf build

cmake -S . -B build \
    -DARM_NONE_EABI_TOOLCHAIN_PATH="$ARM_NONE_EABI_TOOLCHAIN_PATH" \
    -DNRF5_SDK_PATH="$NRF5_SDK_PATH" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_DFU=1 \
    -DBUILD_RESOURCES=1 \
    -DTARGET_DEVICE=PINETIME

cmake --build build --target pinetime-mcuboot-app --parallel