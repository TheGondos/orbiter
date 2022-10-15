// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef ORBITER_H
#define ORBITER_H

#include "Config.h"
#include "Select.h"
#include "Keymap.h"
#include <stdio.h>
#include "Mesh.h"
#include "Astro.h"
#include "GUIManager.h"
#include "Controller.h"
#include <chrono>


class DInput;
class Config;
class State;
class Body;
class Vessel;
class ScreenNote;
class DialogManager;
class OrbiterGraphics;
class OrbiterConnect;
class OrbiterServer;
class OrbiterClient;
//class PlaybackEditor;
class MemStat;
class DDEServer;
class ImageIO;

class DlgCamera;
class DlgInfo;
class DlgTacc;
class DlgMap;
class DlgFocus;
class DlgFunction;
class DlgVishelper;
class DlgLaunchpad;
class DlgRecorder;
class Select;
class InputBox;
class DlgPlaybackEditor;

//-----------------------------------------------------------------------------
// Structure for module callback functions
//-----------------------------------------------------------------------------
typedef void (*OPC_Proc)(void);

//-----------------------------------------------------------------------------
// Name: class TimeData
// Desc: stores timing information for current time step
//-----------------------------------------------------------------------------
class TimeData {
public:
	TimeData ();

	void Reset (double mjd_ref = 0.0);
	// Reset all sim and sys times to 0. Set time warp to 1
	// Disable fixed step mode

	void SetFixedStep(double step);
	// set a fixed time interval for each time step [s]
	// step=0 disables the fixed step modus

	double FixedStep() const { return (bFixedStep ? fixed_step : 0.0); }

	void BeginStep (double deltat, bool running);
	// advance time by deltat (seconds)

	void EndStep (bool running);
	// copy time data from next step to current step

	double JumpTo (double mjd);
	// jump to a new simulation date. Returns jump distance from current time [s]

	void SetWarp (double warp, double delay = 0.0);
	inline double Warp () const { return TWarp; }
	inline bool WarpChanged () const { return bWarpChanged; }
	inline size_t FrameCount() const { return framecount; }

	double MJD (double simt) const { return MJD_ref + Day(simt); }
	// Convert simulation time to MJD

	inline double FPS() const { return fps; }

	double  SysT0;        // current system time since simulation start [s]
	double  SysT1;        // next frame system time (=SysT0+SysDT)
	double  SysDT;        // current system step interval [s]
	double iSysDT;        // 1/SysDT
	double  SimT0;        // current simulation time since simulation start [s]
	double  SimT1;        // next frame simulation time (=SimT0+SimDT)
	double  SimDT;        // current simulation step interval [s] (updated at the beginning of the update phase)
	double  SimDT0;       // time step between currently published state (s0) and the previous state (updated at the end of the update phase)
	double iSimDT;        // 1/SimDT
	double iSimDT0;       // 1/SimDT0
	double  MJD0;         // Modified Julian date at current frame [days]
	double  MJD1;         // Modified Julian date at next frame [days]
	double  MJD_ref;      // Modified Julian date at simulation start [days]

private:
	double  SimT1_ofs, SimT1_inc;  // offset and increment parts of SimT1
	double  fixed_step;   // fixed base time step length (0=variable)
	double  TWarp;        // time acceleration factor
	double  TWarpTarget;  // target acceleration factor
	double  TWarpDelay;   // warp acceleration delay
	bool    bWarpChanged; // time acceleration changed in last step?
	bool    bFixedStep;   // use fixed time steps?
	size_t  framecount;   // number of frames since simulation start (including pause)
	size_t  frame_tick;   // number of frames since last fps calculation
	size_t  sys_tick;     // flush index for fps calculation
	double  syst_acc;     // accumulated system time for fps calculation
	double  fps;          // current frame rate [Hz]
};

//-----------------------------------------------------------------------------
// Name: class Orbiter
// Desc: Main application class
//-----------------------------------------------------------------------------
class ScriptInterface;
//class oapi::GraphicsClient;
class OrbiterGraphics;
class Orbiter {
	friend class ScriptInterface;
	friend class oapi::GraphicsClient;
	friend class OrbiterGraphics;

public:
	Orbiter ();
	~Orbiter ();

