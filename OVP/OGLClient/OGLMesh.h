// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OGL Client module
// ==============================================================

// ==============================================================
// Mesh.h
// class OGLMesh (interface)
//
// This class represents a mesh in terms of OGL interface elements
// (vertex buffers, index lists, materials, textures) which allow
// it to be rendered to the OGL device.
// ==============================================================

#ifndef __OGLMESH_H
#define __OGLMESH_H

#include "OGLClient.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include <glm/glm.hpp>

const uint32_t SPEC_DEFAULT = (uint32_t)(-1); // "default" material/texture flag
const uint32_t SPEC_INHERIT = (uint32_t)(-2); // "inherit" material/texture flag

/**
 * \brief Mesh object with D3D7-specific vertex buffer
 *
 * Meshes consist of one or more vertex groups, and a set of materials and
 * textures.
 */
typedef struct {
	float x;     ///< vertex x position
	float y;     ///< vertex y position
	float z;     ///< vertex z position
	float nx;    ///< vertex x normal
	float ny;    ///< vertex y normal
	float nz;    ///< vertex z normal
	float tx;    ///< vertex x tangent
	float ty;    ///< vertex y tangent
	float tz;    ///< vertex z tangent
	float tu;    ///< vertex u texture coordinate
	float tv;    ///< vertex v texture coordinate
} NTTVERTEX;

class OGLCamera;
class OGLMesh {
public:
	struct GROUPREC {  // mesh group definition
		int nVtx;           // number of vertices
        NTTVERTEX *Vtx;
		uint32_t nIdx;           // number of indices
		uint16_t *Idx;            // vertex index list
        std::unique_ptr<VertexBuffer> VBO;
        std::unique_ptr<IndexBuffer> IBO;
        std::unique_ptr<VertexArray> VBA;
		int MtrlIdx;        // material index
		int TexIdx;         // texture indices
		uint32_t UsrFlag;        // user-defined flag
		uint16_t zBias;           // z-bias value
		uint16_t IntFlag;         // internal flags
		int TexIdxEx[MAXTEX];
		float TexMixEx[MAXTEX];
	};

	/**
	 * \brief Create an empty mesh
	 * \param client graphics client
	 */
	OGLMesh ();
	OGLMesh (int nt, int nm);
	/**
	 * \brief Create a mesh consisting of a single mesh group
	 * \param client graphics client
	 * \param grp vertex group definition
	 * \param deepcopy if true, group contents are copied; otherwise, group
	 *   definition pointer is used directly
	 */
	OGLMesh (GROUPREC *grp, bool deepcopy=true);

	OGLMesh (MESHHANDLE hMesh, bool asTemplate = false);
	OGLMesh (const OGLMesh &mesh); // copy constructor
	~OGLMesh ();

	/**
	 * \brief Add a new vertex group to the mesh
	 * \param grp group definition
	 * \param deepcopy data copy flag (see notes)
	 * \return group index of the added group (>= 0)
	 * \note If deepcopy=true (default), the contents of the group definition
	 *   are copied into the mesh instance. deepcopy=false indicates that
	 *   the group definition was dynamically allocated, and that the pointer can
	 *   be used directly by the mesh. The calling function must not deallocate
	 *   the group after the call.
	 */
	int AddGroup (GROUPREC *grp, bool deepcopy = true);

	/**
	 * \brief Add a new vertex group to the mesh
	 * \param mg group definition in generic parameters
	 * \return group index of the added group (>= 0)
	 * \note This method accepts a MESHGROUPEX structure to define the
	 *   group parameters. It differs from the GROUPREC specification by
	 *   providing the vertex list as an array instead of a vertex buffer.
	 *   This method creates the vertex buffer on the fly.
	 * \note deepcopy is implied. The contents of \a mg can be discarded
	 *   after the call.
	 */
	int AddGroup (const MESHGROUPEX *mg);

	/**
	 * \brief Returns number of vertex groups
	 * \return Number of groups
	 */
	inline int GroupCount() const { return nGrp; }

	/**
	 * \brief Returns a pointer to a mesh group.
	 * \param idx group index (>= 0)
	 * \return Pointer to group structure.
	 */
	inline GROUPREC *GetGroup (int idx) { return Grp[idx]; }

	/**
	 * \brief Returns number of material specifications.
	 * \return Number of materials.
	 */
	inline int MaterialCount() const { return nMtrl; }

	/**
	 * \brief returns a pointer to a material definition.
	 * \param idx material index (>= 0)
	 * \return Pointer to material object.
	 */
	inline OGLMaterial *GetMaterial (int idx) { return Mtrl+idx; }

	/**
	 * \brief Replace a mesh texture.
	 * \param texidx texture index (>= 0)
	 * \param tex texture handle
	 * \return \e true on success, \e false otherwise.
	 */
	bool SetTexture (int texidx, SURFHANDLE tex);

	void SetTexMixture (int ntex, float mix);

	void RenderGroup (GROUPREC *grp);
	void Render (OGLCamera *c, glm::fmat4 &model);

	void TransformGroup (int n, const glm::fmat4 *m);
	int GetGroup (int grp, GROUPREQUESTSPEC *grs);
	int EditGroup (int grp, GROUPEDITSPEC *ges);

	/**
	 * \brief Enable/disable material alpha value for transparency calculation.
	 * \param enable flag for enabling/disabling material alpha calculation.
	 * \note By default, material alpha values are ignored for mesh groups
	 *   with textures, and the texture alpha values are used instead.
	 *   By enabling material alpha calculation, the final alpha value is
	 *   calculated as the product of material and texture alpha value.
	 */
	inline void EnableMatAlpha (bool enable) { bModulateMatAlpha = enable; }

	/**
	 * \brief Globally enable/disable specular reflections for mesh rendering.
	 * \param enable flag for enabling/disabling specular reflections
	 */
	static void GlobalEnableSpecular (bool enable);
	static void GlobalInit();

protected:
	bool CopyGroup (GROUPREC *tgt, const GROUPREC *src);
	bool CopyGroup (GROUPREC *grp, const MESHGROUPEX *mg);
	void DeleteGroup (GROUPREC *grp);
	void ClearGroups ();
	bool CopyMaterial (OGLMaterial *mat7, MATERIAL *mat);

private:
	GROUPREC **Grp;             // list of mesh groups
	int nGrp;                 // number of mesh groups
	OGLTexture **Tex;  // list of mesh textures
	int nTex;                 // number of mesh textures
	OGLMaterial *Mtrl;         // list of mesh materials
	int nMtrl;                // number of mesh materials
	bool bTemplate;             // mesh used as template only (not for rendering)
	bool bModulateMatAlpha;     // mix material and texture alpha channels

	// global mesh flags
	static bool bEnableSpecular;   // enable specular reflection
	static inline Shader *meshShader;
};

#endif // !__OGLMESH_H
