#!/usr/bin/env bash
set -euo pipefail

source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work="$source_root/deps/sdl2"
prefix=${1:-"$work/install"}
version=2.32.10
checksum=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
archive="$work/SDL2-$version.tar.gz"
mkdir -p "$work"
if [ ! -f "$archive" ]; then
    curl -fL --retry 3 --output "$archive" \
        "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-$version.tar.gz"
fi
printf '%s  %s\n' "$checksum" "$archive" | sha256sum --check -
tar -xzf "$archive" -C "$work"
# Carry the native IMU initialization fix until an SDL release includes it.
git -C "$work/SDL2-$version" apply --no-index \
    "$source_root/scripts/patches/sdl2-steam-deck-enable-imu.patch"
cmake -S "$work/SDL2-$version" -B "$work/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR=lib \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF -DSDL_HIDAPI=ON
cmake --build "$work/build" --parallel "${SDL2_BUILD_JOBS:-4}"
cmake --install "$work/build"
PKG_CONFIG_PATH="$prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
    pkg-config --exact-version="$version" sdl2
