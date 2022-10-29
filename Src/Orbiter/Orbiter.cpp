// Copyright (c) Martin Schweiger
// Licensed under the MIT License

//#define STRICT 1
#define OAPI_IMPLEMENTATION

// Enable visual styles. Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb773175(v=vs.85).aspx
//#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//#include <process.h>
//#include <direct.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <iomanip>
//#include <io.h>
#include "cmdline.h"
#include "D3dmath.h"
#include "Log.h"
#include "State.h"
#include "Astro.h"
#include "Camera.h"
#include "Pane.h"
#include "Select.h"
#include "Psys.h"
#include "Base.h"
#include "Vessel.h"
//#include "resource.h"
#include "Orbiter.h"
#include "MenuInfoBar.h"
#include "Script.h"
#include "GraphicsAPI.h"

#include "DlgCamera.h"
#include "DlgInfo.h"
#include "DlgTacc.h"
#include "DlgMap.h"
#include "DlgFocus.h"
#include "DlgFunction.h"
#include "DlgVishelper.h"
#include "DlgLaunchpad.h"
#include "DlgRecorder.h"
#include "Select.h"
#include "PlaybackEd.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image/stb_image.h"

#include <fenv.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#define _putenv putenv
#define _getcwd getcwd
#define _execl execl
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;
using namespace oapi;

#define OUTPUT_DBG
#define LOADSTATUSCOL 0xC08080 //0xFFD0D0

const int MAX_TEXTURE_BUFSIZE = 8000000;
// Texture manager buffer size. Should be determined from
// memory size (System or Video?)

const char* g_strAppTitle = "OpenOrbiter";

const char* MasterConfigFile = "Orbiter_NG.cfg";

const char* CurrentScenario = "(Current state)";
char ScenarioName[256] = "\0";
// some global string resources

char cwd[512];

// =======================================================================
// Global variables

Orbiter*        g_pOrbiter       = NULL;  // application
bool            g_bFrameMoving   = true;
extern bool     g_bAppUseZBuffer;
extern bool     g_bAppUseBackBuffer;
double          g_nearplane      = 5.0;
double          g_farplane       = 5e6;
const double    MinWarpLimit     = 0.1;  // make variable
const double    MaxWarpLimit     = 1e5;  // make variable
int           g_qsaveid        = 0;
int           g_customcmdid    = 0;

// 2D info output flags
bool		    g_bOutputTime    = true;
bool		    g_bOutputFPS     = true;
bool            g_bOutputDim     = true;
bool		    g_bForceUpdate   = true;
bool            g_bShowGrapple   = false;
bool            g_bStateUpdate   = false;

// Timing parameters
int  launch_tick;      // counts the first 3 frames
int  g_vtxcount = 0;   // vertices/frame rendered (for diagnosis)
int  g_tilecount = 0;  // surface tiles/frame rendered (for diagnosis)
TimeData td;             // timing information

// Configuration parameters set from Driver.cfg
int requestDriver     = 0;
int requestFullscreen = 0;
int requestSoftware   = 0;
int requestScreenW    = 640;
int requestScreenH    = 480;
int requestWindowW    = 400;
int requestWindowH    = 300;
int requestZDepth     = 16;

// Logical objects
Camera          *g_camera = 0;         // observer camera
Pane            *g_pane = 0;           // 2D output surface
PlanetarySystem *g_psys = 0;
Vessel          *g_focusobj = 0;       // current vessel with input focus
Vessel          *g_pfocusobj = 0;      // previous vessel with input focus

char DBG_MSG[256] = "";

// =======================================================================
// Function prototypes

void    DestroyWorld ();


// =======================================================================
// _matherr()
// trap global math exceptions
/* FIXME : overload acos to handle error cases
int _matherr(struct _exception *except )
{
	if (!strcmp (except->name, "acos")) {
		except->retval = (except->arg1 < 0.0 ? Pi : 0.0);
		return 1;
	}
	return 0;
}
*/

// =======================================================================
// WinMain()
// Application entry containing message loop
#define _GNU_SOURCE 1
#include <fenv.h>

int main(int argc, const char *argv[])
{
	feenableexcept (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);

	glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);

	if (!glfwInit()) {
		printf("glfwInit failed\n");
		exit(EXIT_FAILURE);
	}

	g_pOrbiter = new Orbiter; // application instance

	// Parse command line
	//orbiter::CommandLine::Parse(g_pOrbiter, strCmdLine);

	// Initialise the log
	INITLOG("/tmp/Orbiter.log", g_pOrbiter->Cfg()->CfgCmdlinePrm.bAppendLog); // init log file
#ifdef ISBETA
	LOGOUT("Build %s BETA [v.%06d]", __DATE__, GetVersion());
#else
	//LOGOUT("Build %s [v.%06d]", __DATE__, GetVersion());
#endif

	// Initialise random number generator
	//srand ((unsigned)time (NULL));
	srand(12345);

	// Create application
	g_pOrbiter->Create ();

	g_pOrbiter->Run ();
	delete g_pOrbiter;
	return 0;
}

// =======================================================================
// InitializeWorld()
// Create logical objects

bool Orbiter::InitializeWorld (char *name)
{
	g_pane = new Pane (gclient, viewW, viewH, viewBPP); TRACENEW
	if (g_camera) delete g_camera;
	g_camera = new Camera (g_nearplane, g_farplane); TRACENEW
	g_camera->ResizeViewport (viewW, viewH);
	if (g_psys) delete g_psys;
	g_psys = new PlanetarySystem (name); TRACENEW
	if (!g_psys->nObj()) {  // sanity check
		DestroyWorld();
		return false;
	}
	return true;
}

// =======================================================================
// DestroyWorld()
// Destroy logical objects

void DestroyWorld ()
{
	if (g_camera) { delete g_camera; g_camera = 0; }
	if (g_psys)   { delete g_psys;   g_psys = 0; }
}

//=============================================================================
// Name: class Orbiter
// Desc: Main application class
//=============================================================================

//-----------------------------------------------------------------------------
// Name: Orbiter()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
Orbiter::Orbiter ()
{
	// override base class defaults
    //m_bAppUseZBuffer  = true;
    //m_fnConfirmDevice = ConfirmDevice;
	nmodule         = 0;
	pConfig         = new Config; TRACENEW
	pState          = NULL;
	bFullscreen     = false;
	viewW = viewH = viewBPP = 0;
	gclient         = NULL;
	hScnInterp      = NULL;
	snote_playback  = NULL;
	nsnote          = 0;
	bVisible        = false;
	bAllowInput     = false;
	bRunning        = false;
	bRequestRunning = false;
	bSession        = false;
	bRenderOnce 	= false;
	bEnableLighting = true;
	bUseStencil     = false;
	bKeepFocus      = false;
	bRealtime       = true;
	bEnableAtt      = true;
	bRecord         = false;
	bPlayback       = false;
	bCapture        = false;
	bFastExit       = false;
	cfglen          = 0;
	ncustomcmd      = 0;
	script          = NULL;

	simheapsize     = 0;

	for (int i = 0; i < 15; i++)
		ctrlKeyboard[i] = ctrlJoystick[i] = ctrlTotal[i] = 0; // reset keyboard and joystick attitude requests

	memset (simkstate, 0, 256);

}

//-----------------------------------------------------------------------------
// Name: ~Orbiter()
// Desc: Application destructor.
//-----------------------------------------------------------------------------
Orbiter::~Orbiter ()
{
	m_pGUIManager->UnregisterCtrl(m_DlgCamera.get());
	m_pGUIManager->UnregisterCtrl(m_DlgInfo.get());
	m_pGUIManager->UnregisterCtrl(m_DlgTacc.get());
	m_pGUIManager->UnregisterCtrl(m_DlgMap.get());
	m_pGUIManager->UnregisterCtrl(m_DlgFocus.get());
	m_pGUIManager->UnregisterCtrl(m_DlgFunction.get());
	m_pGUIManager->UnregisterCtrl(m_DlgVishelper.get());
	m_pGUIManager->UnregisterCtrl(m_DlgLaunchpad.get());
	m_pGUIManager->UnregisterCtrl(m_DlgSelect.get());
	m_pGUIManager->UnregisterCtrl(m_DlgInputBox.get());
	m_pGUIManager->UnregisterCtrl(m_DlgPlaybackEditor.get());
	m_pGUIManager->UnregisterCtrl(m_DlgRecorder.get());
	
	CloseApp ();

	UnloadModule(hVideoModule);
}
//-----------------------------------------------------------------------------
// Name: Create()
//-----------------------------------------------------------------------------
void Orbiter::Create ()
{
	pConfig->Load(MasterConfigFile);
	strcpy (cfgpath, pConfig->CfgDirPrm.ConfigDir);   cfglen = strlen (cfgpath);

	// validate configuration
//	if (pConfig->CfgJoystickPrm.Joy_idx > GetDInput()->NumJoysticks()) pConfig->CfgJoystickPrm.Joy_idx = 0;

	// Read key mapping from file (or write default keymap)
	if (!keymap.Read ("keymap.cfg")) keymap.Write ("keymap.cfg");

    pState = new State(); TRACENEW

	if (pConfig->CfgCmdlinePrm.bFastExit)
		SetFastExit(true);

	Instrument::RegisterBuiltinModes();

	script = new ScriptInterface (this); TRACENEW

	m_DlgLaunchpad = std::make_unique<DlgLaunchpad>("Launchpad");

	// preload fixed plugin modules
	LoadFixedModules ();

	// preload modules from command line requests
	for (auto &plugin: pConfig->CfgCmdlinePrm.LoadPlugins)
		LoadModule("Modules/Plugin", plugin.c_str());

	// preload active plugin modules

	hVideoModule = LoadModule("Modules/Plugin", pConfig->m_videoPlugin.c_str(), true, true);
	CreateRenderWindow();

	GLFWimage icon;
	icon.pixels = stbi_load("Images/Orbiter.png", &icon.width, &icon.height, 0, 4);
	glfwSetWindowIcon(m_pGUIManager->hRenderWnd, 1, &icon);
	stbi_image_free(icon.pixels);

	for (const auto &mod: pConfig->m_actmod) {
		if(mod == pConfig->m_videoPlugin) continue;
		LoadModule ("Modules/Plugin", mod.c_str(), false);
	}
	
	InputController::GlobalInit();

	//CreateRenderWindow();
}

//-----------------------------------------------------------------------------
// Name: SaveConfig()
// Desc: Save configuration files (before closedown)
//-----------------------------------------------------------------------------
void Orbiter::SaveConfig ()
{
	pConfig->Write (); // save current settings
	//m_pLaunchpad->WriteExtraParams ();
}

