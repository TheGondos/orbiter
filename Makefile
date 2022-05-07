ORBITER_SDK_INC=Orbitersdk/include
ORBITER_SDK_LIB=Src/Orbitersdk/Orbitersdk.o
OVP_INCLUDE_PATH=OVP
VSOP_PATH=Src/Celbody/Vsop87
VSOP_LIB=$(VSOP_PATH)/libVsop87.so
OSC_PATH=/home/gondos/dev/Skybolt
EXTERN_PATH=Extern

NOWARN=-Werror -Wno-reorder -Wno-unused-result
#NOWARN=-Wall -Werror -fpermissive -Wno-class-memaccess -Wno-unused-variable -Wno-unused-value -Wno-switch -Wno-parentheses
#NOWARN+=-Wno-write-strings -Wno-delete-non-virtual-dtor -Wno-unused-but-set-variable
DEFINES=-DNOGRAPHICS
#DEFINES=-DINLINEGRAPHICS -DDIRECTINPUT_VERSION=0x0800

#CC=winegcc
#CFLAGS=-mwindows -m32 -O2 -I$(ORBITER_SDK_INC) $(NOWARN) $(DEFINES)
#CXX=wineg++
#CXXFLAGS=-mwindows -m32 -O2 -I$(EXTERN_PATH) -I$(ORBITER_SDK_INC) $(NOWARN) $(DEFINES) -ISrc/Vessel/Atlantis/Atlantis -ISrc/Plugin  -ISrc/Celbody/Satsat -ISrc/Module/LuaScript/LuaInterpreter -I$(OVP_INCLUDE_PATH) -ISrc/Plugin/ScnEditor -ISrc/Celbody/Vsop87/ -ISrc/Celbody/Galsat -ISrc/Orbiter
#LDFLAGS=-rdynamic -mwindows Extern/zlib-ng/libz.a -m32 -Wl,-Bstatic -lwinmm -lcomctl32 -ldxguid -lddraw -Wl,-Bdynamic 

CC=gcc
#CFLAGS=-fsanitize=address -fPIC -g -O0 -I$(ORBITER_SDK_INC) $(DEFINES)
#CFLAGS=-fsanitize=undefined -fPIC -g -I$(ORBITER_SDK_INC) $(DEFINES)
CFLAGS=-fPIC -g -O0 -I$(ORBITER_SDK_INC) $(DEFINES)
CXX=g++
#CXXFLAGS=-fsanitize=address -std=c++17 -fPIC -g -O0 -I$(EXTERN_PATH) -I$(ORBITER_SDK_INC) $(NOWARN) $(DEFINES) -ISrc/Vessel/Atlantis/Atlantis -ISrc/Plugin  -ISrc/Celbody/Satsat -ISrc/Module/LuaScript/LuaInterpreter -I$(OVP_INCLUDE_PATH) -ISrc/Plugin/ScnEditor -ISrc/Celbody/Vsop87/ -ISrc/Celbody/Galsat -ISrc/Orbiter
#CXXFLAGS=-fsanitize=undefined -std=c++17 -fPIC -g -I$(EXTERN_PATH) -I$(ORBITER_SDK_INC) $(NOWARN) $(DEFINES) -ISrc/Vessel/Atlantis/Atlantis -ISrc/Plugin  -ISrc/Celbody/Satsat -ISrc/Module/LuaScript/LuaInterpreter -I$(OVP_INCLUDE_PATH) -ISrc/Plugin/ScnEditor -ISrc/Celbody/Vsop87/ -ISrc/Celbody/Galsat -ISrc/Orbiter

OSC_INC=-IOVP/OrbiterSkyboltClient/src -I/home/gondos/dev/Skybolt/src/ -I/home/gondos/dev/Skybolt/src/Skybolt -I/home/gondos/dev/Skybolt/src/Skybolt/SkyboltVis/ -I/home/gondos/dev/Skybolt/deps

CXXFLAGS=-std=c++17 -fPIC -g -O0 -I$(EXTERN_PATH) -I$(ORBITER_SDK_INC) $(NOWARN) $(DEFINES) $(OSC_INC) -ISrc/Vessel/Atlantis/Atlantis -ISrc/Plugin  -ISrc/Celbody/Satsat -ISrc/Module/LuaScript/LuaInterpreter -I$(OVP_INCLUDE_PATH) -ISrc/Plugin/ScnEditor -ISrc/Celbody/Vsop87/ -ISrc/Celbody/Galsat -ISrc/Orbiter
LDFLAGS=-rdynamic Extern/zlib-ng/libz.a


INSTALL_DIR=$(HOME)/orbiter_test

GENERAL_SRC=Astro.cpp \
	Camera.cpp \
	cmdline.cpp \
	Config.cpp \
	Element.cpp \
	elevmgr.cpp \
	Gamepad.cpp \
	Keymap.cpp \
	LightEmitter.cpp \
	Mesh.cpp \
	Nav.cpp \
	Orbiter.cpp \
	PlaybackEd.cpp \
	Psys.cpp \
	Script.cpp \
	State.cpp \
	Vecmat.cpp \
	VectorMap.cpp \
	GUIManager.cpp

DIALOG_SRC=DlgCamera.cpp \
	DlgFocus.cpp \
	DlgFunction.cpp \
	DlgInfo.cpp \
	DlgLaunchpad.cpp \
	DlgMap.cpp \
	DlgRecorder.cpp \
	DlgTacc.cpp \
	DlgVishelper.cpp \
	Select.cpp

BODY_SRC=Body.cpp \
	BodyIntegrator.cpp \
	Celbody.cpp \
	Planet.cpp \
	Rigidbody.cpp \
	Star.cpp

# Vessel classes
VESSEL_SRC=FlightRecorder.cpp \
	SuperVessel.cpp \
	Vessel.cpp \
	Vesselbase.cpp \
	Vesselstatus.cpp

# Surface base classes
SURFACE_SRC=Base.cpp \
	Baseobj.cpp
# Cockpit classes
COCKPIT_SRC=Defpanel.cpp \
	hud.cpp \
	MenuInfoBar.cpp \
	Pane.cpp \
	Panel.cpp \
	Panel2D.cpp \
	VCockpit.cpp
# MFD classes
MDF_SRC=Mfd.cpp \
	MfdAlign.cpp \
	MfdComms.cpp \
	MfdDocking.cpp \
	MfdHsi.cpp \
	MfdLanding.cpp \
	MfdMap.cpp \
	MfdOrbit.cpp \
	MfdSurface.cpp \
	MfdSync.cpp \
	MfdTransfer.cpp \
	MfdUser.cpp
