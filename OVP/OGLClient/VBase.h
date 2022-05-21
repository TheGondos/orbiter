// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VBase.h
// class VBase (interface)
//
// A VBase is the visual representation of a surface base
// object (a "spaceport" on the surface of a planet or moon,
// usually with runways or landing pads where vessels can
// land and take off.
// ==============================================================

#ifndef __VBASE_H
#define __VBASE_H

#include "VObject.h"
#include "MeshManager.h"
#include "VertexBuffer.h"

class OGLCamera;
class Scene;
class VBase: public VObject {
public:
	VBase (OBJHANDLE _hObj);
	~VBase();

	bool Update ();

	bool RenderSurface (OGLCamera *);
	bool RenderStructures (OGLCamera *);
	void RenderGroundShadow (OGLCamera *);

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
	size_t ntile;               // number of surface tiles
	const SurftileSpec *tspec; // list of tile specs
	struct SurfTile {
		OGLMesh *mesh;
	} *tile;
	OGLMesh **structure_bs;
	OGLMesh **structure_as;
	size_t nstructure_bs, nstructure_as;
	bool lights;               // use nighttextures for base objects
	bool bLocalLight;          // true if lighting is modified
	//D3DLIGHT7 localLight;      // current local lighting parameters

	struct ShadowMesh {
		std::unique_ptr<VertexBuffer> VBO;
		std::unique_ptr<IndexBuffer> IBO;
		std::unique_ptr<VertexArray> VBA;
		int nvtx;
		double ecorr;
	} *shmesh;
	size_t nshmesh;
};

#endif // !__VBASE_H