//-----------------------------------------------------------------------------
// Name: CloseApp()
// Desc: Cleanup for program end
//-----------------------------------------------------------------------------
void Orbiter::CloseApp (bool fast_shutdown)
{
	SaveConfig();
	if(bSession)
		CloseSession();

	int nbMod = nmodule;
	for(int i = nbMod - 1; i >= 0 ; i--) {
		if(!module[i].bVideoPlugin)
			UnloadModule (module[i].hMod);
	}

	if (!fast_shutdown) {
		if (pConfig)  delete pConfig;
		if (pState)   delete pState;
		if (script) delete script;
		if (ncustomcmd) {
			for (int i = 0; i < ncustomcmd; i++) delete []customcmd[i].label;
			delete []customcmd;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: GetVersion()
// Desc: Returns orbiter build version as integer in YYMMDD format
//-----------------------------------------------------------------------------
int Orbiter::GetVersion () const
{
	static int v = 0;
	if (!v) {
		static const char *mstr[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		char ms[32];
		int day, month, year;
		sscanf (__DATE__, "%s%d%d", ms, &day, &year);
		for (month = 0; month < 12; month++)
			if (!_strnicmp (ms, mstr[month], 3)) break;
		v = (year%100)*10000 + (month+1)*100 + day;
	}
	return v;
}

//-----------------------------------------------------------------------------
// Name: LoadFixedModules()
// Desc: Load all plugin modules from the "startup" directory
//-----------------------------------------------------------------------------

void Orbiter::LoadFixedModules ()
{
#if 0
	char cbuf[256];
	char *path = "Modules\\Startup";
	struct _finddata_t fdata;
	intptr_t fh = _findfirst ("Modules\\Startup\\*.dll", &fdata);
	if (fh == -1) return; // no files found
	do {
		strcpy (cbuf, fdata.name);
		cbuf[(strlen(cbuf)-4)] = '\0'; // cut off extension
		LoadModule (path, cbuf);
	} while (!_findnext (fh, &fdata));
	_findclose (fh);
#else
	char cbuf[256];
	struct dirent * dp = NULL;
	const char *path = "Modules/Startup";
	DIR *dir = opendir(path);
	if(dir != NULL) {
		while ((dp = readdir(dir)) != NULL) {
			auto len = strlen(dp->d_name);
			if (!strcasecmp(dp->d_name + len - 3, ".so")) {
				strcpy (cbuf, dp->d_name);
				cbuf[(strlen(cbuf)-3)] = '\0'; // cut off extension
				LoadModule (path, cbuf + 3);
			}
		}
		closedir(dir);
	}
#endif
}

//-----------------------------------------------------------------------------
// Name: LoadModule()
// Desc: Load a named plugin DLL
//-----------------------------------------------------------------------------
MODULEHANDLE Orbiter::LoadModule (const char *path, const char *name, bool fatal, bool video)
{
	char cbuf[256];
	sprintf (cbuf, "%s/lib%s.so", path, name);

	register_module = NULL; // clear the module
	MODULEHANDLE hi = oapiModuleLoad(cbuf);
	if (hi) {
		struct DLLModule *tmp = new struct DLLModule[nmodule+1]; TRACENEW
		if (nmodule) {
			memcpy (tmp, module, nmodule*sizeof(struct DLLModule));
			delete []module;
		}
		module = tmp;

		void (*opcDLLInit)(MODULEHANDLE) = (void(*)(MODULEHANDLE))oapiModuleGetProcAddress(hi, "opcDLLInit");
		if(opcDLLInit)
			opcDLLInit(hi);

		module[nmodule].module = (register_module ? register_module : new oapi::Module (hi));
		module[nmodule].bLocalAlloc = register_module == nullptr;
		module[nmodule].hMod = hi;
		module[nmodule].bVideoPlugin = video;
		module[nmodule].name = new char[strlen(name)+1]; TRACENEW
		strcpy (module[nmodule].name, name);
		nmodule++;
	} else {
		const char *err = dlerror();
		printf ("Failed loading module %s (code %s)\n", cbuf, err);
		if(fatal)
			exit(EXIT_FAILURE);
		else
			return nullptr;
	}
	return hi;
}

//-----------------------------------------------------------------------------
// Name: UnloadModule()
// Desc: Unload a module by its instance
//-----------------------------------------------------------------------------
void Orbiter::UnloadModule (MODULEHANDLE hi)
{
	int i, j, k;
	struct DLLModule *tmp;
	for (i = 0; i < nmodule; i++)
		if (hi == module[i].hMod) break;
	if (i == nmodule) return; // not present
	delete []module[i].name;
	if(module[i].bLocalAlloc)
		delete module[i].module;
	dlclose (module[i].hMod);
	if (nmodule > 1) {
		tmp = new struct DLLModule[nmodule-1]; TRACENEW
		for (j = k = 0; j < nmodule; j++)
			if (j != i) tmp[k++] = module[j];
	} else tmp = 0;
	delete []module;
	module = tmp;
	nmodule--;
}
void Orbiter::UnloadModule (const char *name)
{
	int i, j, k;
	struct DLLModule *tmp;
	for (i = 0; i < nmodule; i++)
		if (!strcmp(name, module[i].name)) break;
	if (i == nmodule) return; // not present
	delete []module[i].name;
	if(module[i].bLocalAlloc)
		delete module[i].module;
	dlclose (module[i].hMod);
	if (nmodule > 1) {
		tmp = new struct DLLModule[nmodule-1]; TRACENEW
		for (j = k = 0; j < nmodule; j++)
			if (j != i) tmp[k++] = module[j];
	} else tmp = 0;
	delete []module;
	module = tmp;
	nmodule--;
}
//-----------------------------------------------------------------------------
// Name: FindModuleProc()
// Desc: Returns address of a procedure in a plugin module
//-----------------------------------------------------------------------------
OPC_Proc Orbiter::FindModuleProc (int nmod, const char *procname)
{
	return (OPC_Proc)oapiModuleGetProcAddress(module[nmod].hMod, procname);
}

//-----------------------------------------------------------------------------
// Name: Launch()
// Desc: Launch simulator
//-----------------------------------------------------------------------------
void Orbiter::Launch (const char *scenario)
{
	bool have_state = false;
	pConfig->Write (); // save current settings
	//m_pLaunchpad->WriteExtraParams ();

	if (!have_state && !pState->Read (ScnPath (scenario))) {
		LOGOUT_ERR ("Scenario not found: %s", scenario);
		TerminateOnError();
	}
	StartSession (pConfig, scenario);
}

void Orbiter::StartSession (Config *pCfg, const char *scenario)
{
	int i;
	SetLogVerbosity (pCfg->CfgDebugPrm.bVerboseLog);
	LOGOUT("");
	LOGOUT("**** Creating simulation session");

	m_pGUIManager->HideCtrl<DlgLaunchpad>();

	// read simulation environment state
	strcpy (ScenarioName, scenario);
	g_qsaveid = 0;
	launch_tick = 3;

	// Generate logical world objects
	BroadcastGlobalInit ();
	RigidBody::GlobalSetup();

	td.Reset (pState->Mjd());
	if (Cfg()->CfgCmdlinePrm.FixedStep > 0.0)
		td.SetFixedStep(Cfg()->CfgCmdlinePrm.FixedStep);
	else if (Cfg()->CfgDebugPrm.FixedStep > 0.0)
		td.SetFixedStep(Cfg()->CfgDebugPrm.FixedStep);

	if (!InitializeWorld (pState->Solsys())) {
		LOGOUT_ERR_FILENOTFOUND_MSG(g_pOrbiter->ConfigPath (pState->Solsys()), "while initialising solar system %s", pState->Solsys());
		TerminateOnError();
		return;
	}
	LOGOUT("Finished initialising world");
	time_prev = std::chrono::steady_clock::now() - std::chrono::microseconds(1); // make sure SimDT > 0 for first frame

	g_psys->InitState (ScnPath (scenario));

	g_focusobj = 0;
	Vessel *vfocus = g_psys->GetVessel (pState->Focus());
	if (!vfocus)
		vfocus = g_psys->GetVessel (0); // in case no focus vessel was defined
	SetFocusObject (vfocus, false);

	LOGOUT("Finished initialising status");

	g_camera->InitState (scenario, g_focusobj);
	if (g_pane) g_pane->SetFOV (g_camera->Aperture());
	LOGOUT ("Finished initialising camera");

	bSession = true;
	bVisible = true;
	bActive = bVisible;
	bRunning = bRequestRunning = true;
	bRenderOnce = false;
	g_bForceUpdate = true;
#ifdef UNDEF
	if (pCfg->CfgLogicPrm.bStartPaused) {
		BeginTimeStep (true);
		UpdateWorld(); // otherwise it doesn't get initialised during pause
		EndTimeStep (true);
		Pause (true);
	}
#endif
	FRecorder_Reset();
	if ((bPlayback = g_focusobj->bFRplayback)) {
		FRecorder_OpenPlayback (pState->PlaybackDir());
		if (g_pane && g_pane->MIBar()) g_pane->MIBar()->SetPlayback(true);
	}

	// let plugins read their states from the scenario file
	for (i = 0; i < nmodule; i++) {
		void (*opcLoadState)(FILEHANDLE) = (void(*)(FILEHANDLE))FindModuleProc (i, "opcLoadState");
		if (opcLoadState) {
			ifstream ifs (ScnPath (scenario));
			char cbuf[256] = "BEGIN_";
			strcat (cbuf, module[i].name);
			if (FindLine (ifs, cbuf)) {
				opcLoadState ((FILEHANDLE)&ifs);
			}
		}
	}

	Module::RenderMode rendermode = bFullscreen ? Module::RENDER_FULLSCREEN : Module::RENDER_WINDOW;

	//for (i = 0; i < nmodule; i++) {
	//	module[i].module->clbkSimulationStart (rendermode);
	//	CHECKCWD(cwd,module[i].name);
	//}

	LOGOUT ("Finished setting up render state");

	const char *scriptcmd = pState->Script();
	hScnInterp = (scriptcmd ? script->RunInterpreter (scriptcmd) : NULL);

	if (gclient) gclient->clbkPostCreation();
	g_psys->PostCreation ();
#define CHECKCWD(cwd,name) { char c[512]; _getcwd(c,512); if(strcmp(c,cwd)) { _chdir(cwd); sprintf (c,"CWD modified by module %s - Fixing.\n",name); printf("%s",c); } }

	for (i = 0; i < nmodule; i++) {
		module[i].module->clbkSimulationStart (rendermode);
		CHECKCWD(cwd,module[i].name);
	}

	if (g_pane) {
		g_pane->InitState (ScnPath (scenario));
		LOGOUT ("Finished initialising panels");
	}

	if (pCfg->CfgLogicPrm.bStartPaused) {
		BeginTimeStep (true);
		UpdateWorld(); // otherwise it doesn't get initialised during pause
		EndTimeStep (true);
		Pause (true);
	}
}

//-----------------------------------------------------------------------------
// Name: CreateRenderWindow()
// Desc: Create the window used for rendering the scene
//-----------------------------------------------------------------------------
void Orbiter::CreateRenderWindow ()
{
	/* Initialize the library */
    glfwWindowHint(GLFW_DOUBLEBUFFER , 1);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
	//	glfwWindowHint(GLFW_SAMPLES, 4);

	m_pGUIManager  = std::make_unique<GUIManager>();

	GetRenderParameters ();

	// Generate logical world objects
	Base::CreateStaticDeviceObjects();

	m_DlgCamera    = std::make_unique<DlgCamera>("Camera");
	m_DlgInfo      = std::make_unique<DlgInfo>("Info");
	m_DlgTacc      = std::make_unique<DlgTacc>("Time Acceleration");
	m_DlgMap       = std::make_unique<DlgMap>("Map");
	m_DlgFocus     = std::make_unique<DlgFocus>("Ship");
	m_DlgFunction  = std::make_unique<DlgFunction>("Custom Functions");
	m_DlgVishelper = std::make_unique<DlgVishelper>("Visual Helpers");
	m_DlgSelect    = std::make_unique<Select>("Select");
	m_DlgInputBox  = std::make_unique<InputBox>("InputBox");
	m_DlgRecorder  = std::make_unique<DlgRecorder>("Recorder");
	m_DlgPlaybackEditor = std::make_unique<DlgPlaybackEditor>("DlgPlaybackEditor");
	m_DlgJoystick  = std::make_unique<DlgJoystick>("Joystick Configuration");
	
	
	m_pGUIManager->RegisterCtrl(m_DlgCamera.get());
	m_pGUIManager->RegisterCtrl(m_DlgInfo.get());
	m_pGUIManager->RegisterCtrl(m_DlgTacc.get());
	m_pGUIManager->RegisterCtrl(m_DlgMap.get());
	m_pGUIManager->RegisterCtrl(m_DlgFocus.get());
	m_pGUIManager->RegisterCtrl(m_DlgFunction.get());
	m_pGUIManager->RegisterCtrl(m_DlgVishelper.get());
	m_pGUIManager->RegisterCtrl(m_DlgLaunchpad.get());
	m_pGUIManager->RegisterCtrl(m_DlgSelect.get());
	m_pGUIManager->RegisterCtrl(m_DlgInputBox.get());
	m_pGUIManager->RegisterCtrl(m_DlgPlaybackEditor.get());
	m_pGUIManager->RegisterCtrl(m_DlgRecorder.get());
	m_pGUIManager->RegisterCtrl(m_DlgJoystick.get());

	// playback screen annotation manager
	snote_playback = gclient->clbkCreateAnnotation ();

	LOGOUT ("Finished setting up render state");
}

void Orbiter::PreCloseSession()
{
	if (gclient && pConfig->CfgDebugPrm.bSaveExitScreen)
		gclient->clbkSaveSurfaceToImage (0, "Images/CurrentState", oapi::IMAGE_JPG);
}

//-----------------------------------------------------------------------------
// Name: CloseSession()
// Desc: Destroy render window and associated devices
//-----------------------------------------------------------------------------
void Orbiter::CloseSession ()
{
	int i;

	bSession = false;
	if      (bRecord)   ToggleRecorder();
	else if (bPlayback) EndPlayback();
	const char *desc = "Current scenario state\n\n![screenshot](Images/CurrentState.jpg)";
	SaveScenario (CurrentScenario, desc);
	if (hScnInterp) {
		script->DelInterpreter (hScnInterp);
		hScnInterp = NULL;
	}

	if (pConfig->CfgDebugPrm.ShutdownMode == 0 && !bFastExit) { // normal cleanup
		if (gclient) {
			gclient->clbkCloseSession (false);
			Base::DestroyStaticDeviceObjects ();
		}
		if (snote_playback) delete snote_playback;
		if (nsnote) {
			for (int i = 0; i < nsnote; i++) delete snote[i];
			delete []snote;
			nsnote = 0;
		}

		if (g_pane) { delete g_pane;   g_pane = 0; }
		Instrument::GlobalExit (gclient);
		meshmanager.Flush(); // destroy buffered meshes
		DestroyWorld ();     // destroy logical objects
		if (gclient)
			gclient->clbkDestroyRenderWindow (false); // destroy graphics objects

		for (i = 0; i < nmodule; i++)
			module[i].module->clbkSimulationEnd();

	} else {
		if (gclient) {
			gclient->clbkCloseSession (true);
			gclient->clbkDestroyRenderWindow (true);
		}

		for (i = 0; i < nmodule; i++)
			module[i].module->clbkSimulationEnd();

		CloseApp (true);
		if (pConfig->CfgDebugPrm.ShutdownMode == 2 || bFastExit) {
			LOGOUT("**** Fast process shutdown\r\n");
			exit (0); // just kill the process
		} else {
			LOGOUT("**** Respawning Orbiter process\r\n");
			const char *name = "modules/server/orbiter.exe";

			_execl (name, name, "-l", NULL);   // respawn the process
		}
	}
	m_pGUIManager->ShowCtrl<DlgLaunchpad>();
	LOGOUT("**** Closing simulation session");
}

// =======================================================================
// Query graphics client for render parameters

void Orbiter::GetRenderParameters ()
{
	if (!gclient) return; // sanity check

	int val;
	gclient->clbkGetViewportSize (&viewW, &viewH);
	viewBPP = (gclient->clbkGetRenderParam (RP_COLOURDEPTH, &val) ? val:0);
	bFullscreen = gclient->clbkFullscreenMode();
	bUseStencil = (pConfig->CfgDevPrm.bTryStencil && 
		gclient->clbkGetRenderParam (RP_STENCILDEPTH, &val) && val >= 1);
}

// =======================================================================
// Send session initialisation signal to various components

void Orbiter::BroadcastGlobalInit ()
{
	Instrument::GlobalInit (gclient);
//	DlgMap::GlobalInit();
}

// =======================================================================
// Render3DEnvironment()
// Draws the scene

void Orbiter::Render3DEnvironment ()
{
	if (gclient) {
		gclient->clbkRenderScene ();
		Output2DData ();
	}
}

void Orbiter::RenderGUI ()
{
	m_pGUIManager->RenderGUI();
}

void Orbiter::DisplayFrame ()
{
	if (gclient) {
		gclient->clbkDisplayFrame ();
	}
}

void Orbiter::ClearFrame ()
{
	if (gclient) {
		gclient->clbkClearFrame ();
	}
}

//-----------------------------------------------------------------------------
// Name: ScreenToClient()
// Desc: Converts screen to client coordinates. In fullscreen mode they are identical
//-----------------------------------------------------------------------------
void Orbiter::ScreenToClient (POINT *pt) const
{
	/*
	if (!IsFullscreen() && hRenderWnd)
		::ScreenToClient (hRenderWnd, pt);*/
}

void Orbiter::KeyCallback(int key, char state) {
	if(key && bSession) {
		simkstate[key] = state;
		if(state) {
			BroadcastBufferedKeyboardEvent (simkstate, key);
			//if (!skipkbd) {
				KbdInputBuffered_System (simkstate, key);
				if (bRunning) KbdInputBuffered_OnRunning (simkstate, key);
			//}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: Run()
// Desc: Message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
int Orbiter::Run ()
{
	if (!pConfig->CfgCmdlinePrm.LaunchScenario.empty())
		Launch (pConfig->CfgCmdlinePrm.LaunchScenario.c_str());
	// otherwise wait for the user to make a selection from the scenario
	// list in the launchpad dialog

	m_pGUIManager->SetupCallbacks();

	while (!m_pGUIManager->ShouldCloseWindow()) {
		glfwPollEvents();
	
		if (bSession) {
			if (bAllowInput) bActive = true, bAllowInput = false;
			if (BeginTimeStep (bRunning)) {
				UpdateWorld();
				EndTimeStep (bRunning);
				if (bVisible) {
					if (bActive) UserInput ();
					bRenderOnce = true;
				}
				if (bRunning && bCapture) {
					CaptureVideoFrame ();
				}
			}
		}
	
		if (bRenderOnce && bVisible) {
			Render3DEnvironment();
			RenderGUI();
			DisplayFrame();
			bRenderOnce = false;
		} else {
			ClearFrame();
			RenderGUI();
			DisplayFrame();
		}
    }
	if(bSession)
		PreCloseSession();

	//hRenderWnd = NULL;
    return 0;
}

void Orbiter::SingleFrame ()
{
	if (bSession) {
		if (bAllowInput) bActive = true, bAllowInput = false;
		if (BeginTimeStep (bRunning)) {
			UpdateWorld();
			EndTimeStep (bRunning);
			if (bVisible) {
				if (bActive) UserInput ();
				Render3DEnvironment();
			}
		}
	} else {
		RenderGUI();
		DisplayFrame();
	}
}

void Orbiter::TerminateOnError ()
{
	LogOut (">>> TERMINATING <<<");
	glfwTerminate();
	printf ("Terminating after critical error. See Orbiter.log for details.");
	exit (EXIT_FAILURE);
}

void Orbiter::InitRotationMode ()
{
	bKeepFocus = true;
	m_pGUIManager->DisableMouseCursor();
}

void Orbiter::ExitRotationMode ()
{
	bKeepFocus = false;
	m_pGUIManager->EnableMouseCursor();
}

//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Stop/continue simulation
//-----------------------------------------------------------------------------
void Orbiter::Pause (bool bPause)
{
	if (bRunning != bPause) return;  // nothing to do
	bRequestRunning = !bPause;
}

void Orbiter::Freeze (bool bFreeze)
{
	if (bRunning != bFreeze) return; // nothing to do
	bRunning = !bFreeze;
	bSession = !bFreeze;

	// broadcast pause state to plugins
	for (int k = 0; k < nmodule; k++) {
		module[k].module->clbkPause (bFreeze);
	}
	if (bFreeze) Suspend ();
	else Resume ();
}

//-----------------------------------------------------------------------------
// Name: SetFocusObject()
// Desc: Select a new user-controlled vessel
//       Return value is the previous focus object
//-----------------------------------------------------------------------------
Vessel *Orbiter::SetFocusObject (Vessel *vessel, bool setview)
{
	if (vessel == g_focusobj) return 0; // nothing to do

	g_pfocusobj = g_focusobj;
	g_focusobj = vessel;

	// Inform pane about focus change
	if (g_pane) g_pane->FocusChanged (g_focusobj);
	InputController::SwitchProfile(g_focusobj->ClassName());

	// switch camera
	if (setview) SetView (g_focusobj, 2);

	// vessel and plugin callback
	if (g_pfocusobj) g_pfocusobj->FocusChanged (false, g_focusobj, g_pfocusobj);
	g_focusobj->FocusChanged (true, g_focusobj, g_pfocusobj);

	for (int i = 0; i < nmodule; i++)
		module[i].module->clbkFocusChanged (g_focusobj, g_pfocusobj);
		//if (module[i].intf->opcFocusChanged)
		//	module[i].intf->opcFocusChanged (g_focusobj, g_pfocusobj);


	return g_pfocusobj;
}

// =======================================================================
// SetView()
// Change camera target, or camera mode (cockpit/external)

void Orbiter::SetView (Body *body, int mode)
{
	g_camera->Attach (body, mode);
	g_bForceUpdate = true;
}

//-----------------------------------------------------------------------------
// Name: InsertVessels
// Desc: Insert a newly created vessel into the simulation
//-----------------------------------------------------------------------------
void Orbiter::InsertVessel (Vessel *vessel)
{
	g_psys->AddVessel (vessel);

	// broadcast vessel creation to plugins
	for (int k = 0; k < nmodule; k++)
		module[k].module->clbkNewVessel ((OBJHANDLE)vessel);

	//if (gclient) gclient->clbkDialogBroadcast (MSG_CREATEVESSEL, vessel);

	vessel->PostCreation();
	vessel->InitSupervessel();
	vessel->ModulePostCreation();
}

//-----------------------------------------------------------------------------
// Name: KillVessels()
// Desc: Kill the vessels that have been marked for deletion in the last time
//       step
//-----------------------------------------------------------------------------
bool Orbiter::KillVessels ()
{
	int i, n = g_psys->nVessel();
	int j;

	for (i = n-1; i >= 0; i--) {
		if (g_psys->GetVessel(i)->KillPending()) {
			Vessel *vessel = g_psys->GetVessel(i);
			// switch to new focus object
			if (vessel == g_focusobj) {
				if (vessel->ProxyVessel() && !vessel->ProxyVessel()->KillPending()) {
					SetFocusObject (vessel->ProxyVessel(), false);
				} else {
					double d, dmin = 1e20;
					Vessel *v, *tgt = 0;
					for (j = 0; j < g_psys->nVessel(); j++) {
						v = g_psys->GetVessel(j);
						if (v->KillPending()) continue;
						if (v != vessel && v->GetEnableFocus()) {
							d = vessel->GPos().dist (v->GPos());
							if (d < dmin) dmin = d, tgt = v;
						}
					}
					if (tgt) SetFocusObject (tgt, false);
					else return false; // no focus object available - give up
				}
			}
			if (vessel == g_pfocusobj)
				g_pfocusobj = 0; // clear previous focus (for Ctrl-F3 fast-switching)

			// switch to new camera target
			if (vessel == g_camera->Target())
				SetView (g_focusobj, 1);
			// broadcast vessel destruction to plugins
			for (int k = 0; k < nmodule; k++) {
				module[k].module->clbkDeleteVessel ((OBJHANDLE)vessel);
			}
			// broadcast vessel destruction to all vessels
			g_psys->BroadcastVessel (MSG_KILLVESSEL, vessel);
			// broadcast vessel destruction to all MFDs
			if (g_pane) g_pane->DelVessel (vessel);
			// broadcast vessel destruction to all open dialogs
			//if (gclient) gclient->clbkDialogBroadcast (MSG_KILLVESSEL, vessel);
			// echo deletion on console window
			// kill the vessel
			g_psys->DelVessel (vessel, 0);
		}
	}
	return true;
}

void Orbiter::NotifyObjectJump (const Body *obj, const Vector &shift)
{
	if (obj == g_camera->Target()) g_camera->Drag (-shift);
	if (g_camera->Target()) g_camera->Update ();

	// notify plugins
	for (int k = 0; k < nmodule; k++)
		module[k].module->clbkVesselJump ((OBJHANDLE)obj);

}

void Orbiter::NotifyObjectSize (const Body *obj)
{
	if (obj == g_camera->Target()) g_camera->Drag (Vector(0,0,0));
}

//-----------------------------------------------------------------------------
// Name: SetWarpFactor()
// Desc: Set time acceleration factor
//-----------------------------------------------------------------------------
void Orbiter::SetWarpFactor (double warp, bool force, double delay)
{
	if (warp == td.Warp())
		return; // nothing to do
	if (bPlayback && pConfig->CfgRecPlayPrm.bReplayWarp && !force) return;
	const double EPS = 1e-6;
	if      (warp < MinWarpLimit) warp = MinWarpLimit;
	else if (warp > MaxWarpLimit) warp = MaxWarpLimit;
	if (fabs (warp-td.Warp()) > EPS) {
		td.SetWarp (warp, delay);
		if (td.WarpChanged()) ApplyWarpFactor();
		if (bRecord && pConfig->CfgRecPlayPrm.bRecordWarp) {
			char cbuf[256];
			if (delay) sprintf (cbuf, "%f %f", warp, delay);
			else       sprintf (cbuf, "%f", warp);
			FRecorder_SaveEvent ("TACC", cbuf);
		}
	}
}

//-----------------------------------------------------------------------------
// Name: IncWarpFactor()
// Desc: Increment time acceleration factor to next power of 10
//-----------------------------------------------------------------------------
void Orbiter::IncWarpFactor ()
{
	const double EPS = 1e-6;
	double logw = log10 (td.Warp());
	SetWarpFactor (pow (10.0, floor (logw+EPS)+1.0));
}

//-----------------------------------------------------------------------------
// Name: DecWarpFactor()
// Desc: Decrement time acceleration factor to next lower power of 10
//-----------------------------------------------------------------------------
void Orbiter::DecWarpFactor ()
{
	const double EPS = 1e-6;
	double logw = log10 (td.Warp());
	SetWarpFactor (pow (10.0, ceil (logw-EPS)-1.0));
}

//-----------------------------------------------------------------------------
// Name: ApplyWarpFactor()
// Desc: Broadcast new warp factor to components and modules
//-----------------------------------------------------------------------------
void Orbiter::ApplyWarpFactor ()
{
	double nwarp = td.Warp();
	for (int i = 0; i < nmodule; i++)
		module[i].module->clbkTimeAccChanged (nwarp, td.Warp());
	if (g_pane) g_pane->SetWarp (nwarp);
	bRealtime = (fabs (nwarp-1.0) < 1e-6);
}

//-----------------------------------------------------------------------------
// Name: SetFOV()
// Desc: Set field of view. Argument is FOV for vertical half-screen [rad]
//-----------------------------------------------------------------------------

void Orbiter::SetFOV (double fov, bool limit_range)
{
	if (g_camera->Aperture() == fov) return;

	fov = g_camera->SetAperture (fov, limit_range);
	if (g_pane) g_pane->SetFOV (fov);
	g_bForceUpdate = true;
}

//-----------------------------------------------------------------------------
// Name: IncFOV()
// Desc: Increase field of view. Argument is delta FOV for vertical half-screen [rad]
//-----------------------------------------------------------------------------

void Orbiter::IncFOV (double dfov)
{
	double fov = g_camera->IncrAperture (dfov);
	if (g_pane) g_pane->SetFOV (fov);
	g_bForceUpdate = true;
}

//-----------------------------------------------------------------------------
// Name: SaveScenario()
// Desc: save current status in-game
//-----------------------------------------------------------------------------
bool Orbiter::SaveScenario (const char *fname, const char *desc)
{
	pState->Update (desc);

	fs::path path = ScnPath (fname);
	fs::path dirname = path.parent_path();
	if(!fs::exists(dirname)) {
		fs::create_directory(dirname);
	}
	ofstream ofs (path);
	if (ofs) {
		// save scenario state
		pState->Write (ofs, pConfig->CfgDebugPrm.bSaveExitScreen ? "CurrentState_img" : "CurrentState");
		g_camera->Write (ofs);
		if (g_pane) g_pane->Write (ofs);
		g_psys->Write (ofs);

		// let plugins save their states to the scenario file
		for (int k = 0; k < nmodule; k++) {
			void (*opcSaveState)(FILEHANDLE) = (void(*)(FILEHANDLE))FindModuleProc (k, "opcSaveState");
			if (opcSaveState) {
				ofs << endl << "BEGIN_" << module[k].name << endl;
				opcSaveState ((FILEHANDLE)&ofs);
				ofs << "END" << endl;
			}
		}
		oapiAddNotification(OAPINOTIF_SUCCESS, "Scenario saved", fname);
		return true;

	} else {
		oapiAddNotification(OAPINOTIF_ERROR, "Failed to save scenario", fname);
		return false;
	}
}

//-----------------------------------------------------------------------------
// Name: Quicksave()
// Desc: save current status in-game
//-----------------------------------------------------------------------------
void Orbiter::Quicksave ()
{
	int i;
	char desc[256], fname[256];
	sprintf (desc, "Orbiter saved state at T = %0.0f", td.SimT0);
	for (i = strlen(ScenarioName)-1; i > 0; i--)
		if (ScenarioName[i-1] == '\\') break;
	sprintf (fname, "Quicksave/%s %04d", ScenarioName+i, ++g_qsaveid);
	SaveScenario (fname, desc);
}

//-----------------------------------------------------------------------------
// write a single frame to bmp file (or to clipboard, if fname==NULL)

void Orbiter::CaptureVideoFrame ()
{
	if (gclient) {
		if (video_skip_count == pConfig->CfgCapturePrm.SequenceSkip) {
			char fname[256];
			sprintf (fname, "%s/%04d", pConfig->CfgCapturePrm.SequenceDir, pConfig->CfgCapturePrm.SequenceStart++);
			oapi::ImageFileFormat fmt = (oapi::ImageFileFormat)pConfig->CfgCapturePrm.ImageFormat;
			float quality = (float)pConfig->CfgCapturePrm.ImageQuality/10.0f;
			gclient->clbkSaveSurfaceToImage (0, fname, fmt, quality);
			video_skip_count = 0;
		} else video_skip_count++;
	}
}

//-----------------------------------------------------------------------------
// Name: PlaybackSave()
// Desc: save the start scenario for a recorded simulation
//-----------------------------------------------------------------------------
void Orbiter::SavePlaybackScn (const char *fname)
{
	char desc[256], scn[256] = "Playback/";
	sprintf (desc, "Orbiter playback scenario at T = %0.0f", td.SimT0);
	strcat (scn, fname);
	SaveScenario (scn, desc);
}

const char *Orbiter::GetDefRecordName (void) const
{
	const char *playbackdir = pState->PlaybackDir();
	int i;
	for (i = strlen(playbackdir)-1; i > 0; i--)
		if (playbackdir[i-1] == '/') break;
	return playbackdir+i;
}

bool Orbiter::ToggleRecorder (bool force, bool append)
{
	if (bPlayback) return true; // don't allow recording during playback

	int i, n = g_psys->nVessel();
	const char *sname;
	char cbuf[256];
	bool bStartRecorder = !bRecord;
	if (bStartRecorder) {
		m_DlgRecorder->GetRecordName (cbuf, 256);
		sname = cbuf;
		if (!append && !FRecorder_PrepareDir (sname, force)) {
			bStartRecorder = false;
			return false;
		}
	} else sname = 0;
	FRecorder_Activate (bStartRecorder, sname, append);
	for (i = 0; i < n; i++)
		g_psys->GetVessel(i)->FRecorder_Activate (bStartRecorder, sname, append);
	if (bStartRecorder)
		SavePlaybackScn (sname);
	return true;
}

void Orbiter::EndPlayback ()
{
	for (int i = 0; i < g_psys->nVessel(); i++)
		g_psys->GetVessel(i)->FRecorder_EndPlayback ();
	FRecorder_ClosePlayback();
	if (snote_playback) snote_playback->ClearText();
	bPlayback = false;
	if (g_pane && g_pane->MIBar()) g_pane->MIBar()->SetPlayback(false);
}

oapi::ScreenAnnotation *Orbiter::CreateAnnotation (bool exclusive, double size, COLORREF col)
{
	if (!gclient) return NULL;
	oapi::ScreenAnnotation *sn = gclient->clbkCreateAnnotation();
	if (!sn) return NULL;
	
	sn->SetSize (size);
	VECTOR3 c = { (col      & 0xFF)/256.0,
		         ((col>>8 ) & 0xFF)/256.0,
				 ((col>>16) & 0xFF)/256.0};
	sn->SetColour (c);
	oapi::ScreenAnnotation **tmp = new oapi::ScreenAnnotation*[nsnote+1]; TRACENEW
	if (nsnote) {
		memcpy (tmp, snote, nsnote*sizeof(oapi::ScreenAnnotation*));
		delete []snote;
	}
	snote = tmp;
	snote[nsnote++] = sn;
	return sn;

	//int w = oclient->GetFramework()->GetRenderWidth();
	//int h = oclient->GetFramework()->GetRenderHeight();

	//ScreenNote *sn = new ScreenNote (this, w, h);
	//sn->SetSize (size);
	//sn->SetColour (col);

	//ScreenNote **tmp = new ScreenNote*[nsnote+1];
	//if (nsnote) {
	//	memcpy (tmp, snote, nsnote*sizeof(ScreenNote*));
	//	delete []snote;
	//}
	//snote = tmp;
	//snote[nsnote++] = sn;
	//return sn;
}

bool Orbiter::DeleteAnnotation (oapi::ScreenAnnotation *sn)
{
	int i, j, k;

	if (!gclient) return false;
	for (i = 0; i < nsnote; i++) {
		if (snote[i] == sn) {
			oapi::ScreenAnnotation **tmp = 0;
			if (nsnote > 1) {
				tmp = new oapi::ScreenAnnotation*[nsnote-1]; TRACENEW
				for (j = k = 0; j < nsnote; j++)
					if (j != i) tmp[k++] = snote[j];
				delete []snote;
			}
			snote = tmp;
			delete sn;
			nsnote--;
			return true;
		}
	}
	return false;
}

static char linebuf[2][70] = {"", ""};

void Orbiter::OutputLoadStatus (const char *msg, int line)
{
	printf("%s\n", msg);
	if (gclient) {
		strncpy (linebuf[line], msg, 64); linebuf[line][63] = '\0';
		gclient->clbkSplashLoadMsg (linebuf[line], line);
	}
}

void Orbiter::OutputLoadTick (int line, bool ok)
{
	if (gclient) {
		char cbuf[256];
		strcpy (cbuf, linebuf[line]);
		strcat (cbuf, ok ? " ok" : " xx");
		gclient->clbkSplashLoadMsg (cbuf, line);
	}
}

//-----------------------------------------------------------------------------
// Name: OpenTextureFile()
// Desc: Return file handle for texture file (0=error)
//       First searches in hightex dir, then in standard dir
//-----------------------------------------------------------------------------
FILE *Orbiter::OpenTextureFile (const char *name, const char *ext)
{
	FILE *ftex = 0;
	char *pch = HTexPath (name, ext); // first try high-resolution directory
	if (pch && (ftex = fopen (pch, "rb"))) {
		LOGOUT_FINE("Texture load: %s", pch);
		return ftex;
	}
	pch = TexPath (name, ext);        // try standard texture directory
	LOGOUT_FINE("Texture load: %s", pch);
	FILE *f = fopen (pch, "rb");
	if(!f) {
		fprintf(stderr, "Orbiter::OpenTextureFile Cannot find texture\n");
		exit(EXIT_FAILURE);
	}
	return f;
}

SURFHANDLE Orbiter::RegisterExhaustTexture (const char *name)
{
	if (gclient) {
		char path[256];
		strcpy (path, name);
		strcat (path, ".dds");
		return gclient->clbkLoadTexture (path, 0x8);
	} else {
        return NULL;
	}
}

//-----------------------------------------------------------------------------
// Load a mesh from file, and store it persistently in the mesh manager
//-----------------------------------------------------------------------------
const Mesh *Orbiter::LoadMeshGlobal (const char *fname)
{
	const Mesh *mesh =  meshmanager.LoadMesh (fname);
	if (gclient) gclient->clbkStoreMeshPersistent ((MESHHANDLE)mesh, fname);
	return mesh;
}

const Mesh *Orbiter::LoadMeshGlobal (const char *fname, LoadMeshClbkFunc fClbk)
{
	bool firstload;
	const Mesh *mesh =  meshmanager.LoadMesh (fname, &firstload);
	if (fClbk) fClbk ((MESHHANDLE)mesh, firstload);
	if (gclient) gclient->clbkStoreMeshPersistent ((MESHHANDLE)mesh, fname);
	return mesh;
}

//-----------------------------------------------------------------------------
// Name: Output2DData()
// Desc: Output HUD and other 2D information on top of the render window
//-----------------------------------------------------------------------------
void Orbiter::Output2DData ()
{
	if(bSession) {
		g_pane->Draw ();
		for (int i = 0; i < nsnote; i++)
			snote[i]->Render();
		if (snote_playback && pConfig->CfgRecPlayPrm.bShowNotes) snote_playback->Render();
	}
}

//-----------------------------------------------------------------------------
// Name: BeginTimeStep()
// Desc: Update timings for the current frame step
//-----------------------------------------------------------------------------
bool Orbiter::BeginTimeStep (bool running)
{
	// Check for a pause/resume request
	if (bRequestRunning != running) {
		running = bRunning = bRequestRunning;
		bool isPaused = !running;
		if (g_pane && g_pane->MIBar()) g_pane->MIBar()->SetPaused (isPaused);

		// broadcast pause state to plugins
		for (int k = 0; k < nmodule; k++)
			module[k].module->clbkPause (isPaused);
	}

	// Note that for times > 1e6 the simulation time is represented by
	// an offset and increment, to avoid floating point underflow roundoff
	// when adding the current time step
	double deltat;
	auto time_curr = std::chrono::steady_clock::now();

	if (launch_tick) {
		// control time interval in first few frames, when loading events occur
		// enforce interval 10ms for first 3 time steps
		deltat = 1e-2;
		time_prev = time_curr - std::chrono::microseconds(10);
		launch_tick--;
	} else {
		// standard time update
		std::chrono::duration<double> time_delta = time_curr - time_prev;
		// skip this step if the interval is smaller than the timer resolution
		deltat = time_delta.count();// *1000.0;//* 0.001;
	}
	if(deltat>0.1) deltat=0.1;

	time_prev = time_curr;
	td.BeginStep (deltat, running);

	if (!running) return true;
	if (td.WarpChanged()) ApplyWarpFactor();

	return true;
}

void Orbiter::EndTimeStep (bool running)
{
	if (running) {
		g_psys->FinaliseUpdate ();
		//ModulePostStep();
	}

	// Copy frame times from T1 to T0
	td.EndStep (running);

	// Update panels
	g_camera->Update ();                           // camera
	if (g_pane) g_pane->Update (td.SimT1, td.SysT1);

	// Update visual states
	if (gclient) gclient->clbkUpdate (bRunning);
	g_bForceUpdate = false;                        // clear flag

	// check for termination of demo mode
	if (SessionLimitReached()) {
		m_pGUIManager->CloseWindow();
	}
}

bool Orbiter::SessionLimitReached() const
{
	if (pConfig->CfgCmdlinePrm.FrameLimit && td.FrameCount() >= pConfig->CfgCmdlinePrm.FrameLimit)
		return true;
	if (pConfig->CfgCmdlinePrm.MaxSysTime && td.SysT0 >= pConfig->CfgCmdlinePrm.MaxSysTime)
		return true;
	if (pConfig->CfgCmdlinePrm.MaxSimTime && td.SimT0 >= pConfig->CfgCmdlinePrm.MaxSimTime)
		return true;
	if (pConfig->CfgDemoPrm.bDemo && td.SysT0 > pConfig->CfgDemoPrm.MaxDemoTime)
		return true;

	return false;
}

bool Orbiter::Timejump (double _mjd, int pmode)
{
	tjump.mode = pmode;
	tjump.dt = td.JumpTo (_mjd);
	g_psys->Timejump ();
	g_camera->Update ();
	if (g_pane) g_pane->Timejump ();

	for (int i = 0; i < nmodule; i++)
		module[i].module->clbkTimeJump (td.SimT0, tjump.dt, _mjd);

	return true;
}

void Orbiter::Suspend (void)
{
	time_suspend = std::chrono::steady_clock::now();
}

void Orbiter::Resume (void)
{
	auto dt = std::chrono::steady_clock::now() - time_suspend;
	time_prev += dt;
}

//-----------------------------------------------------------------------------
// Custom command registration
//-----------------------------------------------------------------------------

int Orbiter::RegisterCustomCmd (const char *label, const char *desc, CustomFunc func, void *context)
{
	int id;
	CUSTOMCMD *tmp = new CUSTOMCMD[ncustomcmd+1]; TRACENEW
	if (ncustomcmd) {
		memcpy (tmp, customcmd, ncustomcmd*sizeof(CUSTOMCMD));
		delete []customcmd;
	}
	customcmd = tmp;

	customcmd[ncustomcmd].label = strdup(label);
	customcmd[ncustomcmd].func = func;
	customcmd[ncustomcmd].context = context;
	customcmd[ncustomcmd].desc = desc;
	id = customcmd[ncustomcmd].id = g_customcmdid++;
	ncustomcmd++;
	return id;
}

bool Orbiter::UnregisterCustomCmd (int cmdId)
{
	int i;
	CUSTOMCMD *tmp = 0;

	for (i = 0; i < ncustomcmd; i++)
		if (customcmd[i].id == cmdId) break;
	if (i == ncustomcmd) return false;

	if (ncustomcmd > 1) {
		tmp = new CUSTOMCMD[ncustomcmd-1]; TRACENEW
		memcpy (tmp, customcmd, i*sizeof(CUSTOMCMD));
		memcpy (tmp+i, customcmd+i+1, (ncustomcmd-i-1)*sizeof(CUSTOMCMD));
	}
	delete []customcmd;
	customcmd = tmp;
	ncustomcmd--;
	return true;
}

//-----------------------------------------------------------------------------
// Name: ModulePreStep()
// Desc: call module pre-timestep callbacks
//-----------------------------------------------------------------------------
void Orbiter::ModulePreStep ()
{
	int i;
	for (i = 0; i < nmodule; i++)
		module[i].module->clbkPreStep (td.SimT0, td.SimDT, td.MJD0);
	for (i = 0; i < g_psys->nVessel(); i++)
		g_psys->GetVessel(i)->ModulePreStep (td.SimT0, td.SimDT, td.MJD0);
}

//-----------------------------------------------------------------------------
// Name: ModulePostStep()
// Desc: call module post-timestep callbacks
//-----------------------------------------------------------------------------
void Orbiter::ModulePostStep ()
{
	int i;
	for (i = 0; i < g_psys->nVessel(); i++)
		g_psys->GetVessel(i)->ModulePostStep (td.SimT1, td.SimDT, td.MJD1);
	for (i = 0; i < nmodule; i++) {
		module[i].module->clbkPostStep (td.SimT1, td.SimDT, td.MJD1);
	}
}

//-----------------------------------------------------------------------------
// Name: UpdateWorld()
// Desc: Update world to current time
//-----------------------------------------------------------------------------
void Orbiter::UpdateWorld ()
{
	// module pre-timestep callbacks
	if (bRunning) ModulePreStep ();

	// update world
	g_bStateUpdate = true;
	if (bRunning && td.SimDT) {
		if (bPlayback) FRecorder_Play();
		g_psys->Update (g_bForceUpdate);           // logical objects
	}

	// module post-timestep callbacks
	if (bRunning) ModulePostStep ();

	g_bStateUpdate = false;

	if (!KillVessels())  { // kill any vessels marked for deletion
		printf("Orbiter::UpdateWorld !KillVessels()\n");
		m_pGUIManager->CloseWindow();
	}
}

const char *Orbiter::KeyState() const
{
	return simkstate;
}

//-----------------------------------------------------------------------------
// Name: UserInput()
// Desc: Process user input via DirectInput keyboard and joystick (but not
//       keyboard messages sent via window message queue)
//-----------------------------------------------------------------------------

void Orbiter::UserInput ()
{
	//static char buffer[256];
	
	//DIDEVICEOBJECTDATA dod[10];
	//LPDIRECTINPUTDEVICE8 didev;
	//int i, dwItems = 10;
	//HRESULT hr;
	bool skipkbd = false;

	//memset(simkstate, 0, 256);
	for (int i = 0; i < 15; i++) ctrlKeyboard[i] = ctrlJoystick[i] = 0; // reset keyboard and joystick attitude requests

	//if (didev = GetDInput()->GetKbdDevice()) {
		// keyboard input: immediate key interpretation
		//hr = didev->GetDeviceState (sizeof(buffer), &buffer);
		//if ((hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST) && SUCCEEDED (didev->Acquire()))
		//	hr = didev->GetDeviceState (sizeof(buffer), &buffer);
		//if (SUCCEEDED (hr))
		//	for (i = 0; i < 256; i++)
		//		simkstate[i] |= buffer[i];
		bool consume = BroadcastImmediateKeyboardEvent (simkstate);
		if (!skipkbd && !consume) {
			KbdInputImmediate_System (simkstate);
			if (bRunning) KbdInputImmediate_OnRunning (simkstate);
		}


		// keyboard input: buffered key events
		//hr = didev->GetDeviceData (sizeof(DIDEVICEOBJECTDATA), dod, &dwItems, 0);
		//if ((hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST) && SUCCEEDED (didev->Acquire()))
		//	hr = didev->GetDeviceData (sizeof(DIDEVICEOBJECTDATA), dod, &dwItems, 0);
		//if (SUCCEEDED (hr)) {
			/*
			BroadcastBufferedKeyboardEvent (simkstate, dod, dwItems);
			if (!skipkbd) {
				KbdInputBuffered_System (simkstate, dod, dwItems);
				if (bRunning) KbdInputBuffered_OnRunning (simkstate, dod, dwItems);
			}
			*/
		//}
		//if (hr == DI_BUFFEROVERFLOW) MessageBeep (-1);
	//}

	for (int i = 0; i < 15; i++) ctrlTotal[i] = ctrlKeyboard[i]; // update attitude requests

	int airfoils[6];
	InputController::ProcessInput(ctrlJoystick, airfoils);
	
	for (int i = 0; i < 15; i++) ctrlTotal[i] += ctrlJoystick[i]; // update thrust requests

	if(bSession) {
		g_camera->UpdateMouse();

		// apply manual attitude control
		g_focusobj->ApplyUserAttitudeControls (ctrlKeyboard, ctrlJoystick, airfoils);
	}
}

//-----------------------------------------------------------------------------
// Name: SendKbdBuffered()
// Desc: Simulate a buffered keyboard event
//-----------------------------------------------------------------------------

bool Orbiter::SendKbdBuffered(int key, int *mod, int nmod, bool onRunningOnly)
{
	if (onRunningOnly && !bRunning) return false;

	char buffer[256];
	memset (buffer, 0, 256);
	for (int i = 0; i < nmod; i++)
		buffer[mod[i]] = 0x80;
	BroadcastBufferedKeyboardEvent (buffer, key);
	KbdInputBuffered_System (buffer, key);
	KbdInputBuffered_OnRunning (buffer, key);
	return true;
}

//-----------------------------------------------------------------------------
// Name: SendKbdImmediate()
// Desc: Simulate an immediate key state
//-----------------------------------------------------------------------------

bool Orbiter::SendKbdImmediate(char kstate[256], bool onRunningOnly)
{
	if (onRunningOnly && !bRunning) return false;
	for (int i = 0; i < 256; i++)
		simkstate[i] |= kstate[i];
	bAllowInput = true; // make sure the render window processes inputs
	return true;
}

//-----------------------------------------------------------------------------
// Name: KbdInputImmediate_System ()
// Desc: General user keyboard immediate key interpretation. Processes keys
//       which are also interpreted when simulation is paused (movably)
//-----------------------------------------------------------------------------
void Orbiter::KbdInputImmediate_System (char *kstate)
{
	bool smooth_cam = true; // make user-selectable

	const double cam_acc = 0.02;
	double cam_vmax = td.SysDT * 1.0;
	double max_dv = cam_vmax*cam_acc;

	static double dphi = 0.0, dtht = 0.0;
	static double dphi_gm = 0.0, dtht_gm = 0.0;
	if (g_camera->IsExternal()) { // external camera view
		// rotate external camera horizontally (track mode)
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateLeft))  dphi = (smooth_cam ? max (-cam_vmax, dphi-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateRight)) dphi = (smooth_cam ? min ( cam_vmax, dphi+max_dv) :  cam_vmax);
		else if (dphi) {
			if (smooth_cam) {
				if (dphi < 0.0) dphi = min (0.0, dphi+max_dv);
				else            dphi = max (0.0, dphi-max_dv);
			} else dphi = 0.0;
		}
		if (dphi) g_camera->ShiftPhi (dphi);

		// rotate external camera vertically (track mode)
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateUp))   dtht = (smooth_cam ? max (-cam_vmax, dtht-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateDown)) dtht = (smooth_cam ? min ( cam_vmax, dtht+max_dv) :  cam_vmax);
		else if (dtht) {
			if (smooth_cam) {
				if (dtht < 0.0) dtht = min (0.0, dtht+max_dv);
				else            dtht = max (0.0, dtht-max_dv);
			} else dtht = 0.0;
		}
		if (dtht) g_camera->ShiftTheta (dtht);

		// rotate external camera horizontally (ground mode)
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltLeft))  dphi_gm = (smooth_cam ? max (-cam_vmax, dphi_gm-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltRight)) dphi_gm = (smooth_cam ? min ( cam_vmax, dphi_gm+max_dv) :  cam_vmax);
		else if (dphi_gm) {
			if (smooth_cam) {
				if (dphi_gm < 0.0) dphi_gm = min (0.0, dphi_gm+max_dv);
				else               dphi_gm = max (0.0, dphi_gm-max_dv);
			} else dphi_gm = 0.0;
		}

		// rotate external camera vertically (ground mode)
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltUp))   dtht_gm = (smooth_cam ? max (-cam_vmax, dtht_gm-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltDown)) dtht_gm = (smooth_cam ? min ( cam_vmax, dtht_gm+max_dv) :  cam_vmax);
		else if (dtht_gm) {
			if (smooth_cam) {
				if (dtht_gm < 0.0) dtht_gm = min (0.0, dtht_gm+max_dv);
				else               dtht_gm = max (0.0, dtht_gm-max_dv);
			} else dtht_gm = 0.0;
		}
		if (dphi_gm || dtht_gm) g_camera->Rotate (0-dphi_gm, -dtht_gm);

	} else {                        // internal camera view
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateLeft))  dphi = (smooth_cam ? max (-cam_vmax, dphi-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateRight)) dphi = (smooth_cam ? min ( cam_vmax, dphi+max_dv) :  cam_vmax);
		else if (dphi) {
			if (smooth_cam) {
				if (dphi < 0.0) dphi = min (0.0, dphi+max_dv);
				else            dphi = max (0.0, dphi-max_dv);
			} else dphi = 0.0;
		}

		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateUp))   dtht = (smooth_cam ? max (-cam_vmax, dtht-max_dv) : -cam_vmax);
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateDown)) dtht = (smooth_cam ? min ( cam_vmax, dtht+max_dv) :  cam_vmax);
		else if (dtht) {
			if (smooth_cam) {
				if (dtht < 0.0) dtht = min (0.0, dtht+max_dv);
				else            dtht = max (0.0, dtht-max_dv);
			} else dtht = 0.0;
		}
		if (dphi || dtht) g_camera->Rotate (-dphi, -dtht, true);
	}


	if (g_camera->IsExternal()) {   // external camera view
		// rotate external camera (track mode)
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateLeft))    g_camera->ShiftPhi   (-td.SysDT);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateRight))   g_camera->ShiftPhi   ( td.SysDT);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateUp))      g_camera->ShiftTheta (-td.SysDT);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRotateDown))    g_camera->ShiftTheta ( td.SysDT);
		// move external camera in/out
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackAdvance))       g_camera->ShiftDist (-td.SysDT);
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_TrackRetreat))       g_camera->ShiftDist ( td.SysDT);
		// tilt ground observer camera
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltLeft))     g_camera->Rotate ( td.SysDT,  0);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltRight))    g_camera->Rotate (-td.SysDT,  0);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltUp))       g_camera->Rotate ( 0,  td.SysDT);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_GroundTiltDown))     g_camera->Rotate ( 0, -td.SysDT);
	} else {                        // internal camera view
		// rotate cockpit camera
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateLeft))  g_camera->Rotate ( td.SysDT,  0, true);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateRight)) g_camera->Rotate (-td.SysDT,  0, true);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateUp))    g_camera->Rotate ( 0,  td.SysDT, true);
		//if (keymap.IsLogicalKey (kstate, OAPI_LKEY_CockpitRotateDown))  g_camera->Rotate ( 0, -td.SysDT, true);
		// shift 2-D panels
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_PanelShiftLeft))     g_pane->ShiftPanel ( td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed, 0.0);
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_PanelShiftRight))    g_pane->ShiftPanel (-td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed, 0.0);
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_PanelShiftUp))       g_pane->ShiftPanel (0.0,  td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed);
		if (keymap.IsLogicalKey (kstate, OAPI_LKEY_PanelShiftDown))     g_pane->ShiftPanel (0.0, -td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed);
	}
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_IncFOV)) IncFOV ( 0.4*g_camera->Aperture()*td.SysDT);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_DecFOV)) IncFOV (-0.4*g_camera->Aperture()*td.SysDT);
}

