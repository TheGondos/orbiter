// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// RingMgr.h
// Render planetary ring systems
// ==============================================================

#ifndef __RINGMGR_H
#define __RINGMGR_H

#include "OGLClient.h"
#include "VPlanet.h"
#include "MeshManager.h"
#include "OGLCamera.h"
#include "VertexBuffer.h"

#define MAXRINGRES 3


struct RINGMESH {
	std::unique_ptr<VertexBuffer> VBO; // mesh vertex buffer
	std::unique_ptr<IndexBuffer> IBO;
	std::unique_ptr<VertexArray> VAO;
	OGLTexture *texture;
};


class RingManager {
public:
	RingManager (const VPlanet *vplanet, double inner_rad, double outer_rad);
	~RingManager ();

	static void GlobalInit ();

	void SetMeshRes (int res);

	inline double InnerRad() const { return irad; }
	inline double OuterRad() const { return orad; }

	bool Render (glm::mat4 &mWorld, OGLCamera *c, bool front);

protected:
	void CreateRing (RINGMESH &m, double irad, double orad, int nsect, OGLTexture *tex);
	uint32_t LoadTextures (OGLTexture **tex);

private:
	const VPlanet *vp;
	RINGMESH mesh[MAXRINGRES];
	OGLTexture *tex[MAXRINGRES];
	uint32_t rres, tres, ntex;
	double irad, orad;
};

#endif // !__RINGMGR_H