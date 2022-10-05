// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define OAPI_IMPLEMENTATION

#include "Orbiter.h"
#include "Psys.h"
#include "Pane.h"
#include "VCockpit.h"
#include "GraphicsAPI.h"
#include "Log.h"
#include "Util.h"
#include "cryptstring.h"
#include <unistd.h>

extern Orbiter *g_pOrbiter;
extern PlanetarySystem *g_psys;
extern Pane *g_pane;

using namespace oapi;

// ======================================================================
// class GraphicsClient

GraphicsClient::GraphicsClient (MODULEHANDLE hInstance): Module (hInstance)
{
	VideoData.fullscreen = false;
	VideoData.forceenum = true;
	VideoData.trystencil = false;
	VideoData.novsync = true;
	VideoData.pageflip = true;
	VideoData.deviceidx = -1;
	VideoData.modeidx = 0;
	VideoData.winw = 1280;
	VideoData.winh = 800;
	surfBltTgt = RENDERTGT_NONE;
	splashFont = 0;
/*
    // Create WIC factory for formatted image output
    HRESULT hr = CoCreateInstance (
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pIWICFactory)
    );
	if (hr != S_OK)
		m_pIWICFactory = NULL;
		*/
}

// ======================================================================

GraphicsClient::~GraphicsClient ()
{
	if (splashFont) clbkReleaseFont (splashFont);
}

// ======================================================================

bool GraphicsClient::clbkInitialise ()
{
	// set default parameters from config data
	Config *cfg = g_pOrbiter->Cfg();
	VideoData.fullscreen = cfg->CfgDevPrm.bFullscreen;
	VideoData.forceenum  = cfg->CfgDevPrm.bForceEnum;
	VideoData.trystencil = cfg->CfgDevPrm.bTryStencil;
	VideoData.novsync    = cfg->CfgDevPrm.bNoVsync;
	VideoData.pageflip   = cfg->CfgDevPrm.bPageflip;
	VideoData.deviceidx  = cfg->CfgDevPrm.Device_idx;
	VideoData.modeidx    = (int)cfg->CfgDevPrm.Device_mode;
	VideoData.winw       = (int)cfg->CfgDevPrm.WinW;
	VideoData.winh       = (int)cfg->CfgDevPrm.WinH;

	return true;
}

// ======================================================================

void GraphicsClient::RegisterVisObject (OBJHANDLE hObj, VISHANDLE vis)
{
	((Body*)hObj)->RegisterVisual (vis);
}

void GraphicsClient::UnregisterVisObject (OBJHANDLE hObj)
{
	((Body*)hObj)->UnregisterVisual();
}

// ======================================================================

int GraphicsClient::clbkVisEvent (OBJHANDLE hObj, VISHANDLE vis, visevent msg, visevent_data context)
{
	return 0;
}

// ======================================================================

ParticleStream *GraphicsClient::clbkCreateParticleStream (PARTICLESTREAMSPEC *pss)
{
	return NULL;
}

// ======================================================================

ParticleStream *GraphicsClient::clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir)
{
	return NULL;
}

// ======================================================================

ParticleStream *GraphicsClient::clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir)
{
	return NULL;
}

// ======================================================================

ParticleStream *GraphicsClient::clbkCreateReentryStream (PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel)
{
	return NULL;
}

// ======================================================================

ScreenAnnotation *GraphicsClient::clbkCreateAnnotation ()
{
	TRACENEW; return new ScreenAnnotation (this);
}

// ======================================================================
bool GraphicsClient::TexturePath (const char *fname, char *path) const
{
	char tmp[256];
	strcpy(tmp, fname);

	for(int i=0;i<strlen(fname);i++) {
		if(tmp[i]=='\\') tmp[i]='/';
	}

	fname=tmp;

	// first try htex directory
	strcpy (path, g_pOrbiter->Cfg()->CfgDirPrm.HightexDir);
	strcat (path, fname);
	std::string hres = oapiGetFilePath(path);
	if (access( hres.c_str(), F_OK ) != -1) {
		strcpy(path, hres.c_str());
		return true;
	}

	// try tex directory
	strcpy (path, g_pOrbiter->Cfg()->CfgDirPrm.TextureDir);
	strcat (path, fname);
	std::string lres = oapiGetFilePath(path);
	if (access( lres.c_str(), F_OK ) != -1) {
		strcpy(path, lres.c_str());
		return true;
	}
	return false;
}

