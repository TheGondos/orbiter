// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OpenGL Client module
// ==============================================================

// ==============================================================
// TileMgr.cpp
// class TileManager (implementation)
//
// Planetary surface rendering management, including a simple
// LOD (level-of-detail) algorithm for surface patch resolution.
// ==============================================================

#include "TileMgr.h"
#include "VPlanet.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "OGLClient.h"
#include "Shader.h"
#include "OGLCamera.h"
#include "Scene.h"
#include <cstring>

using namespace oapi;

// Max supported patch resolution level
int SURF_MAX_PATCHLEVEL = 14;
const int NOTILE = -1; // "no tile" flag

struct IDXLIST {
	int idx, ofs;
};

// Some debugging parameters
int tmissing = 0;

// =======================================================================
// Local prototypes

void ApplyPatchTextureCoordinates (VBMESH &mesh, VertexBuffer *vtx, const TEXCRDRANGE &range);
int compare_idx (const void *el1, const void *el2);

// =======================================================================

TileManager::TileManager (const VPlanet *vplanet)
{
	vp = vplanet;
	obj = vp->GetObject();
	char name[256];
	oapiGetObjectName (obj, name, 256);
	objname = new char[strlen(name)+1];
	strcpy (objname, name);
	ntex = 0;
	nhitex = 0;
	nmask = 0;
	nhispec = 0;
	maxlvl = maxbaselvl = 0;
	microtex = 0;
	microlvl = 0.0;
	bPreloadTile = false;//g_client->Cfg()->PlanetPreloadMode;
}

// =======================================================================

TileManager::~TileManager ()
{
	int i, maxidx = patchidx[maxbaselvl];

	if (ntex) {
		for (i = 0; i < ntex; i++)
			texbuf[i]->Release();
		delete []texbuf;
	}
	if (nmask) {
		for (i = 0; i < nmask; i++)
			specbuf[i]->Release();
		delete []specbuf;
	}
	for (i = 0; i < maxidx; i++)
		if (tiledesc[i].vtx) delete tiledesc[i].vtx;
	delete []tiledesc;
	delete []objname;
}

// =======================================================================

bool TileManager::LoadPatchData ()
{
	// Read information about specular reflective patch masks
	// from a binary data file

	FILE *binf;
	uint8_t minres, maxres, flag;
	int i, idx, npatch;
	nmask = 0;
	char fname[256], cpath[256];
	strcpy (fname, objname);
	strcat (fname, "_lmask.bin");
	if (!(bGlobalSpecular || bGlobalLights) || !g_client->TexturePath (fname, cpath) || !(binf = fopen (cpath, "rb"))) {

		for (i = 0; i < patchidx[maxbaselvl]; i++)
			tiledesc[i].flag = 1;
		return false; // no specular reflections, no city lights

	} else {

		uint16_t *tflag = 0;
		LMASKFILEHEADER lmfh;
		size_t ret = fread (&lmfh, sizeof (lmfh), 1, binf);
		if(ret != sizeof (lmfh) ) {
			fclose (binf);
			printf("Error reading patch data\n");
			exit(EXIT_FAILURE);
		}
		if (!strncmp (lmfh.id, "PLTA0100", 8)) { // v.1.00 format
			minres = lmfh.minres;
			maxres = lmfh.maxres;
			npatch = lmfh.npatch;
			tflag = new uint16_t[npatch];
			ret = fread (tflag, sizeof(uint16_t), npatch, binf);
			if(ret != sizeof(uint16_t) * npatch) {
				fclose (binf);
				printf("Error reading patch data\n");
				exit(EXIT_FAILURE);
			}
		} else {                                 // pre-v.1.00 format
			fseek (binf, 0, SEEK_SET);
			ret = fread (&minres, 1, 1, binf);
			if(ret != 1) {
				fclose (binf);
				printf("Error reading patch data\n");
				exit(EXIT_FAILURE);
			}
			ret = fread (&maxres, 1, 1, binf);
			if(ret != 1) {
				fclose (binf);
				printf("Error reading patch data\n");
				exit(EXIT_FAILURE);
			}
			npatch = patchidx[maxres] - patchidx[minres-1];
			tflag = new uint16_t[npatch];
			for (i = 0; i < npatch; i++) {
				ret = fread (&flag, 1, 1, binf);
				if(ret != 1) {
					fclose (binf);
					printf("Error reading patch data\n");
					exit(EXIT_FAILURE);
				}
				tflag[i] = flag;
			}
			//LOGOUT1P("*** WARNING: Old-style texture contents file %s_lmask.bin", cbody->Name());
		}
		fclose (binf);

		for (i = idx = 0; i < patchidx[maxbaselvl]; i++) {
			if (i < patchidx[minres-1]) {
				tiledesc[i].flag = 1; // no mask information -> assume opaque, no lights
			} else {
				flag = (uint8_t)tflag[idx++];
				tiledesc[i].flag = flag;
				if (((flag & 3) == 3) || (flag & 4))
					nmask++;
			}
		}
		if (tflag) delete []tflag;
		return true;
	}
}

// =======================================================================