//-----------------------------------------------------------------------------
// Name: KbdInputImmediate_OnRunning ()
// Desc: User keyboard input query for running simulation (ship controls etc.)
//-----------------------------------------------------------------------------
void Orbiter::KbdInputImmediate_OnRunning (char *kstate)
{
	if (g_focusobj->ConsumeDirectKey (kstate)) return;  // key is consumed by focus vessel

	// main/retro/hover thruster settings
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_IncMainThrust))   g_focusobj->IncMainRetroLevel ( 0.2*td.SimDT);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_DecMainThrust))   g_focusobj->IncMainRetroLevel (-0.2*td.SimDT);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_KillMainRetro)) { g_focusobj->SetThrusterGroupLevel (THGROUP_MAIN, 0.0);
		                                                         g_focusobj->SetThrusterGroupLevel (THGROUP_RETRO, 0.0); }
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_FullMainThrust))  g_focusobj->OverrideMainLevel ( 1.0);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_FullRetroThrust)) g_focusobj->OverrideMainLevel (-1.0);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_IncHoverThrust))  g_focusobj->IncThrusterGroupLevel (THGROUP_HOVER,  0.2*td.SimDT);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_DecHoverThrust))  g_focusobj->IncThrusterGroupLevel (THGROUP_HOVER, -0.2*td.SimDT);

	// Reaction control system
	if (bEnableAtt) {
		// rotational mode
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSPitchUp))     ctrlKeyboard[THGROUP_ATT_PITCHUP]   = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSPitchUp))   ctrlKeyboard[THGROUP_ATT_PITCHUP]   =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSPitchDown))   ctrlKeyboard[THGROUP_ATT_PITCHDOWN] = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSPitchDown)) ctrlKeyboard[THGROUP_ATT_PITCHDOWN] =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSYawLeft))     ctrlKeyboard[THGROUP_ATT_YAWLEFT]   = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSYawLeft))   ctrlKeyboard[THGROUP_ATT_YAWLEFT]   =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSYawRight))    ctrlKeyboard[THGROUP_ATT_YAWRIGHT]  = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSYawRight))  ctrlKeyboard[THGROUP_ATT_YAWRIGHT]  =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSBankLeft))    ctrlKeyboard[THGROUP_ATT_BANKLEFT]  = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSBankLeft))  ctrlKeyboard[THGROUP_ATT_BANKLEFT]  =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSBankRight))   ctrlKeyboard[THGROUP_ATT_BANKRIGHT] = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSBankRight)) ctrlKeyboard[THGROUP_ATT_BANKRIGHT] =  100;
		// linear mode
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSUp))          ctrlKeyboard[THGROUP_ATT_UP]        = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSUp))        ctrlKeyboard[THGROUP_ATT_UP]        =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSDown))        ctrlKeyboard[THGROUP_ATT_DOWN]      = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSDown))      ctrlKeyboard[THGROUP_ATT_DOWN]      =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSLeft))        ctrlKeyboard[THGROUP_ATT_LEFT]      = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSLeft))      ctrlKeyboard[THGROUP_ATT_LEFT]      =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSRight))       ctrlKeyboard[THGROUP_ATT_RIGHT]     = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSRight))     ctrlKeyboard[THGROUP_ATT_RIGHT]     =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSForward))     ctrlKeyboard[THGROUP_ATT_FORWARD]   = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSForward))   ctrlKeyboard[THGROUP_ATT_FORWARD]   =  100;
		if      (keymap.IsLogicalKey (kstate, OAPI_LKEY_RCSBack))        ctrlKeyboard[THGROUP_ATT_BACK]      = 1000;
		else if (keymap.IsLogicalKey (kstate, OAPI_LKEY_LPRCSBack))      ctrlKeyboard[THGROUP_ATT_BACK]      =  100;
	}

	// Elevator trim control
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_IncElevatorTrim)) g_focusobj->IncTrim (AIRCTRL_ELEVATORTRIM);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_DecElevatorTrim)) g_focusobj->DecTrim (AIRCTRL_ELEVATORTRIM);

	// Wheel brake control
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_WheelbrakeLeft))  g_focusobj->SetWBrakeLevel (1.0, 1, false);
	if (keymap.IsLogicalKey (kstate, OAPI_LKEY_WheelbrakeRight)) g_focusobj->SetWBrakeLevel (1.0, 2, false);

	// left/right MFD control
	if (KEYMOD_SHIFT (kstate)) {
		if (KEYMOD_LSHIFT (kstate) && g_pane->MFD(MFD_LEFT)) g_pane->MFD(MFD_LEFT)->ConsumeKeyImmediate (kstate);
		if (KEYMOD_RSHIFT (kstate) && g_pane->MFD(MFD_RIGHT)) g_pane->MFD(MFD_RIGHT)->ConsumeKeyImmediate (kstate);
	}
}