// ======================================================================

bool GraphicsClient::PlanetTexturePath(const char* planetname, char* path) const
{
	g_pOrbiter->Cfg()->PTexPath(path, planetname);
	return true;
}

// ======================================================================

SURFHANDLE GraphicsClient::GetVCHUDSurface (const VCHUDSPEC **hudspec) const
{
	VirtualCockpit *vc;
	if (g_pane && g_pane->GetHUD() && (vc = g_pane->GetVC())) {
		*hudspec = vc->GetHUDParams ();
		return vc->GetHUDSurf();
	} else
		return NULL;
}

// ======================================================================

SURFHANDLE GraphicsClient::GetMFDSurface (MfdId mfdid) const
{
	return (g_pane ? g_pane->GetMFDSurface (mfdid) : NULL);
}

// ======================================================================

SURFHANDLE GraphicsClient::GetVCMFDSurface (MfdId mfdid, const VCMFDSPEC **mfdspec) const
{
	*mfdspec = g_pane->GetVCMFDParams (mfdid);
	if (*mfdspec && g_pane && g_pane->GetVC() && g_pane->MFD (mfdid)) {
		return g_pane->MFD(mfdid)->Surface();
	} else
		return NULL;
}

// ======================================================================

int GraphicsClient::GetBaseTileList (OBJHANDLE hBase, const SurftileSpec **tile) const
{
	return ((Base*)hBase)->GetTileList (tile);
}

// ======================================================================

void GraphicsClient::GetBaseStructures (OBJHANDLE hBase, MESHHANDLE **mesh_bs, int *nmesh_bs, MESHHANDLE **mesh_as, int *nmesh_as) const
{
	((Base*)hBase)->ExportBaseStructures ((Mesh***)mesh_bs, nmesh_bs, (Mesh***)mesh_as, nmesh_as);
}

// ======================================================================

void GraphicsClient::GetBaseShadowGeometry (OBJHANDLE hBase, MESHHANDLE **mesh_sh, double **elev, int *nmesh_sh) const
{
	((Base*)hBase)->ExportShadowGeometry ((Mesh***)mesh_sh, elev, nmesh_sh);
}

// ======================================================================

const void *GraphicsClient::GetConfigParam (int paramtype) const
{
	return g_pOrbiter->Cfg()->GetParam (paramtype);
}

// ======================================================================
/*
HWND GraphicsClient::clbkCreateRenderWindow ()
{
	HWND hWnd;

	if (VideoData.fullscreen) {
		hWnd = CreateWindow (strWndClass, "", // dummy window
			WS_POPUP | WS_EX_TOPMOST| WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, 0, 0, hModule, (LPVOID)this);
	} else {
		hWnd = CreateWindow (strWndClass, "",
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, VideoData.winw, VideoData.winh, 0, 0, hModule, (LPVOID)this);
	}
	return hWnd;
}
*/
// ======================================================================

void GraphicsClient::clbkDestroyRenderWindow (bool fastclose)
{
	if (splashFont) {
		clbkReleaseFont (splashFont);
		splashFont = 0;
	}
}

// ======================================================================

void GraphicsClient::Render2DOverlay ()
{
	g_pane->Render();
}

// ======================================================================

bool GraphicsClient::ElevationGrid (ELEVHANDLE hElev, int ilat, int ilng, int lvl,
	int pilat, int pilng, int plvl, int16_t *pelev, int16_t *elev, double *emean) const
{
	if (!hElev) return false;
	ElevationManager *emgr = (ElevationManager*)hElev;
	emgr->ElevationGrid (ilat, ilng, lvl, pilat, pilng, plvl, pelev, elev, emean);
	return true;
}

// ======================================================================

void GraphicsClient::ShowDefaultSplash ()
{

}

// ======================================================================

void GraphicsClient::clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, bool additive)
{
	// can we do any default device-independent processing here?
}

// ======================================================================

void GraphicsClient::clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive)
{
	// if not implemented by the client, just use default (opaque) rendering
	clbkRender2DPanel (hSurf, hMesh, T, additive);
}

// ======================================================================

int GraphicsClient::clbkBeginBltGroup (SURFHANDLE tgt)
{
	if (tgt == RENDERTGT_NONE)
		return -3;
	if (surfBltTgt != RENDERTGT_NONE && surfBltTgt != tgt)
		return -2;
	surfBltTgt = tgt;
	return 0;
}

// ======================================================================

