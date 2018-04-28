target_sources(cathook PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/atlas.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/colors.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawex.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawing.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/drawmgr.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/EffectChams.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/EffectGlow.cpp")

add_subdirectory(menu)