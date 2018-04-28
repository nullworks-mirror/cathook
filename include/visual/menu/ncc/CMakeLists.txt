target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/Item.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/ItemSublist.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/ItemTitle.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/ItemVariable.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/List.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/Menu.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PlayerListEntry.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PlayerList.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/Root.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/Tooltip.hpp")

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")