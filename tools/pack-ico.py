#!/usr/bin/env python3
"""Pack one or more PNG files into a Windows .ico (PNG-payload format).

Each input PNG becomes one entry in the ICO directory. PNG-payload
ICOs are supported by Windows Vista and later — the canonical way to
ship multi-resolution app icons (including 256×256) without
exploding file size or losing alpha.

ImageMagick or Pillow would do the same, but both are heavy
dependencies that don't ship by default on macOS / mingw / CI; this
script uses only the Python standard library.

Usage: pack-ico.py output.ico in1.png in2.png ...
"""
import struct
import sys


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(f"usage: {sys.argv[0]} OUT.ico PNG [PNG ...]")
    out_path, *png_paths = sys.argv[1:]

    images = []
    for p in png_paths:
        with open(p, "rb") as f:
            data = f.read()
        if data[:8] != b"\x89PNG\r\n\x1a\n":
            sys.exit(f"{p}: not a PNG file")
        # IHDR width/height live at bytes 16..24 (big-endian u32 each).
        w, h = struct.unpack(">II", data[16:24])
        # The ICO directory records dimensions in a single byte, so
        # 256 is encoded as 0 and any value >= 256 must use the 0
        # sentinel (resolved by the embedded PNG itself at decode
        # time).
        ico_w = 0 if w >= 256 else w
        ico_h = 0 if h >= 256 else h
        images.append((ico_w, ico_h, data))

    # ICONDIR header: reserved=0, type=1 (icon, not cursor), count.
    header = struct.pack("<HHH", 0, 1, len(images))
    # Each ICONDIRENTRY is 16 bytes; image payloads start right after
    # the directory ends.
    payload_offset = 6 + 16 * len(images)
    entries = b""
    payload = b""
    for w, h, data in images:
        entries += struct.pack(
            "<BBBBHHII",
            w, h,
            0,        # color count (palette size, 0 = no palette)
            0,        # reserved
            1,        # color planes (1 for icons)
            32,       # bits per pixel
            len(data),
            payload_offset,
        )
        payload += data
        payload_offset += len(data)

    with open(out_path, "wb") as f:
        f.write(header + entries + payload)


if __name__ == "__main__":
    main()
