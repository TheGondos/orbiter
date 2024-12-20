// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// class Pane
// Controls all GDI output on the 2D window
// (HUD, MDF, lists, etc.)

#ifndef __PANE_H
#define __PANE_H

#ifndef STRICT
#define STRICT 1
#endif
#include "Orbiter.h"
#include "Body.h"
#include "Mfd.h"
#include "hud.h"
#include "Select.h"

#define MSG_KILLVESSEL    0x1000
#define MSG_KILLNAVSENDER 0x1001
#define MSG_CREATEVESSEL  0x1002
#define MSG_PAUSE         0x1003
#define MSG_FOCUSVESSEL   0x1004

// =======================================================================
// forward declarations

class HUD;
class Vessel;

struct MFDspec {        // panel MFD specs
	Instrument *instr;  // pointer to MFD instance
	int lastmode;       // last MFD mode 
	bool exist;         // MFD present?
	bool active;        // MFD switched on?
	double upDTscale;   // refresh interval scale
	EXTMFDSPEC prm;     // MFD parameters
};

// =======================================================================
// class Pane

class HUD_Orbit;
class HUD_Surface;
class HUD_Docking;
class Instrument;
class DefaultPanel;
class Panel2D;
class Panel;
class VirtualCockpit;
class MenuInfoBar;
class Pane {
	friend class HUD;
	friend class HUD_Orbit;
	friend class HUD_Surface;
	friend class HUD_Docking;
	friend class Instrument;
	friend class DefaultPanel;
	friend class Panel2D;
	friend class Panel;
	friend class VirtualCockpit;
	friend class MenuInfoBar;

public:
	Pane (oapi::GraphicsClient *gclient, int width, int height, int bpp);
	// Create a new pane with dimension width x height x bpp

	~Pane ();

	int Width() const { return W; }
	int Height() const { return H; }
	int BitsPerPixel() const { return BPP; }
	// Return pane dimensions

	void Update (double simt, double syst);
	// Update GDI pane display for simulation time simt and system time syst

	void Draw ();
	// Update drawn elements of 2D pane (executed after scene rendering)

	void Render ();
	// Update rendered elements of 2D pane

	void Timejump ();
	// discontinuous update

	void FocusChanged (const Vessel *focus);
	// Called after focus has changed to "focus"

	void DelVessel (const Vessel *vessel);
	// notification that "vessel" is about to be destroyed

	void SetSketchpadDefault (oapi::Sketchpad *skp);
	// set HUD defaults for drawing objects (font, text colour, pen)

	bool MFDConsumeKeyBuffered (MfdId id, int key);
	// Process a buffered key for MFD id

	bool ProcessMouse_System(oapi::MouseEvent event, int state, int x, int y, const char *kstate);

	bool ProcessMouse_OnRunning (oapi::MouseEvent event, int state, int x, int y, const char *kstate);
	// Process a mouse click/release

	void SetFOV (double _fov);
	// Register field of view for output

	void SetWarp (double _warp);
	// Register time acceleration for output

	void RedrawCockpitAreas (int mode);
	// Trigger a redraw of all active areas of the current cockpit

	void TogglePanelMode ();
	// Switch between panel display modes

	bool SetPanelMode (int pmode, bool force = false);
	// Set panel display mode

	inline int GetPanelMode () const { return panelmode; }
	// 0=none, 1=MFDs only, 2=2D panel view, 3=virtual cockpit

	inline DefaultPanel *GetDefaultPanel() { return defpanel; }

	int SelectPanel (int id);
	// select 2D panel by id.

	int SelectVC (int id);
	// select VC position by id.

	int SwitchPanel (int dir);
	// switch to different 2D panel or VC position.
	// dir: 0=left, 1=right, 2=up, 3=down from current
	// returns id of new panel, or -1 if not switched

	void ShiftPanel (double dx, double dy);
	// move instrument panel on screen. dx, dy: pixels/sec

	void ShowHUD (bool yes = true);
	// turn head up display on/off

	inline void ToggleHUD () { ShowHUD (hud == 0); }
	// Toggle HUD on/off

	void SwitchHUDMode ();
	// Cycle to next mode

	bool SetHUDMode (int _hudmode);
	// Set HUD (head up display) mode. Returns true if mode has changed

	bool SetHUDMode (int _hudmode, const HUDPARAM *prm);
	// Set HUD mode with additional mode-specific parameters.
	// Returns true if mode has changed.

	int GetHUDMode (HUDPARAM *prm = NULL) const;
	//inline int GetHUDMode () const;
	// return HUD mode

	void SetHUDColour (int idx, double intens, bool force=false);
	// Set HUD colour from colour index and intensity

	void ToggleHUDColour ();
	// switch between default HUD display colours

	double HudIntens () const { return hudIntens; }
	void SetHUDIntens (double val);
	void IncHUDIntens ();
	void DecHUDIntens ();

	inline HUD *GetHUD () { return hud; }

	inline void DrawDefaultHUD (oapi::Sketchpad *skp);
	inline void RenderDefaultHUD ();
	void RenderCustomHUD (MESHHANDLE hMesh, SURFHANDLE *hTex);

	SURFHANDLE GetMFDSurface (MfdId id);

	void ToggleMFD_on (MfdId id);
	// switch MFD on or off

	void RefreshMFD (int id);
	// If open, close and re-open MFD (e.g. to allow resizing)

	double SetMFDRefreshIntervalMultiplier (int id, double multiplier);
	// modify the refresh interval of an MFD

