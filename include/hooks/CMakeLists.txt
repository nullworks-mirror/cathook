target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/HookedMethods.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/HookTools.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PreDataUpdate.hpp")

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
