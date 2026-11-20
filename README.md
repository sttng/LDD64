# LDD64: LEGO Digital Designer 64bit version

The restored C++ source of LEGO Digital Designer 4.3.11 with current third-party libraries,
building a 64-bit program for **Windows**, **macOS** and **Linux** (Wayland and X11).

LDD64 is version **4.64.0** on every platform. The About box shows "4.64", and saved models carry
`versionMajor="4" versionMinor="64"`, which LDD 4.3.11 ignores when loading.

| | Windows | macOS | Linux |
|---|---|---|---|
| Project (`src/Products/LDD/Projects/LDD/`) | `VS2022/LDD.sln` | `XCode/LDD64.xcodeproj` | `Linux/CMakeLists.txt` |
| Toolchain | Visual Studio 2022 (v143) or later | Xcode 13 or later | GCC or Clang, CMake 3.16+, GTK 3 |
| Platforms | x64 | x86_64 + arm64, macOS 10.13+ (arm64: 11+) | x86_64, arm64 |
| Output | `src/Products/LDD/Bin/x64/<config>/LDD64.exe` | `LEGO Digital Designer.app` | `<build>/ldd64` |

The Xcode project also builds the Quick Look generator, the Quick Look extensions and the
Spotlight importer.

## Third-party libraries

`thirdparty/` holds only the files the projects compile, each library with its licence.

| Library | Version | Used on | Notes |
|---|---|---|---|
| zlib | 1.4 | Windows, Linux | macOS uses the system zlib |
| libpng | 1.6.59 | Windows, Linux | macOS uses ImageIO |
| IJG libjpeg | 10 | Windows, Linux | macOS uses ImageIO |
| Expat | 2.10.0 | all | configured as LDD 4.3.11 was (namespaces, DTDs) |
| Lua | 5.1.5 | all | the LuaPlus build 1100 core with the 5.1.2–5.1.5 fixes; LDD's scripts need Lua 5.1 |
| Qhull | 2020.2 | all | `libqhull` with CM Labs' Vortex changes (marked `VORTEX:`) |
| Boost | 1.92.0 | all | the headers Vortex needs, replacing its private Boost 1.42 |
| CM Labs Vortex | 5.0.0 (restored) | all | restored from LDD 4.3.11; see its `README.md` |

Differences that come from the updates:
* **Expat:**
  * A file with an empty root element reports its error once instead of twice.
  * An `ldraw.xml` of an unsupported version with an empty root is now rejected.
  * Invalid UTF-8 in element or attribute names is rejected (CVE-2016-0718).
* **Lua 5.1.5:** the 5.1 bug fixes (`string.format`, `unpack`, `string.byte`/`sub` range checks,
  and others). LDD's own scripts behave the same.
* **Qhull:** convex hulls number their vertices differently. Volumes are the same; distances can
  differ in the last bits.
* **libjpeg 10:** decoded channel values differ by at most 1 from libjpeg 6b.
* **libpng 1.6:** reads a PNG whose only fault is a damaged `IEND` checksum.
* **libpng and zlib:** written files have different compressed bytes, and some hard-to-compress
  zip entries are written deflated instead of stored. The files are valid either way.

## Building

**Windows.** The cursors and manifest that `LDD.rc` names are LEGO artwork. Extract them once from an installed LDD 4.3.11 (Python 3, `pip install pefile`):
```
python tools\extract_rsrc.py "C:\Program Files (x86)\LEGO Company\LEGO Digital Designer\LDD.exe" src\Products\LDD\Resources\Win\Extracted
```
Then build `VS2022\LDD.sln` as `Release` / `x64`.

**Linux.** The Linux build is the Mac branch of the code with a GTK 3 platform layer
(`src/DCL/Source/Platform/Linux`):
* OpenGL in a GtkGLArea, Pango text, GStreamer for sound, XDG folders.
* Menus, short-cuts and mouse buttons follow the Windows build.

On Debian, Ubuntu or Mint:
```
sudo apt install build-essential cmake ninja-build pkg-config libgtk-3-dev libgl-dev libglu1-mesa-dev libgstreamer1.0-dev
cmake -S src/Products/LDD/Projects/LDD/Linux -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux
```
* **Cursors:** extract them first as on Windows (`python3 tools/extract_rsrc.py …`); otherwise LDD
  shows the standard arrow.
* **Sound:** without GStreamer's development files the build is silent.
* **Install:** `cmake --install build/linux` puts the program in `<prefix>/lib/ldd64`, with a
  launcher and the `.lxf` / `.lxfml` file types.

**macOS.** Build the `LEGO Digital Designer` scheme of `XCode/LDD64.xcodeproj`; it builds the
libraries and plug-ins with the app, as Release. It is signed ad hoc. If the Quick Look
extensions don't load, set a team (a free Personal Team is enough) for the app and the
`LXFPreview` / `LXFThumbnail` targets.

