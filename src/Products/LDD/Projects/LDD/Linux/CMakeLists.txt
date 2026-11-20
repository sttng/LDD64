# LDD64 for Linux (Wayland and X11): the Mac branch of the code with the platform layer in
# src/DCL/Source/Platform/Linux, and the third-party libraries of the VS2022 / Xcode projects.
#
#   cmake -S src/Products/LDD/Projects/LDD/Linux -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/linux        -> build/linux/ldd64
#
# Packages (Debian / Ubuntu / Mint): build-essential cmake ninja-build pkg-config libgtk-3-dev
# libgl-dev libglu1-mesa-dev; libgstreamer1.0-dev for sound (optional).
cmake_minimum_required(VERSION 3.16)
project(LDD64Linux VERSION 4.64.0 LANGUAGES C CXX)

get_filename_component(LDD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE)
include(${CMAKE_CURRENT_SOURCE_DIR}/Sources.cmake)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# As the Xcode project: -Os, no floating-point contraction (the x64 builds' results), NDEBUG.
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
add_compile_options(-ffp-contract=off -w)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0 gtk+-unix-print-3.0 pangocairo)
# Sound (optional): GStreamer plays LDD's MP3 / WAV sounds; without it LDD is silent.
pkg_check_modules(GST IMPORTED_TARGET gstreamer-1.0)
set(OpenGL_GL_PREFERENCE LEGACY)   # libGL: it also exports the ARB / EXT entry points the Mac code calls
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(TP ${LDD_ROOT}/thirdparty)

add_library(ldd_zlib STATIC ${LDD_ZLIB_SOURCES})
target_include_directories(ldd_zlib PUBLIC ${TP}/zlib-1.4)
target_compile_definitions(ldd_zlib PRIVATE HAVE_UNISTD_H)

add_library(ldd_png STATIC ${LDD_LIBPNG_SOURCES})
target_include_directories(ldd_png PUBLIC ${LDD_ROOT}/src/Products/LDD/Projects/LDD/Config ${TP}/libpng-1.6.59)
target_link_libraries(ldd_png PUBLIC ldd_zlib)
target_link_libraries(ldd_png PRIVATE m)

add_library(ldd_jpeg STATIC ${LDD_LIBJPEG_SOURCES})
target_include_directories(ldd_jpeg PUBLIC ${LDD_ROOT}/src/Products/LDD/Projects/LDD/Config ${TP}/jpeg-10)

add_library(ldd_expat STATIC ${LDD_EXPAT_SOURCES})
target_include_directories(ldd_expat PUBLIC ${TP}/expat-2.10.0/lib)
target_compile_definitions(ldd_expat PUBLIC XML_STATIC PRIVATE HAVE_ARC4RANDOM_BUF)

add_library(ldd_luaplus STATIC ${LDD_LUAPLUS_SOURCES})
target_include_directories(ldd_luaplus PUBLIC ${TP}/luaplus51-build1100/Src/LuaPlus
                                              ${TP}/luaplus51-build1100/Src/LuaPlus/src)

add_library(ldd_qhull STATIC ${LDD_QHULL_SOURCES})
target_include_directories(ldd_qhull PUBLIC ${TP}/qhull-2020.2-vortex/src PRIVATE ${TP}/vortex-5.0.0-ldd41/include)
target_compile_definitions(ldd_qhull PRIVATE qh_VORTEX)
target_compile_options(ldd_qhull PRIVATE -fno-strict-aliasing)

add_library(ldd_vortex STATIC ${LDD_VORTEX_SOURCES})
target_include_directories(ldd_vortex PUBLIC ${TP}/vortex-5.0.0-ldd41/include ${TP}/boost-1.92.0/include
                                       PRIVATE ${TP}/vortex-5.0.0-ldd41/src ${TP}/qhull-2020.2-vortex/src)
target_compile_options(ldd_vortex PRIVATE -fno-strict-aliasing)
target_link_libraries(ldd_vortex PUBLIC ldd_qhull)

