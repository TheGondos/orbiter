// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                  ORBITER MODULE: ExtMFD
//                  Part of the ORBITER SDK
//            Copyright (C) 2006 Martin Schweiger
//                   All rights reserved
//
// MFDWindow.h
//
// Class interface for MFDWindow. Defines the properties and state
// of an MFD display in a dialog box
// ==============================================================

#ifndef __MFDWINDOW_H
#define __MFDWINDOW_H

#include "Orbitersdk.h"

class MFDWindow: public ExternMFD {
public:
	MFDWindow (const MFDSPEC &spec);
	virtual ~MFDWindow ();
	void SetVessel (OBJHANDLE hV) override;
	void ProcessButton (int bt, int event);
	void StickToVessel (bool stick);

	void clbkFocusChanged (OBJHANDLE hFocus) override;
	bool m_poweroff;

private:
	int BW, BH;       // button width and height
	int fnth;         // button font height
	bool vstick;      // stick to vessel
};

#endif // !__MFDWINDOW_H