Project files of LDD64. Build instructions and library versions: README.md at the top of the
repository.

  VS2022/LDD.sln          Visual Studio 2022 (v143), x64: LDD64.exe + zlib, libpng, libjpeg, expat,
                          LuaPlus, qhull, Vortex. Output src/Products/LDD/Bin/x64/<config>/
  XCode/LDD64.xcodeproj   Xcode 13+, x86_64 + arm64, macOS 10.13+: "LEGO Digital Designer.app" +
                          expat, LuaPlus, qhull, Vortex, Quick Look generator and extensions,
                          Spotlight importer. Build the "LEGO Digital Designer" scheme.
  Linux/CMakeLists.txt    CMake 3.16+, GTK 3: ldd64.
  Config/                 libjpeg / libpng configuration headers of the Windows and Linux builds.

Before the first Windows build (on Linux optional, for the cursors), extract the original cursors
and manifest (LEGO artwork, not in the repository) from an installed original LDD.exe:
  python tools\extract_rsrc.py <path to LDD.exe> src\Products\LDD\Resources\Win\Extracted

The programs need the original data (Assets.lif, Sound.lif; on Windows also the brick database the
original installs into %APPDATA%): run them in or next to an original installation.