    void Create ();
	void Launch (const char *scenario);
	void CloseApp (bool fast_shutdown = false);
	int GetVersion () const;
	void StartSession (Config *pCfg, const char *scenario);
	void CreateRenderWindow ();
	void PreCloseSession();
	void CloseSession ();
	void GetRenderParameters ();
	bool InitializeWorld (char *name);
	void ScreenToClient (POINT *pt) const;
    //LRESULT MsgProc (HWND, UINT, WPARAM, LPARAM);
	void Render3DEnvironment();
	void RenderGUI();
	void DisplayFrame();
	void ClearFrame();
	void Output2DData ();
	void OutputLoadStatus (const char *msg, int line);
	void OutputLoadTick (int line, bool ok = true);
	void TerminateOnError();
	void InitRotationMode ();
	void ExitRotationMode ();
	bool StickyFocus() const { return bKeepFocus; }
	int Run ();
	void SingleFrame ();
    void Pause (bool bPause);
	void Freeze (bool bFreeze);
	inline void TogglePause () { Pause (bRunning); }
	bool Timejump (double _mjd, int pmode);
	void Suspend (void); // elapsed time between Suspend() and Resume() is ignored
	void Resume (void); // A Suspend/Resume pair must be closed within a time step
	bool SaveScenario (const char *fname, const char *desc);
	void SaveConfig ();
	void Quicksave ();
	void StartCaptureFrames () { video_skip_count = 0; bCapture = true; }
	void StopCaptureFrames () { bCapture = false; }
	bool IsCapturingFrames() const { return bCapture; }
	void CaptureVideoFrame ();
	const char *KeyState() const;

	void CloseDialog (GUIElement *e);
	void RegisterGUIElement(GUIElement *);
	void UnregisterGUIElement(GUIElement *);

	void UpdateDeallocationProgress();

	// plugin module loading/unloading
	MODULEHANDLE LoadModule (const char *path, const char *name, bool fatal = true, bool video = false);   // load a plugin
	void UnloadModule (MODULEHANDLE hi); // unload a plugin
	MODULEHANDLE hVideoModule;

	Vessel *SetFocusObject (Vessel *vessel, bool setview = true);
	// Select a new user-controlled vessel
	// Return value is old focus object, or 0 if focus hasn't changed

	void SetView (Body *body, int mode);
	// Change camera tracking or cockpit target
	// mode: 0=internal, 1=external, 2=don't change

	void InsertVessel (Vessel *vessel);
	// Insert a newly created vessel into the simulation

	bool KillVessels();
	// Kill the vessels that have been marked for deletion in the last time step

	inline double ManCtrlLevel (THGROUP_TYPE thgt, int device) const {
		switch (device) {
		case MANCTRL_KEYBOARD: return 0.001*ctrlKeyboard[thgt];
		case MANCTRL_JOYSTICK: return 0.001*ctrlJoystick[thgt];
		default:               return 0.001*ctrlTotal[thgt];
		}
	}

	void NotifyObjectJump (const Body *obj, const Vector &shift);
	void NotifyObjectSize (const Body *obj);

	void SetWarpFactor (double warp, bool force = false, double delay = 0.0);
	// Set time acceleration factor

	void SetFOV (double fov, bool limit_range = true);
	// Set camera field of view to fov (vertical half-screen) [rad]

	void IncFOV (double dfov);
	// Increase camera field of view by dfov

	// Accessor functions
	inline GLFWwindow *GetRenderWnd() const { return m_pGUIManager->hRenderWnd; }
	inline bool    IsFullscreen() const { return bFullscreen; }
	inline int   ViewW() const { return viewW; }
	inline int   ViewH() const { return viewH; }
	inline int   ViewBPP() const { return viewBPP; }
	inline Config* Cfg() const { return pConfig; }
	inline ScriptInterface *Script() const { return script; }
	//inline DialogManager *DlgMgr() const { return pDlgMgr; }
	inline State*  PState() const { return pState; }
	inline bool    IsActive() const { return bActive; } // temporary
	inline bool    IsRunning() const { return bRunning; }
	inline bool    UseStencil() const { return bUseStencil; }
	inline void    SetFastExit (bool fexit) { bFastExit = fexit; }
	inline bool    UseHtmlInline () { return (pConfig->CfgDebugPrm.bHtmlScnDesc == 1 || pConfig->CfgDebugPrm.bHtmlScnDesc == 2); }

