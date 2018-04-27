# UNFINISHED - DO NOT USE.

cmake_minimum_required(VERSION 3.0)
project(cathook VERSION 0.0.1 DESCRIPTION "Free Source Engine Trainer")

find_package(SDL2 REQUIRED)
find_package(OpenSSL REQUIRED)

# cat packages
find_package(glez REQUIRED)
find_package(xoverlay REQUIRED)
find_package(SimpleIPC REQUIRED)

get_target_property(SimpleIPC_INCLUDE_DIRS SimpleIPC INTERFACE_INCLUDE_DIRECTORIES)
message("DIRS: ${SimpleIPC_INCLUDE_DIRS}")

add_library(cathook SHARED "")

set_target_properties(cathook PROPERTIES COMPILE_FLAGS "-m32 -msse -msse2 -msse3" LINK_FLAGS "-m32 -static")

target_compile_definitions(cathook PRIVATE
        GAME=tf2
        ENABLE_VISUALS=1
        ENABLE_IPC=1
        ENABLE_GUI=1
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
        "ucccccp"
        "${SDL2_INCLUDE_DIRS}"
        "${glez_INCLUDE_DIRS}"
        "${xoverlay_INCLUDE_DIRS}"
        "${SimpleIPC_INCLUDE_DIRS}")

add_subdirectory(include)
add_subdirectory(src)

find_library(LIBSDL2     NAMES libSDL2-2.0.so.0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIBTIER0    NAMES tier0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIBVSTDLIB  NAMES vstdlib PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)

# TODO glez_LIBRARIES?
target_link_libraries(cathook ${LIBTIER0} ${LIBVSTDLIB} glez xoverlay ssl GLEW stdc++ ${LIBSDL2})

set_target_properties(cathook PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)