# API implementations
API_SRC=CamAPI.cpp \
	DrawAPI.cpp \
	GraphicsAPI.cpp \
	MFDAPI.cpp \
	ModuleAPI.cpp \
	OrbiterAPI.cpp

# Graphics utils
GFX_SRC= \
	D3dmath.cpp

#	${GDICLIENT_DIR}/GDIClient.cpp
# Utils
UTILS_SRC=Log.cpp \
	Util.cpp \
	DynamicModule.cpp \
	ZTreeMgr.cpp

HST_SRC:=HST.cpp HST_Lua.cpp
HST_OBJS:=$(foreach src, $(HST_SRC), Src/Vessel/HST/$(src:.cpp=.o))

IMGUI_SRC=imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
IMGUI_OBJS=$(foreach src, $(IMGUI_SRC), Extern/imgui/$(src:.cpp=.o))

OBJS=$(foreach src, $(GENERAL_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(DIALOG_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(BODY_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(VESSEL_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(SURFACE_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(COCKPIT_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(MDF_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(API_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(GFX_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(foreach src, $(UTILS_SRC), Src/Orbiter/$(src:.cpp=.o))
OBJS+=$(IMGUI_OBJS)

install:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DBOOST_LOG_DYN_LINK -fPIC -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(ORBITER_LIB): Src/Orbiter/Orbiter.exe

Extern/lua/liblua.so:
	cd Extern/lua/ && make -j$(nproc) liblua.so

Extern/lua/liblua.a:
	cd Extern/lua/ && make -j$(nproc) liblua.a

Extern/SOIL2/libSOIL2.a:
	cd Extern/SOIL2/ && make -j$(nproc) static

Extern/zlib-ng/libz.a:
	cd Extern/zlib-ng && ./configure --static --zlib-compat && $(MAKE)

Src/Orbiter/Orbiter.res: Src/Orbiter/Orbiter.rc
	wrc  -m32 --pedantic -v $^ -o $@

Src/Orbiter/xorbiter: Extern/zlib-ng/libz.a Extern/lua/liblua.a
Src/Orbiter/xorbiter: $(OBJS)
#	$(CXX)  $^ -o $@ $(LDFLAGS) -rdynamic -lasan -ldl -lpthread -lglfw -Wl,-rpath='$$ORIGIN:$$ORIGIN/Modules:$$ORIGIN/Modules/Plugins'
#	$(CXX)  $^ -o $@ $(LDFLAGS) -rdynamic -lubsan -ldl -lpthread -lglfw -Wl,-rpath='$$ORIGIN:$$ORIGIN/Modules:$$ORIGIN/Modules/Plugins'
	$(CXX)  $^ -o $@ $(LDFLAGS) -rdynamic -ldl -lpthread -lglfw -Wl,-rpath='$$ORIGIN:$$ORIGIN/Modules:$$ORIGIN/Modules/Plugins'

Utils/scramble/scramble.exe: Utils/scramble/scramble.o
	$(CXX) $(LDFLAGS) $^ -o $@

Utils/fchecksum/fchechsum.exe: Utils/fchecksum/fchecksum.o
	$(CXX) $(LDFLAGS) $^ -o $@

.PHONY: install
install: Src/Orbiter/xorbiter Src/Celbody/Vsop87/libVsop87.so Src/Celbody/Vsop87/Sun/libSun.so Src/Celbody/Vsop87/Mercury/libMercury.so Src/Celbody/Vsop87/Venus/libVenus.so
install: Src/Celbody/Vsop87/Earth/libEarth.so Src/Celbody/Moon/libMoon.so Src/Celbody/Vsop87/Mars/libMars.so
install: Src/Celbody/Vsop87/Jupiter/libJupiter.so Src/Celbody/Galsat/libGalsat.so Src/Celbody/Galsat/Io/libIo.so Src/Celbody/Galsat/Europa/libEuropa.so
install: Src/Celbody/Galsat/Ganymede/libGanymede.so Src/Celbody/Galsat/Callisto/libCallisto.so
install: Src/Celbody/Vsop87/Saturn/libSaturn.so Src/Celbody/Vsop87/Uranus/libUranus.so Src/Celbody/Vsop87/Neptune/libNeptune.so
install: Src/Vessel/DeltaGlider/libDeltaGlider.so
install: Src/Vessel/ShuttleA/libShuttleA.so Src/Vessel/ShuttleA/ShuttleA_PL/libShuttleA_PL.so
install: Src/Vessel/ShuttlePB/libShuttlePB.so
install: Src/Celbody/Vsop87/Earth/Atmosphere/EarthAtmJ71G/libEarthAtmJ71G.so
install: Src/Celbody/Vsop87/Venus/Atmosphere/VenusAtm2006/libVenusAtm2006.so
install: Src/Celbody/Vsop87/Mars/Atmosphere/MarsAtm2006/libMarsAtm2006.so
install: Src/Module/LuaScript/LuaInline/libLuaInline.so
#install: Src/Vessel/Atlantis/AtlantisConfig/libAtlantisConfig.so Src/Vessel/Atlantis/Atlantis/libAtlantis.so
install: Src/Vessel/Atlantis/Atlantis/libAtlantis.so Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so
#install: Src/Vessel/DeltaGlider/DGConfigurator/libDGConfigurator.so
#install: Src/Plugin/LaunchpadExtensions/AtmConfig/libAtmConfig.so
install: Src/Celbody/Satsat/Dione/libDione.so
install: Src/Celbody/Satsat/Enceladus/libEnceladus.so
install: Src/Celbody/Satsat/Hyperion/libHyperion.so
install: Src/Celbody/Satsat/Iapetus/libIapetus.so
install: Src/Celbody/Satsat/Mimas/libMimas.so
install: Src/Celbody/Satsat/Rhea/libRhea.so
install: Src/Celbody/Satsat/Tethys/libTethys.so
install: Src/Celbody/Satsat/Titan/libTitan.so
install: Src/Vessel/HST/libHST.so
install: Src/Plugin/LuaMFD/libLuaMFD.so
install: Src/Plugin/TransX/libTransX.so
install: Src/Plugin/ExtMFD/libExtMFD.so
#install: OVP/D3D7Client/libD3D7Client.so
install: OVP/OGLClient/libOGLClient.so
#install: OVP/OrbiterSkyboltClient/libOrbiterSkyboltClient.so
install: Sound/XRSound/XRSound/src/libXRSound.so
#	cp OVP/D3D7Client/libD3D7Client.so $(INSTALL_DIR)/Modules/Startup/
#	cp OVP/D3D7Client/libD3D7Client.so $(INSTALL_DIR)/Modules/Plugin/
	mkdir -p $(INSTALL_DIR)
	cd Addons/XRVessels && make -j12 install
	cd Addons/AeroBrakeMFD && make -j12 install

	cp BinAssets/Star.bin $(INSTALL_DIR)/
	cp BinAssets/Constell.bin $(INSTALL_DIR)/

	mkdir -p $(INSTALL_DIR)/Modules/Plugin/
	cp Src/Plugin/LuaMFD/libLuaMFD.so $(INSTALL_DIR)/Modules/Plugin/
	cp Src/Plugin/TransX/libTransX.so $(INSTALL_DIR)/Modules/Plugin/
	cp Src/Plugin/ExtMFD/libExtMFD.so $(INSTALL_DIR)/Modules/Plugin/
	cp Sound/XRSound/XRSound/src/libXRSound.so $(INSTALL_DIR)/Modules/Plugin/

	rm -fr $(INSTALL_DIR)/XRSound
	mkdir -p $(INSTALL_DIR)/XRSound
	cp -aR Sound/XRSound/XRSound/assets/XRSound/* $(INSTALL_DIR)/XRSound


	cp OVP/OGLClient/libOGLClient.so $(INSTALL_DIR)/Modules/Plugin/
#	cp OVP/OrbiterSkyboltClient/libOrbiterSkyboltClient.so $(INSTALL_DIR)/Modules/Plugin/
	cp OVP/OGLClient/*.fs $(INSTALL_DIR)/
	cp OVP/OGLClient/*.vs $(INSTALL_DIR)/
	cp Extern/imgui/misc/fonts/Roboto-Medium.ttf $(INSTALL_DIR)/

#Main binary
	mkdir -p $(INSTALL_DIR)
	#cp Src/Orbiter/Orbiter.exe Src/Orbiter/Orbiter.exe.so $(INSTALL_DIR)
	cp Src/Orbiter/xorbiter $(INSTALL_DIR)

#Configs
	mkdir -p $(INSTALL_DIR)/Config
	cp -aR ./Src/Celbody/Vsop87/*/Config/* $(INSTALL_DIR)/Config

#Meshes
	mkdir -p $(INSTALL_DIR)/Meshes
	cp Meshes/Vab.msh $(INSTALL_DIR)/Meshes/vab.msh

#Textures
	mkdir -p $(INSTALL_DIR)/Textures
	cp -aR Textures/* $(INSTALL_DIR)/Textures/
	#FIXME: hack, not available
	cp Textures/FCD04_n.dds $(INSTALL_DIR)/Textures/Fcd07_n.dds
	#FIXME: hack, not available
	cp Textures/FCD04_n.dds $(INSTALL_DIR)/Textures/Fcd08_n.dds
	#FIXME: hack, not available
	cp Textures/FCD04_n.dds $(INSTALL_DIR)/Textures/Fcd09_n.dds
	#FIXME
	cp Textures/Roof01.dds $(INSTALL_DIR)/Textures/Roof01_n.dds
	#FIXME
	cp Textures/door01.dds $(INSTALL_DIR)/Textures/Door01_n.dds
	#FIXME
	cp Textures/solpanel.dds $(INSTALL_DIR)/Textures/Solpanel_n.dds
	#FIXME
	cp Textures/Runway2.dds $(INSTALL_DIR)/Textures/Runway2_n.dds
	#FIXME
	cp Textures/Ball.dds $(INSTALL_DIR)/Textures/Ball_n.dds
	#FIXME
	cp Textures/Cape17.dds $(INSTALL_DIR)/Textures/Cape17_n.dds
	#FIXME
	cp Textures/Cape18.dds $(INSTALL_DIR)/Textures/Cape18_n.dds
	#FIXME
	cp Textures/Cape19.dds $(INSTALL_DIR)/Textures/Cape19_n.dds
	#FIXME
	cp Textures/Cape20.dds $(INSTALL_DIR)/Textures/Cape20_n.dds
	#FIXME
	cp Textures/Cape20.dds $(INSTALL_DIR)/Textures/Cape21.dds
	#FIXME
	cp Textures/Cape20.dds $(INSTALL_DIR)/Textures/Cape21_n.dds
	#FIXME
	cp Textures/Cape22.dds $(INSTALL_DIR)/Textures/Cape22_n.dds
	#FIXME
	cp Textures/concrete1.dds $(INSTALL_DIR)/Textures/Concrete1_n.dds


	cp Src/Vessel/ShuttlePB/Textures/ShuttlePB.dds $(INSTALL_DIR)/Textures/shuttlepb.dds
	cp Src/Vessel/ShuttlePB/Textures/ShuttlePBwing.dds $(INSTALL_DIR)/Textures/shuttlepbwing.dds
#Vessels
	mkdir -p $(INSTALL_DIR)/Modules
	cp Extern/lua/liblua.so $(INSTALL_DIR)/Modules
	mkdir -p $(INSTALL_DIR)/Extern/lua/
	cp Extern/lua/liblua.so $(INSTALL_DIR)/Extern/lua/liblua.so
	mkdir -p $(INSTALL_DIR)/Script
	cp -aR Script/* $(INSTALL_DIR)/Script

	cp Src/Vessel/HST/libHST.so $(INSTALL_DIR)/Modules

	cp Src/Vessel/DeltaGlider/libDeltaGlider.so $(INSTALL_DIR)/Modules
	mkdir -p $(INSTALL_DIR)/Meshes/DG
	cp Src/Vessel/DeltaGlider/Meshes/* $(INSTALL_DIR)/Meshes/DG
	mkdir -p $(INSTALL_DIR)/Textures/DG
	cp -aR Src/Vessel/DeltaGlider/Textures/* $(INSTALL_DIR)/Textures/DG/

	cp Src/Vessel/DeltaGlider/Config/Deltaglider.cfg $(INSTALL_DIR)/Config/DeltaGlider.cfg
	mkdir -p $(INSTALL_DIR)/Script/dg/
	cp Src/Vessel/DeltaGlider/Script/aap.lua $(INSTALL_DIR)/Script/dg/

	cp Src/Vessel/ShuttleA/libShuttleA.so $(INSTALL_DIR)/Modules
	cp Src/Vessel/ShuttleA/ShuttleA_PL/libShuttleA_PL.so $(INSTALL_DIR)/Modules
	cp Src/Vessel/ShuttleA/ShuttleA_PL/Config/ShuttleA_pl.cfg $(INSTALL_DIR)/Config/
	cp Src/Vessel/ShuttlePB/libShuttlePB.so $(INSTALL_DIR)/Modules
	mkdir -p $(INSTALL_DIR)/Meshes/ShuttleA/
	cp Src/Vessel/ShuttleA/Meshes/* $(INSTALL_DIR)/Meshes/ShuttleA/
	mkdir -p $(INSTALL_DIR)/Textures/ShuttleA/
	cp Src/Vessel/ShuttleA/Textures/* $(INSTALL_DIR)/Textures/ShuttleA/
	cp Src/Vessel/ShuttleA/ShuttleA_PL/Textures/* $(INSTALL_DIR)/Textures/ShuttleA/
	mkdir -p $(INSTALL_DIR)/Bitmaps/ShuttleA/
	cp Src/Vessel/ShuttleA/Bitmaps/* $(INSTALL_DIR)/Bitmaps/ShuttleA/

#	cp Src/Vessel/Atlantis/AtlantisConfig/libAtlantisConfig.so $(INSTALL_DIR)/Modules/
	cp Src/Vessel/Atlantis/Atlantis/libAtlantis.so $(INSTALL_DIR)/Modules/
	cp Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so $(INSTALL_DIR)/Modules/
	cp Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so $(INSTALL_DIR)/Modules/
	mkdir -p $(INSTALL_DIR)/Meshes/Atlantis/
	cp Src/Vessel/Atlantis/Atlantis/Meshes/* $(INSTALL_DIR)/Meshes/Atlantis/
	cp Src/Vessel/Atlantis/Bitmaps/tkbk_label.bmp $(INSTALL_DIR)/Bitmaps

	mkdir -p $(INSTALL_DIR)/Textures/Atlantis/
	cp Src/Vessel/Atlantis/Atlantis/Textures/* $(INSTALL_DIR)/Textures/Atlantis/
	#cp Textures/exhaust_atrcs.dds $(INSTALL_DIR)/Textures/Exhaust_atrcs.dds
	cp Meshes/Carina.msh $(INSTALL_DIR)/Meshes/carina.msh
	cp Src/Vessel/Atlantis/Atlantis_Tank/Meshes/Atlantis_tank.msh $(INSTALL_DIR)/Meshes/Atlantis/Atlantis_tank.msh
	cp Src/Vessel/Atlantis/Atlantis_Tank/Textures/Tank.dds $(INSTALL_DIR)/Textures/Atlantis/TANK.DDS
	cp Src/Vessel/Atlantis/Atlantis_SRB/Meshes/Atlantis_SRB.msh $(INSTALL_DIR)/Meshes/Atlantis/Atlantis_srb.msh
	cp Src/Vessel/Atlantis/Atlantis_SRB/Textures/* $(INSTALL_DIR)/Textures/Atlantis/

	mkdir -p $(INSTALL_DIR)/Modules/Startup/
#	cp Src/Vessel/DeltaGlider/DGConfigurator/libDGConfigurator.so $(INSTALL_DIR)/Modules/Startup/
#	cp Src/Plugin/LaunchpadExtensions/AtmConfig/libAtmConfig.so $(INSTALL_DIR)/Modules/Startup/
#lua
	cp Src/Module/LuaScript/LuaInline/libLuaInline.so $(INSTALL_DIR)
	cp Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so $(INSTALL_DIR)
#Scenarios
	cp -aR Scenarios $(INSTALL_DIR)
	cp -aR Meshes $(INSTALL_DIR)
	cp -aR Src/Vessel/*/Meshes/* $(INSTALL_DIR)/Meshes
	cp -aR Textures $(INSTALL_DIR)
	cp -aR Textures2 $(INSTALL_DIR)
	cp -aR Src/Vessel/*/Config/* $(INSTALL_DIR)/Config

	mkdir -p $(INSTALL_DIR)/Textures/ISS
	cp -aR Src/Vessel/ISS/Textures/* $(INSTALL_DIR)/Textures/ISS

#Celestial bodies
#	cp Src/Celbody/Sol/Config/Sol.cfg $(INSTALL_DIR)/Config
#	cp Src/Celbody/Moon/Config/Moon.cfg $(INSTALL_DIR)/Config
	cp -aR Src/Celbody/*/Config/* $(INSTALL_DIR)/Config
	cp -aR Src/Celbody/Galsat/*/Config/* $(INSTALL_DIR)/Config
	cp -aR Src/Celbody/Satsat/*/Config/* $(INSTALL_DIR)/Config
	cp Config/Vessels/*.cfg $(INSTALL_DIR)/Config
	mkdir -p $(INSTALL_DIR)/Config/Titan/Marker
	cp "Src/Celbody/Satsat/Titan/Config/Titan/Marker/Titan Probes.mkr" $(INSTALL_DIR)/Config/Titan/Marker

#VSOP data
	mkdir -p $(INSTALL_DIR)/Config/Sun/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87E_sun.dat $(INSTALL_DIR)/Config/Sun/Data/Vsop87E.dat
	mkdir -p $(INSTALL_DIR)/Config/Mercury/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_mer.dat $(INSTALL_DIR)/Config/Mercury/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Venus/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_mer.dat $(INSTALL_DIR)/Config/Mercury/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Venus/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_ven.dat $(INSTALL_DIR)/Config/Venus/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Earth/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_ear.dat $(INSTALL_DIR)/Config/Earth/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Mars/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_mar.dat $(INSTALL_DIR)/Config/Mars/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Jupiter/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_jup.dat $(INSTALL_DIR)/Config/Jupiter/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Saturn/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_sat.dat $(INSTALL_DIR)/Config/Saturn/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Uranus/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_ura.dat $(INSTALL_DIR)/Config/Uranus/Data/Vsop87B.dat
	mkdir -p $(INSTALL_DIR)/Config/Neptune/Data/
	cp Src/Celbody/Vsop87/Data/Vsop87B_nep.dat $(INSTALL_DIR)/Config/Neptune/Data/Vsop87B.dat
	cp Src/Celbody/Galsat/ephem_e15.dat $(INSTALL_DIR)/Config/Jupiter/Data/
	cp Src/Celbody/Satsat/tass17.dat $(INSTALL_DIR)/Config/Saturn/Data/
#Modules
	mkdir -p $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/libVsop87.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/libVsop87.so $(INSTALL_DIR)/Modules/
	cp Src/Celbody/Vsop87/Sun/libSun.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Mercury/libMercury.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Venus/libVenus.so $(INSTALL_DIR)/Modules/Celbody
	mkdir -p $(INSTALL_DIR)/Modules/Celbody/Venus/Atmosphere/
	cp Src/Celbody/Vsop87/Venus/Atmosphere/VenusAtm2006/libVenusAtm2006.so $(INSTALL_DIR)/Modules/Celbody/Venus/Atmosphere/
	cp Src/Celbody/Vsop87/Earth/libEarth.so $(INSTALL_DIR)/Modules/Celbody
	mkdir -p $(INSTALL_DIR)/Modules/Celbody/Earth/Atmosphere/
	cp Src/Celbody/Vsop87/Earth/Atmosphere/EarthAtmJ71G/libEarthAtmJ71G.so $(INSTALL_DIR)/Modules/Celbody/Earth/Atmosphere/
	cp Src/Celbody/Moon/libMoon.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Mars/libMars.so $(INSTALL_DIR)/Modules/Celbody
	mkdir -p $(INSTALL_DIR)/Modules/Celbody/Mars/Atmosphere/
	cp Src/Celbody/Vsop87/Mars/Atmosphere/MarsAtm2006/libMarsAtm2006.so $(INSTALL_DIR)/Modules/Celbody/Mars/Atmosphere/
	cp Src/Celbody/Galsat/libGalsat.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Jupiter/libJupiter.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Galsat/Io/libIo.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Galsat/Europa/libEuropa.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Galsat/Ganymede/libGanymede.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Galsat/Callisto/libCallisto.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Satsat/libSatsat.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Saturn/libSaturn.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Satsat/Dione/libDione.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Enceladus/libEnceladus.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Hyperion/libHyperion.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Iapetus/libIapetus.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Mimas/libMimas.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Rhea/libRhea.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Tethys/libTethys.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Satsat/Titan/libTitan.so $(INSTALL_DIR)/Modules/Celbody/
	cp Src/Celbody/Vsop87/Uranus/libUranus.so $(INSTALL_DIR)/Modules/Celbody
	cp Src/Celbody/Vsop87/Neptune/libNeptune.so $(INSTALL_DIR)/Modules/Celbody

$(VSOP_LIB): $(VSOP_PATH)/Vsop87.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libVsop87.so -o $@ $^

$(VSOP_PATH)/Sun/libSun.so: $(VSOP_PATH)/Sun/Sun.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libSun.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

$(VSOP_PATH)/Mercury/libMercury.so: $(VSOP_PATH)/Mercury/Mercury.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libMercury.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

$(VSOP_PATH)/Venus/libVenus.so: $(VSOP_PATH)/Venus/Venus.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libVenus.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

$(VSOP_PATH)/Venus/Atmosphere/VenusAtm2006/libVenusAtm2006.so: $(VSOP_PATH)/Venus/Atmosphere/VenusAtm2006/VenusAtm2006.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libVenusAtm2006.so -o $@ $^

$(VSOP_PATH)/Earth/libEarth.so: $(VSOP_PATH)/Earth/Earth.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libEarth.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

$(VSOP_PATH)/Earth/Atmosphere/EarthAtmJ71G/libEarthAtmJ71G.so: $(VSOP_PATH)/Earth/Atmosphere/EarthAtmJ71G/EarthAtmJ71G.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libEarthAtmJ71G.so -o $@ $^

Src/Celbody/Moon/libMoon.so: Src/Celbody/Moon/Moon.o Src/Celbody/Moon/ELP82.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libMoon.so -o $@ $^

$(VSOP_PATH)/Mars/libMars.so: $(VSOP_PATH)/Mars/Mars.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libMars.so -o $@ $^

$(VSOP_PATH)/Mars/Atmosphere/MarsAtm2006/libMarsAtm2006.so: $(VSOP_PATH)/Mars/Atmosphere/MarsAtm2006/MarsAtm2006.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libMarsAtm2006.so -o $@ $^

LIB_GALSAT=Src/Celbody/Galsat/libGalsat.so
$(LIB_GALSAT): Src/Celbody/Galsat/Galsat.o Src/Celbody/Galsat/Lieske.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libGalsat.so -o $@ $^

$(VSOP_PATH)/Jupiter/libJupiter.so: $(VSOP_PATH)/Jupiter/Jupiter.o $(LIB_GALSAT) $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libJupiter.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

Src/Celbody/Galsat/Io/libIo.so: Src/Celbody/Galsat/Io/Io.o $(LIB_GALSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libIo.so -o $@ $^

Src/Celbody/Galsat/Europa/libEuropa.so: Src/Celbody/Galsat/Europa/Europa.o $(LIB_GALSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libEuropa.so -o $@ $^

Src/Celbody/Galsat/Ganymede/libGanymede.so: Src/Celbody/Galsat/Ganymede/Ganymede.o $(LIB_GALSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libGanymede.so -o $@ $^

Src/Celbody/Galsat/Callisto/libCallisto.so: Src/Celbody/Galsat/Callisto/Callisto.o $(LIB_GALSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libCallisto.so -o $@ $^

LIB_SATSAT=Src/Celbody/Satsat/libSatsat.so
$(LIB_SATSAT): Src/Celbody/Satsat/Satsat.o Src/Celbody/Satsat/Tass17.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libSatsat.so -o $@ $^

$(VSOP_PATH)/Saturn/libSaturn.so: Src/Celbody/Vsop87/Saturn/Saturn.o $(VSOP_LIB) $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libSaturn.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

Src/Celbody/Satsat/Dione/libDione.so: Src/Celbody/Satsat/Dione/Dione.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libDione.so -o $@ $^

Src/Celbody/Satsat/Enceladus/libEnceladus.so: Src/Celbody/Satsat/Enceladus/Enceladus.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libEnceladus.so -o $@ $^

Src/Celbody/Satsat/Hyperion/libHyperion.so: Src/Celbody/Satsat/Hyperion/Hyperion.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libHyperion.so -o $@ $^

Src/Celbody/Satsat/Iapetus/libIapetus.so: Src/Celbody/Satsat/Iapetus/Iapetus.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libIapetus.so -o $@ $^

Src/Celbody/Satsat/Mimas/libMimas.so: Src/Celbody/Satsat/Mimas/Mimas.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libMimas.so -o $@ $^

Src/Celbody/Satsat/Rhea/libRhea.so: Src/Celbody/Satsat/Rhea/Rhea.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libRhea.so -o $@ $^

Src/Celbody/Satsat/Tethys/libTethys.so: Src/Celbody/Satsat/Tethys/Tethys.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libTethys.so -o $@ $^

Src/Celbody/Satsat/Titan/libTitan.so: Src/Celbody/Satsat/Titan/Titan.o $(LIB_SATSAT)
	$(CXX) -fPIC -g -shared -Wl,-soname,libTitan.so -o $@ $^

$(VSOP_PATH)/Uranus/libUranus.so: Src/Celbody/Vsop87/Uranus/Uranus.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libUranus.so -o $@ $^

$(VSOP_PATH)/Neptune/libNeptune.so: Src/Celbody/Vsop87/Neptune/Neptune.o $(VSOP_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libNeptune.so -o $@ $^

VESSEL_COMMON_SRC=Instrument.cpp
VESSEL_DG_SRC=DeltaGlider.cpp \
	DGLua.cpp \
	AAPSubsys.cpp \
	AerodynSubsys.cpp \
	AvionicsSubsys.cpp \
	DGSubsys.cpp \
	DockingSubsys.cpp \
	FailureSubsys.cpp \
	GearSubsys.cpp \
	HoverSubsys.cpp \
	HudCtrl.cpp \
	LightSubsys.cpp \
	MainRetroSubsys.cpp \
	MfdSubsys.cpp \
	PressureSubsys.cpp \
	RcsSubsys.cpp \
	ScramSubsys.cpp \
	ThermalSubsys.cpp \
	DGSwitches.cpp \
	FuelMfd.cpp \
	Horizon.cpp \
	InstrAoa.cpp \
	InstrHsi.cpp \
	InstrVs.cpp \
	MomentInd.cpp

VESSEL_COMMON_OBJS=$(foreach src, $(VESSEL_COMMON_SRC), Src/Vessel/Common/$(src:.cpp=.o))
VESSEL_DG_OBJS=$(foreach src, $(VESSEL_DG_SRC), Src/Vessel/DeltaGlider/$(src:.cpp=.o))

# Resources
#DeltaGlider.rc

Utils/meshc/meshc: Utils/meshc/Mesh.o Utils/meshc/meshc.o
	$(CXX)  $^ -o $@ $(LDFLAGS)

Src/Vessel/DeltaGlider/meshres.h: Src/Vessel/DeltaGlider/Meshes/deltaglider.msh
	Utils/meshc/meshc /I $^ /P - /O $@

Src/Vessel/DeltaGlider/meshres_vc.h: Src/Vessel/DeltaGlider/Meshes/deltaglider_vc.msh
	Utils/meshc/meshc /I $^ /P _VC /O $@

Src/Vessel/DeltaGlider/meshres_p0.h: Src/Vessel/DeltaGlider/Meshes/dg_2dpanel0.msh
	Utils/meshc/meshc /I $^ /P _P0 /O $@

Src/Vessel/DeltaGlider/meshres_p1.h: Src/Vessel/DeltaGlider/Meshes/dg_2dpanel1.msh
	Utils/meshc/meshc /I $^ /P _P1 /O $@

$(VESSEL_DG_OBJS): Src/Vessel/DeltaGlider/meshres.h Src/Vessel/DeltaGlider/meshres_vc.h
$(VESSEL_DG_OBJS): Src/Vessel/DeltaGlider/meshres_p0.h Src/Vessel/DeltaGlider/meshres_p1.h

Src/Vessel/DeltaGlider/DeltaGlider.res: Src/Vessel/DeltaGlider/DeltaGlider.rc
	wrc  -m32 --pedantic -v $^ -o $@

Src/Vessel/DeltaGlider/libDeltaGlider.so: $(VESSEL_DG_OBJS) $(VESSEL_COMMON_OBJS) Extern/lua/liblua.so
Src/Vessel/DeltaGlider/libDeltaGlider.so: $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libDeltaGlider.so -o $@ $^

Src/Vessel/HST/libHST.so: $(HST_OBJS) $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libHST.so -o $@ $^


#Src/Vessel/DeltaGlider/DGConfigurator/libDGConfigurator.so: Src/Vessel/DeltaGlider/DGConfigurator/DGConfigurator.o $(ORBITER_SDK_LIB)
#	$(CXX) -fPIC -g -shared -Wl,-soname,libDGConfigurator.so -o $@ $^

VESSEL_SHUTTLEA_SRC=adiball.cpp \
	adictrl.cpp \
	airlockswitch.cpp \
	attref.cpp \
	auxpodctrl.cpp \
	dockcvrswitch.cpp \
	gearswitch.cpp \
	hudbutton.cpp \
	InstrVs.cpp \
	mfdbutton.cpp \
	navbutton.cpp \
	needlepair.cpp \
	paneltext.cpp \
	payloadctrl.cpp \
	rcsswitch.cpp \
	ShuttleA.cpp \
	ShuttleALua.cpp \
	sliderpair.cpp \
	switches.cpp

VESSEL_SHUTTLEA_OBJS=$(foreach src, $(VESSEL_SHUTTLEA_SRC), Src/Vessel/ShuttleA/$(src:.cpp=.o))
Src/Vessel/ShuttleA/libShuttleA.so: $(ORBITER_SDK_LIB)
Src/Vessel/ShuttleA/libShuttleA.so: $(VESSEL_SHUTTLEA_OBJS) $(VESSEL_COMMON_OBJS)
	$(CXX) -fPIC -g -shared -Wl,-soname,libShuttleA.so -o Src/Vessel/ShuttleA/libShuttleA.so $(VESSEL_SHUTTLEA_OBJS) $(VESSEL_COMMON_OBJS) Extern/lua/liblua.so $(ORBITER_SDK_LIB)

Src/Vessel/ShuttleA/ShuttleA_PL/libShuttleA_PL.so: $(ORBITER_SDK_LIB)
Src/Vessel/ShuttleA/ShuttleA_PL/libShuttleA_PL.so: Src/Vessel/ShuttleA/ShuttleA_PL/ShuttleA_pl.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libShuttleA_PL.so -o Src/Vessel/ShuttleA/ShuttleA_PL/libShuttleA_PL.so Src/Vessel/ShuttleA/ShuttleA_PL/ShuttleA_pl.o $(ORBITER_SDK_LIB)

Src/Vessel/ShuttlePB/libShuttlePB.so: $(ORBITER_SDK_LIB)
Src/Vessel/ShuttlePB/libShuttlePB.so: Src/Vessel/ShuttlePB/ShuttlePB.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libShuttlePB.so -o Src/Vessel/ShuttlePB/libShuttlePB.so Src/Vessel/ShuttlePB/ShuttlePB.o $(ORBITER_SDK_LIB)

#Dragonfly
DRAGON_FLY_SRC=	Dragonfly.cpp \
	Internal.cpp \
	instruments.cpp \
	Matrix.cpp \
	quaternion.cpp \
	vectors.cpp \
	Hsystems.cpp \
	Thermal.cpp \
	Esystems.cpp

DRAGON_FLY_OBJS=$(foreach src, $(DRAGON_FLY_SRC), Src/Vessel/Dragonfly/$(src:.cpp=.o))
Src/Vessel/Dragonfly/libDragonfly.so: $(ORBITER_SDK_LIB)
Src/Vessel/Dragonfly/libDragonfly.so: $(DRAGON_FLY_OBJS)# $(VESSEL_COMMON_OBJS)
	$(CXX) -fPIC -g -shared -Wl,-soname,libDragonfly.so -o Src/Vessel/Dragonfly/libDragonfly.so $(DRAGON_FLY_OBJS) $(VESSEL_COMMON_OBJS) $(ORBITER_SDK_LIB)


#Atlantis
Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so: Src/Vessel/Atlantis/Atlantis_SRB/Atlantis_SRB.o Src/Vessel/Atlantis/Common.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libAtlantis_SRB.so -Wl,-rpath='$$ORIGIN' -o Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so Src/Vessel/Atlantis/Common.o Src/Vessel/Atlantis/Atlantis_SRB/Atlantis_SRB.o $(ORBITER_SDK_LIB)

Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so: Src/Vessel/Atlantis/Atlantis_Tank/Atlantis_Tank.o Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so
	$(CXX) -fPIC -g -shared -Wl,-soname,libAtlantis_Tank.so -Wl,-rpath='$$ORIGIN' -o Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so Src/Vessel/Atlantis/Atlantis_Tank/Atlantis_Tank.o Src/Vessel/Atlantis/Atlantis_SRB/libAtlantis_SRB.so

Src/Vessel/Atlantis/Atlantis/Atlantis.res: Src/Vessel/Atlantis/Atlantis/Atlantis.rc
	wrc  -m32 --pedantic -v  Src/Vessel/Atlantis/Atlantis/Atlantis.rc -o Src/Vessel/Atlantis/Atlantis/Atlantis.res

Src/Vessel/Atlantis/Atlantis/meshres.h: Src/Vessel/Atlantis/Atlantis/Meshes/Atlantis.msh
	Utils/meshc/meshc /I $^ /P - /O $@
	
Src/Vessel/Atlantis/Atlantis/meshres_vc.h: Src/Vessel/Atlantis/Atlantis/Meshes/AtlantisVC.msh
	Utils/meshc/meshc /I $^ /P _VC /O $@
	
Src/Vessel/Atlantis/Atlantis/Atlantis.o: Src/Vessel/Atlantis/Atlantis/meshres.h Src/Vessel/Atlantis/Atlantis/meshres_vc.h

#Src/Vessel/Atlantis/Atlantis/libAtlantis.so: Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so Src/Vessel/Atlantis/Atlantis/Atlantis.res Src/Plugin/Common/Dialog/TabDlg.o Src/Plugin/Common/Dialog/Graph.o Src/Vessel/Atlantis/Atlantis/PlBayOp.o Src/Vessel/Atlantis/Atlantis/Atlantis.o Src/Vessel/Atlantis/Atlantis/AscentAP.o
Src/Vessel/Atlantis/Atlantis/libAtlantis.so: Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so Src/Vessel/Atlantis/Atlantis/PlBayOp.o Src/Vessel/Atlantis/Atlantis/Atlantis.o Src/Vessel/Atlantis/Atlantis/AscentAP.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libAtlantis.so -Wl,-rpath='$$ORIGIN' -o Src/Vessel/Atlantis/Atlantis/libAtlantis.so Src/Vessel/Atlantis/Atlantis/PlBayOp.o Src/Vessel/Atlantis/Atlantis/Atlantis.o Src/Vessel/Atlantis/Atlantis/AscentAP.o Src/Vessel/Atlantis/Atlantis_Tank/libAtlantis_Tank.so $(ORBITER_SDK_LIB)

Src/Vessel/Atlantis/AtlantisConfig/libAtlantisConfig.so: Src/Vessel/Atlantis/AtlantisConfig/AtlantisConfig.o Src/Vessel/Atlantis/Atlantis/Atlantis.res
	$(CXX) -fPIC -g -shared -Wl,-soname,libAtlantisConfig.so -o Src/Vessel/Atlantis/AtlantisConfig/libAtlantisConfig.so Src/Vessel/Atlantis/AtlantisConfig/AtlantisConfig.o $(ORBITER_SDK_LIB) Src/Vessel/Atlantis/Atlantis/Atlantis.res

Src/Plugin/LaunchpadExtensions/AtmConfig/libAtmConfig.so: Src/Plugin/LaunchpadExtensions/AtmConfig/AtmConfig.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libAtmConfig.so -o Src/Plugin/LaunchpadExtensions/AtmConfig/libAtmConfig.so Src/Plugin/LaunchpadExtensions/AtmConfig/AtmConfig.o $(ORBITER_SDK_LIB)

#Lua stuff
Src/Module/LuaScript/LuaInline/libLuaInline.so: Src/Module/LuaScript/LuaInline/LuaInline.o Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so
	$(CXX) -fPIC -g -shared -Wl,-soname,libLuaInline.so -o Src/Module/LuaScript/LuaInline/libLuaInline.so Src/Module/LuaScript/LuaInline/LuaInline.o Extern/lua/liblua.so $(ORBITER_SDK_LIB) Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so -Wl,-rpath='$$ORIGIN'

Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so: Src/Module/LuaScript/LuaInterpreter/Interpreter.o Src/Module/LuaScript/LuaInterpreter/lua_vessel_mtd.o
	$(CXX) -fPIC -g -shared -Wl,-soname,libLuaInterpreter.so -o Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so Src/Module/LuaScript/LuaInterpreter/Interpreter.o Src/Module/LuaScript/LuaInterpreter/lua_vessel_mtd.o Extern/lua/liblua.so $(ORBITER_SDK_LIB)

Src/Plugin/LuaMFD/libLuaMFD.so: Extern/lua/liblua.so Src/Plugin/LuaMFD/LuaMFD.o Src/Plugin/LuaMFD/MfdInterpreter.o Src/Module/LuaScript/LuaInterpreter/libLuaInterpreter.so Src/Module/LuaScript/LuaInline/libLuaInline.so
	$(CXX) -fPIC -g -shared -Wl,-soname,libLuaMFD.so -o $@ $^ -Wl,-rpath='$$ORIGIN:.'

Src/Plugin/ExtMFD/libExtMFD.so: Src/Plugin/ExtMFD/ExtMFD.o Src/Plugin/ExtMFD/MFDWindow.o $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libExtMFD.so -o $@ $^ -Wl,-rpath='$$ORIGIN:.'

TRANSX_SRC:=basefunction.cpp \
    doublelink.cpp \
    globals.cpp \
    graph.cpp \
    intercept.cpp \
    mapfunction.cpp \
    mfdfunction.cpp \
    mfdvarhandler.cpp \
    mfdvariable.cpp \
    mfdvartypes.cpp \
    orbitelements.cpp \
    parser.cpp \
    planfunction.cpp \
    shiplist.cpp \
    transx.cpp \
    TransXFunction.cpp \
    transxstate.cpp \
    viewstate.cpp

TRANSX_OBJS=$(foreach src, $(TRANSX_SRC), Src/Plugin/TransX/$(src:.cpp=.o))

Src/Plugin/TransX/libTransX.so: $(TRANSX_OBJS) $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libExtMFD.so -o $@ $^ -Wl,-rpath='$$ORIGIN:.'

#D3D7
D3D7_SRC=Camera.cpp \
	CelSphere.cpp \
	CloudMgr.cpp \
	cloudmgr2.cpp \
	CSphereMgr.cpp \
	D3D7Client.cpp \
	D3D7Config.cpp \
	D3D7Enum.cpp \
	D3D7Extra.cpp \
	D3D7Frame.cpp \
	D3D7Util.cpp \
	HazeMgr.cpp \
	Light.cpp \
	Log.cpp \
	Mesh.cpp \
	MeshMgr.cpp \
	Particle.cpp \
	RingMgr.cpp \
	Scene.cpp \
	spherepatch.cpp \
	SurfMgr.cpp \
	surfmgr2.cpp \
	Texture.cpp \
	tilelabel.cpp \
	TileMgr.cpp \
	tilemgr2.cpp \
	VBase.cpp \
	VideoTab.cpp \
	VObject.cpp \
	VPlanet.cpp \
	VStar.cpp \
	VVessel.cpp \
	ztreemgr.cpp
#D3D7_OBJS=$(foreach src, $(D3D7_SRC), OVP/D3D7Client/$(src:.cpp=.o))

#OVP/D3D7Client/D3D7Client.res: OVP/D3D7Client/D3D7Client.rc
#	wrc  -m32 --pedantic -v $^ -o $@

#OVP/D3D7Client/libD3D7Client.so: $(D3D7_OBJS) OVP/D3D7Client/D3D7Client.res $(ORBITER_SDK_LIB)
#	$(CXX) -fPIC -g -shared -Wl,-soname,libD3D7Client.so -o OVP/D3D7Client/libD3D7Client.so Src/Vessel/DeltaGlider/DeltaGlider.res $(D3D7_OBJS) $(ORBITER_SDK_LIB) -Wl,-Bstatic -lwinmm -lcomctl32 -ldxguid -lddraw -Wl,-Bdynamic 

OGL_SRC=Shader.cpp CelSphere.cpp Renderer.cpp VertexBuffer.cpp Scene.cpp imgui_impl_opengl3.cpp imgui_impl_glfw.cpp
OGL_SRC+=OGLCamera.cpp MeshManager.cpp VObject.cpp VVessel.cpp Texture.cpp VPlanet.cpp SphereMesh.cpp VStar.cpp
OGL_SRC+=TileMgr.cpp SurfMgr.cpp RingMgr.cpp HazeMgr.cpp CloudMgr.cpp tilemgr2.cpp surfmgr2.cpp tilelabel.cpp
OGL_SRC+=VBase.cpp cloudmgr2.cpp fcaseopen.cpp OGLClient.cpp Particle.cpp bmp.cpp
OGL_OBJS=$(foreach src, $(OGL_SRC), OVP/OGLClient/$(src:.cpp=.o)) OVP/OGLClient/glad.o Extern/SOIL2/libSOIL2.a
OVP/OGLClient/libOGLClient.so: $(OGL_OBJS) $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -O0 -shared -Wl,-soname,libOGLClient.so -o $@ $^ -lGL -lglfw

OSC_SRC=DistantCelestialBodyFactory.cpp ObjectUtil.cpp OrbiterEntityFactory.cpp OsgSketchpad.cpp SkyboltClient.cpp TextureBlitter.cpp
OSC_SRC+=ModelFactory.cpp OpenGlContext.cpp  OrbiterModel.cpp OverlayPanelFactory.cpp SkyboltParticleStream.cpp VideoTab.cpp
OSC_SRC+=imgui_impl_glfw.cpp imgui_impl_opengl3.cpp glad.c
OSC_TP_SRC=OrbiterElevationTileSource.cpp OrbiterImageTileSource.cpp
OSC_OBJS=$(foreach src, $(OSC_SRC), OVP/Skybolt/OrbiterSkyboltClient/src/OrbiterSkyboltClient/$(src:.cpp=.o))
OSC_OBJS+=$(foreach src, $(OSC_TP_SRC), OVP/Skybolt/OrbiterSkyboltClient/src/OrbiterSkyboltClient/TileSource/$(src:.cpp=.o))
OSC_OBJS+=OVP/OrbiterSkyboltClient/src/OrbiterSkyboltClient/ThirdParty/ztreemgr.o
OSC_OBJS+=OVP/OrbiterSkyboltClient/src/OrbiterSkyboltClient/ThirdParty/nanovg/nanovg.o

OVP/Skybolt/OrbiterSkyboltClient/libOrbiterSkyboltClient.so: $(OSC_OBJS) $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -O0 -shared -Wl,-soname,libOrbiterSkyboltClient.so -o $@ $^ -losg -lGL -lglfw -losgText -lSkyboltEngine -L/home/gondos/dev/Skybolt -lboost_program_options
#-lGLEW 
#Sound
XRSOUND_SRC:=XRSound.cpp XRSoundImpl.cpp ISound.cpp \
	DefaultSoundGroupPreSteps.cpp \
	ModuleXRSoundEngine.cpp \
	SoundPreSteps.cpp \
	VesselXRSoundEngine.cpp \
	XRSoundConfigFileParser.cpp \
	XRSoundDLL.cpp \
	XRSoundEngine.cpp \
	XRSoundEngine30.cpp \
	Utils/ConfigFileParser.cpp \
	Utils/FileList.cpp

XRSOUND_OBJS:=$(foreach src, $(XRSOUND_SRC), Sound/XRSound/XRSound/src/$(src:.cpp=.o))
Sound/XRSound/XRSound/src/libXRSound.so: $(XRSOUND_OBJS) $(ORBITER_SDK_LIB)
	$(CXX) -fPIC -g -shared -Wl,-soname,libXRSound.so -o $@ $^ -lopenal -lsndfile


.PHONY: clean
clean:
	find . -name '*.o'|xargs rm -f
	find . -name '*.a'|grep -v Extern|xargs rm -f
	find . -name '*.res'|xargs rm -f
	find . -name '*.exe'|xargs rm -f
	find . -name '*.exe.so'|xargs rm -f
	find . -name 'lib*.so'|grep -v Extern|xargs rm -f