# The application: the Xcode target's sources (the .mm files as C++: their Linux parts) and the
# Linux platform layer.
set(LINUX_DIR ${LDD_ROOT}/src/DCL/Source/Platform/Linux)
file(GLOB LDD_LINUX_SOURCES CONFIGURE_DEPENDS ${LINUX_DIR}/*.cpp)
set(LDD_ALL_SOURCES ${LDD_LDD_SOURCES} ${LDD_LINUX_SOURCES})
foreach(s ${LDD_ALL_SOURCES})
    if(s MATCHES "\\.mm$")
        set_source_files_properties(${s} PROPERTIES LANGUAGE CXX COMPILE_OPTIONS "-xc++")
    endif()
endforeach()
add_executable(ldd64 ${LDD_ALL_SOURCES})
set(LDD_INCLUDES
    src/DCL/Include src/DCL/Source src/DCL/Source/Base src/DCL/Source/Connectivity
    src/DCL/Source/Connectivity/ModelIO src/DCL/Source/Lua src/DCL/Source/Network src/DCL/Source/Physics
    src/DCL/Source/Physics/Vortex src/DCL/Source/Render src/DCL/Source/Render/Image
    src/DCL/Source/Render/OpenGLRender src/DCL/Source/Sound src/DCL/Source/Storage src/DCL/Source/UI
    src/Products src/Products/LDD src/Products/LDD/Source src/Products/LDD/Source/LDDLib
    src/Products/LDD/Source/LDDLib/Commands)
foreach(i ${LDD_INCLUDES})
    target_include_directories(ldd64 PRIVATE ${LDD_ROOT}/${i})
endforeach()
# The Linux layer comes first: it holds the stand-ins for the macOS headers the Mac branch includes.
target_include_directories(ldd64 BEFORE PRIVATE ${LINUX_DIR})
target_compile_definitions(ldd64 PRIVATE LDD_LINUX=1 XML_STATIC GL_SILENCE_DEPRECATION
                           LDD_VERSION_STRING="${PROJECT_VERSION}")
if(GST_FOUND)
    target_compile_definitions(ldd64 PRIVATE LDD_LINUX_GSTREAMER=1)
    target_link_libraries(ldd64 PRIVATE PkgConfig::GST)
else()
    message(STATUS "GStreamer not found: no sound (package libgstreamer1.0-dev)")
endif()
target_link_libraries(ldd64 PRIVATE ldd_vortex ldd_luaplus ldd_expat ldd_png ldd_jpeg ldd_zlib
                      PkgConfig::GTK3 OpenGL::GL GLU Threads::Threads ${CMAKE_DL_LIBS} m)
# Keep the bundled zlib / libpng / libjpeg / expat private: GTK's image loaders use the system's.
target_link_options(ldd64 PRIVATE -Wl,--exclude-libs,ALL)

# Cursors/ (extracted from the original LDD.exe by tools/extract_rsrc.py) and LDD64's icon go next
# to the executable.
set(LDD_EXTRACTED ${LDD_ROOT}/src/Products/LDD/Resources/Win/Extracted)
file(GLOB LDD_CURSOR_FILES ${LDD_EXTRACTED}/cursor_*.cur)
if(LDD_CURSOR_FILES)
    add_custom_command(TARGET ldd64 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ldd64>/Cursors
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LDD_CURSOR_FILES} $<TARGET_FILE_DIR:ldd64>/Cursors
        VERBATIM)
endif()
set(LDD_ICON ${LDD_ROOT}/src/Products/LDD/Resources/ldd64.ico)
set(LDD_LDRAW_MAP ${LDD_ROOT}/src/Products/LDD/Resources/ldraw.xml)
add_custom_command(TARGET ldd64 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LDD_ICON} $<TARGET_FILE_DIR:ldd64>/ldd64.ico
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LDD_LDRAW_MAP} $<TARGET_FILE_DIR:ldd64>/ldraw.xml
    VERBATIM)

# cmake --install: lib/ldd64 (ldd64, Cursors/, ldd64.ico, ldraw.xml; LDD's data files go there
# too), a launcher and the model file types. The launcher names the configured prefix.
include(GNUInstallDirs)
set(LDD_PROGRAM_DIR ${CMAKE_INSTALL_LIBDIR}/ldd64)
set(LDD_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${LDD_PROGRAM_DIR})
install(TARGETS ldd64 DESTINATION ${LDD_PROGRAM_DIR})
if(LDD_CURSOR_FILES)
    install(FILES ${LDD_CURSOR_FILES} DESTINATION ${LDD_PROGRAM_DIR}/Cursors)
endif()
install(FILES ${LDD_ICON} ${LDD_LDRAW_MAP} DESTINATION ${LDD_PROGRAM_DIR})
configure_file(${LDD_ROOT}/src/Products/LDD/Resources/Linux/ldd64.desktop.in ${CMAKE_BINARY_DIR}/ldd64.desktop @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/ldd64.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${LDD_ROOT}/src/Products/LDD/Resources/Linux/ldd64-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