int GraphicsClient::clbkEndBltGroup ()
{
	if (surfBltTgt == RENDERTGT_NONE)
		return -2;
	surfBltTgt = RENDERTGT_NONE;
	return 0;
}

// ======================================================================
/*
extern TCHAR *g_strAppTitle;

HWND GraphicsClient::InitRenderWnd (HWND hWnd)
{
	if (!hWnd) { // create a dummy window
		hWnd = CreateWindow (strWndClass, "",
			WS_POPUP | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, 0, 0, hModule, 0);
	}
	SetWindowLongPtr (hWnd, GWLP_USERDATA, (LONG_PTR)this);
	// store class instance with window for access in the message handler

	char title[256], cbuf[128];
	strcpy (title, g_strAppTitle);
	GetWindowText (hWnd, cbuf, 128);
	if (cbuf[0]) {
		strcat (title, " ");
		strcat (title, cbuf);
	}
	SetWindowText (hWnd, title);
	hRenderWnd = hWnd;
	return hRenderWnd;
}

// ======================================================================


LRESULT GraphicsClient::RenderWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	// graphics-specific stuff to go here
	default:
		return g_pOrbiter->MsgProc (hWnd, uMsg, wParam, lParam);
	}
    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}
*/
// ==================================================================
// Functions for the celestial sphere

int GraphicsClient::LoadStars (int n, StarRec *rec)
{
	// read binary data from file
	FILE *f = fopen ("Star.bin", "rb");
	if (!f) return 0; // error reading data base
	n = fread (rec, sizeof(StarRec), n, f);
	fclose (f);
	return n;
}

// ==================================================================

int GraphicsClient::LoadConstellationLines (int n, ConstRec *rec)
{
	FILE *f = fopen ("Constell.bin", "rb");
	if (!f) return 0;
	n = fread (rec, sizeof(ConstRec), n, f);
	fclose (f);
	return n;
}

// ==================================================================

int GraphicsClient::GetCelestialMarkers (const LABELLIST **cm_list) const
{
	int nlist;
	*cm_list = g_psys->LabelList (&nlist);
	return nlist;
}

// ==================================================================

int GraphicsClient::GetSurfaceMarkers (OBJHANDLE hObj, const LABELLIST **sm_list) const
{
	int nlist;
	*sm_list = ((Planet*)hObj)->LabelList (&nlist);
	return nlist;
}

// ==================================================================

int GraphicsClient::GetSurfaceMarkerLegend (OBJHANDLE hObj, const LABELTYPE **lspec) const
{
	Planet *planet = (Planet*)hObj;
	*lspec = planet->LabelLegend();
	return planet->NumLabelLegend();
}

// ======================================================================
// ======================================================================
// class ParticleStream

ParticleStream::ParticleStream (GraphicsClient *_gc, PARTICLESTREAMSPEC *pss)
{
	gc = _gc;
	level = NULL;
	hRef = NULL;
	lpos = _V(0,0,0);  pos = &lpos;
	ldir = _V(0,0,0);  dir = &ldir;
}

ParticleStream::~ParticleStream ()
{
	if (hRef) // stream is being deleted while still attached
		((Vessel*)hRef)->DelParticleStream (this);
		// notify vessel
}

void ParticleStream::Attach (OBJHANDLE hObj, const VECTOR3 *ppos, const VECTOR3 *pdir, const double *srclvl)
{
	hRef = hObj;
	SetVariablePos (ppos);
	SetVariableDir (pdir);
	SetLevelPtr (srclvl);
}

void ParticleStream::Attach (OBJHANDLE hObj, const VECTOR3 &_pos, const VECTOR3 &_dir, const double *srclvl)
{
	hRef = hObj;
	SetFixedPos (_pos);
	SetFixedDir (_dir);
	SetLevelPtr (srclvl);
}

void ParticleStream::Detach ()
{
	level = NULL;
	hRef = NULL;
	pos = &lpos;
	dir = &ldir;
}

void ParticleStream::SetFixedPos (const VECTOR3 &_pos)
{
	lpos = _pos;  pos = &lpos;
}

void ParticleStream::SetFixedDir (const VECTOR3 &_dir)
{
	ldir = _dir;  dir = &ldir;
}

void ParticleStream::SetVariablePos (const VECTOR3 *ppos)
{
	pos = ppos;
}

void ParticleStream::SetVariableDir (const VECTOR3 *pdir)
{
	dir = pdir;
}

