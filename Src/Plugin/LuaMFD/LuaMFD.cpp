// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define STRICT 1
#define ORBITER_MODULE
#include "orbitersdk.h"
#include "LuaMFD.h"

using std::min;

// ==============================================================
// Global variables

int g_MFDmode;
InterpreterList *g_IList = NULL;

// ==============================================================
// MFD class implementation

// Constructor
ScriptMFD::ScriptMFD (DWORD w, DWORD h, VESSEL *vessel)
: MFD2 (w, h, vessel)
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
{
	if(hFont) {
		oapiReleaseFont(hFont);
	}
}

// Return button labels
char *ScriptMFD::ButtonLabel (int bt)
{
	// The labels for the two buttons used by our MFD mode
	static const char *label[5] = {"INP", "NEW", "DEL", "PG>", "<PG"};
	return (char*)(bt < 5 ? label[bt] : 0);
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

bool ScriptMFD::ConsumeKeyBuffered (DWORD key)
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
	if (!(event & PANEL_MOUSE_LBDOWN)) return false;
	static const DWORD btkey[5] = { OAPI_KEY_I, OAPI_KEY_N, OAPI_KEY_D, OAPI_KEY_PERIOD, OAPI_KEY_COMMA };
	if (bt < 5) return ConsumeKeyBuffered (btkey[bt]);
	else return false;
}

void ScriptMFD::SetFontSize (double size)
{
	if (hFont) oapiReleaseFont (hFont);
	int h = (int)(H*size*9.0/200.0);
	hFont = oapiCreateFont (-h, true, "Courier New");
	fw = fh = 0;
}

bool ScriptMFD::ScriptInput (void *id, char *str, void *data)
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
		oapiOpenInputBox ((char*)"Input script command:", ScriptInput, 0, 40, (void*)this);
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
	pg = min (pg, vi->nenv-1);
	InvalidateDisplay();
}

void ScriptMFD::SetPage (DWORD newpg)
{
	DWORD npg = vi->nenv;
	if (newpg == (DWORD)-1) newpg = npg-1;
	else if  (newpg >= npg) newpg = 0;
	if (newpg != pg) {
		pg = newpg;
		InvalidateDisplay();
	}
}

// Repaint the MFD
bool ScriptMFD::Update (oapi::Sketchpad *skp)
{
	DWORD npg = vi->nenv;
	pg = min (pg, npg-1);
	InterpreterList::Environment *env = vi->env[pg];
	int yofs = (5*ch)/4;
	char cbuf[256];
	sprintf (cbuf, "Term %d/%d", pg+1, npg);
	Title (skp, cbuf);
	if (env->interp->IsBusy())
		skp->Text (W-cw*5, 1, "busy", 4);

	oapi::Pen *pen = GetDefaultPen(0);
	skp->SetPen(pen);
	skp->MoveTo (0, yofs); skp->LineTo (W, yofs);

	oapi::Font *oldFont = skp->SetFont(hFont);
	if (!fh) {
		DWORD cs = skp->GetCharSize();
		fw = (cs >> 16) & 0xffff;
		fh = cs & 0xffff;
		nchar = (W-fw/2)/fw;
		nline = (H-yofs-fh/2)/fh;
	}
	DWORD nbuf = env->interp->LineCount();
	MFDInterpreter::LineSpec *ls = env->interp->FirstLine();
	int xofs = fw/2;
	COLORREF col = 0;
	for (; ls && nbuf > nline; ls = ls->next) { // skip lines scrolled out of sight
		nbuf--;
	}
	for (; ls; ls = ls->next) {
		if (ls->col != col) {
			col = ls->col;
			skp->SetTextColor (col);
		}
		skp->Text (xofs, yofs, ls->buf, min(strlen(ls->buf),(size_t)nchar));
		yofs += fh;
	}
	skp->SetFont(oldFont);
	return true;
}

// MFD message parser
OAPI_MSGTYPE ScriptMFD::MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		// Our new MFD mode has been selected, so we create the MFD and
		// return a pointer to it.
		return (OAPI_MSGTYPE) new ScriptMFD(LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam);
	}
	return 0;
}

// ==============================================================
// API interface

DLLCLBK void InitModule (HINSTANCE hDLL)
{
	MFDMODESPECEX spec;
	spec.name = (char*)"Terminal MFD";
	spec.key = OAPI_KEY_T;
	spec.context = NULL;
	spec.msgproc = ScriptMFD::MsgProc;
	g_MFDmode = oapiRegisterMFDMode (spec);
	g_IList = new InterpreterList;
}

DLLCLBK void ExitModule (HINSTANCE hDLL)
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

DLLCLBK void opcOpenRenderViewport (HWND hWnd, DWORD w, DWORD h, BOOL bFullscreen)
{
}

DLLCLBK void opcCloseRenderViewport ()
{
	g_IList->DeleteList();
}