//-----------------------------------------------------------------------------
// Name: KbdInputBuffered_System ()
// Desc: General user keyboard buffered key interpretation. Processes keys
//       which are also interpreted when simulation is paused
//-----------------------------------------------------------------------------
void Orbiter::KbdInputBuffered_System (char *kstate, int key)
{
		if      (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_Pause))                TogglePause();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_Quicksave))            Quicksave();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_StepIncFOV))           SetFOV (ceil((g_camera->Aperture()*DEG+1e-6)/5.0)*5.0*RAD);
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_StepDecFOV))           SetFOV (floor((g_camera->Aperture()*DEG-1e-6)/5.0)*5.0*RAD);
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_MainMenu))             { if (g_pane->MIBar()) g_pane->MIBar()->ToggleAutohide(); }
//		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgHelp))              pDlgMgr->EnsureEntry<DlgHelp> ();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgCamera))            m_pGUIManager->ShowCtrl<DlgCamera>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgSimspeed))          m_pGUIManager->ShowCtrl<DlgTacc>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgCustomCmd))         m_pGUIManager->ShowCtrl<DlgFunction>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgInfo))              m_pGUIManager->ShowCtrl<DlgInfo>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgMap))               m_pGUIManager->ShowCtrl<DlgMap>();
		//else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgNavaid))            OpenDialogEx (IDD_NAVAID, (DLGPROC)Navaid_DlgProc, DLG_CAPTIONCLOSE);
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgRecorder))          m_pGUIManager->ShowCtrl<DlgRecorder>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_ToggleCamInternal))    SetView (g_focusobj, !g_camera->IsExternal());
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgVisHelper))         m_pGUIManager->ShowCtrl<DlgVishelper>();
//		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgCapture))           pDlgMgr->EnsureEntry<DlgCapture> ();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DlgSelectVessel))      m_pGUIManager->ShowCtrl<DlgFocus>();
		else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_TogglePlanetarium)) {
			pConfig->TogglePlanetarium();
			g_psys->ActivatePlanetLabels(Cfg()->PlanetariumItem(PLN_ENABLE));
		} else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_ToggleRecPlay)) {
			if (bPlayback) EndPlayback();
			else ToggleRecorder ();
		} else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_Quit)) {
			m_pGUIManager->CloseWindow();
		} else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_SelectPrevVessel)) {
			if (g_pfocusobj) SetFocusObject (g_pfocusobj);
		}

		if (g_camera->IsInternal()) {
			if      (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_CockpitResetCam))  g_camera->ResetCockpitDir();
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_TogglePanelMode))  g_pane->TogglePanelMode();
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_PanelSwitchLeft))  g_pane->SwitchPanel (0);
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_PanelSwitchRight)) g_pane->SwitchPanel (1);
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_PanelSwitchUp))    g_pane->SwitchPanel (2);
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_PanelSwitchDown))  g_pane->SwitchPanel (3);

			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_CockpitDontLean))    g_focusobj->LeanCamera (0); // g_camera->MoveTo (Vector(0,0,0)), g_camera->ResetCockpitDir();
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_CockpitLeanForward)) g_focusobj->LeanCamera (1); // g_camera->MoveTo (g_focusobj->camdr_fwd), g_camera->ResetCockpitDir();
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_CockpitLeanLeft))    g_focusobj->LeanCamera (2); // g_camera->MoveTo (g_focusobj->camdr_left), g_camera->ResetCockpitDir(60*RAD, g_camera->ctheta0);
			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_CockpitLeanRight))   g_focusobj->LeanCamera (3); // g_camera->MoveTo (g_focusobj->camdr_right), g_camera->ResetCockpitDir(-60*RAD, g_camera->ctheta0);

			else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_HUDColour))        g_pane->ToggleHUDColour();
		} else {
			if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_ToggleTrackMode))  g_camera->SetTrackMode ((ExtCamMode)(((int)g_camera->GetExtMode()+1)%3));
		}
}

