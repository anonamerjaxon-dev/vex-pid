#!/usr/bin/env bash
# Fetches the real PROS headers, so tools/syntax_check.sh can check this
# project against the real API instead of against stand-ins.
#
# It downloads the PROS kernel source for one tag and keeps just its include/
# directory, in tools/.pros_headers/ (which is git-ignored - it is somebody
# else's source, and it is large). Nothing here touches the robot or the build.
#
# Why this exists: a syntax check is only worth anything if the headers it
# checks against are the real ones. A hand-written stand-in built by reading
# this project's code cannot catch a wrong API name.
set -euo pipefail
cd "$(dirname "$0")"

VERSION="${1:-4.1.0}"
DEST=".pros_headers"
URL="https://github.com/purduesigbots/pros/archive/refs/tags/${VERSION}.tar.gz"

mkdir -p "$DEST"
TARBALL="$DEST/pros-${VERSION}.tar.gz"

echo "Downloading PROS ${VERSION} sources..."
echo "  $URL"
curl -fSL --max-time 300 "$URL" -o "$TARBALL"

echo "Unpacking..."
tar xzf "$TARBALL" -C "$DEST"

rm -rf "$DEST/include"
mv "$DEST/pros-${VERSION}/include" "$DEST/include"
rm -rf "$DEST/pros-${VERSION}" "$TARBALL"

if [ ! -d "$DEST/include/pros" ]; then
    echo "Something went wrong: $DEST/include/pros is missing." >&2
    exit 1
fi

echo "Done. The real PROS ${VERSION} headers are in tools/$DEST/include."
echo "Now run: tools/syntax_check.sh"
