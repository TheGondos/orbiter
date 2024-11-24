# Copyright (c) Gondos
# Licensed under the MIT License
find_package(OpenGL REQUIRED)
find_package(Fontconfig)
find_package(Freetype REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ORBITER_BINARY_PLUGIN_DIR})
file(GLOB OGL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vs ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.fs ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.inc)

install(FILES ${OGL_SHADERS}
    DESTINATION ${ORBITER_INSTALL_ROOT_DIR}/Modules/OGLClient
)
#
add_library(OGLClient SHARED
Shader.cpp CelSphere.cpp Renderer.cpp VertexBuffer.cpp Scene.cpp imgui_impl_opengl3.cpp Light.cpp
OGLCamera.cpp OGLMesh.cpp MeshManager.cpp VObject.cpp VVessel.cpp Texture.cpp VPlanet.cpp VStar.cpp ztreemgr.cpp
TileMgr.cpp SurfMgr.cpp RingMgr.cpp HazeMgr.cpp CloudMgr.cpp tilemgr2.cpp surfmgr2.cpp tilelabel.cpp
VBase.cpp cloudmgr2.cpp OGLClient.cpp Particle.cpp CSphereMgr.cpp spherepatch.cpp OGLPad.cpp  nanovg/src/nanovg.c
SOIL2/srcs/etc1_utils.c SOIL2/srcs/image_DXT.c SOIL2/srcs/image_helper.c SOIL2/srcs/SOIL2.c
glad.c  ${EXTERN_DIR}/libnsbmp/src/libnsbmp.c
)

target_compile_definitions(OGLClient
    PRIVATE
		NVG_NO_STB=1 FONS_USE_FREETYPE=1
)

#OGL_OBJS=$(foreach src, $(OGL_SRC), OVP/OGLClient/$(src:.cpp=.o)) OVP/OGLClient/glad.o Extern/SOIL2/libSOIL2.a
target_include_directories(OGLClient
	PUBLIC ${ORBITER_BINARY_SDK_DIR}/include
    PUBLIC SOIL2/incs
	PUBLIC ${EXTERN_DIR}/libnsbmp/include
	PUBLIC ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(OGLClient
#	${ORBITER_LIB}
	${ORBITER_SDK_LIB}
	Fontconfig::Fontconfig
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
