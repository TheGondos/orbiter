// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __LUAMFD_H
#define __LUAMFD_H

#include "MfdInterpreter.h"

// ==============================================================
// MFD class interface

class ScriptMFD: public MFD {
public:
	ScriptMFD (int w, int h, VESSEL *vessel);
	~ScriptMFD();
	const char *ButtonLabel (int bt);
	int ButtonMenu (const MFDBUTTONMENU **menu) const;
	bool ConsumeKeyBuffered (int key);
	bool ConsumeButton (int bt, int event);
	bool Update (oapi::Sketchpad *skp);
	bool Input (const char *line);
	void QueryCommand ();
	void CreateInterpreter ();
	void DeleteInterpreter ();
	void SetPage (int newpg);
	static bool ScriptInput (void *id, const char *str, void *data);
	static OAPI_MSGTYPE MsgProc (MFD_msg msg, MfdId mfd,  MFDMODEOPENSPEC *param, VESSEL *vessel);

protected:
	void SetFontSize (double size);

private:
	InterpreterList::VesselInterp *vi;
	OBJHANDLE hVessel;      // vessel object handle
	std::thread interpTh;        // interpreter thread handle
	oapi::Font *hFont;            // font handle
	int pg;               // current page   
	int fw, fh;           // character width, height
	int nchar;            // characters per line displayed
	int nline;            // max number of lines displayed
};

#endif // !__LUAMFD_H