	// DirectInput components
	// memory monitor
	MemStat *memstat;
	long simheapsize; // memory allocated during CreateRenderWindow

	// Onscreen annotation
	inline oapi::ScreenAnnotation *SNotePB() const { return snote_playback; }
	oapi::ScreenAnnotation *CreateAnnotation (bool exclusive, double size, COLORREF col);
	bool DeleteAnnotation (oapi::ScreenAnnotation *sn);

	// File locations - THESE FUNCTIONS ARE NOT THREADSAFE!
	inline char *ConfigPath (const char *name) { return pConfig->ConfigPath (name); }
	inline char *MeshPath   (const char *name) { return pConfig->MeshPath (name); }
	inline char *TexPath    (const char *name, const char *ext = 0)
		{ return pConfig->TexPath (name, ext); }
	inline char *HTexPath   (const char *name, const char *ext = 0)
		{ return pConfig->HTexPath (name, ext); }
	inline const char *ScnPath    (const char *name) { return pConfig->ScnPath (name); }

	FILE *OpenTextureFile (const char *name, const char *ext);
	// return texture file handle. Searches in hightex and standard directories

	SURFHANDLE RegisterExhaustTexture (const char *name);

	Keymap keymap;
	// keyboard mapper

	// Flight recorder
	char *FRsysname;             // system event playback name
	std::ifstream *FRsys_stream; // system event playback file
	double frec_sys_simt;        // system event timer
//	PlaybackEditor *FReditor;    // playback editor instance
	bool ToggleRecorder (bool force = false, bool append = false);
	void EndPlayback ();
	inline int RecorderStatus() const { return (bRecord ? 1 : bPlayback ? 2 : 0); }
	inline bool IsPlayback() const { return bPlayback; }
	const char *GetDefRecordName (void) const;
	void FRecorder_Reset ();
	// reset flight recorder status
	bool FRecorder_PrepareDir (const char *fname, bool force);
	// clear the flight recording directory
	void FRecorder_Activate (bool active, const char *fname, bool append = false);
	// activate the flight recorder
	void FRecorder_SaveEvent (const char *event_type, const char *event);
	// save a system event
	void FRecorder_OpenPlayback (const char *scname);
	// open system playback file
	void FRecorder_ClosePlayback ();
	// close system playback file
	void FRecorder_SuspendPlayback ();
	// closes the system event stream (for on-the-fly editing)
	void FRecorder_RescanPlayback ();
	// re-read the system event stream up to current playback time
	// (for on-the-fly editing)
	void FRecorder_Play ();
	// scan system playback file to current sim time
	void FRecorder_ToggleEditor ();
	// toggle the playback editor

	typedef struct {
		const char *label;
		const char *desc;
		int id;
		CustomFunc func;
		void *context;
	} CUSTOMCMD;

	struct {
		double dt;
		int mode;
	} tjump;

	int RegisterCustomCmd (const char *label, const char *desc, CustomFunc func, void *context);
	bool UnregisterCustomCmd (int cmdId);

	MeshManager     meshmanager;    // global mesh manager

	// Load a mesh from file, and store it persistently in the mesh manager
	const Mesh *LoadMeshGlobal (const char *fname);
	const Mesh *LoadMeshGlobal (const char *fname, LoadMeshClbkFunc fClbk);

	// graphics client shortcuts
	inline SURFHANDLE LoadTexture (const char *fname, int flags = 0)
	{ return (gclient ? gclient->clbkLoadTexture (fname, flags) : NULL); }

