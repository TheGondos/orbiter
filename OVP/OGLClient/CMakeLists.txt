# Copyright (c) Gondos
# Licensed under the MIT License
find_package(OpenGL REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ORBITER_BINARY_PLUGIN_DIR})
file(GLOB OGL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.vs ${CMAKE_CURRENT_SOURCE_DIR}/*.fs)

install(FILES ${OGL_SHADERS}
    DESTINATION ${ORBITER_INSTALL_ROOT_DIR}/Modules/OGLClient
)

add_library(OGLClient SHARED
Shader.cpp CelSphere.cpp Renderer.cpp VertexBuffer.cpp Scene.cpp imgui_impl_opengl3.cpp imgui_impl_glfw.cpp
OGLCamera.cpp MeshManager.cpp VObject.cpp VVessel.cpp Texture.cpp VPlanet.cpp SphereMesh.cpp VStar.cpp
TileMgr.cpp SurfMgr.cpp RingMgr.cpp HazeMgr.cpp CloudMgr.cpp tilemgr2.cpp surfmgr2.cpp tilelabel.cpp
VBase.cpp cloudmgr2.cpp fcaseopen.cpp OGLClient.cpp Particle.cpp bmp.cpp
SOIL2/srcs/etc1_utils.c SOIL2/srcs/image_DXT.c SOIL2/srcs/image_helper.c SOIL2/srcs/SOIL2.c
glad.c
)
#OGL_OBJS=$(foreach src, $(OGL_SRC), OVP/OGLClient/$(src:.cpp=.o)) OVP/OGLClient/glad.o Extern/SOIL2/libSOIL2.a
target_include_directories(OGLClient
	PUBLIC ${ORBITER_SOURCE_SDK_INCLUDE_DIR}
    PUBLIC SOIL2/incs
)

target_link_libraries(OGLClient
#	${ORBITER_LIB}
	${ORBITER_SDK_LIB}
	imgui
	GL
#    ${OPENGL_LIBRARY}
)

add_dependencies(OGLClient
	${OrbiterTgt}
	Orbitersdk
)

set_target_properties(OGLClient
	PROPERTIES
	FOLDER Modules
)

# Installation
install(TARGETS OGLClient
#	RUNTIME
	DESTINATION ${ORBITER_INSTALL_PLUGIN_DIR}
)