bool TileManager::LoadTileData ()
{
	// Read table of contents file for high-resolution planet tiles

	FILE *file;

	if (maxlvl <= 8) // no tile data required
		return false;

	char fname[256], cpath[256];
	strcpy (fname, objname);
	strcat (fname, "_tile.bin");
	
	if (!g_client->TexturePath (fname, cpath) || !(file = fopen (cpath, "rb")))
		return false; // TOC file not found

	// read file header
	char idstr[9] = "        ";
	size_t ret = fread (idstr, 1, 8, file);
	if(ret != 8) {
		fclose (file);
		printf("Error reading tile data\n");
		exit(EXIT_FAILURE);
	}
	if (!strncmp (idstr, "PLTS", 4)) {
		tilever = 1;
	} else { // no header: old-style file format
		tilever = 0;
		fseek (file, 0, SEEK_SET);
	}

	int i, j;
	int32_t n;
	ret = fread (&n, sizeof(int32_t), 1, file);
	if(ret != sizeof(int32_t)) {
		fclose (file);
		printf("Error reading tile data\n");
		exit(EXIT_FAILURE);
	}
	TILEFILESPEC *tfs = new TILEFILESPEC[n];
	ret = fread (tfs, sizeof(TILEFILESPEC), n, file);
	if(ret != sizeof(TILEFILESPEC) * n) {
		fclose (file);
		printf("Error reading tile data\n");
		exit(EXIT_FAILURE);
	}

	if (bPreloadTile) {
		if (tilever >= 1) { // convert texture offsets to indices
			IDXLIST *idxlist = new IDXLIST[n];
			for (i = 0; i < n; i++) {
				idxlist[i].idx = i;
				idxlist[i].ofs = tfs[i].sidx;
			}
			qsort (idxlist, n, sizeof(IDXLIST), compare_idx);
			for (i = 0; i < n && idxlist[i].ofs != NOTILE; i++)
				tfs[idxlist[i].idx].sidx = i;

			for (i = 0; i < n; i++) {
				idxlist[i].idx = i;
				idxlist[i].ofs = tfs[i].midx;
			}
			qsort (idxlist, n, sizeof(IDXLIST), compare_idx);
			for (i = 0; i < n && idxlist[i].ofs != NOTILE; i++)
				tfs[idxlist[i].idx].midx = i;

			tilever = 0;
			delete []idxlist;
		}
	}

	TILEDESC *tile8 = tiledesc + patchidx[7];
	for (i = 0; i < 364; i++) { // loop over level 8 tiles
		TILEDESC &tile8i = tile8[i];
		for (j = 0; j < 4; j++)
			if (tfs[i].subidx[j])
				AddSubtileData (tile8i, tfs, i, j, 9);
	}

	fclose (file);
	delete []tfs;
	return true;
}

// =======================================================================

int compare_idx (const void *el1, const void *el2)
{
	IDXLIST *idx1 = (IDXLIST*)el1;
	IDXLIST *idx2 = (IDXLIST*)el2;
	return (idx1->ofs < idx2->ofs ? -1 : idx1->ofs > idx2->ofs ? 1 : 0);
}

// =======================================================================

bool TileManager::AddSubtileData (TILEDESC &td, TILEFILESPEC *tfs, int idx, int sub, int lvl)
{
	int j, subidx = tfs[idx].subidx[sub];
	TILEFILESPEC &t = tfs[subidx];
	bool bSubtiles = false;
	for (j = 0; j < 4; j++)
		if (t.subidx[j]) { bSubtiles = true; break; }
	if (t.flags || bSubtiles) {
		if ((int)lvl <= maxlvl) {
			td.subtile[sub] = tilebuf->AddTile();
			td.subtile[sub]->flag = t.flags;
			td.subtile[sub]->tex.idx = t.sidx;
			if (bGlobalSpecular || bGlobalLights) {
				if (t.midx != (uint32_t)NOTILE) {
					td.subtile[sub]->ltex.idx = t.midx;
				}
			} else {
				td.subtile[sub]->flag = 1; // remove specular flag
			}
			td.subtile[sub]->flag |= 0x80; // 'Not-loaded' flag
			if (!tilever)
				td.subtile[sub]->flag |= 0x40; // 'old-style index' flag
			// recursively step down to higher resolutions
			if (bSubtiles) {
				for (j = 0; j < 4; j++) {
					if (t.subidx[j]) AddSubtileData (*td.subtile[sub], tfs, subidx, j, lvl+1);
				}
			}
			nhitex++;
			if (t.midx != (uint32_t)NOTILE) nhispec++;
		} else td.subtile[sub] = NULL;
	}
	return true;
}

// =======================================================================

void TileManager::LoadTextures (char *modstr)
{
	int i;

	// pre-load level 1-8 textures
	ntex = patchidx[maxbaselvl];
	texbuf = new OGLTexture*[ntex];
	char fname[256];
	strcpy (fname, objname);
	if (modstr) strcat (fname, modstr);
	strcat (fname, ".tex");
	if ((ntex = g_client->GetTexMgr()->LoadTextures (fname, texbuf, 0, ntex))) {
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
		TILEDESC *tile8 = tiledesc + patchidx[7];
		PreloadTileTextures (tile8, nhitex, nhispec);
	}
}

// =======================================================================

void TileManager::PreloadTileTextures (TILEDESC *tile8, int ntex, int nmask)
{
	// Load tile surface and mask/light textures, and copy them into the tile tree

	char fname[256];
	int i, j, nt = 0, nm = 0;
	OGLTexture **texbuf = NULL, **maskbuf = NULL;

	if (ntex) {  // load surface textures
		texbuf = new OGLTexture *[ntex];
		strcpy (fname, objname);
		strcat (fname, "_tile.tex");
		nt = g_client->GetTexMgr()->LoadTextures (fname, texbuf, 0, ntex);
	}
	if (nmask) { // load mask/light textures
		maskbuf = new OGLTexture *[nmask];
		strcpy (fname, objname);
		strcat (fname, "_tile_lmask.tex");
		nm = g_client->GetTexMgr()->LoadTextures (fname, maskbuf, 0, nmask);
	}
	// copy textures into tile tree
	for (i = 0; i < 364; i++) {
		TILEDESC *tile8i = tile8+i;
		for (j = 0; j < 4; j++)
			if (tile8i->subtile[j])
				AddSubtileTextures (tile8i->subtile[j], texbuf, nt, maskbuf, nm);
	}
	// release unused textures
	if (nt) {
//		for (i = 0; i < nt; i++)
//			if (texbuf[i])
//				texbuf[i]->Release();
		delete []texbuf;
	}
	if (nm) {
//		for (i = 0; i < nm; i++)
//			if (maskbuf[i])
//				maskbuf[i]->Release();
		delete []maskbuf;
	}
}

// =======================================================================

