Project files of the restored LEGO Digital Designer 4.3.11. Build instructions and library
versions: README.md at the top of the repository.

  VS2022/LDD.sln          Visual Studio 2022 (v143): LDD.exe + zlib, libpng, libjpeg, expat, LuaPlus,
                          qhull, Vortex. x86 (Win32) and x64. Output src/Products/LDD/Bin/<config>/LDD.exe
  XCode/LDD64.xcodeproj   Xcode 13+, x86_64 + arm64, macOS 10.13+: "LEGO Digital Designer.app" +
                          expat, LuaPlus, qhull, Vortex, Quick Look generator and extensions,
                          Spotlight importer.
  Config/                 libjpeg / libpng configuration headers of the Windows build.

Before the first Windows build, extract the original icons / cursors / manifest (LEGO artwork, not in
the repository) from an installed original LDD.exe:
  python tools\extract_rsrc.py <path to LDD.exe> src\Products\LDD\Resources\Win\Extracted

The programs need the original data (Assets.lif, Sound.lif; on Windows also the brick database the
original installs into %APPDATA%): run them in or next to an original installation.