	//inline SURFHANDLE CreateSurface (int w, int h, int attrib)
	//{ return (gclient ? gclient->clbkCreateSurfaceEx (w, h, attrib) : NULL); }

	//inline SURFHANDLE CreateTexture (int w, int h)
	//{ return (gclient ? gclient->clbkCreateTexture (w, h) : NULL); }

	inline bool ReleaseSurface (SURFHANDLE surf)
	{ return (gclient ? gclient->clbkReleaseSurface (surf) : false); }

	inline bool SetSurfaceColourKey (SURFHANDLE surf, uint32_t ckey)
	{ return (gclient ? gclient->clbkSetSurfaceColourKey (surf, ckey) : false); }

	inline uint32_t GetDeviceColour (uint8_t r, uint8_t g, uint8_t b)
	{ return (gclient ? gclient->clbkGetDeviceColour (r, g, b) : 0); }

	inline bool Blt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int flag = 0)
	{ return (gclient ? gclient->clbkBlt (tgt, tgtx, tgty, src, flag) : false); }

	inline bool Blt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int srcx, int srcy, int w, int h, int flag = 0)
	{ return (gclient ? gclient->clbkBlt (tgt, tgtx, tgty, src, srcx, srcy, w, h, flag) : false); }

	inline bool FillSurface (SURFHANDLE surf, uint32_t col)
	{ return (gclient ? gclient->clbkFillSurface (surf, col) : false); }

	inline bool FillSurface (SURFHANDLE surf, int tgtx, int tgty, int w, int h, uint32_t col)
	{ return (gclient ? gclient->clbkFillSurface (surf, tgtx, tgty, w, h, col) : false); }

	bool SendKbdBuffered(int key, int *mod = 0, int nmod = 0, bool onRunningOnly = false);
	// Simulate a buffered keypress with an optional list of modifier keys

	bool SendKbdImmediate(char kstate[256], bool onRunningOnly = false);
	// Simulate an immediate key state

	bool MouseEvent (oapi::MouseEvent event, int state, int x, int y);
protected:
	void UserInput ();
	void KbdInputImmediate_System    (char *kstate);
	void KbdInputImmediate_OnRunning (char *buffer);
	void KbdInputBuffered_System     (char *kstate, int key);
	void KbdInputBuffered_OnRunning  (char *kstate, int key);
	bool BroadcastMouseEvent (oapi::MouseEvent event, int state, int x, int y);
	bool BroadcastImmediateKeyboardEvent (char *kstate);
	void BroadcastBufferedKeyboardEvent (char *kstate, int key);

	void BroadcastGlobalInit();

	bool BeginTimeStep (bool running);
	// Initialise the next frame time from the current system time. Returns true if
	// time was advanced or if running==false (paused). Returns false if not enough time
	// has passed since the current frame time (i.e. skip this update)

	void EndTimeStep (bool running);
	// Finish step update by copying next frame time data to current frame time data

	bool SessionLimitReached() const;
	// Return true if a session duration limit has been reached (frame limit/time limit, if any)

	void ModulePreStep ();
	void ModulePostStep ();
	void UpdateWorld ();

	void IncWarpFactor ();
	void DecWarpFactor ();
	// Increment/decrement time acceleration factor to next power of 10

	void ApplyWarpFactor ();
	// broadcast new warp factor to components and modules

public:
	std::unique_ptr<GUIManager> m_pGUIManager;
	std::unique_ptr<DlgCamera> m_DlgCamera;
	std::unique_ptr<DlgInfo> m_DlgInfo;
	std::unique_ptr<DlgTacc> m_DlgTacc;
	std::unique_ptr<DlgMap> m_DlgMap;
	std::unique_ptr<DlgFocus> m_DlgFocus;
	std::unique_ptr<DlgFunction> m_DlgFunction;
	std::unique_ptr<DlgVishelper> m_DlgVishelper;
	std::unique_ptr<DlgLaunchpad> m_DlgLaunchpad;
	std::unique_ptr<DlgRecorder> m_DlgRecorder;
	std::unique_ptr<Select> m_DlgSelect;
	std::unique_ptr<InputBox> m_DlgInputBox;
	std::unique_ptr<DlgPlaybackEditor> m_DlgPlaybackEditor;
	std::unique_ptr<DlgJoystick> m_DlgJoystick;

