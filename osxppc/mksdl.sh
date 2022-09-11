#!/bin/bash

set -e
set -x

REPODIR="sdl-2.0.3"
BUILDDIR="build-osxppc"
CC="${CC:-/gcc/7.3.0/bin/gcc-7}"
CXX="${CXX:-/gcc/7.3.0/bin/g++-7}"
CPU=G3

if [[ ! -d "$REPODIR" ]]; then
	git clone --depth 1 --branch release-2.0.3 https://github.com/libsdl-org/SDL "$REPODIR"
fi

cd "$REPODIR"
git reset --hard release-2.0.3
git apply < ../sdl-tiger.patch

mkdir -p "$BUILDDIR"
cd "$BUILDDIR"

CC=${CC} CXX=${CXX} CFLAGS="-mcpu=${CPU}" CXXFLAGS="-mcpu=${CPU}" LDFLAGS="-mcpu=${CPU}" \
	../configure --without-x --disable-joystick --disable-haptic --disable-altivec
make
cp build/.libs/libSDL2.a ../../