void TileManager::AddSubtileTextures (TILEDESC *td, OGLTexture **tbuf, int nt, OGLTexture **mbuf, int nm)
{
	int i;

	uint32_t tidx = td->tex.idx;  // copy surface texture
	if (tidx != (uint32_t)NOTILE) {
		if (tidx < (uint32_t)nt) {
			td->tex.obj = tbuf[tidx];
			tbuf[tidx] = nullptr;
		} else {                   // inconsistency
			tmissing++;
			td->tex.obj = nullptr;
		}
	} else td->tex.obj = nullptr;

	uint32_t midx = td->ltex.idx;  // copy mask/light texture
	if (midx != (uint32_t)NOTILE) {
		if (midx < (uint32_t)nm) {
			td->ltex.obj = mbuf[midx];
			mbuf[midx] = nullptr;
		} else {                  // inconsistency
			tmissing++;
			td->ltex.obj = nullptr;
		}
	} else td->ltex.obj = nullptr;
	td->flag &= ~0x80; // remove "not loaded" flag

	for (i = 0; i < 4; i++) {
		if (td->subtile[i]) AddSubtileTextures (td->subtile[i], tbuf, nt, mbuf, nm);
	}
}

// =======================================================================

void TileManager::LoadSpecularMasks ()
{
	int i;
	int n;
	char fname[256];

	if (nmask) {
		strcpy (fname, objname);
		strcat (fname, "_lmask.tex");
		specbuf = new OGLTexture *[nmask];
		if ((n = g_client->GetTexMgr()->LoadTextures (fname, specbuf, 0, nmask))) {
			if (n < nmask) {
				//LOGOUT1P("Transparency texture mask file too short: %s_lmask.tex", cbody->Name());
				//LOGOUT("Disabling specular reflection for this planet");
				delete []specbuf;
				specbuf = NULL;
				nmask = 0;
				for (i = 0; i < patchidx[maxbaselvl]; i++)
					tiledesc[i].flag = 1;
			} else {
				for (i = n = 0; i < patchidx[maxbaselvl]; i++) {
					if (((tiledesc[i].flag & 3) == 3) || (tiledesc[i].flag & 4)) {
						if (n < nmask) tiledesc[i].ltex.obj = specbuf[n++];
						else tiledesc[i].flag = 1;
					}
					if (!bGlobalLights) tiledesc[i].flag &= 0xFB;
					if (!bGlobalSpecular) tiledesc[i].flag &= 0xFD, tiledesc[i].flag |= 1;
				}
			}
		} else {
			//LOGOUT1P("Transparency texture mask file not found: %s_lmask.tex", cbody->Name());
			//LOGOUT("Disabling specular reflection for this planet");
			nmask = 0;
			for (i = 0; i < patchidx[maxbaselvl]; i++)
				tiledesc[i].flag = 1;
		}
	}
}

// ==============================================================

