// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                ORBITER MODULE: DeltaGlider
//                  Part of the ORBITER SDK
//
// HudCtrl.h
// Class for HUD control subsystem
// ==============================================================

#ifndef __HUDCTRL_H
#define __HUDCTRL_H

#include "DeltaGlider.h"
#include "DGSubsys.h"
#include "DGSwitches.h"

// ==============================================================
// HUD related constants

const double HUD_OPERATING_SPEED = 0.15;

// ==============================================================
// HUD control subsystem
// ==============================================================

class HUDModeButtons;
class HUDBrightnessDial;
class HUDColourButton;
class HUDUpDownSwitch;

class HUDControl: public DGSubsystem {
public:
	HUDControl (DeltaGlider *vessel);
	void clbkSaveState(FILEHANDLE scn) override;
	bool clbkParseScenarioLine(const char *line) override;
	int GetHUDMode () const;
	void SetHUDMode (int mode);
	void ToggleHUDMode ();
	void RetractHud();
	void ExtendHud();
	void RevertHud ();
	void ModHUDBrightness (bool increase);
	void clbkPostStep (double simt, double simdt, double mjd) override;
	bool clbkLoadPanel2D (int panelid, PANELHANDLE hPanel, int viewW, int viewH) override;
	bool clbkLoadVC (int vcid) override;
	void clbkReset2D(int panelid, MESHHANDLE hMesh) override;
	void clbkResetVC (int vcid, DEVMESHHANDLE hMesh) override;
	int clbkConsumeBufferedKey (int key, bool down, char *kstate) override;

private:
	int last_mode;
	AnimState2 hud_state;
	double hud_brightness;

	HUDModeButtons *modebuttons;   // mode buttons object
	HUDBrightnessDial *brightdial; // HUD brightness dial object
	HUDColourButton *colbutton;    // HUD colour button object
	HUDUpDownSwitch *updownswitch; // HUD extend/retract switch object

	int ELID_MODEBUTTONS;          // element ID: mode buttons
	int ELID_HUDBRIGHTNESS;        // element ID: brightness dial
	int ELID_HUDCOLOUR;            // element ID: colour button
	int ELID_HUDRETRACT;           // element ID: HUD extend/retract switch

	unsigned int anim_vc_hudbdial;         // VC HUD brightness dial
	unsigned int anim_vc_hud;              // VC HUD folding away
};

// ==============================================================
// HUD mode selector buttons

class HUDModeButtons: public PanelElement {
public:
	HUDModeButtons (HUDControl *hc);
	~HUDModeButtons ();
	void DefineAnimationsVC (const VECTOR3 &axis, int meshgrp, int meshgrp_label,
		int vofs[3], int vofs_label[3]);
	void SetMode (int mode);
	void Reset2D (int panelid, MESHHANDLE hMesh);
	void ResetVC (DEVMESHHANDLE hMesh);
	void LoadPanel2D(int panelid, PANELHANDLE hPanel, int viewW, int viewH);
	void LoadVC (int vcid);
	bool Redraw2D (SURFHANDLE surf);
	bool RedrawVC (DEVMESHHANDLE hMesh, SURFHANDLE surf);
	bool ProcessMouse2D (int event, int mx, int my);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	HUDControl *ctrl;
	DGButton3 *btn[3];
	int vmode; // currently displayed HUD mode
};

// ==============================================================
// HUD brightness dial

class HUDBrightnessDial: public PanelElement {
public:
	HUDBrightnessDial (HUDControl *hc);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	HUDControl *ctrl;
};

// ==============================================================
// HUD colour selector button

class HUDColourButton: public DGButton2 {
public:
	HUDColourButton (HUDControl *hc);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	HUDControl *ctrl;
};

// ==============================================================
// HUD retract/extend switch

class HUDUpDownSwitch: public DGSwitch1 {
public:
	HUDUpDownSwitch (HUDControl *hc);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	HUDControl *ctrl;
};

#endif // !__HUDCTRL_H