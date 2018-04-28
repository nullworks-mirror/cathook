target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/CreateMove.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/nographics.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/others.cpp")

if(EnableVisuals)
    target_sources(cathook PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/Paint.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/PaintTraverse.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/sdl.cpp")
endif()