void TileManager::Render (glm::mat4 &wmat, double scale, int level, double viewap, bool bfog)
{
	VECTOR3 gpos;
	glm::mat4 imat;

	level = std::max(1, std::min (level, maxlvl));

	RenderParam.wmat = wmat;
	RenderParam.wmat_tmp = wmat;
	imat = glm::inverse(wmat);
	RenderParam.cdir = _V(imat[3][0], imat[3][1], imat[3][2]); // camera position in local coordinates (units of planet radii)
	RenderParam.cdir = _V(imat[0][3], imat[1][3], imat[2][3]); // camera position in local coordinates (units of planet radii)
	RenderParam.cpos = vp->PosFromCamera() * scale;
	normalise (RenderParam.cdir);                        // camera direction
	RenderParam.bfog = bfog;

	oapiGetRotationMatrix (obj, &RenderParam.grot);
	RenderParam.grot *= scale;
	oapiGetGlobalPos (obj, &gpos);

	RenderParam.objsize = oapiGetSize (obj);
	RenderParam.cdist = vp->CamDist() / vp->mSize; // camera distance in units of planet radius
	RenderParam.viewap = (viewap ? viewap : acos (1.0/std::max (1.0, RenderParam.cdist)));
	RenderParam.sdir = tmul (RenderParam.grot, -gpos);
	normalise (RenderParam.sdir); // sun direction in planet frame

	// limit resolution for fast camera movements
	double limitstep, cstep = acos (dotp (RenderParam.cdir, pcdir));
	int maxlevel = SURF_MAX_PATCHLEVEL;
	static double limitstep0 = 5.12 * pow(2.0, -(double)SURF_MAX_PATCHLEVEL);
	for (limitstep = limitstep0; cstep > limitstep && maxlevel > 5; limitstep *= 2.0)
		maxlevel--;
	level = std::min (level, maxlevel);

	RenderParam.tgtlvl = level;

	int startlvl = std::max(1, std::min (level, 8));
	int hemisp, ilat, ilng, idx;
	int  nlat = NLAT[startlvl];
	int *nlng = NLNG[startlvl];
	int texofs = patchidx[startlvl-1];
	TILEDESC *td = tiledesc + texofs;

	TEXCRDRANGE range = {0,1,0,1};

//	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	if (level <= 4) {

		RenderSimple (level, td);

	} else {
		tilebuf->hQueueMutex.lock(); // make sure we can write to texture request queue
		for (hemisp = idx = 0; hemisp < 2; hemisp++) {
			if (hemisp) { // flip world transformation to southern hemisphere
//				D3DMAT_MatrixMultiply (&RenderParam.wmat, &RenderParam.wmat, &Rsouth);
				RenderParam.wmat = RenderParam.wmat * Rsouth;
//				D3DMAT_Copy (&RenderParam.wmat_tmp, &RenderParam.wmat);
				RenderParam.wmat_tmp = RenderParam.wmat;
				RenderParam.grot.m12 = -RenderParam.grot.m12;
				RenderParam.grot.m13 = -RenderParam.grot.m13;
				RenderParam.grot.m22 = -RenderParam.grot.m22;
				RenderParam.grot.m23 = -RenderParam.grot.m23;
				RenderParam.grot.m32 = -RenderParam.grot.m32;
				RenderParam.grot.m33 = -RenderParam.grot.m33;
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
	}

	//dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	pcdir = RenderParam.cdir; // store camera direction

}

// =======================================================================

void TileManager::ProcessTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, TILEDESC *tile,
	const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, int flag,
	const TEXCRDRANGE &bkp_range, OGLTexture *bkp_tex, OGLTexture *bkp_ltex, int bkp_flag)
{
	// Check if patch is visible from camera position
	static const double rad0 = sqrt(2.0)*PI05*0.5;
	VECTOR3 cnt = TileCentre (hemisp, ilat, nlat, ilng, nlng);
	double rad = rad0/(double)nlat;
	double adist = acos (dotp (RenderParam.cdir, cnt)) - rad;
	
	if (adist >= RenderParam.viewap) {
		tilebuf->DeleteSubTiles (tile); // remove tile descriptions below
		return;
	}

	// Set world transformation matrix for patch
	SetWorldMatrix (ilng, nlng, ilat, nlat);

	// Check if patch bounding box intersects viewport
	if (!TileInView (lvl, ilat)) {
		tilebuf->DeleteSubTiles (tile); // remove tile descriptions below
		return;
	}

	// Reduce resolution for distant or oblique patches
	bool bStepDown = (lvl < RenderParam.tgtlvl);
	bool bCoarseTex = false;
	if (bStepDown && lvl >= 8 && adist > 0.0) {
		double lat1, lat2, lng1, lng2, clat, clng, crad;
		double adist_lng, adist_lat, adist2;
		TileExtents (hemisp, ilat, nlat, ilng, nlng, lat1, lat2, lng1, lng2);
		oapiLocalToEqu (obj, RenderParam.cdir, &clng, &clat, &crad);
		if      (clng < lng1-PI) clng += PI2;
		else if (clng > lng2+PI) clng -= PI2;
		if      (clng < lng1) adist_lng = lng1-clng;
		else if (clng > lng2) adist_lng = clng-lng2;
		else                  adist_lng = 0.0;
		if      (clat < lat1) adist_lat = lat1-clat;
		else if (clat > lat2) adist_lat = clat-lat2;
		else                  adist_lat = 0.0;
		adist2 = std::max (adist_lng, adist_lat);

		// reduce resolution further for tiles that are visible
		// under a very oblique angle
		double cosa = cos(adist2);
		double a = sin(adist2);
		double b = RenderParam.cdist-cosa;
		double ctilt = b*cosa/sqrt(a*a*(1.0+2.0*b)+b*b); // tile visibility tilt angle cosine
		if (adist2 > rad*(2.0*ctilt+0.3)) {
			bStepDown = false;
			if (adist2 > rad*(4.2*ctilt+0.3))
				bCoarseTex = true;
		}
	}

	// Recursion to next level: subdivide into 2x2 patch
	if (bStepDown) {
		int i, j, idx = 0;
		float du = (range.tumax-range.tumin) * 0.5f;
		float dv = (range.tvmax-range.tvmin) * 0.5f;
		TEXCRDRANGE subrange;
		static TEXCRDRANGE fullrange = {0,1,0,1};
		for (i = 1; i >= 0; i--) {
			subrange.tvmax = (subrange.tvmin = range.tvmin + (1-i)*dv) + dv;
			for (j = 0; j < 2; j++) {
				subrange.tumax = (subrange.tumin = range.tumin + j*du) + du;
				TILEDESC *subtile = tile->subtile[idx];
				bool isfull = true;
				if (!subtile) {
					tile->subtile[idx] = subtile = tilebuf->AddTile();
					isfull = false;
				} else if (subtile->flag & 0x80) { // not yet loaded
					if ((tile->flag & 0x80) == 0) // only load subtile texture if parent texture is present
						tilebuf->LoadTileAsync (objname, subtile);
					isfull = false;
				}
				if (isfull)
					isfull = (subtile->tex.obj != nullptr);
				if (isfull)
					ProcessTile (lvl+1, hemisp, ilat*2+i, nlat*2, ilng*2+j, nlng*2, subtile,
						fullrange, subtile->tex.obj, subtile->ltex.obj, subtile->flag,
						subrange, tex, ltex, flag);
				else
					ProcessTile (lvl+1, hemisp, ilat*2+i, nlat*2, ilng*2+j, nlng*2, subtile,
						subrange, tex, ltex, flag,
						subrange, tex, ltex, flag);
				idx++;
			}
		}
	} else {
		// actually render the tile at this level
		double sdist = acos (dotp (RenderParam.sdir, cnt));
		if (bCoarseTex) {
			if (sdist > PI05+rad && bkp_flag & 2) bkp_flag &= 0xFD, bkp_flag |= 1; // supress specular reflection on dark side
			RenderTile (lvl, hemisp, ilat, nlat, ilng, nlng, sdist, tile, bkp_range, bkp_tex, bkp_ltex, bkp_flag);
		} else {
			if (sdist > PI05+rad && flag & 2) flag &= 0xFD, flag |= 1; // supress specular reflection on dark side
			RenderTile (lvl, hemisp, ilat, nlat, ilng, nlng, sdist, tile, range, tex, ltex, flag);
		}
		tilebuf->DeleteSubTiles (tile); // remove tile descriptions below
	}
}

// ==============================================================

