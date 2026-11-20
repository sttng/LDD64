#!/usr/bin/env python3
"""Render the icon artwork and package it: iconset PNGs, .icns, Icon Composer bundle.

Usage: pack.py OUTDIR [NAME]
Needs rsvg-convert (librsvg) on PATH.
"""
import json
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen  # noqa: E402
import gen2  # noqa: E402
import gen3  # noqa: E402

out = sys.argv[1]
name = sys.argv[2] if len(sys.argv) > 2 else "ldd64"


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as fh:
        fh.write(text)


def render(svg_path, px, png_path):
    os.makedirs(os.path.dirname(png_path), exist_ok=True)
    subprocess.run(["rsvg-convert", "-w", str(px), "-h", str(px), svg_path, "-o", png_path], check=True)


# --- layers (for Icon Composer: no background, no baked ground shadow) -------------
d = gen2.Defs("b")
parts = gen3.build(d)
write(f"{out}/layers/1-brick.svg", gen2.fullbleed(parts["body"], d))
d = gen2.Defs("s")
parts = gen3.build(d)
write(f"{out}/layers/2-studs.svg", gen2.fullbleed(parts["studs"], d))

# --- full-bleed masters + macOS-grid sources ------------------------------------------
def inset_for(px):
    if px <= 64:
        return 1024 / 16
    if px == 128:
        return 1024 * 12 / 128
    return 100.0


for variant, bgname in (("default", "sun"), ("dark", "dark")):
    inner, defs = gen3.compose(bgname)
    p = f"{out}/fullbleed/{name}-1024-{variant}.svg"
    write(p, gen2.fullbleed(inner, defs))
    render(p, 1024, p[:-4] + ".png")
    for inset in sorted({inset_for(px) for px in (16, 128, 256)}):
        inner, defs = gen3.compose(bgname)
        write(f"{out}/macos/{name}-macos-{variant}-inset{int(inset)}.svg", gen2.macos_grid(inner, defs, inset))

iconset = [
    ("icon_16x16.png", 16), ("icon_16x16@2x.png", 32),
    ("icon_32x32.png", 32), ("icon_32x32@2x.png", 64),
    ("icon_128x128.png", 128), ("icon_128x128@2x.png", 256),
    ("icon_256x256.png", 256), ("icon_256x256@2x.png", 512),
    ("icon_512x512.png", 512), ("icon_512x512@2x.png", 1024),
]
for fn, px in iconset:
    render(f"{out}/macos/{name}-macos-default-inset{int(inset_for(px))}.svg", px, f"{out}/{name}.iconset/{fn}")
render(f"{out}/macos/{name}-macos-dark-inset100.svg", 1024, f"{out}/macos/{name}-macos-dark-1024.png")
bbox = None

# --- .icns ------------------------------------------------------------------
slots = [  # OSType, iconset file
    ("icp4", "icon_16x16.png"), ("icp5", "icon_32x32.png"),
    ("ic07", "icon_128x128.png"), ("ic08", "icon_256x256.png"),
    ("ic09", "icon_512x512.png"), ("ic10", "icon_512x512@2x.png"),
    ("ic11", "icon_16x16@2x.png"), ("ic12", "icon_32x32@2x.png"),
    ("ic13", "icon_128x128@2x.png"), ("ic14", "icon_256x256@2x.png"),
]
body = b""
for ostype, fn in slots:
    data = open(f"{out}/{name}.iconset/{fn}", "rb").read()
    body += ostype.encode() + struct.pack(">I", len(data) + 8) + data
with open(f"{out}/{name}.icns", "wb") as fh:
    fh.write(b"icns" + struct.pack(">I", len(body) + 8) + body)

# --- Icon Composer bundle ---------------------------------------------------
def group(label, image):
    return {
        "layers": [{"image-name": image, "name": label}],
        "name": label,
        "shadow": {"kind": "neutral", "opacity": 0.5},
        "translucency": {"enabled": False, "value": 0.5},
    }


doc = {
    "fill": {"linear-gradient": ["srgb:1.00000,0.91373,0.54118,1.00000", "srgb:1.00000,0.76078,0.20000,1.00000"]},
    "fill-specializations": [
        {"appearance": "dark", "value": {"automatic-gradient": "srgb:0.13725,0.13725,0.14118,1.00000"}},
    ],
    "groups": [group("Studs", "2-studs.svg"), group("Brick", "1-brick.svg")],  # front to back
    "supported-platforms": {"squares": ["macOS"]},
}
write(f"{out}/{name}.icon/icon.json", json.dumps(doc, indent=2) + "\n")
import shutil
os.makedirs(f"{out}/{name}.icon/Assets", exist_ok=True)
shutil.copy(f"{out}/layers/1-brick.svg", f"{out}/{name}.icon/Assets/1-brick.svg")
shutil.copy(f"{out}/layers/2-studs.svg", f"{out}/{name}.icon/Assets/2-studs.svg")
print("done", out)
