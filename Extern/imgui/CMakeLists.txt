
# ImGui
set(IMGUI_LIB_NAME "imgui")

set(IMGUI_SOURCES
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imgui_stacklayout.cpp"
)

set(IMGUI_HEADERS
    "imgui/imgui.h"
)

set(IMGUI_INC_PATH "${IMGUI_LIB_NAME}/")

add_library(imgui OBJECT
        ${IMGUI_SOURCES}
        ${IMGUI_HEADERS}
)
install(FILES ${CMAKE_SOURCE_DIR}/Extern/imgui/imgui/misc/fonts/Roboto-Medium.ttf
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}
)
install(FILES ${CMAKE_SOURCE_DIR}/Extern/imgui/imgui/misc/fonts/Roboto-Bold.ttf
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}
)
#target_compile_definitions(imgui
#    PRIVATE
#        IMGUI_IMPL_OPENGL_LOADER_GLAD=1
#)

#target_include_directories(imgui
#    PUBLIC
#        "${IMGUI_INC_PATH}"
#        "${GLFW_INC_PATH}"
#        "${GLAD_INC_PATH}"
#)
