# Current dependencies:
#   Git
#   SDL2
#   GLEW

set(EnableCotire 1 CACHE BOOL "Enable CoTiRe (Compile Time Reducer)")
if (EnableCotire)
    set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/cotire/CMake")
    cmake_policy(SET CMP0011 NEW)
    include(cotire)
    file(GLOB_RECURSE ignore_files *.cpp)
    set(ignore_files ${ignore_files} CACHE INTERNAL "")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
endif()
set(CMAKE_BUILD_TYPE_VALUES "Debug;Release" CACHE INTERNAL "List of supported build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_BUILD_TYPE_VALUES})

cmake_minimum_required(VERSION 3.0)
project(cathook VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
add_library(cathook SHARED "")

set(GameSpecific 1 CACHE BOOL "Build for specific target game (As opposed to universal, but slower, lib)")
set(Game "tf2" CACHE STRING "Target game")
set(GameValues "tf2;hl2dm;dab;tf2c;css;dynamic" CACHE INTERNAL "List of supported game types")
set_property(CACHE Game PROPERTY STRINGS ${GameValues})

set(DrawType "IMGUI" CACHE STRING "Target drawing api")
set(DrawTypes "No Visuals;Glez;Engine;IMGUI;IMGUI Streamproof" CACHE INTERNAL "Select the drawing api that cathook should use")
set_property(CACHE DrawType PROPERTY STRINGS ${DrawTypes})

set(EnableVisuals 0)
set(EnableGlezDrawing 0)
set(EnableEngineDrawing 0)
set(EnableImGuiDrawing 0)
set(ExternalDrawing 0)

if(NOT DrawType STREQUAL "No Visuals")
    set(EnableVisuals 1)
endif()
if(DrawType STREQUAL "Glez")
    set(EnableGlezDrawing 1)
elseif(DrawType STREQUAL "Engine")
    set(EnableEngineDrawing 1)
elseif(DrawType STREQUAL "IMGUI")
    set(EnableImGuiDrawing 1)
elseif(DrawType STREQUAL "IMGUI Streamproof")
    set(EnableImGuiDrawing 1)
    set(ExternalDrawing 1)
endif()

set(EnableUnityBuilds 1 CACHE BOOL "Enable Profiler")
set(EnableProfiler 0 CACHE BOOL "Enable Profiler")
set(EnableGUI 1 CACHE BOOL "Enable GUI")
set(EnableIPC 1 CACHE BOOL "Enable IPC")
set(DataPath "/opt/cathook/data" CACHE FILEPATH "Data location")
set(VACBypass 0 CACHE BOOL "Textmode VAC bypass")
set(Textmode 0 CACHE BOOL "Various textmode-only features for bots")
set(EnableTextmodeStdin 0 CACHE BOOL "Textmode Stdin -> Console bridge (EXPERIMENTAL)")
set(EnableWarnings 0 CACHE BOOL "Enable compile warnings")
set(EnableOnlineFeatures 0 CACHE BOOL "Enable online features (WIP)")
set(EnableIRC 1 CACHE BOOL "Enable IRC for messaging and ignore-cathook")
set(EnableLogging 1 CACHE BOOL "Enable logging to /tmp/")
set(EnableClip 1 CACHE BOOL "Enable anti electron/chromium freezing")

if(NOT EnableVisuals)
    set(EnableGUI 0)
endif()

if (EnableVisuals)
    if (EnableImGuiDrawing)
        find_package(Freetype REQUIRED)
        target_include_directories(cathook PRIVATE "${FREETYPE_INCLUDE_DIRS}")
        target_link_libraries(cathook ${FREETYPE_LIBRARIES})
    endif()
endif()

find_package(Git REQUIRED)
find_package(Boost REQUIRED)
find_package(SDL2 REQUIRED)

# cat packages

execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --pretty=\"%h\"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --pretty=\"%ai\"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMITTER_DATE
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

find_library(ValveLibrarySDL2    NAMES libSDL2-2.0.so.0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryTier0   NAMES tier0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryVStdLib NAMES vstdlib PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)

if(EnableIPC)
    add_subdirectory(external/simple-ipc)
    target_link_libraries(cathook SimpleIPC)
endif()

if(EnableOnlineFeatures)
    add_subdirectory(external/co-library)
    target_link_libraries(cathook co-library)
endif()

if(EnableVisuals)
    if (EnableGlezDrawing)
        add_subdirectory(external/libglez)
    endif()
    target_include_directories(cathook PRIVATE include/visual)
    find_package(SDL2 REQUIRED)
    find_package(GLEW REQUIRED)
    if(ExternalDrawing)
        add_subdirectory(external/libxoverlay)
        target_link_libraries(cathook xoverlay)
        target_include_directories(cathook PRIVATE external/libxoverlay/include)
    endif()
    target_include_directories(cathook PRIVATE "${SDL2_INCLUDE_DIRS}")
    if (EnableGlezDrawing)
        target_link_libraries(cathook glez)
    endif()
    target_link_libraries(cathook ${ValveLibrarySDL2} ${GLEW_LIBRARIES})
endif()

configure_file(include/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h @ONLY)
configure_file(include/version.h.in ${CMAKE_SOURCE_DIR}/include/version.h @ONLY)

set(CMAKE_CXX_FLAGS "-m32 -march=native -fexceptions -fno-gnu-unique -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-rdynamic -ggdb -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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
        "external/source-sdk-2013-headers/mp/src/mathlib"
        "external/source-sdk-2013-headers/mp/src/public"
        "external/source-sdk-2013-headers/mp/src/common"
        "external/source-sdk-2013-headers/mp/src/public/tier0"
        "external/source-sdk-2013-headers/mp/src/public/tier1"
        "external/source-sdk-2013-headers/mp/src/tier1"
        "external/source-sdk-2013-headers/mp/src")

if(EnableWarnings)
    target_compile_options(cathook PRIVATE -Wall -Wextra)
else()
    target_compile_options(cathook PRIVATE -w)
endif()


target_include_directories(cathook PRIVATE include include/hooks)

add_custom_target(data
        bash ./install-data "${DataPath}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM)

# TODO glez_LIBRARIES?
target_link_libraries(cathook ${ValveLibraryTier0} ${ValveLibraryVStdLib})
set_target_properties(cathook PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(external)
add_subdirectory(modules)

if (EnableCotire)
    set_target_properties(cathook PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${CMAKE_SOURCE_DIR}/include/common.hpp")
    set_target_properties(cathook PROPERTIES COTIRE_ADD_UNITY_BUILD true)
    set_target_properties(cathook PROPERTIES COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES 30)
    set_source_files_properties(${ignore_files} PROPERTIES COTIRE_EXCLUDED true)
    cotire(cathook)
endif()

add_custom_command(TARGET cathook POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cathook> "${CMAKE_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:cathook>")
if (EnableCotire)
    add_custom_command(TARGET cathook_unity POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cathook_unity> "${CMAKE_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:cathook_unity>")
endif()
