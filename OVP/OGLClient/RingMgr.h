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
#include "OGLMesh.h"

#define MAXRINGRES 3

// ==============================================================
// class RingManager (interface)
// ==============================================================
/**
 * \brief Rendering of planet rings at different resolutions
 */
class RingManager {
public:
	RingManager (const vPlanet *vplanet, double inner_rad, double outer_rad);
	~RingManager ();

	static void GlobalInit ();

	void SetMeshRes (unsigned int res);

	inline double InnerRad() const { return irad; }
	inline double OuterRad() const { return orad; }

	bool Render (OGLCamera *c, glm::mat4 &mWorld, bool zenable);

protected:
	OGLMesh *CreateRing (double irad, double orad, int nsect);
	unsigned int LoadTextures ();

private:
	const vPlanet *vp;
	OGLMesh *mesh[MAXRINGRES];
	OGLTexture * tex[MAXRINGRES];
	unsigned int rres, tres, ntex;
	double irad, orad;

	static inline Shader *meshShader;
};

#endif // !__RINGMGR_H