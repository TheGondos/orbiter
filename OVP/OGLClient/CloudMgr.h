// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// CloudMgr.h
// class CloudManager (interface)
//
// Planetary rendering management for cloud layers, including a simple
// LOD (level-of-detail) algorithm for patch resolution.
// ==============================================================

#ifndef __CLOUDMGR_H
#define __CLOUDMGR_H

#include "TileMgr.h"

class CloudManager: public TileManager {
public:
	CloudManager (const VPlanet *vplanet);

	void Render (glm::mat4 &wmat, double scale, int level, double viewap = 0.0);

protected:
	void RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, double sdist,
		TILEDESC *tile, const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, int flag) override;

private:
	int cloudtexidx;
};

#endif // !__CLOUDMGR_H