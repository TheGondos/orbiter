// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OpenGL Client module
// ==============================================================

// ==============================================================
// HazeMgr.h
// class HazeManager (interface)
//
// Planetary atmospheric haze rendering
// Implemented as transparent overlay on planetary disc
// ==============================================================

#ifndef __HAZEMGR_H
#define __HAZEMGR_H

#include "OGLClient.h"
#include "VertexBuffer.h"

#define HORIZON_NSEG 128  // number of mesh segments

class vPlanet;
class OGLCamera;
class HazeManager {
public:
	HazeManager (const vPlanet *vplanet);

	static void GlobalInit ();

	void Render (OGLCamera *c, glm::mat4 &wmat, bool dual = false);

private:
	OBJHANDLE obj;
	const vPlanet *vp;
	VECTOR3 basecol;
	double rad;    // planet radius
	float  hralt;  // relative horizon altitude
	float  dens0;  // atmosphere density factor
	double hshift; // horizon reference shift factor
	double cloudalt; // cloud layer altitude
	float  hscale; // inner haze ring radius (in planet radii)
	static uint16_t Idx[HORIZON_NSEG*2+2];
	static int nIdx;
	static struct HVERTEX {
		float x,y,z;
		glm::vec4 dcol;
		float tu, tv; } Vtx[HORIZON_NSEG*2];
	static float CosP[HORIZON_NSEG], SinP[HORIZON_NSEG];
	static OGLTexture *horizon;
	static std::unique_ptr<VertexBuffer> VBO;
	static std::unique_ptr<IndexBuffer> IBO;
	static std::unique_ptr<VertexArray> VBA;
	static inline Shader *hazeShader;
};

#endif // !__HAZEMGR_H
