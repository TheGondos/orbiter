// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                 ORBITER MODULE: ShuttleA
//                  Part of the ORBITER SDK
//
// mfdbutton.h
// User interface for MFD buttons
// ==============================================================

#ifndef __MFDBUTTON_H
#define __MFDBUTTON_H

#include "ShuttleA.h"
#include "../Common/Instrument.h"

// ==============================================================

class MFDButtonCol: public PanelElement {
public:
	MFDButtonCol (VESSEL3 *v, int _mfdid, int _lr);
	void AddMeshData2D (MESHHANDLE hMesh, int grpidx);
	bool Redraw2D (SURFHANDLE surf);
	bool ProcessMouse2D (int event, int mx, int my);

private:
	int mfdid;
	int lr;
	int xcnt;
};

// ==============================================================

class MFDButtonRow: public PanelElement {
public:
	MFDButtonRow (VESSEL3 *v, int _mfdid);
	bool ProcessMouse2D (int event, int mx, int my);

private:
	int mfdid;
};

#endif // !__MFDBUTTON_H