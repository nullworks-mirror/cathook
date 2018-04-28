target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/atlas.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/colors.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawex.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawing.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawmgr.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/EffectChams.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/EffectGlow.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/glez.h"
        "${CMAKE_CURRENT_LIST_DIR}/xoverlay.h")

add_subdirectory(menu)

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")