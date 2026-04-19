#!/bin/sh
# Regenerate platform icons from assets/icon.svg.
#
# Run from the repo root after editing the SVG. The outputs (.ico /
# .icns) are committed binaries — the build doesn't run this script,
# so contributors don't need rsvg-convert / iconutil installed just
# to compile.
#
# Requires: rsvg-convert (brew install librsvg), iconutil (macOS
# only, system tool), python3 (system).

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SVG=$ROOT/assets/icon.svg
OUT_ICO=$ROOT/assets/icon.ico
OUT_ICNS=$ROOT/assets/icon.icns

if ! command -v rsvg-convert >/dev/null; then
    echo "error: rsvg-convert not found (try: brew install librsvg)" >&2
    exit 1
fi
if ! command -v iconutil >/dev/null; then
    echo "error: iconutil not found (macOS-only system tool)" >&2
    exit 1
fi

WORK=$(mktemp -d)
trap "rm -rf $WORK" EXIT

# Render the sizes both formats need. Windows wants 16/32/48/256 at
# minimum (we add 20/40/64 for high-DPI Explorer and taskbar);
# macOS .icns wants 16/32/64/128/256/512/1024 with retina @2x copies
# (so 32 = 16@2x, 64 = 32@2x, etc.).
for s in 16 20 32 40 48 64 128 256 512 1024; do
    rsvg-convert -w "$s" -h "$s" "$SVG" -o "$WORK/icon-$s.png"
done

# --- macOS .icns ---
ICONSET=$WORK/icon.iconset
mkdir -p "$ICONSET"
cp "$WORK/icon-16.png"   "$ICONSET/icon_16x16.png"
cp "$WORK/icon-32.png"   "$ICONSET/icon_16x16@2x.png"
cp "$WORK/icon-32.png"   "$ICONSET/icon_32x32.png"
cp "$WORK/icon-64.png"   "$ICONSET/icon_32x32@2x.png"
cp "$WORK/icon-128.png"  "$ICONSET/icon_128x128.png"
cp "$WORK/icon-256.png"  "$ICONSET/icon_128x128@2x.png"
cp "$WORK/icon-256.png"  "$ICONSET/icon_256x256.png"
cp "$WORK/icon-512.png"  "$ICONSET/icon_256x256@2x.png"
cp "$WORK/icon-512.png"  "$ICONSET/icon_512x512.png"
cp "$WORK/icon-1024.png" "$ICONSET/icon_512x512@2x.png"
iconutil -c icns -o "$OUT_ICNS" "$ICONSET"
echo "wrote $OUT_ICNS"

# --- Windows .ico (PNG-payload format, Vista+) ---
python3 "$ROOT/tools/pack-ico.py" "$OUT_ICO" \
    "$WORK/icon-16.png"  "$WORK/icon-20.png" \
    "$WORK/icon-32.png"  "$WORK/icon-40.png" \
    "$WORK/icon-48.png"  "$WORK/icon-64.png" \
    "$WORK/icon-256.png"
echo "wrote $OUT_ICO"
