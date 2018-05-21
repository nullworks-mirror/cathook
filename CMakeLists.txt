# UNFINISHED - DO NOT USE.

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
endif()
set(CMAKE_BUILD_TYPE_VALUES "Debug;Release" CACHE INTERNAL "List of supported build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_BUILD_TYPE_VALUES})

cmake_minimum_required(VERSION 3.0)
project(cathook VERSION 0.0.1)
add_library(cathook SHARED "")

set(GameSpecific 1 CACHE BOOL "Build for specific target game (As opposed to universal, but slower, lib)")
set(Game "tf2" CACHE STRING "Target game")
set(GameValues "tf2;hl2dm;dab;tf2c;css;dynamic" CACHE INTERNAL "List of supported game types")
set_property(CACHE Game PROPERTY STRINGS ${GameValues})


set(EnableVisuals 1 CACHE BOOL "Enable Visuals")
set(LagbotMode 0 CACHE BOOL "Build light cathook made for lagbots")
set(ExternalDrawing 0 CACHE BOOL "External Visuals")
set(EnableGUI 1 CACHE BOOL "Enable GUI")
set(EnableIPC 1 CACHE BOOL "Enable IPC")
set(DataPath "/opt/cathook/data" CACHE FILEPATH "Data location")
set(VACBypass 0 CACHE BOOL "Textmode VAC bypass")
set(Textmode 0 CACHE BOOL "Various textmode-only features for bots")
set(EnableTextmodeStdin 0 CACHE BOOL "Textmode Stdin -> Console bridge (EXPERIMENTAL)")
set(EnableWarnings 1 CACHE BOOL "Enable compile warnings")
set(EnableNullGraphics 0 CACHE BOOL "Enable experimental textmode hooks (CRASHES)")

if(NOT EnableVisuals)
    set(EnableGUI 0)
endif()

find_package(Git REQUIRED)
find_package(OpenSSL REQUIRED)

# cat packages

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

find_library(ValveLibrarySDL2    NAMES libSDL2-2.0.so.0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryTier0   NAMES tier0 PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
find_library(ValveLibraryVStdLib NAMES vstdlib PATHS "${PROJECT_SOURCE_DIR}/lib" NO_DEFAULT_PATH)

if(EnableIPC)
    find_package(SimpleIPC REQUIRED)
    get_target_property(SimpleIPC_INCLUDE_DIRS SimpleIPC INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(cathook PRIVATE "${SimpleIPC_INCLUDE_DIRS}")
endif()

if(EnableVisuals)
    find_package(glez REQUIRED)
    get_target_property(glez_INCLUDE_DIRS glez INTERFACE_INCLUDE_DIRECTORIES)
    find_package(SDL2 REQUIRED)
    find_package(GLEW REQUIRED)
    if(ExternalDrawing)
        find_package(xoverlay REQUIRED)
        get_target_property(xoverlay_INCLUDE_DIRS xoverlay INTERFACE_INCLUDE_DIRECTORIES)
        target_include_directories(cathook PRIVATE
                "${xoverlay_INCLUDE_DIRS}")
        target_link_libraries(cathook xoverlay)
    endif()
    target_include_directories(cathook PRIVATE
            "${SDL2_INCLUDE_DIRS}"
            "${glez_INCLUDE_DIRS}")
    target_link_libraries(cathook ${ValveLibrarySDL2} glez ${GLEW_LIBRARIES})
endif()

configure_file(include/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h @ONLY)
configure_file(include/version.h.in ${CMAKE_SOURCE_DIR}/include/version.h @ONLY)

set_target_properties(cathook PROPERTIES COMPILE_FLAGS "-m32 -msse -msse2 -msse3 -fexceptions" LINK_FLAGS "-m32 -fno-gnu-unique -fexceptions")

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

target_include_directories(cathook PRIVATE "ucccccp")

if(EnableWarnings)
    target_compile_options(cathook PRIVATE -Wall -Wextra)
endif()

add_subdirectory(include)
add_subdirectory(src)

add_custom_target(data
        bash ./install-data "${DataPath}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM)

# TODO glez_LIBRARIES?
target_link_libraries(cathook ${ValveLibraryTier0} ${ValveLibraryVStdLib} ssl)
set_target_properties(cathook PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Strip
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Stripping info from library")
    add_custom_command(TARGET cathook POST_BUILD
            COMMAND strip --strip-all "$<TARGET_FILE:cathook>")
endif()

add_custom_command(TARGET cathook POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cathook> "${CMAKE_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:cathook>")
