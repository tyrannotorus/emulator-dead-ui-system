#!/bin/sh
# Draws fonts/label-cells-*.bin, the small text's characters, from fonts/NotoSans-Bold.ttf.zip. Run this after changing the
# font, or the sizes or colours in make-label-cells.cpp; the files are kept in git and built into the library.
#   tools/make-label-cells.sh [FREETYPE_SOURCE]      (default third_party/freetype; needs clang++, CMake, Ninja, Python 3)
set -e
cd "$(dirname "$0")/.."
FT=${1:-third_party/freetype}
[ -f "$FT/CMakeLists.txt" ] || { echo "no FreeType source at $FT"; exit 2; }
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
python3 -I -m zipfile -e fonts/NotoSans-Bold.ttf.zip "$T"
cmake -S "$FT" -B "$T/ft" -G Ninja -DCMAKE_BUILD_TYPE=Release -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_HARFBUZZ=ON \
    -DFT_DISABLE_BROTLI=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_ZLIB=ON > /dev/null
cmake --build "$T/ft" > /dev/null
clang++ -std=c++17 -O1 -Isrc -I"$FT/include" tools/make-label-cells.cpp src/canvas.cpp "$T/ft/libfreetype.a" -o "$T/make"
"$T/make" "$T/NotoSans-Bold.ttf" fonts
ls -l fonts/label-cells-*.bin
