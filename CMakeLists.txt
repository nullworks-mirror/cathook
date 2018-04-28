# UNFINISHED - DO NOT USE.

cmake_minimum_required(VERSION 3.0)
project(cathook VERSION 0.0.1 DESCRIPTION "Free Source Engine Trainer")

set(GameSpecific 1 CACHE BOOL "Build for specific target game (As opposed to universal, but slower, lib)")
set(Game "tf2" CACHE STRING "Target game")
set(GameValues "tf2;hl2dm;dab;tf2c;css;dynamic" CACHE INTERNAL "List of supported game types")
set_property(CACHE Game PROPERTY STRINGS ${GameValues})

set(EnableVisuals 1 CACHE BOOL "Enable Visuals")
set(ExternalDrawing 0 CACHE BOOL "External Visuals")
set(EnableGUI 1 CACHE BOOL "Enable GUI")
set(EnableIPC 1 CACHE BOOL "Enable IPC")
set(DataPath "/opt/cathook/data" CACHE STRING "Data location")
set(VACBypass 0 CACHE BOOL "Textmode VAC bypass")
set(Textmode 0 CACHE BOOL "Various textmode-only features for bots")
set(EnableTextmodeStdin 0 CACHE BOOL "Textmode Stdin -> Console bridge (EXPERIMENTAL)")
set(EnableWarnings 1 CACHE BOOL "Enable compile warnings")
set(EnableNullGraphics 0 CACHE BOOL "Enable experimental textmode hooks (CRASHES)")

find_package(Git REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenSSL REQUIRED)

# cat packages
find_package(glez REQUIRED)
find_package(xoverlay REQUIRED)
find_package(SimpleIPC REQUIRED)

execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --pretty=\"%h\"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --pretty=\"%ai\"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_TIME
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

get_target_property(SimpleIPC_INCLUDE_DIRS SimpleIPC INTERFACE_INCLUDE_DIRECTORIES)

configure_file(include/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h @ONLY)
configure_file(include/version.h.in ${CMAKE_SOURCE_DIR}/include/version.h @ONLY)

add_library(cathook SHARED "")

set_target_properties(cathook PROPERTIES COMPILE_FLAGS "-m32 -msse -msse2 -msse3" LINK_FLAGS "-m32 -static")

target_compile_definitions(cathook PRIVATE
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

target_include_directories(cathook SYSTEM PRIVATE
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

if(EnableWarnings)
    target_compile_options(cathook PRIVATE -Wundef -Werror)
endif()

add_subdirectory(include)
add_subdirectory(src)

find_library(ValveLibrarySDL2    NAMES libSDL2-2.0.so.0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryTier0   NAMES tier0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryVStdLib NAMES vstdlib PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)

# TODO glez_LIBRARIES?
target_link_libraries(cathook ${ValveLibrarySDL2} ${ValveLibraryTier0} ${ValveLibraryVStdLib} glez xoverlay ssl GLEW stdc++)

set_target_properties(cathook PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)