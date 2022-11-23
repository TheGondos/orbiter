// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VBase.h
// class vBase (interface)
//
// A vBase is the visual representation of a surface base
// object (a "spaceport" on the surface of a planet or moon,
// usually with runways or landing pads where vessels can
// land and take off.
// ==============================================================

#ifndef __vBase_H
#define __vBase_H

#include "VObject.h"
#include "OGLMesh.h"

class vBase: public vObject {
	friend class vPlanet;

public:
	vBase (OBJHANDLE _hObj, const Scene *scene);
	~vBase();

	bool Update ();

	bool RenderSurface ();
	bool RenderStructures ();
	void RenderGroundShadow (float depth);
	static void GlobalInit();

private:
	void SetupShadowMeshes ();

	/**
	 * \brief Modify local lighting due to planet shadow or
	 *   atmospheric dispersion.
	 * \param light pointer to D3DLIGHT7 structure receiving modified parameters
	 * \param nextcheck time interval until next lighting check [s]
	 * \return \e true if lighting modifications should be applied, \e false
	 *   if global lighting conditions apply.
	 */
	//bool ModLighting (LPD3DLIGHT7 light, double &nextcheck);

	double Tchk;               // next update
	double Tlghtchk;           // next lighting update
	int ntile;               // number of surface tiles
	const SurftileSpec *tspec; // list of tile specs
	struct SurfTile {
		OGLMesh *mesh;
	} *tile;
	OGLMesh **structure_bs;
	OGLMesh **structure_as;
	int nstructure_bs, nstructure_as;
	bool lights;               // use nighttextures for base objects
	bool bLocalLight;          // true if lighting is modified
	//D3DLIGHT7 localLight;      // current local lighting parameters

	struct ShadowMesh {
		VertexBuffer *vbuf;
		IndexBuffer *idx;
		VertexArray *va;
		int nvtx, nidx;
		double ecorr;
	} *shmesh;
	int nshmesh;
	static inline Shader *shadowShader;
};

#endif // !__vBase_H