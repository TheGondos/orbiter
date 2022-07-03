// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// spherepatch.h
// Create meshes for spheres and sphere patches
// ==============================================================

#ifndef __SPHEREPATCH_H
#define __SPHEREPATCH_H

#include "OGLClient.h"
#include "VertexBuffer.h"

struct N2TVERTEX {
	float x, y, z, nx, ny, nz;
	float tu0, tv0, tu1, tv1;
	inline N2TVERTEX() {}
	//VERTEX_2TEX ()
	//{ x = y = z = nx = ny = nz = tu0 = tv0 = tu1 = tv1 = 0.0f; }
	inline N2TVERTEX (glm::vec3 &p, glm::vec3 &n, float u0, float v0, float u1, float v1)
	{ x = p.x, y = p.y, z = p.z, nx = n.x, ny = n.y, nz = n.z;
  	  tu0 = u0, tv0 = v0, tu1 = u1, tv1 = v1; }
};

struct VBMESH {
	VBMESH();
	~VBMESH();
	void MapVertices (); // copy vertices from vtx to vb
	VertexBuffer *vb; // mesh vertex buffer
	VertexArray *va;
//	VertexBuffer *bb; // bounding box vertex buffer
	N2TVERTEX *vtx;           // separate storage of vertices (NULL if not available)
	VECTOR4 *bbvtx;             // bounding box vertices
	uint32_t nv;                   // number of vertices
	uint16_t *idx;                 // list of indices
	IndexBuffer *ib;
	uint32_t ni;                   // number of indices
};

void CreateSphere (VBMESH &mesh, uint32_t nrings, bool hemisphere, int which_half, int texres);

void CreateSpherePatch (VBMESH &mesh, int nlng, int nlat, int ilat, int res, int bseg = -1,
	bool reduce = true, bool outside = true, bool store_vtx = false, bool shift_origin = false);

void DestroyVBMesh (VBMESH &mesh);

#endif // !__SPHEREPATCH_H