target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/CBaseContainer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/CBaseWidget.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/CBaseWindow.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/compatibilitylayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/CTextLabel.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/GUI.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/IWidget.cpp")

add_subdirectory(ncc)