void TileManager::RenderSimple (int level, TILEDESC *tile)
{
	// render complete sphere (used at low LOD levels)

	//extern D3DMATERIAL7 def_mat;
	int idx, npatch = patchidx[level] - patchidx[level-1];
//	RenderParam.dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &RenderParam.wmat);

	static Shader s("Tile.vs","Tile.fs");

	s.Bind();
	OGLCamera *c = g_client->GetScene()->GetCamera();
	auto *vpm = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection",*vpm);
	s.SetMat4("u_Model",RenderParam.wmat);
    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();
	s.SetVec3("u_SunDir", sundir);

	if(vp->prm.bAddBkg) {
		VECTOR3 v3bgcol = g_client->GetScene()->SkyColour();
		glm::vec3 bgcol;
		bgcol.x = v3bgcol.x;
		bgcol.y = v3bgcol.y;
		bgcol.z = v3bgcol.z;
		s.SetVec3("u_bgcol", bgcol);
	} else {
		glm::vec3 bgcol(0,0,0);
		s.SetVec3("u_bgcol", bgcol);
	}

	if(vp->prm.bFog) {
		s.SetVec4("u_FogColor", vp->prm.mFogColor);
		s.SetFloat("u_FogDensity", vp->prm.mFogDensity);
	} else {
		s.SetFloat("u_FogDensity", 0);
	}

//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
/*PATCH_TPL_3.VAO->Bind();
glDrawElements(GL_TRIANGLES, PATCH_TPL_3.IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
PATCH_TPL_3.VAO->UnBind();
s.UnBind();*/
//return;
//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	for (idx = 0; idx < npatch; idx++) {
		VBMESH &mesh = PATCH_TPL[level][idx]; // patch template
		//bool purespec = ((tile[idx].flag & 3) == 2);
		//bool mixedspec = ((tile[idx].flag & 3) == 3);

		glBindTexture(GL_TEXTURE_2D, tile[idx].tex.obj->m_TexId);

		mesh.VAO->Bind();
        glDrawElements(GL_TRIANGLES, mesh.IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
		mesh.VAO->UnBind();
		glBindTexture(GL_TEXTURE_2D,  0);
//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

/*
		// step 1: render full patch, either completely diffuse or completely specular
		if (purespec) { // completely specular
			RenderParam.dev->GetMaterial (&pmat);
			RenderParam.dev->SetMaterial (&watermat);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, TRUE);
		}
		//RenderParam.dev->SetMaterial (&def_mat);
		RenderParam.dev->SetTexture (0, tile[idx].tex);
		RenderParam.dev->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, mesh.vb, 0,
			mesh.nv, mesh.idx, mesh.ni, 0);
		if (purespec) {
			RenderParam.dev->SetMaterial (&pmat);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
		}

		// step 2: add specular highlights (mixed patches only)
		if (mixedspec) {
			RenderParam.dev->GetMaterial (&pmat);
			RenderParam.dev->SetMaterial (&watermat);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, TRUE);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_INVSRCALPHA);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
			RenderParam.dev->SetTexture (0, tile[idx].ltex);
			RenderParam.dev->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, mesh.vb, 0,
				mesh.nv, mesh.idx, mesh.ni, 0);
			RenderParam.dev->SetMaterial (&pmat);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
			RenderParam.dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}*/
	}
	s.UnBind();
}

// =======================================================================
// returns the direction of the tile centre from the planet centre in local
// planet coordinates

VECTOR3 TileManager::TileCentre (int hemisp, int ilat, int nlat, int ilng, int nlng)
{
	double cntlat = PI*0.5 * ((double)ilat+0.5)/(double)nlat,      slat = sin(cntlat), clat = cos(cntlat);
	double cntlng = PI*2.0 * ((double)ilng+0.5)/(double)nlng + PI, slng = sin(cntlng), clng = cos(cntlng);
	if (hemisp) return _V(clat*clng, -slat, -clat*slng);
	else        return _V(clat*clng,  slat,  clat*slng);
}

// =======================================================================

void TileManager::TileExtents (int hemisp, int ilat, int nlat, int ilng, int nlng, double &lat1, double &lat2, double &lng1, double &lng2)
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

bool TileManager::TileInView (int lvl, int ilat)
{
	//FIXME
	return true;
	/*
	const double eps = 1e-3;
	bool bx1, bx2, by1, by2, bz1, bz2, bbvis;
	int v;
	D3DVALUE x, y;
	VERTEX_XYZH *vtx;
	VBMESH &mesh = PATCH_TPL[lvl][ilat];
	bbtarget->ProcessVertices (D3DVOP_TRANSFORM, 0, 8, mesh.bb, 0, RenderParam.dev, 0);
	bbtarget->Lock (DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_DISCARDCONTENTS, (LPVOID*)&vtx, NULL);
	bx1 = bx2 = by1 = by2 = bz1 = bz2 = bbvis = false;
	for (v = 0; v < 8; v++) {
		if (vtx[v].z > 0.0)  bz1 = true;
		if (vtx[v].z <= 1.0+eps) bz2 = true;
		if (vtx[v].z <= 1.0) x =  vtx[v].x, y =  vtx[v].y;
		else                 x = -vtx[v].x, y = -vtx[v].y;
		if (x > vpX0)        bx1 = true;
		if (x < vpX1)        bx2 = true;
		if (y > vpY0)        by1 = true;
		if (y < vpY1)        by2 = true;
		if (bbvis = bx1 && bx2 && by1 && by2 && bz1 && bz2) break;
	}
	bbtarget->Unlock();
	return bbvis;*/
}

// =======================================================================

void TileManager::SetWorldMatrix (int ilng, int nlng, int ilat, int nlat)
{
	// set up world transformation matrix
	glm::mat4 rtile;
	double lng = PI*2.0 * (double)ilng/(double)nlng + PI; // add pi so texture wraps at +-180�

	double sinr = sin(lng), cosr = cos(lng);
	rtile[0][0] = rtile[2][2] = (float)cosr;
	rtile[2][0] = -(rtile[0][2] = (float)sinr);
	rtile[1][1] = rtile[3][3] = 1.0f;

	if (nlat > 8) {
		// The reference point for these tiles has been shifted from the centre of the sphere
		// to the lower left corner of the tile, to reduce offset distances which cause rounding
		// errors in the single-precision world matrix. The offset calculations are done in
		// double-precision before copying them into the world matrix.
		double lat = PI05 * (double)ilat/(double)nlat;
		double s = RenderParam.objsize;
		double dx = s*cos(lng)*cos(lat); // the offsets between sphere centre and tile corner
		double dy = s*sin(lat);
		double dz = s*sin(lng)*cos(lat);
//		RenderParam.wmat_tmp[3][0] = (float)(dx*RenderParam.grot.m11 + dy*RenderParam.grot.m12 + dz*RenderParam.grot.m13 + RenderParam.cpos.x);
//		RenderParam.wmat_tmp[3][1] = (float)(dx*RenderParam.grot.m21 + dy*RenderParam.grot.m22 + dz*RenderParam.grot.m23 + RenderParam.cpos.y);
//		RenderParam.wmat_tmp[3][2] = (float)(dx*RenderParam.grot.m31 + dy*RenderParam.grot.m32 + dz*RenderParam.grot.m33 + RenderParam.cpos.z);
		RenderParam.wmat_tmp[0][3] = (float)(dx*RenderParam.grot.m11 + dy*RenderParam.grot.m12 + dz*RenderParam.grot.m13 + RenderParam.cpos.x);
		RenderParam.wmat_tmp[1][3] = (float)(dx*RenderParam.grot.m21 + dy*RenderParam.grot.m22 + dz*RenderParam.grot.m23 + RenderParam.cpos.y);
		RenderParam.wmat_tmp[2][3] = (float)(dx*RenderParam.grot.m31 + dy*RenderParam.grot.m32 + dz*RenderParam.grot.m33 + RenderParam.cpos.z);
//		D3DMAT_MatrixMultiply (&wtrans, &RenderParam.wmat_tmp, &rtile);

		RenderParam.wtrans = RenderParam.wmat_tmp * rtile;
	} else {
		RenderParam.wtrans = RenderParam.wmat * rtile;
//		D3DMAT_MatrixMultiply (&wtrans, &RenderParam.wmat, &rtile);
	}

	//RenderParam.dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &wtrans);
}