//-----------------------------------------------------------------------------
// Name: KbdInputBuffered_OnRunning ()
// Desc: User keyboard buffered key interpretation in running simulation
//-----------------------------------------------------------------------------
void Orbiter::KbdInputBuffered_OnRunning (char *kstate, int key)
{
	bool bdown = kstate[key&0xff] != 0;

	if (g_focusobj->ConsumeBufferedKey (key, bdown, kstate)) // offer key to vessel for processing
		return;
	if (!bdown) // only process key down events
		return;
	if (key == OAPI_KEY_LSHIFT || key == OAPI_KEY_RSHIFT) return;    // we don't process modifier keys

	// simulation speed control
	if      (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_IncSimSpeed)) IncWarpFactor ();
	else if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_DecSimSpeed)) DecWarpFactor ();

	if (KEYMOD_CONTROL (kstate)) {    // CTRL-Key combinations

		//switch (key) {
		//case DIK_F3:    // switch focus to previous vessel
		//	if (g_pfocusobj) SetFocusObject (g_pfocusobj);
		//	break;
		//}

	} else if (KEYMOD_SHIFT (kstate)) {  // Shift-key combinations (reserved for MFD control)

		int id = (KEYDOWN (kstate, OAPI_KEY_LSHIFT) ? MFD_LEFT : MFD_RIGHT);
		g_pane->MFDConsumeKeyBuffered (id, key);

	} else if (KEYMOD_ALT (kstate)) {    // ALT-Key combinations

	} else { // unmodified keys

		//switch (key) {
		//case DIK_F3:       // switch vessel
		//	OpenDialogEx (IDD_JUMPVESSEL, (DLGPROC)SelVessel_DlgProc, DLG_CAPTIONCLOSE | DLG_CAPTIONHELP);
		//	break;
		//}
	}
}

