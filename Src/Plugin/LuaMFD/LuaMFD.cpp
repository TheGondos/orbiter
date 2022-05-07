// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define STRICT 1
#define ORBITER_MODULE
#include "Orbitersdk.h"
#include "LuaMFD.h"
#include <cstring>

// ==============================================================
// Global variables

int g_MFDmode;
InterpreterList *g_IList = NULL;

// ==============================================================
// MFD class implementation

// Constructor
ScriptMFD::ScriptMFD (int w, int h, VESSEL *vessel)
: MFD (w, h, vessel)
{
	hVessel = pV->GetHandle();
	hFont = NULL;
	SetFontSize (0.8);
	vi = g_IList->FindVesselInterp (hVessel);
	if (!vi) {
		g_IList->AddInterpreter (hVessel);
		vi = g_IList->FindVesselInterp (hVessel);
	}
	pg = 0;
}

// Destructor
ScriptMFD::~ScriptMFD ()
{}

// Return button labels
const char *ScriptMFD::ButtonLabel (int bt)
{
	// The labels for the two buttons used by our MFD mode
	static const char *label[5] = {"INP", "NEW", "DEL", "PG>", "<PG"};
	return (bt < 5 ? label[bt] : 0);
}

// Return button menus
int ScriptMFD::ButtonMenu (const MFDBUTTONMENU **menu) const
{
	// The menu descriptions for the two buttons
	static const MFDBUTTONMENU mnu[5] = {
		{"Input command", 0, 'i'},
		{"Create new terminal", "page", 'n'},
		{"Delete terminal page", 0, 'd'},
		{"Next page", 0, '.'},
		{"Previous page", 0, ','}
	};
	if (menu) *menu = mnu;
	return 5; // return the number of buttons used
}

bool ScriptMFD::ConsumeKeyBuffered (int key)
{
	switch (key) {
	case OAPI_KEY_I:
		QueryCommand();
		return true;
	case OAPI_KEY_N:
		CreateInterpreter();
		return true;
	case OAPI_KEY_D:
		DeleteInterpreter();
		return true;
	case OAPI_KEY_PERIOD:
		SetPage (pg+1);
		return true;
	case OAPI_KEY_COMMA:
		SetPage (pg-1);
		return true;
	}
	return false;
}

bool ScriptMFD::ConsumeButton (int bt, int event)
{
	//Open popup on button up, or the ImGui modal will absorbe the event
	if (!(event & PANEL_MOUSE_LBUP)) return false;
	static const int btkey[5] = { OAPI_KEY_I, OAPI_KEY_N, OAPI_KEY_D, OAPI_KEY_PERIOD, OAPI_KEY_COMMA };
	if (bt < 5) return ConsumeKeyBuffered (btkey[bt]);
	else return false;
}

void ScriptMFD::SetFontSize (double size)
{
	if (hFont) oapiReleaseFont (hFont);
	int h = (int)(H*size*9.0/200.0);
	hFont =  oapiCreateFont(-h, true, "Arial", FONT_NORMAL);
	fw = fh = 0;
}

bool ScriptMFD::ScriptInput (void *id, const char *str, void *data)
{
	return ((ScriptMFD*)data)->Input (str);
}

bool ScriptMFD::Input (const char *line)
{
	InterpreterList::Environment *env = vi->env[pg];
	env->interp->AddLine (line, 0xFFFFFF);
	if (env->cmd[0]) return false;
	strncpy (env->cmd, line, 1024);
	return true;
}

void ScriptMFD::QueryCommand ()
{
	InterpreterList::Environment *env = vi->env[pg];
	if (!env->interp->IsBusy())
		oapiOpenInputBox ("Input script command:", ScriptInput, 0, 40, (void*)this);
}

void ScriptMFD::CreateInterpreter ()
{
	g_IList->AddInterpreter (hVessel);
	pg = vi->nenv - 1;
	InvalidateDisplay();
}

void ScriptMFD::DeleteInterpreter ()
{
	g_IList->DeleteInterpreter (hVessel, pg);
	pg = std::min (pg, vi->nenv-1);
	InvalidateDisplay();
}

void ScriptMFD::SetPage (int newpg)
{
	int npg = vi->nenv;
	if (newpg == (int)-1) newpg = npg-1;
	else if  (newpg >= npg) newpg = 0;
	if (newpg != pg) {
		pg = newpg;
		InvalidateDisplay();
	}
}

// Repaint the MFD
bool ScriptMFD::Update (oapi::Sketchpad *skp)
{
	int npg = vi->nenv;
	pg = std::min (pg, npg-1);
	InterpreterList::Environment *env = vi->env[pg];
	int yofs = (5*ch)/4;
	char cbuf[256];
	sprintf (cbuf, "Term %d/%d", pg+1, npg);
	Title (skp, cbuf);
	if (env->interp->IsBusy())
		skp->Text (W-cw*5, 1, "busy", 4);

	//SelectDefaultPen (hDC, 0);
	skp->MoveTo (0, yofs); skp->LineTo (W, yofs);

	/*HGDIOBJ oFont = SelectObject (hDC, hFont);
	if (!fh) {
		TEXTMETRIC tm;
		GetTextMetrics (hDC, &tm);
		fw = tm.tmAveCharWidth;
		fh = tm.tmHeight-tm.tmInternalLeading;
		nchar = (W-fw/2)/fw;
		nline = (H-yofs-fh/2)/fh;
	}*/
	int nbuf = env->interp->LineCount();
	MFDInterpreter::LineSpec *ls = env->interp->FirstLine();
	int xofs = fw/2;
	uint32_t col = 0;
	for (; ls && nbuf > nline; ls = ls->next) { // skip lines scrolled out of sight
		nbuf--;
	}
	for (; ls; ls = ls->next) {
		if (ls->col != col) {
			col = ls->col;
			//SetTextColor (hDC, col);
		}
		skp->Text (xofs, yofs, ls->buf, std::min((int)strlen(ls->buf),nchar));
		yofs += fh;
	}
	//SelectObject (hDC, oFont);
	return true;
}

// MFD message parser
OAPI_MSGTYPE ScriptMFD::MsgProc (MFD_msg msg, MfdId mfd,  MFDMODEOPENSPEC *param, VESSEL *vessel)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENEDEX:
		// Our new MFD mode has been selected, so we create the MFD and
		// return a pointer to it.
		return (OAPI_MSGTYPE) new ScriptMFD(param->w, param->h, vessel);
	default: break;
	}
	return 0;
}

// ==============================================================
// API interface

DLLCLBK void InitModule (oapi::DynamicModule *hDLL)
{
	static const char *name = "Terminal MFD";
	MFDMODESPECEX spec;
	spec.name = name;
	spec.key = OAPI_KEY_T;
	spec.context = NULL;
	spec.msgproc = ScriptMFD::MsgProc;
	g_MFDmode = oapiRegisterMFDMode (spec);
	g_IList = new InterpreterList;
}

DLLCLBK void ExitModule (void *hDLL)
{
	oapiUnregisterMFDMode (g_MFDmode);
	delete g_IList;
}

DLLCLBK void opcPostStep (double simt, double simdt, double mjd)
{
	if (g_IList) {
		g_IList->Update (simt, simdt, mjd);
	}
}
/*
DLLCLBK void opcOpenRenderViewport (HWND hWnd, int w, int h, bool bFullscreen)
{
}
*/
DLLCLBK void opcCloseRenderViewport ()
{
	g_IList->DeleteList();
}