// ==============================================================

bool TileManager::SpecularColour (glm::vec4 *col)
{
	if (!atmc) {
		col->r = col->g = col->b = spec_base;
		return false;
	} else {
		double fac = 0.7; // needs thought ...
		double cosa = dotp (RenderParam.cdir, RenderParam.sdir);
		double alpha = 0.5*acos(cosa); // sun reflection angle
		double scale = sin(alpha)*fac;
		col->r = (float)std::max(0.0, spec_base - scale*atmc->color0.x);
		col->g = (float)std::max(0.0, spec_base - scale*atmc->color0.y);
		col->b = (float)std::max(0.0, spec_base - scale*atmc->color0.z);
		return true;
	}
}

// ==============================================================

void TileManager::GlobalInit ()
{
	bGlobalSpecular = false;// *(bool*)gclient->GetConfigParam (CFGPRM_SURFACEREFLECT);
	bGlobalRipple   = false;//bGlobalSpecular && *(bool*)gclient->GetConfigParam (CFGPRM_SURFACERIPPLE);
	bGlobalLights   = false;//*(bool*)gclient->GetConfigParam (CFGPRM_SURFACELIGHTS);

	// Level 1 patch template
	CreateSphere ( PATCH_TPL_1, 6, false, 0, 64);

	// Level 2 patch template
	CreateSphere ( PATCH_TPL_2, 8, false, 0, 128);

	// Level 3 patch template
	CreateSphere ( PATCH_TPL_3, 12, false, 0, 256);

	// Level 4 patch templates
	CreateSphere ( PATCH_TPL_4[0], 16, true, 0, 256);
	CreateSphere ( PATCH_TPL_4[1], 16, true, 1, 256);

	// Level 5 patch template
	CreateSpherePatch2 ( PATCH_TPL_5, 4, 1, 0, 18);

	// Level 6 patch templates
	CreateSpherePatch2 ( PATCH_TPL_6[0], 8, 2, 0, 10, 16);
	CreateSpherePatch2 ( PATCH_TPL_6[1], 4, 2, 1, 12);

	// Level 7 patch templates
	CreateSpherePatch2 ( PATCH_TPL_7[0], 16, 4, 0, 12, 12, false);
	CreateSpherePatch2 ( PATCH_TPL_7[1], 16, 4, 1, 12, 12, false);
	CreateSpherePatch2 ( PATCH_TPL_7[2], 12, 4, 2, 10, 16, true);
	CreateSpherePatch2 ( PATCH_TPL_7[3],  6, 4, 3, 12, -1, true);

	// Level 8 patch templates
	CreateSpherePatch2 ( PATCH_TPL_8[0], 32, 8, 0, 12, 15, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[1], 32, 8, 1, 12, 15, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[2], 30, 8, 2, 12, 16, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[3], 28, 8, 3, 12, 12, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[4], 24, 8, 4, 12, 12, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[5], 18, 8, 5, 12, 12, false, true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[6], 12, 8, 6, 10, 16, true,  true, true);
	CreateSpherePatch2 ( PATCH_TPL_8[7],  6, 8, 7, 12, -1, true,  true, true);

	// Patch templates for level 9 and beyond
	const int n = 8;
	const int nlng8[8] = {32,32,30,28,24,18,12,6};
	const int res8[8] = {15,15,16,12,12,12,12,12};
	int mult = 2, idx, lvl, i, j;
	for (lvl = 9; lvl <= SURF_MAX_PATCHLEVEL; lvl++) {
		idx = 0;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < mult; j++) {
				if (idx < n*mult)
					CreateSpherePatch2 ( PATCH_TPL[lvl][idx], nlng8[i]*mult, n*mult, idx, 12, res8[i], false, true, true, true);
				else
					CreateSpherePatch2 ( PATCH_TPL[lvl][idx], nlng8[i]*mult, n*mult, idx, 12, -1, true, true, true, true);
				idx++;
			}
		}
		mult *= 2;
	}

	// create the system-wide tile cache
	tilebuf = new TileBuffer ();
/*
	// create the vertex buffer for tile bounding box checks
	static D3DVERTEXBUFFERDESC bbvbd = 
	{ sizeof(D3DVERTEXBUFFERDESC), D3DVBCAPS_SYSTEMMEMORY, D3DFVF_XYZRHW, 8 };
	d3d->CreateVertexBuffer (&bbvbd, &bbtarget, 0);
*/
	// viewport size for clipping calculations
	/*
	D3DVIEWPORT7 vp;
	dev->GetViewport (&vp);
	vpX0 = vp.dwX, vpX1 = vpX0 + vp.dwWidth;
	vpY0 = vp.dwY, vpY1 = vpY0 + vp.dwHeight;
*/
	// rotation matrix for flipping patches onto southern hemisphere
//	D3DMAT_RotX (&Rsouth, PI);
	double sinr = sin(PI), cosr = cos(PI);
//	Rsouth[1][1] = Rsouth[2][2] = (float)cosr;
//	Rsouth[1][2] = -(Rsouth[2][1] = (float)sinr);
//	Rsouth[0][0] = Rsouth[3][3] = 1.0f;
	Rsouth[1][1] = Rsouth[2][2] = (float)cosr;
	Rsouth[2][1] = -(Rsouth[1][2] = (float)sinr);
	Rsouth[0][0] = Rsouth[3][3] = 1.0f;
}

