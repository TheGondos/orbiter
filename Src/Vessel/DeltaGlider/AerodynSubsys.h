// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                ORBITER MODULE: DeltaGlider
//                  Part of the ORBITER SDK
//
// AerodynSubsys.h
// Subsystem for aerodynamic controls (selector dial, elevator
// trim, airbrake)
// ==============================================================

#ifndef __AERODYNSUBSYS_H
#define __AERODYNSUBSYS_H

#include "DGSwitches.h"
#include "DGSubsys.h"

// ==============================================================
// Aerodynamic control subsystem
// ==============================================================

class AerodynSelector;
class Airbrake;
class ElevatorTrim;

class AerodynCtrlSubsystem: public DGSubsystem {
public:
	AerodynCtrlSubsystem (DeltaGlider *v);
	Airbrake *AirbrakeSubsys() { return airbrake; }
	void SetMode (int mode);
	void ExtendAirbrake ();
	void RetractAirbrake ();
	const AnimState2 &AirbrakeState() const;

private:
	AerodynSelector *selector;
	Airbrake *airbrake;
	ElevatorTrim *elevtrim;
};

// ==============================================================
// Control selector dial
// ==============================================================

class AerodynSelectorDial;
class AerodynSelector: public DGSubsystem {
	friend class AerodynSelectorDial;

public:
	AerodynSelector (AerodynCtrlSubsystem *_subsys);
	bool clbkLoadPanel2D (int panelid, PANELHANDLE hPanel, int viewW, int viewH) override;
	bool clbkLoadVC (int vcid) override;
	void SetMode (int mode);
	int GetMode () const;
protected:
	bool IncMode();
	bool DecMode();

private:
	AerodynSelectorDial *dial;
	int ELID_DIAL;
};

// ==============================================================

class AerodynSelectorDial: public DGDial1 {
public:
	AerodynSelectorDial (AerodynSelector *comp);
	void Reset2D (int panelid, MESHHANDLE hMesh);
	void ResetVC (DEVMESHHANDLE hMesh);
	bool Redraw2D (SURFHANDLE surf);
	bool RedrawVC (DEVMESHHANDLE hMesh, SURFHANDLE surf);
	bool ProcessMouse2D (int event, int mx, int my);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	AerodynSelector *component;
};

// ==============================================================
// Airbrake
// ==============================================================

class AirbrakeLever;
class Airbrake: public DGSubsystem {
	friend class AirbrakeLever;

public:
	Airbrake (AerodynCtrlSubsystem *_subsys);
	void Extend ();
	void Retract ();
	inline const AnimState2 &State() const { return brake_state; }
	inline int TargetState() const { return airbrake_tgt; } // 0,1,2
	void clbkPostStep (double simt, double simdt, double mjd) override;
	bool clbkLoadPanel2D (int panelid, PANELHANDLE hPanel, int viewW, int viewH) override;
	bool clbkLoadVC (int vcid) override;
	void clbkSaveState (FILEHANDLE scn) override;
	bool clbkParseScenarioLine (const char *line) override;
	void clbkPostCreation () override;
	bool clbkPlaybackEvent (double simt, double event_t, const char *event_type, const char *event) override;
	int clbkConsumeBufferedKey (int key, bool down, char *kstate) override;

private:
	AirbrakeLever *lever;
	int ELID_LEVER;
	int airbrake_tgt;
	unsigned int anim_brake;            // handle for airbrake animation
	unsigned int anim_airbrakelever;    // VC airbrake lever
	AnimState2 brake_state, lever_state;
};

// ==============================================================

class AirbrakeLever: public PanelElement {
public:
	AirbrakeLever (Airbrake *comp);
	void Reset2D (int panelid, MESHHANDLE hMesh);
	void ResetVC (DEVMESHHANDLE hMesh);
	bool Redraw2D (SURFHANDLE surf);
	bool ProcessMouse2D (int event, int mx, int my);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	Airbrake *component;
	int state;
};

// ==============================================================
// Elevator trim control
// ==============================================================

class ElevatorTrimWheel;
class ElevatorTrim: public DGSubsystem {
	friend class ElevatorTrimWheel;

public:
	ElevatorTrim (AerodynCtrlSubsystem *_subsys);
	void clbkSaveState (FILEHANDLE scn) override;
	bool clbkParseScenarioLine (const char *line) override;
	bool clbkLoadPanel2D (int panelid, PANELHANDLE hPanel, int viewW, int viewH) override;
	bool clbkLoadVC (int vcid) override;

private:
	ElevatorTrimWheel *trimwheel;
	int ELID_TRIMWHEEL;
	unsigned int anim_vc_trimwheel;     // VC elevator trim wheel
};

// ==============================================================

class ElevatorTrimWheel: public PanelElement {
public:
	ElevatorTrimWheel (ElevatorTrim *comp);
	void Reset2D (int panelid, MESHHANDLE hMesh);
	void ResetVC (DEVMESHHANDLE hMesh);
	bool Redraw2D (SURFHANDLE surf);
	bool RedrawVC (DEVMESHHANDLE hMesh, SURFHANDLE surf);
	bool ProcessMouse2D (int event, int mx, int my);
	bool ProcessMouseVC (int event, VECTOR3 &p);

private:
	ElevatorTrim *component;
	double trimpos2D, trimposVC;
};

#endif // ___AERODYNSUBSYS_H