	bool OpenMFD (MfdId id, int type, std::ifstream *ifs = 0);
	// open specified instrument as left/right MFD. Returns true if MFD mode has changed
	// If scenario stream is provided, parameters are read from this stream

	bool CloseMFD (int id);
	//  Deactivates the specified MFD. Returns false if id invalid or MFD not active

	void MFDModeDisabled (int mode);
	// notify pane that an MFD mode has been disabled (shut down MFD if required)

	int BroadcastMFDMessage (int mfdmode, int msg, void *data);
	// calls the ProcessMessage method for all MFDs currently in mode 'mfdmode'

	int BroadcastMFDMessage (int msg, void *data);
	// calls the ProcessMessage method for all MFDs

	void RegisterMFD (int id, const MFDSPEC &spec);
	void RegisterMFD (int id, const EXTMFDSPEC *spec);
	void UnregisterMFD (int id);
	void RegisterExternMFD (ExternMFD *mfd, const MFDSPEC &spec);
	bool UnregisterExternMFD (ExternMFD *mfd);

	Instrument *MFD (MfdId mfdid);

	void RepaintMFDButtons (MfdId id, Instrument *instr);
	Instrument::Spec GetVCMFDSpec ();
	const VCMFDSPEC *GetVCMFDParams (MfdId id);
	const VirtualCockpit *GetVC() const { return vcockpit; }
	VirtualCockpit *GetVC() { return vcockpit; }

	Panel2D *GetPanel2D() { return panel2d; }

	void RegisterPanelBackground (SURFHANDLE hSurf, int flag, uint32_t ck);
	void RegisterPanelArea (int id, const RECT &pos, int draw_mode, int mouse_mode, int bkmode);
	void SetPanelNeighbours (int left, int right, int top, int bottom);
	void SetVCNeighbours (int left, int right, int top, int bottom);
	void TriggerPanelRedrawArea (int pid, int aid);
	bool BltPanelAreaBackground (int aid, SURFHANDLE surf);

	void RegisterVCMFD (int id, const VCMFDSPEC *spec);
	void RegisterVCHUD (const VCHUDSPEC *spec);
	void ShiftVC (const Vector &shift);
	void RegisterVCArea (int id, const RECT &tgtrect, int draw_mode, int mouse_mode, int bkmode, SURFHANDLE tgt);
	void SetVCAreaClickmode_Spherical (int id, const Vector &cnt, double rad);
	void SetVCAreaClickmode_Quadrilateral (int id, const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4);
	void TriggerVCRedrawArea (int vcid, int area_id);
	void TriggerRedrawArea (int pid, int vcid, int area_id);

	void SetPanel2DBlink (VECTOR3 v[4]);

	bool GlobalToHomog (const Vector &glob, glm::fvec3 &homog) const;
	// transform global position glob into homogeneous viewport
	// coordinates (x=-1: left edge of viewing fustrum etc.)
	// return value indicates point within fustrum (does not check
	// front and back planes)

	bool GlobalToScreen (const Vector &glob, int &x, int &y) const;
	bool GlobalToScreen (const Vector &glob, double &x, double &y) const;
	// return screen coordinates for global position glob
	// return value indicates point visible on screen
	// x and y are undefined if not visible

	void ScreenToGlobal (int x, int y, Vector &glob) const;
	// return global direction corresponding to screen coordinate x,y

	void InitState (const char *scn);
	bool Read (std::ifstream &ifs);
	void Write (std::ostream &ofs) const;

	int f1W, f1H;  // Font1 character size (Courier)
	int f2W, f2H;  // Font2 character size (Arial)

	inline MenuInfoBar *MIBar() const { return mibar; }

	void Resize(int width, int height);
private:
	void InitResources ();
	// Generate GDI resources

	void FreeResources ();
	// Free GDI resources

	void UpdateResources();
	// Update resources when resizing

	oapi::GraphicsClient *gc; // client instance
	int W, H, BPP;            // pane dimensions
	int scaleW;
	int colidx;              // HUD colour index
	COLORREF hudCol;         // HUD colour
	double hudIntens;        // HUD intensity (VC only)
	oapi::Font *hudfont[2];  // HUD font resource
	oapi::Pen *hudpen;       // HUD pen resource
	SURFHANDLE mfdTex_blank; // dummy texture for blank MFD surfaces
	MenuInfoBar *mibar;      // main menu and info displays

	int panelmode;           // 0=none, 1=MFDs only, 2=2D panels, 3=virtual cockpit

	HUD *hud;                  // head up display
	int hudmode;               // HUD mode
	DefaultPanel *defpanel;    // generic cockpit view
	Panel2D *panel2d;          // 2D instrument panel
	Panel *panel;              // 2D instrument panel (old style)
	VirtualCockpit *vcockpit;  // virtual cockpit
	double panel_dx, panel_dy; // panel shift
	int vcid;                  // currently selected VC position

	MFDspec mfd[MAXMFD];       // panel MFD displays
	std::vector<ExternMFD *> m_emfd;
	bool mfdsize_pow2;         // force power-2 MFD sizes?
	int mfd_hires_threshold;   // MFD size at which to switch to 512
	int mfd_vc_size;           // texture size for VC MFD displays (256/512/1024)

	struct {
		MESHHANDLE mesh2d;
		SURFHANDLE tex;
	} blinkmesh;
};

// =======================================================================
// inline functions

inline void Pane::DrawDefaultHUD (oapi::Sketchpad *skp)
{ if (hud) hud->DrawDefault (skp); }

inline void Pane::RenderDefaultHUD ()
{ if (hud) hud->RenderDefault (); }

#endif // !__PANE_H