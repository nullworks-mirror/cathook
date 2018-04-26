# UNFINISHED - DO NOT USE.

cmake_minimum_required(VERSION 3.0)
project(cathook VERSION 0.0.1 DESCRIPTION "Free Source Engine Trainer")

find_library(SDL2 REQUIRED)

add_library(cathook SHARED "")

set_target_properties(cathook PROPERTIES COMPILE_FLAGS "-m32 -msse -msse2 -msse3" LINK_FLAGS "-m32")

target_compile_definitions(cathook PRIVATE
        GAME=tf2
        ENABLE_VISUALS=1
        ENABLE_IPC=1
        ENABLE_NULL_GRAPHICS=0
        TEXTMODE_STDIN=0
        TEXTMODE_VAC=0
        DATA_PATH="/opt/cathook/data"
        NO_LTO=0
        _GLIBCXX_USE_CXX11_ABI=0
        _POSIX=1
        FREETYPE_GL_USE_VAO=1
        RAD_TELEMETRY_DISABLED=1
        LINUX=1
        USE_SDL=1
        _LINUX=1
        POSIX=1
        GNUC=1
        NO_MALLOC_OVERRIDE=1)

target_include_directories(cathook PRIVATE
        "source-sdk-2013-headers/mp/src/mathlib"
        "source-sdk-2013-headers/mp/src/public"
        "source-sdk-2013-headers/mp/src/common"
        "source-sdk-2013-headers/mp/src/public/tier0"
        "source-sdk-2013-headers/mp/src/public/tier1"
        "source-sdk-2013-headers/mp/src/tier1"
        "source-sdk-2013-headers/mp/src")

target_include_directories(cathook PRIVATE
        "simple-ipc/src/include"
        "libglez/include"
        "libxoverlay/include"
        "ucccccp"
        "${SDL2_INCLUDE_DIRS}")

add_subdirectory(include)
add_subdirectory(src)

add_library(libc SHARED IMPORTED)
set_target_properties(libc PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libc.so.6")

add_library(freetype SHARED IMPORTED)
set_target_properties(freetype PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libfreetype.so")

add_library(glew SHARED IMPORTED)
set_target_properties(glew PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libGLEW.so")

add_library(sdl2 SHARED IMPORTED)
set_target_properties(sdl2 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libSDL2-2.0.so.0")

add_library(stdcpp SHARED IMPORTED)
set_target_properties(stdcpp PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libstdc++.so.6")

add_library(tier0 SHARED IMPORTED)
set_target_properties(tier0 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libtier0.so")

add_library(vstdlib SHARED IMPORTED)
set_target_properties(vstdlib PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/libvstdlib.so")

target_link_libraries(cathook libc freetype glew sdl2 stdcpp tier0 vstdlib)