// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// SurfMgr.h
// class SurfaceManager (interface)
//
// Planetary surface rendering management, including a simple
// LOD (level-of-detail) algorithm for surface patch resolution.
// ==============================================================

#ifndef __SURFMGR_H
#define __SURFMGR_H

#include "TileMgr.h"

class SurfaceManager: public TileManager {
public:
	SurfaceManager (const vPlanet *vplanet);
	void SetMicrotexture (const char *fname) override;
	void Render (glm::mat4 &wmat, double scale, int level, double viewap = 0.0, bool bfog = false) override;

protected:
	void RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, double sdist,
		TILEDESC *tile, const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, uint32_t flag) override;

};

#endif // !__SURFMGR_H