The application and model-file icons on all platforms are LDD64's red brick (`ldd64.ico`,
`ldd64_document.ico`, and `Mac/ldd64.icns`, `Mac/ldd64_document.icns` in
`src/Products/LDD/Resources`). The macOS app icon puts it on a yellow tile, drawn to Apple's
app icon guidelines; its sources, including a layered Icon Composer version, are in `Mac/Icon`.

## Running

The program needs the data of an original LDD 4.3 installation, which is not in this repository:
* **Windows:** put `LDD64.exe` and the `ldraw.xml` built next to it in the installation folder,
  next to `Assets.lif`. The original `LDD.exe` can stay. The brick database comes from the
  original installer (in `%APPDATA%`).
* **macOS:** copy the original bundle's `Contents/Resources` data into the built bundle:
  `Assets.lif`, `Sound.lif`, `Help/`, `preferences.ini`, `Cursor*.png`. On another
  Mac, open the app once with Open from the context menu, or run
  `xattr -dr com.apple.quarantine "LEGO Digital Designer.app"`.
* **Linux:** copy `Assets.lif`, `Sound.lif`, `Help/` and `preferences.ini` next to `ldd64`. User
  data goes to `~/.local/share/LEGO Company/LEGO Digital Designer/`. To open a model, run
  `ldd64 model.lxf` or drop the file on the window.

**LDraw mapping.** `src/Products/LDD/Resources/ldraw.xml` is the community LDraw ↔ LEGO mapping
(2025-08-11, http://slswww.free.fr/ldraw.xml), much newer than the original's. The builds put it
next to the program (in the app bundle on macOS); it is used for LDraw import and export.

**Start screen and About box (optional).** `src/Products/LDD/Resources/Assets` holds the two
pictures at their paths in `Assets.lif`: `Graphics/StartScreen/start-screen.png` and
`Graphics/General/About window.png`. They have the red brick and "4.64" in the title, and the
About box credits list LDD64's libraries.

To use them, do one of these:
* Replace the files inside `Assets.lif`.
* Unpack `Assets.lif` into a folder `Assets` in its place and replace them there. LDD reads that
  folder when there is no `Assets.lif`; this has been tried on Linux.

## Settings

[docs/PREFERENCES.md](docs/PREFERENCES.md) lists every `preferences.ini` setting, LDD 4.3's and
LDD64's. For example, `DoServerCall=no` starts without the "Internet access failed" message,
since LEGO's servers are gone.

Settings can also be given on the command line, e.g. `-ViewDistance 5000`. LDD64's own settings
are off by default.

For large models:
```
ViewDistance=6000
ScaledWheelZoom=yes
FastNavigation=yes
```

## Changes from the original

LDD64 fixes the original's defects: checked file writes; checks of untrusted input (archives,
images, model files, server replies, Lua scripts) against out-of-bounds access, endless loops and
writes outside a folder; uninitialised members, leaks and missing checks. LuaPlus has two fixes.
The restored original code, without these changes, is kept in a separate repository.

For large models, LDD64 gives the same results (model, picks, pair states, building guide) much
faster. Timings are from the measurements in the commits; the building-guide times are Linux
with software OpenGL.

| Area | What changed |
|---|---|
| Picking | A spatial index for `Scene::Pick` (every connection test). It visits the same elements in the same order. |
| XML | Text is split in one pass. Reading a 131,072-brick model took over two minutes before. |
| Memory | Compact connections, lazy Vortex tables, pooled collision data, shared equal vertex buffers. 70,000 loose bricks peak at 2.16 GB instead of 2.51 GB. |
| 3D view | Meshes outside the view are skipped after the original's sort; material changes are batched. A frame inside 70,000 bricks takes 50 ms instead of 1 s. New settings: `ViewDistance`, `ScaledWheelZoom`, `FastNavigation`, level of detail. |
| Broad phase (Vortex) | No pair table that grows with the square of the objects; search trees for inserting objects; a part disables its geometries' pairs once. Loading 70,000 bricks takes about 30 s instead of 3 min. |
| Carrying, pasting | Large groups' boxes jump through the broad phase instead of being walked; faster snap searches. |
| Loading | Each inserted brick's test box moves through the search trees; a batch's accepted bricks form one pair group; palette bricks are looked up once per kind of brick. 262,144 bricks load in 40 s instead of 57 s. On the Mac, numbers are read and written without a CoreFoundation formatter per number. |
| Saving | The LXFML writer looks refIDs up instead of copying maps. Saving 524,288 bricks takes 35 s instead of about 5 min. |
| Building guide | Occlusion tests draw only bricks that can hide the brick; the main loop wakes for each test; substeps are found by hash; swept collision tests use the search trees. 16,384 bricks take 45 s instead of 326 s; 262,144 take 33 min instead of 4+ h. |

## Source conventions

* `PORT64` marks the changes for 64-bit builds.
* `NOTE` describes a defect of the original and its fix, or a deliberate deviation.
