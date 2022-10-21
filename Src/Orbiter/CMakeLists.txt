# Copyright (c) Martin Schweiger
# Licensed under the MIT License

# SAFESEH linker flag must be turned off because the DX7 libraries don't support it'
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -SAFESEH:NO")

# Sources for all Orbiter executable targets
set(common_src
# General source files
	Astro.cpp
	Camera.cpp
	cmdline.cpp
	Config.cpp
	Element.cpp
	elevmgr.cpp
	Keymap.cpp
	LightEmitter.cpp
	Mesh.cpp
	Nav.cpp
	Orbiter.cpp
	PlaybackEd.cpp
	Psys.cpp
	Script.cpp
	State.cpp
	Vecmat.cpp
	VectorMap.cpp
# Launchpad
	DlgLaunchpad.cpp
# Dialogs
	DlgCamera.cpp
#	DlgCapture.cpp
	DlgFocus.cpp
	DlgFunction.cpp
#	DlgHelp.cpp
	DlgInfo.cpp
	DlgMap.cpp
#	DlgMenuCfg.cpp
	DlgRecorder.cpp
	DlgTacc.cpp
	DlgVishelper.cpp
	Select.cpp
# Body classes
	Body.cpp
	BodyIntegrator.cpp
	Celbody.cpp
	Planet.cpp
	Rigidbody.cpp
	Star.cpp
# Vessel classes
	FlightRecorder.cpp
	SuperVessel.cpp
	Vessel.cpp
	Vesselbase.cpp
	Vesselstatus.cpp
# Surface base classes
	Base.cpp
	Baseobj.cpp
# Cockpit classes
	Defpanel.cpp
	hud.cpp
	MenuInfoBar.cpp
	Pane.cpp
	Panel.cpp
	Panel2D.cpp
	VCockpit.cpp
# MFD classes
	Mfd.cpp
	MfdAlign.cpp
	MfdComms.cpp
	MfdDocking.cpp
	MfdHsi.cpp
	MfdLanding.cpp
	MfdMap.cpp
	MfdOrbit.cpp
	MfdSurface.cpp
	MfdSync.cpp
	MfdTransfer.cpp
	MfdUser.cpp
# API implementations
	CamAPI.cpp
	DrawAPI.cpp
	GraphicsAPI.cpp
	MFDAPI.cpp
	ModuleAPI.cpp
	OrbiterAPI.cpp
# Graphics utils
	D3dmath.cpp
# Utils
	Log.cpp
	Util.cpp
	ZTreeMgr.cpp
	GUIManager.cpp
	Controller.cpp
	ControllerNodes.cpp
	imgui_impl_glfw.cpp
	${EXTERN_DIR}/md4c/src/md4c.c
	${EXTERN_DIR}/imgui_md/imgui_md.cpp
)

# Sources for DX7 built-in graphics version only
set(graphics_src
# DX7 inline graphics
	cloudmgr2.cpp
	CSphereMgr.cpp
	cspheremgr2.cpp
	D3d7enum.cpp
	D3d7frame.cpp
	ExtraRender.cpp
	OGraphics.cpp
	Particle.cpp
	Scene.cpp
	ScreenNote.cpp
	Spherepatch.cpp
	surfmgr2.cpp
	Texture.cpp
	tilelabel.cpp
	TileMgr.cpp
	tilemgr2.cpp
	VBase.cpp
	Vobject.cpp
	VPlanet.cpp
	Vstar.cpp
	Vvessel.cpp
)

set(Orbiter_includes
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/Orbitersdk/include
	${CMAKE_SOURCE_DIR}/OVP
	${CMAKE_CURRENT_BINARY_DIR}
	${ZLIB_INCLUDE_DIR}
	${EXTERN_DIR}
)

set(Orbiter_common_libs
	$<TARGET_FILE:Orbitersdk>
)

set(Orbiter_depends
	Orbitersdk
)

# Orbiter executable (graphics server version)
add_executable(Orbiter_server
	${common_src}
)

set_target_properties(Orbiter_server
	PROPERTIES
	#LINK_FLAGS "/SUBSYSTEM:CONSOLE /ENTRY:WinMainCRTStartup"
	RUNTIME_OUTPUT_DIRECTORY ${ORBITER_BINARY_MODULE_DIR}/Server
	OUTPUT_NAME Orbiter
	VS_DEBUGGER_WORKING_DIRECTORY ${ORBITER_BINARY_ROOT_DIR}
	FOLDER Core
)

target_include_directories(Orbiter_server PUBLIC
	${Orbiter_includes}
	${EXTERN_DIR}/imgui-notify/example/src
	${EXTERN_DIR}/md4c/src
	${EXTERN_DIR}/imgui_md
)

target_link_libraries(Orbiter_server
	${Orbiter_common_libs}
	${Orbiter_server_libs}
	${CMAKE_DL_LIBS}
	glfw
	imgui
	imgui-node-editor
	imgui-knobs
	z
	lua::lib
)

add_dependencies(Orbiter_server
	${Orbiter_depends}
)

install(FILES Orbiter.png
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}/Images
)

# Installation: Orbiter executable (graphics-server versions)
install(TARGETS
	Orbiter_server
	RUNTIME
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}
)
# If the standard Orbiter executable is not built...

set_target_properties(Orbiter_server
	PROPERTIES
	ENABLE_EXPORTS 1
)
# ...we have to use the NG version to export the API links
install(FILES $<TARGET_LINKER_FILE:Orbiter_server>
	DESTINATION ${ORBITER_INSTALL_SDK_DIR}/lib
)
# ...and copy Orbiter.exe into build dir (not install dir) so that tests work
add_custom_command(TARGET Orbiter_server
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Orbiter_server> ${ORBITER_BINARY_ROOT_DIR}
)

# Copy library to its Orbitersdk/lib location so that external project integrated into the build can find it
add_custom_command(TARGET ${OrbiterTgt}
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${ORBITER_BINARY_SDK_DIR}/lib
#	COMMAND ${CMAKE_COMMAND} -E copy ${ORBITER_LIB}.exe ${ORBITER_BINARY_SDK_DIR}/lib/ 
)
