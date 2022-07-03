// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// =======================================================================
// CSphereMgr.cpp
// Rendering of the celestial sphere background at variable
// resolutions.
// =======================================================================

#include "CSphereMgr.h"
#include "Scene.h"
#include "OGLCamera.h"
#include "Texture.h"
#include <cstring>

using namespace oapi;

// =======================================================================
// Externals

extern int patchidx[9];
extern VBMESH *PATCH_TPL[14];
extern char DBG_MSG[256];

void ApplyPatchTextureCoordinates (VBMESH &mesh, VertexBuffer *vtx, const TEXCRDRANGE &range);

static int ntot, nrad, nrender;

uint32_t CSphereManager::vpX0, CSphereManager::vpX1, CSphereManager::vpY0, CSphereManager::vpY1;
double CSphereManager::diagscale;
int *CSphereManager::patchidx = 0;
int **CSphereManager::NLNG = 0;
int *CSphereManager::NLAT = 0;
VBMESH **CSphereManager::PATCH_TPL = 0;
//const D3D7Config *CSphereManager::cfg = NULL;

// =======================================================================
// =======================================================================
// Class CSphereManager

CSphereManager::CSphereManager (const Scene *scene)
{
//	cfg = gc->Cfg();
	scn = scene;
	patchidx = TileManager::patchidx;
	PATCH_TPL = TileManager::PATCH_TPL;
	NLNG = TileManager::NLNG;
	NLAT = TileManager::NLAT;

	char *c = (char*)g_client->GetConfigParam (CFGPRM_CSPHERETEXTURE);
	if (!c[0]) {
		disabled = true;
		return;
	} else {
		strncpy (texname, c, 64);
		disabled = false;
	}

	double tmp;
	tmp = *(double*)g_client->GetConfigParam (CFGPRM_CSPHEREINTENS);
	intensity = (float)tmp;

	maxlvl = 8; // g_pOrbiter->Cfg()->CSphereMaxLevel;
	maxbaselvl = std::min ((uint32_t)8, maxlvl);
	int maxidx = patchidx[maxbaselvl];
	bPreloadTile = false;//(cfg->PlanetPreloadMode != 0);
	nhitex = nhispec = 0;

	tiledesc = new TILEDESC[maxidx];
	memset (tiledesc, 0, maxidx*sizeof(TILEDESC));

	LoadPatchData ();
	LoadTileData ();
	LoadTextures ();

	MATRIX3 R = {2000,0,0, 0,2000,0, 0,0,2000};

	// rotation from galactic to ecliptic frame
	double theta = 60.18*RAD;
	double phi = 90.02*RAD;
	double lambda = 173.6*RAD;
	double sint = sin(theta), cost = cos(theta);
	double sinp = sin(phi), cosp = cos(phi);
	double sinl = sin(lambda), cosl = cos(lambda);
	ecl2gal = _M(cosp,0,sinp, 0,1,0, -sinp,0,cosp);
	ecl2gal = mul (_M(1,0,0, 0,cost,sint, 0,-sint,cost), ecl2gal);
	ecl2gal = mul (_M(cosl,0,sinl, 0,1,0, -sinl,0,cosl), ecl2gal);
	R = mul (ecl2gal, R);

	trans = glm::mat4(1.0);
	trans[0][0] = (float)R.m11;
	trans[0][1] = (float)R.m12;
	trans[0][2] = (float)R.m13;
	trans[1][0] = (float)R.m21;
	trans[1][1] = (float)R.m22;
	trans[1][2] = (float)R.m23;
	trans[2][0] = (float)R.m31;
	trans[2][1] = (float)R.m32;
	trans[2][2] = (float)R.m33;
}

// =======================================================================

CSphereManager::~CSphereManager ()
{
	if (disabled) return;

	int i, maxidx = patchidx[maxbaselvl];

	if (ntex) {
		for (i = 0; i < ntex; i++)
			texbuf[i]->Release();
		delete []texbuf;
	}

	for (i = 0; i < maxidx; i++) {
		if (tiledesc[i].vtx) delete tiledesc[i].vtx;
	}
	delete []tiledesc;	
}

// =======================================================================

void CSphereManager::GlobalInit ()
{
	GLint vp[4];
	//returns four values: the x and y window coordinates of the viewport, followed by its width and height
	glGetIntegerv(GL_VIEWPORT, vp);

	vpX0 = vp[0], vpX1 = vpX0 + vp[2];
	vpY0 = vp[1], vpY1 = vpY0 + vp[3];
	// viewport size for clipping calculations

	diagscale = (double)vp[2]/(double)vp[3];
	diagscale = sqrt(1.0 + diagscale*diagscale);
}

// =======================================================================

void CSphereManager::CreateDeviceObjects ()
{
	GLint vp[4];
	//returns four values: the x and y window coordinates of the viewport, followed by its width and height
	glGetIntegerv(GL_VIEWPORT, vp);

	vpX0 = vp[0], vpX1 = vpX0 + vp[2];
	vpY0 = vp[1], vpY1 = vpY0 + vp[3];
	// viewport size for clipping calculations

	diagscale = (double)vp[2]/(double)vp[3];
	diagscale = sqrt(1.0 + diagscale*diagscale);
}

