target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/CBaseContainer.h"
        "${CMAKE_CURRENT_LIST_DIR}/CBaseWidget.h"
        "${CMAKE_CURRENT_LIST_DIR}/CBaseWindow.h"
        "${CMAKE_CURRENT_LIST_DIR}/compatlayer.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/CTextLabel.h"
        "${CMAKE_CURRENT_LIST_DIR}/guicommon.h"
        "${CMAKE_CURRENT_LIST_DIR}/GUI.h"
        "${CMAKE_CURRENT_LIST_DIR}/IWidget.h")

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(ncc)