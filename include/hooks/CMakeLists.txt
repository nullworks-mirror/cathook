target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/CreateMove.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/hookedmethods.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/others.hpp")

if(EnableVisuals)
    target_sources(cathook PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/PaintTraverse.hpp")
endif()

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")