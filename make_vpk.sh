#!/usr/bin/env bash
set -euo pipefail

VPK_TITLE="Bugdom"
VPK_APPID="BGDM00099"

if [ $# -ne 1 ]
then
    echo "Usage: $0 ROOT_DIR"
    exit 1
fi

ROOT_DIR="$1"
BUILD_DIR="$ROOT_DIR/build"
VPK_DIR="$BUILD_DIR/vpk_build"

if ! [[ -d "$ROOT_DIR" ]]
then
    echo "Directory '$ROOT_DIR' does not exist."
    exit 1
fi
if ! [[ -d "$BUILD_DIR" ]]
then
    echo "Directory '$BUILD_DIR' does not exist."
    exit 1
fi

# copy files into new folder in build directory
mkdir -p "$VPK_DIR"
cp -r "$ROOT_DIR/sce_sys" "$VPK_DIR/"

vita-mksfoex -s TITLE_ID="${VPK_APPID}" "${VPK_TITLE}" "$VPK_DIR/sce_sys/param.sfo"
7z a -tzip "$BUILD_DIR/${VPK_TITLE}.vpk" -r "$VPK_DIR/*" "$BUILD_DIR/eboot.bin" "$BUILD_DIR/Data" "$BUILD_DIR/ReadMe.txt"
