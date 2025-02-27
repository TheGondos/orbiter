// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                 ORBITER MODULE: SolarSail
//                  Part of the ORBITER SDK
//
// SolarSail.h
// SolarSail class interface
// ==============================================================

#ifndef __SOLARSAIL_H
#define __SOLARSAIL_H

#define STRICT 1
#include "Orbitersdk.h"

#define MAXNBHR 6 // max node neighbours

struct NBHR {            // node neigbourhood structure
	int nnd;              // number of neighbours
	int nd[MAXNBHR];      // neighbour index list
	double dst0[MAXNBHR];   // neighbour distance list
	bool fix;               // fix node position
};

// ==============================================================
// SolarSail interface

class SolarSail: public VESSEL3 {
public:
	SolarSail (OBJHANDLE hVessel, int flightmodel);

	// one-time global setup across all instances
	static void GlobalSetup();

	void clbkSetClassCaps (FILEHANDLE cfg) override;
	void clbkPreStep (double simt, double simdt, double mjd) override;
	void clbkPostStep (double simt, double simdt, double mjd) override;
	void clbkGetRadiationForce (const VECTOR3 &mflux, VECTOR3 &F, VECTOR3 &pos) override;
	void clbkVisualCreated (VISHANDLE vis, int refcount) override;
	void clbkVisualDestroyed (VISHANDLE vis, int refcount) override;
	int  clbkGeneric (int msgid, int prm, void *context) override;

	// update sail nodal displacements
	void UpdateSail (const VECTOR3 *rpressure);
	void SetPaddle (int p, double pos);

private:
	MESHHANDLE hMesh;           // mesh instance handle
	VECTOR3 mf;                 // radiation mass flux
	unsigned int anim_paddle[4];        // steering paddle animation identifiers
	double paddle_rot[4];       // paddle logical rotation state (0-1, 0.5=neutral)
	double paddle_vis[4];       // paddle visual rotation state

	void DefineAnimations();

	// script interface-related methods
	int Lua_InitInterpreter (void *context);
	int Lua_InitInstance (void *context);

	static void SetupElasticity (MESHHANDLE hMesh);
	static MESHHANDLE hMeshTpl; // global mesh template
	static int sail_nvtx, sail_ntri;
	static NBHR *nbhr;
	static VECTOR3 *sail_vbuf;   // vertex temporary buffer
};

#endif // !__SOLARSAIL_H