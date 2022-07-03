// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// CSphereMgr.h:
// Rendering of the celestial sphere background at variable
// resolutions.
// ==============================================================

#ifndef __CSPHEREMGR_H
#define __CSPHEREMGR_H

#include "TileMgr.h"

// =======================================================================
// Class CSphereManager

class CSphereManager {
public:
	CSphereManager (const Scene *scene);
	~CSphereManager ();

	static void GlobalInit ();
	static void CreateDeviceObjects ();
	static void DestroyDeviceObjects ();

	void Render (int level, int bglvl);

protected:
	bool LoadPatchData ();
	bool LoadTileData ();
	void LoadTextures ();

	void ProcessTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, TILEDESC *tile,
		const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, uint32_t flag,
		const TEXCRDRANGE &bkp_range, OGLTexture *bkp_tex, OGLTexture *bkp_ltex, uint32_t bkp_flag);

	void RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng,
		TILEDESC *tile, const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, uint32_t flag);

	glm::mat4 GetWorldMatrix (int ilng, int nlng, int ilat, int nlat);

	VECTOR3 TileCentre (int hemisp, int ilat, int nlat, int ilng, int nlng);
	// returns the direction of the tile centre from the planet centre in local
	// planet coordinates

	void TileExtents (int hemisp, int ilat, int nlat, int ilg, int nlng, double &lat1, double &lat2, double &lng1, double &lng2);

	bool TileInView (int lvl, int ilat, glm::mat4 &wm);
	// Check if specified tile intersects viewport

//	static const D3D7Config *cfg;    // configuration parameters
	const Scene *scn;
	static int *patchidx;            // texture offsets for different LOD levels
	static VBMESH **PATCH_TPL;
	static int **NLNG;
	static int *NLAT;

private:
	char texname[64];
	float intensity;                 // opacity of background image
	bool disabled;                   // background image disabled?
	uint32_t maxlvl;                    // max. patch resolution level
	uint32_t maxbaselvl;                // max. resolution level, capped at 8
	uint32_t ntex;                      // total number of loaded textures for levels <= 8
	uint32_t nhitex;                    // number of textures for levels > 8
	uint32_t nhispec;                   // number of specular reflection masks (level > 8)
	TILEDESC *tiledesc;              // tile descriptors for levels 1-8
	OGLTexture **texbuf;    // texture buffer for surface textures (level <= 8)
	bool bPreloadTile;               // preload high-resolution tile textures
	MATRIX3 ecl2gal;                 // rotates from ecliptic to galactic frame
	glm::mat4 trans;                 // transformation from ecliptic to galactic frame

	TileBuffer *tilebuf;
	struct RENDERPARAM {
		int tgtlvl;                  // target resolution level
		glm::mat4 wmat;              // world matrix
		VECTOR3 camdir;              // camera direction in galactic frame
		double viewap;               // viewport aperture (semi-diagonal)
	} RenderParam;

	static uint32_t vpX0, vpX1, vpY0, vpY1; // viewport boundaries
	static double diagscale;
};

#endif // !__CSPHEREMGR_H