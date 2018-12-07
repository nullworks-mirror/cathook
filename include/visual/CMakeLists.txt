if(EnableGUI)
    add_subdirectory(menu)
endif()

target_include_directories(cathook PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