// =======================================================================

void CSphereManager::DestroyDeviceObjects ()
{
}

// =======================================================================

bool CSphereManager::LoadPatchData ()
{
	// OBSOLETE

	int i;
	for (i = 0; i < patchidx[maxbaselvl]; i++)
		tiledesc[i].flag = 1;
	return false;
}

// =======================================================================

bool CSphereManager::LoadTileData ()
{
	if (maxlvl <= 8) // no tile data required
		return false;

	// TODO
	return true;
}

// =======================================================================

void CSphereManager::LoadTextures ()
{
	int i;

	// pre-load level 1-8 textures
	char fname[256];
	strcpy (fname, texname);
	strcat (fname, ".tex");
	ntex = patchidx[maxbaselvl];
	texbuf = new OGLTexture *[ntex];
	if (ntex = g_client->GetTexMgr()->LoadTextures (fname, texbuf, 0, ntex)) {
		while ((int)ntex < patchidx[maxbaselvl]) maxlvl = --maxbaselvl;
		while ((int)ntex > patchidx[maxbaselvl]) texbuf[--ntex]->Release();
		// not enough textures loaded for requested resolution level
		for (i = 0; i < patchidx[maxbaselvl]; i++)
			tiledesc[i].tex.obj = texbuf[i];
	} else {
		delete []texbuf;
		texbuf = 0;
		// no textures at all!
	}

	//  pre-load highres tile textures
	if (bPreloadTile && nhitex) {
		//TILEDESC *tile8 = tiledesc + patchidx[7];
		//PreloadTileTextures (tile8, nhitex, nhispec);
	}
}

// =======================================================================

void CSphereManager::Render (int level, int bglvl)
{
	if (disabled) return;
	ntot = nrad = nrender = 0;

	float intens = intensity;
	if (bglvl) {
		intens *= (float)exp(-bglvl*0.05);
	}

	if (!intens) return; // sanity check

	level = std::min (level, (int)maxlvl);

	RenderParam.tgtlvl = level;
	RenderParam.wmat = trans;

	RenderParam.viewap = atan(diagscale * scn->GetCamera()->GetTanAp());

	int startlvl = std::min (level, 8);
	int hemisp, ilat, ilng, idx;
	int  nlat = NLAT[startlvl];
	int *nlng = NLNG[startlvl];
	int texofs = patchidx[startlvl-1];
	TILEDESC *td = tiledesc + texofs;
	TEXCRDRANGE range = {0,1,0,1};
	tilebuf = TileManager::tilebuf;

	glm::dmat3 m = *scn->GetCamera()->GetGRot();
	MATRIX3 rcam;

	rcam.m11 = m[0][0];
	rcam.m12 = m[0][1];
	rcam.m13 = m[0][2];
	rcam.m21 = m[1][0];
	rcam.m22 = m[1][1];
	rcam.m23 = m[1][2];
	rcam.m31 = m[2][0];
	rcam.m32 = m[2][1];
	rcam.m33 = m[2][2];

	rcam = mul (ecl2gal, rcam);
	RenderParam.camdir = _V(rcam.m13, rcam.m23, rcam.m33);
/*
	dev->SetRenderState (D3DRENDERSTATE_LIGHTING, FALSE);
	dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	dev->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	if (intens < 1.0f) {
		dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(0,0,0,intens));
		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	}
*/
	tilebuf->hQueueMutex.lock();
	for (hemisp = idx = 0; hemisp < 2; hemisp++) {
		if (hemisp) { // flip world transformation to southern hemisphere
			RenderParam.wmat = RenderParam.wmat * TileManager::Rsouth;
		}
		for (ilat = nlat-1; ilat >= 0; ilat--) {
			for (ilng = 0; ilng < nlng[ilat]; ilng++) {
				ProcessTile (startlvl, hemisp, ilat, nlat, ilng, nlng[ilat], td+idx, 
					range, td[idx].tex.obj, td[idx].ltex.obj, td[idx].flag,
					range, td[idx].tex.obj, td[idx].ltex.obj, td[idx].flag);
				idx++;
			}
		}
	}
	tilebuf->hQueueMutex.unlock();
/*
	dev->SetRenderState (D3DRENDERSTATE_LIGHTING, TRUE);
	dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	dev->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	if (intens < 1.0f) {
		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
*/
	//sprintf (DBG_MSG, "total: %d, after radius culling: %d, after in-view culling: %d", ntot, nrad, nrender);
}

// =======================================================================