//-----------------------------------------------------------------------------
// Name: UserJoyInput_System ()
// Desc: General user joystick input (also functional when paused)
//-----------------------------------------------------------------------------
#if 0
void Orbiter::UserJoyInput_System (DIJOYSTATE2 *js)
{
	if (LOWORD (js->rgdwPOV[0]) != 0xFFFF) {
		int dir = js->rgdwPOV[0];
		if (g_camera->IsExternal()) {  // use the joystick's coolie hat to rotate external camera
			if (js->rgbButtons[2]) { // shift instrument panel
				if      (dir <  5000 || dir > 31000) g_camera->Rotate (0,  td.SysDT);
				else if (dir > 13000 && dir < 23000) g_camera->Rotate (0, -td.SysDT);
				if      (dir >  4000 && dir < 14000) g_camera->Rotate (-td.SysDT, 0);
				else if (dir > 22000 && dir < 32000) g_camera->Rotate ( td.SysDT, 0);
			} else {
				if      (dir <  5000 || dir > 31000) g_camera->AddTheta (-td.SysDT);
				else if (dir > 13000 && dir < 23000) g_camera->AddTheta ( td.SysDT);
				if      (dir >  4000 && dir < 14000) g_camera->AddPhi   ( td.SysDT);
				else if (dir > 22000 && dir < 32000) g_camera->AddPhi   (-td.SysDT);
			}
		} else { // internal view
			if (js->rgbButtons[2]) { // shift instrument panel
				if      (dir <  5000 || dir > 31000) g_pane->ShiftPanel (0.0,  td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed);
				else if (dir > 13000 && dir < 23000) g_pane->ShiftPanel (0.0, -td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed);
				if      (dir >  4000 && dir < 14000) g_pane->ShiftPanel (-td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed, 0.0);
				else if (dir > 22000 && dir < 32000) g_pane->ShiftPanel ( td.SysDT*pConfig->CfgLogicPrm.PanelScrollSpeed, 0.0);
			} else {                 // rotate camera
				if      (dir <  5000 || dir > 31000) g_camera->Rotate (0,  td.SysDT, true);
				else if (dir > 13000 && dir < 23000) g_camera->Rotate (0, -td.SysDT, true);
				if      (dir >  4000 && dir < 14000) g_camera->Rotate (-td.SysDT, 0, true);
				else if (dir > 22000 && dir < 32000) g_camera->Rotate ( td.SysDT, 0, true);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: UserJoyInput_OnRunning ()
// Desc: User joystick input query for running simulation (ship controls etc.)
//-----------------------------------------------------------------------------
void Orbiter::UserJoyInput_OnRunning (DIJOYSTATE2 *js)
{
	if (bEnableAtt) {
		if (js->lX) {
			if (js->rgbButtons[2]) { // emulate rudder control
				if (js->lX > 0) ctrlJoystick[THGROUP_ATT_YAWRIGHT] =   js->lX;
				else            ctrlJoystick[THGROUP_ATT_YAWLEFT]  =  -js->lX;
			} else {                 // rotation (bank)
				if (js->lX > 0) ctrlJoystick[THGROUP_ATT_BANKRIGHT] =  js->lX;
				else            ctrlJoystick[THGROUP_ATT_BANKLEFT]  = -js->lX;
			}
		}
		if (js->lY) {                // rotation (pitch) or translation (vertical)
			if (js->lY > 0) ctrlJoystick[THGROUP_ATT_PITCHUP]   = ctrlJoystick[THGROUP_ATT_UP]    =  js->lY;
			else            ctrlJoystick[THGROUP_ATT_PITCHDOWN] = ctrlJoystick[THGROUP_ATT_DOWN]  = -js->lY;
		}
		if (js->lRz) {               // rotation (yaw) or translation (transversal)
			if (js->lRz > 0) ctrlJoystick[THGROUP_ATT_YAWRIGHT] = ctrlJoystick[THGROUP_ATT_RIGHT] =  js->lRz;
			else             ctrlJoystick[THGROUP_ATT_YAWLEFT]  = ctrlJoystick[THGROUP_ATT_LEFT]  = -js->lRz;
		}
	}
/*
	if (pDI->joyprop.bThrottle) { // main thrusters via throttle control
		long lZ4 = *(long*)(((uint8_t*)js)+pDI->joyprop.ThrottleOfs) >> 3;
		if (lZ4 != plZ4) {
			if (ignorefirst) {
				if (abs(lZ4-plZ4) > 10) ignorefirst = false;
				else return;
			}
			double th = -0.008 * (plZ4 = lZ4);
			if (th > 1.0) th = 1.0;
			g_focusobj->SetThrusterGroupLevel (THGROUP_MAIN, th);
			g_focusobj->SetThrusterGroupLevel (THGROUP_RETRO, 0.0);
		}
	}
	*/
}
#endif
bool Orbiter::MouseEvent (oapi::MouseEvent event, int state, int x, int y)
{
	if (g_pane && g_pane->MIBar() && g_pane->MIBar()->ProcessMouse (event, state, x, y)) return true;
	if (BroadcastMouseEvent (event, state, x, y)) return true;
	if (event == oapi::MOUSE_MOVE) return false; // may be lifted later

	if (bRunning) {
		if (g_pane && g_pane->ProcessMouse_OnRunning (event, state, x, y, simkstate)) return true;
	}
	if (g_pane && g_pane->ProcessMouse_System(event, state, x, y, simkstate)) return true;
	if (g_camera && g_camera->ProcessMouse (event, state, x, y, simkstate)) return true;
	return false;
}

bool Orbiter::BroadcastMouseEvent (oapi::MouseEvent event, int state, int x, int y)
{
	bool consume = false;
	for (int k = 0; k < nmodule; k++) {
		if (module[k].module->clbkProcessMouse (event, state, x, y))
			consume = true;
	}
	return consume;
}

bool Orbiter::BroadcastImmediateKeyboardEvent (char *kstate)
{
	bool consume = false;
	for (int k = 0; k < nmodule; k++)
		if (module[k].module->clbkProcessKeyboardImmediate (kstate, bRunning))
			consume = true;
	return consume;
}

void Orbiter::BroadcastBufferedKeyboardEvent (char *kstate, int key)
{
	//bool consume = false;
	for (int k = 0; k < nmodule; k++) {
		if (module[k].module->clbkProcessKeyboardBuffered (key, kstate, bRunning)) {
//			consume = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: AttachGraphicsClient()
// Desc: Link an external graphics render interface
//-----------------------------------------------------------------------------
bool Orbiter::AttachGraphicsClient (oapi::GraphicsClient *gc)
{
	if (gclient) return false; // another client is already attached
	register_module = gc;
	gclient = gc;
	gclient->clbkInitialise();
	return true;
}

//-----------------------------------------------------------------------------
// Name: RemoveGraphicsClient()
// Desc: Unlink an external graphics render interface
//-----------------------------------------------------------------------------
bool Orbiter::RemoveGraphicsClient (oapi::GraphicsClient *gc)
{
	if (!gclient || gclient != gc) return false; // no client attached
	gclient = NULL;
	return true;
}

void Orbiter::MakeContextCurrent(bool b)
{
	if(gclient) gclient->clbkMakeContextCurrent(b);
}

void Orbiter::RegisterGUIElement(GUIElement *e)
{
	m_pGUIManager->RegisterCtrl(e);
}

void Orbiter::UnregisterGUIElement(GUIElement *e)
{
	m_pGUIManager->UnregisterCtrl(e);
}

void Orbiter::UpdateDeallocationProgress()
{
}

void Orbiter::CloseDialog (GUIElement *e)
{
	e->show = false;
}

//=============================================================================
// Implementation of class TimeData
//=============================================================================

TimeData::TimeData ()
{
	Reset();
}

void TimeData::Reset (double mjd_ref)
{
	TWarp = TWarpTarget = 1.0;
	TWarpDelay = 0.0;
	SysT0 = SysT1 = SysDT = 0.0;
	SimT0 = SimT1 = SimDT = SimDT0 = 0.0;
	SimT1_ofs = SimT1_inc = 0.0;
	MJD_ref = MJD0 = MJD1 = mjd_ref;
	fps = syst_acc = 0.0;
	framecount = frame_tick = sys_tick = 0;
	bWarpChanged = false;

	fixed_step = 0.0;
	bFixedStep = false;
}

void TimeData::SetFixedStep(double step)
{
	fixed_step = step;
	bFixedStep = (fixed_step > 0.0);
}

void TimeData::BeginStep (double deltat, bool running)
{
	bWarpChanged = false;
	SysT1 = SysT0 + (SysDT = deltat);
	iSysDT = 1.0/SysDT; // note that delta_ms==0 is trapped earlier

	framecount++;
	frame_tick++;
	syst_acc += SysDT;
	if ((size_t)SysT1 != sys_tick) {
		fps = frame_tick/syst_acc;
		frame_tick = 0;
		syst_acc = 0.0;
		sys_tick = (size_t)SysT1;
	}

	if (running) { // only advance simulation time if simulation is not paused

		if (TWarp != TWarpTarget) {
			if (TWarpDelay == 0.0)
				TWarp = TWarpTarget;
			else if (TWarpTarget > TWarp)
				TWarp = min (TWarpTarget, TWarp * pow (10, SysDT/TWarpDelay));
			else
				TWarp = max (TWarpTarget, TWarp * pow (10, -SysDT/TWarpDelay));
			bWarpChanged = true;
		}

		SimDT = (bFixedStep ? fixed_step : SysDT) * TWarp;
		iSimDT = 1.0/SimDT;
		if ((SimT1_inc += SimDT) > 1e6) {
			SimT1_ofs += 1e6;
			SimT1_inc -= 1e6;
		}
		SimT1 = SimT1_ofs + SimT1_inc;
		MJD1 = MJD_ref + Day (SimT1);
	}
}

void TimeData::EndStep (bool running)
{
	SysT0 = SysT1;

	if (running) {
		SimT0 = SimT1;
		SimDT0 = SimDT;
		iSimDT0 = iSimDT;
		MJD0 = MJD1;
	}
}

double TimeData::JumpTo (double mjd)
{
	double dt = (mjd-MJD0)*86400.0;
	MJD0 = MJD1 = mjd;
	SimT0 = SimT1 = SimT1_ofs = (mjd-MJD_ref)*86400.0;
	SimT1_inc = 0.0;
	return dt;
}

void TimeData::SetWarp (double warp, double delay) {
	TWarpTarget = warp;
	TWarpDelay  = delay;
	if (delay == 0.0) {
		TWarp = warp;
		bWarpChanged = true;
	}
}
