// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                 ORBITER MODULE:  ShuttleA Payload
//                 
//
// ShuttleA_pl.h
// Class interface for Shuttle-A Payload vessel class module
// ==============================================================

#ifndef __SHUTTLEA_PL_H
#define __SHUTTLEA_PL_H

#include "Orbitersdk.h"

// ==========================================================
// Some vessel class caps
// ==========================================================
const static double DROGUE_DEPLOY_DELAY=8.0;

// ==========================================================
// Interface for derived vessel class: ShuttleA
// ==========================================================

class ShuttleA_PL: public VESSEL2 {
public:
	ShuttleA_PL (OBJHANDLE hObj, int fmodel);
	~ShuttleA_PL ();
	MESHHANDLE mesh_main;
	MESHHANDLE mesh_drogue;
	MESHHANDLE mesh_parachute;

	ATTACHMENTHANDLE  payload_attachment[3];
	int Parachute_mode;	//0- nill; 1-drogue; 2-main parachute deployed	; 3- landed 
    double timer;		//there is a timer so the chute won't eject too soon after jett
	
	// Overloaded callback functions
	void clbkLoadStateEx (FILEHANDLE scn, void *vs) override;
	void clbkSaveState (FILEHANDLE scn) override;
	void clbkSetClassCaps (FILEHANDLE cfg) override;
	void clbkPostStep (double simt, double simdt, double mjd) override;
private:
	
	void SetNormalConfig();
	void SetDrogueConfig();
	void SetParachuteConfig();
	void SetPostLandingConfig();
};
#endif