// ==============================================================

void TileManager::GlobalExit ()
{
	int i;
	DestroyVBMesh (PATCH_TPL_1);
	DestroyVBMesh (PATCH_TPL_2);
	DestroyVBMesh (PATCH_TPL_3);
	for (i = 0; i <  2; i++) DestroyVBMesh (PATCH_TPL_4[i]);
	DestroyVBMesh (PATCH_TPL_5);
	for (i = 0; i <  2; i++) DestroyVBMesh (PATCH_TPL_6[i]);
	for (i = 0; i <  4; i++) DestroyVBMesh (PATCH_TPL_7[i]);
	for (i = 0; i <  8; i++) DestroyVBMesh (PATCH_TPL_8[i]);

	const int n = 8;
	int mult = 2, lvl;
	for (lvl = 9; lvl <= SURF_MAX_PATCHLEVEL; lvl++) {
		for (i = 0; i < n*mult; i++) DestroyVBMesh (PATCH_TPL[lvl][i]);
		mult *= 2;
	}
	delete tilebuf;

	//bbtarget->Release();
	//bbtarget = 0;
}

// ==============================================================

void TileManager::SetMicrotexture (const char *fname)
{
	//if (fname)
	//	g_client->GetTexMgr()->GetTexture (fname, &microtex, 0);
	//else
		microtex = 0;
}

// ==============================================================

void TileManager::SetMicrolevel (double lvl)
{
	microlvl = lvl;
}

// ==============================================================
// static member initialisation

int TileManager::patchidx[9] = {0, 1, 2, 3, 5, 13, 37, 137, 501};

//const D3D7Config *TileManager::cfg = NULL;
bool TileManager::bGlobalSpecular = false;
bool TileManager::bGlobalRipple = false;
bool TileManager::bGlobalLights = false;

TileBuffer *TileManager::tilebuf = NULL;
glm::mat4   TileManager::Rsouth;
int TileManager::vbMemCaps = 0;

VBMESH TileManager::PATCH_TPL_1;
VBMESH TileManager::PATCH_TPL_2;
VBMESH TileManager::PATCH_TPL_3;
VBMESH TileManager::PATCH_TPL_4[2];
VBMESH TileManager::PATCH_TPL_5;
VBMESH TileManager::PATCH_TPL_6[2];
VBMESH TileManager::PATCH_TPL_7[4];
VBMESH TileManager::PATCH_TPL_8[8];
VBMESH TileManager::PATCH_TPL_9[16];
VBMESH TileManager::PATCH_TPL_10[32];
VBMESH TileManager::PATCH_TPL_11[64];
VBMESH TileManager::PATCH_TPL_12[128];
VBMESH TileManager::PATCH_TPL_13[256];
VBMESH TileManager::PATCH_TPL_14[512];
VBMESH *TileManager::PATCH_TPL[15] = {
	0, &PATCH_TPL_1, &PATCH_TPL_2, &PATCH_TPL_3, PATCH_TPL_4, &PATCH_TPL_5,
	PATCH_TPL_6, PATCH_TPL_7, PATCH_TPL_8, PATCH_TPL_9, PATCH_TPL_10,
	PATCH_TPL_11, PATCH_TPL_12, PATCH_TPL_13, PATCH_TPL_14
};

int TileManager::NLAT[9] = {0,1,1,1,1,1,2,4,8};
int TileManager::NLNG5[1] = {4};
int TileManager::NLNG6[2] = {8,4};
int TileManager::NLNG7[4] = {16,16,12,6};
int TileManager::NLNG8[8] = {32,32,30,28,24,18,12,6};
int *TileManager::NLNG[9] = {0,0,0,0,0,NLNG5,NLNG6,NLNG7,NLNG8};

int TileManager::vpX0, TileManager::vpX1, TileManager::vpY0, TileManager::vpY1;

// =======================================================================
// Nonmember functions

void ApplyPatchTextureCoordinates (VBMESH &mesh, VertexBuffer *vtx, const TEXCRDRANGE &range)
{
	N2TVERTEX *tgtdata = (N2TVERTEX *)vtx->Map();
	if (mesh.vtx) { // direct access to vertex data
		memcpy (tgtdata, mesh.vtx, mesh.nv*sizeof(N2TVERTEX));
	} else {        // need to lock the buffer
		N2TVERTEX *srcdata = (N2TVERTEX *)mesh.VBO->Map();
		memcpy (tgtdata, srcdata, mesh.nv*sizeof(N2TVERTEX));
		mesh.VBO->UnMap();
	}
	float tuscale = range.tumax-range.tumin, tuofs = range.tumin;
	float tvscale = range.tvmax-range.tvmin, tvofs = range.tvmin;
	for (uint32_t i = 0; i < mesh.nv; i++) {
		tgtdata[i].tu0 = tgtdata[i].tu0*tuscale + tuofs;
		tgtdata[i].tv0 = tgtdata[i].tv0*tvscale + tvofs;
	}
	vtx->UnMap();
}


// =======================================================================
// =======================================================================
// Class TileBuffer: implementation

TileBuffer::TileBuffer ()
{
	nbuf = 0;
	nused = 0;
	last = 0;

	bLoadMip = true;
	bRunThread = true;
	nqueue = queue_in = queue_out = 0;
	hLoadThread = std::thread(LoadTile_ThreadProc, this);
}

// =======================================================================

TileBuffer::~TileBuffer()
{
	{
		std::lock_guard<std::mutex> lock(hQueueMutex);
		bRunThread = false;
		hCV.notify_one();
		hLoadThread.join();
	}

	if (nbuf) {
		for (int i = 0; i < nbuf; i++)
			if (buf[i]) {
				if (!(buf[i]->flag & 0x80)) { // if loaded, release tile textures
					if (buf[i]->tex.obj)  buf[i]->tex.obj->Release();
					if (buf[i]->ltex.obj) buf[i]->ltex.obj->Release();
				}
				if (buf[i]->vtx)
					delete buf[i]->vtx;
				delete buf[i];
			}
		delete []buf;
	}
}

// =======================================================================

