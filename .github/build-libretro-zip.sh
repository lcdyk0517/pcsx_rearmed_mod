#!/bin/bash
set -e

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <arch> <platform> [make_args]"
  echo "  arch: arm32, arm64"
  echo "  platform: CortexA73_G12B, rpi1, rpi2, rpi3, rpi4, armv7-neon, classic_armv7_a7, classic_armv8_a35, miyoo"
  exit 1
fi

arch=$1
plat=$2
shift 2

# Parse BUILTIN_GPU from extra args
gpu="neon"
for arg in "$@"; do
  if [[ $arg == BUILTIN_GPU=* ]]; then
    gpu="${arg#BUILTIN_GPU=}"
  fi
done

if [[ $arch = "arm32" ]]; then
  export CC=arm-linux-gnueabihf-gcc
  export CXX=arm-linux-gnueabihf-g++
  export LD=arm-linux-gnueabihf-ld
elif [[ $arch = "arm64" ]]; then
  export CC=aarch64-linux-gnu-gcc
  export CXX=aarch64-linux-gnu-g++
  export LD=aarch64-linux-gnu-ld
else
  echo "unsupported arch: $arch"
  exit 1
fi

echo "Building: arch=$arch platform=$plat gpu=$gpu"

make -f Makefile.libretro platform=$plat "$@" -j$(nproc)
zip -9 "pcsx_rearmed_libretro_${arch}_${plat}_${gpu}.zip" pcsx_rearmed_libretro.so
make -f Makefile.libretro platform=$plat clean