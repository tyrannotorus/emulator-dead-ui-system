#!/bin/sh
# Libraries the Brick has and the toolchain's sysroot has not, pulled over adb so a build can link against
# them, into TG5040_DEVLIBS (default build-brick/devlibs), run from the project's root. Never shipped: the
# device has them.
#   devlibs.sh SERIAL        (adb devices -l)
set -e
SERIAL=${1:?usage: devlibs.sh SERIAL}
OUT=${TG5040_DEVLIBS:-build-brick/devlibs}
mkdir -p "$OUT"
# libcurl for downloads, libvulkan for recomp modules
for lib in libcurl.so.4 libvulkan.so.1; do
    adb -s "$SERIAL" pull /usr/lib/$lib $OUT/$lib > /dev/null
    echo "devlibs: $OUT/$lib ($(wc -c < $OUT/$lib) bytes)"
done