TILEDESC *TileBuffer::AddTile ()
{
	TILEDESC *td = new TILEDESC;
	memset (td, 0, sizeof(TILEDESC));
	int i, j;

	if (nused == nbuf) {
		TILEDESC **tmp = new TILEDESC*[nbuf+16];
		if (nbuf) {
			memcpy (tmp, buf, nbuf*sizeof(TILEDESC*));
			delete []buf;
		}
		memset (tmp+nbuf, 0, 16*sizeof(TILEDESC*));
		buf = tmp;
		nbuf += 16;
		last = nused;
	} else {
		for (i = 0; i < nbuf; i++) {
			j = (i+last)%nbuf;
			if (!buf[j]) {
				last = j;
				break;
			}
		}
        if (i == nbuf) {
			/* Problems! */;
        }
	}
	buf[last] = td;
	td->ofs = last;
	nused++;
	return td;
}

// =======================================================================

void TileBuffer::DeleteSubTiles (TILEDESC *tile)
{
	for (int i = 0; i < 4; i++)
		if (tile->subtile[i]) {
			if (DeleteTile (tile->subtile[i]))
				tile->subtile[i] = 0;
		}
}

// =======================================================================

bool TileBuffer::DeleteTile (TILEDESC *tile)
{
	bool del = true;
	for (int i = 0; i < 4; i++)
		if (tile->subtile[i]) {
			if (DeleteTile (tile->subtile[i]))
				tile->subtile[i] = 0;
			else
				del = false;
		}
	if (tile->vtx) {
		//tile->vtx->Release();
		tile->vtx = 0;
	}
	if (tile->tex.obj || !del) {
		return false; // tile or subtile contains texture -> don't deallocate
	} else {
		buf[tile->ofs] = 0; // remove from list
		delete tile;
		nused--;
		return true;
	}
}

// =======================================================================

void ClearVertexBuffers (TILEDESC *td)
{
	for (int j = 0; j < 4; j++) {
		TILEDESC *sub = td->subtile[j];
		if (sub) {
			if (!sub->tex.obj || sub->flag & 0x80) {
				// child has no own texture, i.e. uses part of parent texture
				if (sub->vtx) {
					//sub->vtx->Release();
					sub->vtx = 0;
				}
				ClearVertexBuffers (sub); // recursion up the tree
			}
		}
	}
}

// =======================================================================

bool TileBuffer::LoadTileAsync (const char *name, TILEDESC *tile)
{
	bool ok = true;
    std::lock_guard<std::mutex> lock(hQueueMutex);

	if (nqueue == MAXQUEUE)
		ok = false; // queue full
	else {
		for (int i = 0; i < nqueue; i++) {
			int j = (i+queue_out) % MAXQUEUE;
			if (loadqueue[j].td == tile)
			{ ok = false; break; }// request already present
		}
	}

	if (ok) {
		QUEUEDESC *qd = loadqueue+queue_in;
		qd->name = name;
		qd->td = tile;

		nqueue++;
		queue_in = (queue_in+1) % MAXQUEUE;
	    hCV.notify_one();
	}
	return ok;
}

// =======================================================================

int TileBuffer::LoadTile_ThreadProc (TileBuffer *tb)
{
	//static const long TILESIZE = 32896; // default texture size for old-style texture files
	bool load;
	static QUEUEDESC qd;
	//static int nloaded = 0; // temporary
	//int flag = (tb->bLoadMip ? 0:4);
	//int idle = 10;//1000/gc->Cfg()->PlanetLoadFrequency;
	char fname[256];

	while (bRunThread) {
		{
			std::unique_lock<std::mutex> lock(hQueueMutex);
			while(nqueue == 0 && bRunThread)
			{
				hCV.wait(lock);
			}
			if ((load = (nqueue > 0))) {
				memcpy (&qd, loadqueue+queue_out, sizeof(QUEUEDESC));
			}
		}

		if (load) {
			TILEDESC *td = qd.td;
			OGLTexture *tex = nullptr, *mask = nullptr;
			uint32_t tidx = 0, midx = 0;
			//long ofs;

			if ((td->flag & 0x80) == 0) {
				printf("wtf?\n");
				exit(-1);
//				MessageBeep (-1);
			}

			tidx = td->tex.idx;
			if (tidx == (uint32_t)NOTILE)
				tex = NULL; // "no texture" flag
			else {
				//ofs = (td->flag & 0x40 ? (long)tidx * TILESIZE : (long)tidx);
				strcpy (fname, qd.name);
				strcat (fname, "_tile.tex");
				
				//if (g_client->GetTexMgr()->LoadTexture (fname, ofs, &tex, flag) != S_OK)
					tex = NULL;
			}

			// Load the specular mask and/or light texture
			if (((td->flag & 3) == 3) || (td->flag & 4)) {
				midx = td->ltex.idx;
				if (midx == (uint32_t)NOTILE)
					mask = NULL; // "no mask" flag
				else {
					//ofs = (td->flag & 0x40 ? (long)midx * TILESIZE : (long)midx);
					strcpy (fname, qd.name);
					strcat (fname, "_tile_lmask.tex");
					//if (g_client->GetTexMgr()->LoadTexture (fname, ofs, &mask) != S_OK)
						mask = NULL;
				}
			}
			// apply loaded components
			{
				std::unique_lock<std::mutex> lock(hQueueMutex);
				td->tex.obj  = tex;
				td->ltex.obj = mask;
				if (tex) {
					if (td->vtx) {
						//td->vtx->Release();
						td->vtx = 0;
					}
					ClearVertexBuffers (td); // invalidate child vertex buffers
				}
				
				td->flag &= 0x3F; // mark as loaded
				nqueue--;
				queue_out = (queue_out+1) % MAXQUEUE;
			}
		}
	}
	return 0;
}

// =======================================================================

bool TileBuffer::bRunThread = true;
int TileBuffer::nqueue = 0;
int TileBuffer::queue_in = 0;
int TileBuffer::queue_out = 0;
std::mutex TileBuffer::hQueueMutex;
std::condition_variable TileBuffer::hCV;
struct TileBuffer::QUEUEDESC TileBuffer::loadqueue[MAXQUEUE] = {0};