void CSphereManager::ProcessTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, TILEDESC *tile,
	const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, uint32_t flag,
	const TEXCRDRANGE &bkp_range, OGLTexture *bkp_tex, OGLTexture *bkp_ltex, uint32_t bkp_flag)
{
	ntot++;

	static const double rad0 = sqrt(2.0)*PI05;
	VECTOR3 cnt = TileCentre (hemisp, ilat, nlat, ilng, nlng);
	double rad = rad0/(double)nlat;
	double alpha = acos (dotp (RenderParam.camdir, cnt));
	double adist = alpha - rad;
	if (adist > RenderParam.viewap) return;

	nrad++;

	auto wm = GetWorldMatrix (ilng, nlng, ilat, nlat);

	// Check if patch bounding box intersects viewport
	if (!TileInView (lvl, ilat, wm)) {
		//tilebuf->DeleteSubTiles (tile); // remove tile descriptions below
		return;
	}

	RenderTile (lvl, hemisp, ilat, nlat, ilng, nlng, tile, range, tex, ltex, flag);
	nrender++;
}

// =======================================================================

glm::mat4 CSphereManager::GetWorldMatrix (int ilng, int nlng, int ilat, int nlat)
{
	// set up world transformation matrix
	glm::mat4 rtile = glm::fmat4(0);
	glm::mat4 wtrans;

	double lng = PI2 * (double)ilng/(double)nlng + PI; // add pi so texture wraps at +-180�

	double sinr = sin(lng), cosr = cos(lng);
	rtile[0][0] = rtile[2][2] = (float)cosr;
	rtile[2][0] = -(rtile[0][2] = (float)sinr);
	rtile[1][1] = rtile[3][3] = 1.0f;

	wtrans = RenderParam.wmat * rtile;

	return wtrans;
}

// =======================================================================

void CSphereManager::RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng,
	TILEDESC *tile, const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, uint32_t flag)
{
	VertexBuffer *vb;        // processed vertex buffer
	VBMESH &mesh = PATCH_TPL[lvl][ilat]; // patch template

	if (range.tumin == 0 && range.tumax == 1) {
		vb = mesh.vb; // use vertex buffer directly
	} else {
		if (!tile->vtx) {
			tile->vtx = new VertexBuffer(nullptr, mesh.nv*sizeof(N2TVERTEX));

			ApplyPatchTextureCoordinates (mesh, tile->vtx, range);
		}
		vb = tile->vtx; // use buffer with transformed texture coords
	}

	glBindTexture(GL_TEXTURE_2D, tex->m_TexId);
	mesh.vb->Bind();
	mesh.ib->Bind();
	glDrawElements(GL_TRIANGLES, mesh.ib->GetCount(), GL_UNSIGNED_SHORT, 0);
	mesh.ib->UnBind();
	mesh.vb->UnBind();
}

// =======================================================================
// returns the direction of the tile centre from the planet centre in local
// planet coordinates

VECTOR3 CSphereManager::TileCentre (int hemisp, int ilat, int nlat, int ilng, int nlng)
{
	double cntlat = PI05 * ((double)ilat+0.5)/(double)nlat,      slat = sin(cntlat), clat = cos(cntlat);
	double cntlng = PI2  * ((double)ilng+0.5)/(double)nlng + PI, slng = sin(cntlng), clng = cos(cntlng);
	if (hemisp) return _V(clat*clng, -slat, -clat*slng);
	else        return _V(clat*clng,  slat,  clat*slng);
}

// =======================================================================

void CSphereManager::TileExtents (int hemisp, int ilat, int nlat, int ilng, int nlng, double &lat1, double &lat2, double &lng1, double &lng2)
{
	lat1 = PI05 * (double)ilat/(double)nlat;
	lat2 = lat1 + PI05/(double)nlat;
	lng1 = PI2 * (double)ilng/(double)nlng + PI;
	lng2 = lng1 + PI2/nlng;
	if (hemisp) {
		double tmp = lat1; lat1 = -lat2; lat2 = -tmp;
		tmp = lng1; lng1 = -lng2; lng2 = -tmp;
		if (lng2 < 0) lng1 += PI2, lng2 += PI2;
	}
}

// =======================================================================

bool CSphereManager::TileInView (int lvl, int ilat, glm::mat4 &wm)
{
	const double eps = 1e-3;
	bool bx1, bx2, by1, by2, bz1, bz2, bbvis;
	int v;
	float x, y;
	VBMESH &mesh = PATCH_TPL[lvl][ilat];
	bx1 = bx2 = by1 = by2 = bz1 = bz2 = bbvis = false;
	for (v = 0; v < 8; v++) {
		const VECTOR4 &bb = mesh.bbvtx[v];
		glm::vec4 vec;
		vec.x = bb.x;
		vec.y = bb.y;
		vec.z = bb.z;
		vec.w = bb.w;
		vec = vec * wm;

		if (vec.z > 0.0)  bz1 = true;
		if (vec.z <= 1.0+eps) bz2 = true;
		if (vec.z <= 1.0) x = vec.x, y = vec.y;
		else                 x = -vec.x, y = -vec.y;
		if (x > vpX0)        bx1 = true;
		if (x < vpX1)        bx2 = true;
		if (y > vpY0)        by1 = true;
		if (y < vpY1)        by2 = true;
		if (bbvis = bx1 && bx2 && by1 && by2 && bz1 && bz2) break;
	}
	return bbvis;
}