void ParticleStream::SetLevelPtr (const double *srclvl)
{
	level = srclvl;
}


// ======================================================================
// ======================================================================
// class ScreenAnnotation

ScreenAnnotation::ScreenAnnotation (GraphicsClient *_gc)
{
	gc = _gc;
	_gc->clbkGetViewportSize (&viewW, &viewH);
	txt = 0;
	buflen = 0;
	txtscale = 0;
	font = NULL;
	//hFont = NULL;
	Reset();
}

ScreenAnnotation::~ScreenAnnotation ()
{
	if (txt) delete[]txt;
	if (font) gc->clbkReleaseFont (font);
	//DeleteObject (hFont);
}

void ScreenAnnotation::Reset ()
{
	ClearText();
	SetPosition (0.1, 0.1, 0.5, 0.6);
	SetColour (_V(1.0,0.7,0.2));
	SetSize (1.0);
}

void ScreenAnnotation::SetPosition (double x1, double y1, double x2, double y2)
{
	nx1 = (int)(x1*viewW); nx2 = (int)(x2*viewW); nw = nx2-nx1;
	ny1 = (int)(y1*viewH); ny2 = (int)(y2*viewH); nh = ny2-ny1;
}

void ScreenAnnotation::SetText (const char *str)
{
	int len = strlen (str)+1;
	if (len > buflen) {
		if (txt) delete []txt;
		txt = new char[len]; TRACENEW
		buflen = len;
	}
	strcpy (txt, str);
	txtlen = len-1;
	for (int i = 0; i < txtlen-1; i++)
		if (txt[i] == ' ' && txt[i+1] == ' ') {
			txt[i] = '\r';
			txt[i+1] = '\n';
		}
}

void ScreenAnnotation::ClearText ()
{
	txtlen = 0;
}

void ScreenAnnotation::SetSize (double scale)
{
	if (scale != txtscale) {
		txtscale = scale;
		hf = (int)(viewH*txtscale/35.0);
		if (font) gc->clbkReleaseFont (font);
		font = gc->clbkCreateFont (-hf, true, "Sans");
		//if (hFont) DeleteObject (hFont);
		//hFont = CreateFont (-hf, 0, 0, 0, 400, 0, 0, 0, 0, 3, 2, 1, 49, "Arial");
	}
}

void ScreenAnnotation::SetColour (const VECTOR3 &col)
{
	int r = std::min (255, (int)(col.x*256.0));
	int g = std::min (255, (int)(col.y*256.0));
	int b = std::min (255, (int)(col.z*256.0));
	txtcol  = r | (g << 8) | (b << 16);
	txtcol2 = (r/2) | ((g/2) << 8) | ((b/2) << 16);
}

void ScreenAnnotation::Render ()
{
	if (!txtlen) return;

	Sketchpad *skp = gc->clbkGetSketchpad (0);
	if (skp) {
		skp->SetFont (font);
		skp->SetTextColor (txtcol2);
		skp->SetBackgroundMode (oapi::Sketchpad::BK_TRANSPARENT);
		skp->TextBox (nx1+1, ny1+1, nx2+1, ny2+1, txt, txtlen);
		skp->SetTextColor (txtcol);
		skp->TextBox (nx1, ny1, nx2, ny2, txt, txtlen);
		gc->clbkReleaseSketchpad (skp);
	}
}

// ======================================================================
// Nonmember functions

//-----------------------------------------------------------------------
// Name: WndProc()
// Desc: Static msg handler which passes messages from the render window
//       to the application class.
//-----------------------------------------------------------------------
/*
DLLEXPORT LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GraphicsClient *gc = (GraphicsClient*)GetWindowLongPtr (hWnd, GWLP_USERDATA);
	if (gc) return gc->RenderWndProc (hWnd, uMsg, wParam, lParam);
	else return DefWindowProc (hWnd, uMsg, wParam, lParam);
}*/

// ======================================================================
// API interface: register/unregister the graphics client

DLLEXPORT bool oapiRegisterGraphicsClient (GraphicsClient *gc)
{
	return g_pOrbiter->AttachGraphicsClient (gc);
}

DLLEXPORT bool oapiUnregisterGraphicsClient (GraphicsClient *gc)
{
	return g_pOrbiter->RemoveGraphicsClient (gc);
}

DLLEXPORT void oapiMakeContextCurrent (bool b)
{
	g_pOrbiter->MakeContextCurrent(b);
}
