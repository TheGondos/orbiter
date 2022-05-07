#pragma once

#include "OGLClient.h"
#include "VertexBuffer.h"
#include <memory>
#include <glm/glm.hpp>

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
	void UpdateVertexBuffer ();
	std::unique_ptr<VertexBuffer> VBO; // mesh vertex buffer
	std::unique_ptr<IndexBuffer> IBO;
	std::unique_ptr<VertexArray> VAO;

	N2TVERTEX *vtx;           // separate storage of vertices (NULL if not available)
	VECTOR4 *bbvtx;             // bounding box vertices
	uint32_t nv;
};

void CreateSphere (VBMESH &mesh, int nrings, bool hemisphere, int which_half, int texres);

void CreateSpherePatch2 (VBMESH &mesh, int nlng, int nlat, int ilat, int res, int bseg = -1,
	bool reduce = true, bool outside = true, bool store_vtx = false, bool shift_origin = false);

void DestroyVBMesh (VBMESH &mesh);