private:
	Config         *pConfig;
	State          *pState;
	//DialogManager  *pDlgMgr;
	//GLFWwindow     *hRenderWnd;    // render window handle (NULL if no render support)
	bool            bRenderOnce;   // flag for single frame render request
	bool            bEnableLighting;
	bool			bUseStencil;   // render device provides stencil buffer (and user requests it)
	bool            bKeepFocus;    // disable focus switch on mouse move (during rotations)
	bool            ignorefirst;   // flag for first joystick action
	long            plZ4;          // previous joystick throttle setting
	int             video_skip_count; // count skipped frames for frame sequence capturing
	oapi::ScreenAnnotation **snote;// onscreen annotations
	int             nsnote;        // number of annotations
	oapi::ScreenAnnotation *snote_playback;// onscreen annotation during playback
	ScriptInterface *script;
	INTERPRETERHANDLE hScnInterp;

	// render parameters (only used if graphics client is present)
	bool			bFullscreen;   // renderer in fullscreen mode
	int             viewW, viewH;  // render viewport dimensions
	int		    	viewBPP;       // render colour depth (bits per pixel)

	char            cfgpath[256];
	int             cfglen;
	char            simkstate[256];// accumulated simulated key state

	std::chrono::time_point<std::chrono::steady_clock> time_prev;       // used for time step calculation
	std::chrono::time_point<std::chrono::steady_clock> time_suspend;    // used for time-skipping within a step
	bool            bActive;       // render window has focus
	bool            bAllowInput;   // allow input processing for the next frame even if render window doesn't have focus
	bool            bVisible;      // render window exists and is visible
	bool            bRunning;      // simulation is running
	bool            bRequestRunning; // pause/resume request
	bool            bSession;      // simulation session in progress
	bool			bRealtime;     // TRUE if TWarp == 1
	bool            bEnableAtt;    // TRUE if manual attitude control (keyboard or joystick) is enabled
	bool            bRecord;       // true if flight is being recorded
	bool            bPlayback;     // true if flight is being played back
	bool            bCapture;      // capturing frame sequence is active
	bool            bFastExit;     // terminate on simulation end?

	// Manual joystick/keyboard attitude inputs
	int ctrlJoystick[15];
	int ctrlKeyboard[15];
	int ctrlTotal[15];

	void SavePlaybackScn (const char *fname);

	// === The plugin module interface ===
	struct DLLModule {               // list of plugin modules
		oapi::Module *module;
		MODULEHANDLE hMod;
		//OPC_Interface *intf;
		char *name;
		bool bVideoPlugin;
		bool bLocalAlloc;         // locally allocated; should be freed by Orbiter core
	} *module;
	int nmodule;                  // number of plugins

	oapi::Module *register_module;  // used during module registration
	friend OAPIFUNC void oapiRegisterModule (oapi::Module* module);

	void LoadFixedModules ();                   // load all startup plugins
	OPC_Proc FindModuleProc (int nmod, const char *procname);
	// returns address of a procedure in a plugin module, or NULL if procedure not found
	// list of custom commands
	CUSTOMCMD *customcmd;
	int ncustomcmd;
	friend class DlgFunction;

public:
	// external graphics client
	bool AttachGraphicsClient (oapi::GraphicsClient *gc);
	bool RemoveGraphicsClient (oapi::GraphicsClient *gc);
	inline oapi::GraphicsClient *GetGraphicsClient () { return gclient; }
	void MakeContextCurrent(bool);
private:
	oapi::GraphicsClient *gclient;       // external graphics client (renderer)
	OrbiterGraphics *oclient;            // inline graphics client

public:
	inline OrbiterGraphics *GetInlineGraphicsClient() { return oclient; }
	// (to access special inline graphics features. Eventually this should no longer
	// be necessary)
	void KeyCallback(int key, char state);

};

#endif // !ORBITER_H
