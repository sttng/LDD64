# LDD64 app icon (macOS)

A shaded red 2x2 brick on a yellow rounded-square background, in the same camera pose as the
original LDD icon (fitted to its outline: turned 39.5°, looking down 32.5°, with perspective).
Studs are 0.19 stud-pitch high with an embossed "LDD" label (Inter Bold, SIL Open Font License, converted to outlines). Drawn to Apple's current app icon guidelines:
https://developer.apple.com/design/human-interface-guidelines/app-icons

## Files

| Path | What it is |
|---|---|
| `../ldd64.icns` | Ready-to-use macOS icon, 16 px to 1024 px: the app icon of the Xcode project. |
| `ldd64.icon/` | Icon Composer bundle (background + 2 layers) for the layered Liquid Glass icon on macOS 26. **Untested, see below.** |
| `layers/1-brick.svg`, `layers/2-studs.svg` | The two foreground layers, 1024x1024, transparent, paths only. Back to front. |
| `fullbleed/` | Square, unmasked, opaque 1024 px masters (default and dark), SVG and PNG. |
| `ldd64.iconset/` | The ten PNGs inside the `.icns` (`iconutil -c icns ldd64.iconset` rebuilds it on a Mac). |
| `macos/` | SVG sources of the pre-Tahoe macOS grid (shape and shadow baked in), plus a dark 1024 px render. |
| `src/` | Scripts that generate every file here: `python3 src/pack3.py OUTDIR ldd64` (needs `rsvg-convert`, NumPy, fontTools and the Inter font). |

## How it follows the guidelines

- 1024x1024 square canvas, sRGB.
- Layered: a plain background plus two foreground layers (brick body, studs).
- Layers and full-bleed masters are square and unmasked, so the system applies the corner shape.
- Vector artwork with hard edges. The brick carries its own soft plastic shading and a ground shadow; Apple allows custom effects if used deliberately, and they are kept off the background layer.
- One centered object built from a few filled, overlapping shapes. The only text is the small embossed stud label; no thin lines; outer corners are slightly rounded.
- Default and dark variants keep identical artwork and only change the background.

Backgrounds: default `#FFE98A` to `#FFC233`, dark `#34353A` to `#141416` (top to bottom).

## Using it

**Any macOS version:** `../ldd64.icns` is the project's app icon (`CFBundleIconFile` `ldd64`).

A `.icns` is not masked by the system, so this file carries the shape itself, using Apple's macOS
template grid: an 824 px rounded square (corner radius 185.4) on the 1024 px canvas with a 28 px, 50 % black
shadow offset 12 px down. The 16, 32 and 64 px images use the template's tighter small-size margin
(1/16 of the canvas) and 128 px uses 12/128.

**Layered icon for macOS 26 (optional):** open `ldd64.icon` in Icon Composer, check the Default,
Dark and Mono previews, save, then add the `.icon` file to the Xcode project next to the sources
(not inside an asset catalog) and set the target's app icon name to `ldd64`. Keep `ldd64.icns` in
the project for older systems.

`ldd64.icon/icon.json` was written by hand from a community description of the format (Apple does
not publish it) and has not been opened in Icon Composer. If it is rejected: create a new Icon
Composer document, drag in `layers/2-studs.svg` and `layers/1-brick.svg` (studs above brick, one
group each) and choose a light background for Default and a dark one for Dark.
