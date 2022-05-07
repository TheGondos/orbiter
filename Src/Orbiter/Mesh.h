// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// Classes to support meshes under D3D:
// Triangle, Mesh
// =======================================================================

#ifndef __MESH_H
#define __MESH_H

#define OAPI_IMPLEMENTATION
#undef min
#undef max
#include <iostream>
#include "OrbiterAPI.h"
#include <glm/glm.hpp>

typedef char Str256[256];

const uint32_t SPEC_DEFAULT = (uint32_t)-1; // "default" material/texture flag
const uint32_t SPEC_INHERIT = (uint32_t)-2; // "inherit" material/texture flag

// =======================================================================
// Class Triangle
// triangular surface patch

class Triangle {
public:
	Triangle ();
	Triangle (const Triangle &tri);
	Triangle (int n0, int n1, int n2);

	void SetNodes (int n0, int n1, int n2);

	int nd[3];  // node index list
	int nm[3];  // normal index list

	bool hasNodes;   // true if nd[] is valid
	bool hasNormals; // true if nm[] is valid
};

// =======================================================================
// mesh group descriptor

typedef struct {
	NTVERTEX  *Vtx;
	uint16_t      *Idx;
	int     nVtx;
	int     nIdx;
	uint32_t     MtrlIdx;
	uint32_t     TexIdx;
	int     UsrFlag;
	int16_t      zBias;
	uint16_t      Flags;
	uint32_t    TexIdxEx[MAXTEX];
	float     TexMixEx[MAXTEX];
} GroupSpec;

// =======================================================================
// Class Mesh

class Mesh {
public:
	Mesh ();
	// Create an empty mesh

	Mesh (NTVERTEX *vtx, int nvtx, uint16_t *idx, int nidx,
		int matidx = SPEC_DEFAULT, int texidx = SPEC_DEFAULT);
	// Create a single-group mesh

	Mesh (const Mesh &mesh);
	// copy constructor

	~Mesh ();

	void Set (const Mesh &mesh);

	void Setup ();
	// call after all groups are assembled or whenever groups change,
	// to set up group parameters

	int GetFlags () const { return Flags; }
	void SetFlags (int flags) { Flags = flags; }

	void SetupGroup (int grp);
	// Re-apply setup for a particular group (e.g. after transformation)

	inline int nGroup() const { return nGrp; }
	// Number of groups

	inline int nMaterial() const { return nMtrl; }
	// Number of materials

	inline int nTexture() const { return nTex; }
	// Number of textures

	inline GroupSpec *GetGroup (int grp) { return (grp < nGrp ? Grp+grp : 0); }
	// return a pointer to the group specification for group grp

	inline int GetGroupUsrFlag (int grp) const { return (grp < nGrp ? Grp[grp].UsrFlag : 0); }
	// return the user-defined flag for group grp

	int AddGroup (NTVERTEX *vtx, int nvtx, uint16_t *idx, int nidx,
		int mtrl_idx = SPEC_INHERIT, int tex_idx = SPEC_INHERIT,
		int16_t zbias = 0, int flag = 0, bool deepcopy = false);
	// Add new group to the mesh and return its group index
	// The lists are handled by the mesh and should not be released by
	// the calling program

	bool AddGroupBlock (int grp, const NTVERTEX *vtx, int nvtx, const uint16_t *idx, int nidx);
	// Add geometry (vertices and indices) to an existing group.
	// Indices (idx) are zero-based. When adding them to the group, index
	// offsets are added automatically.

	bool DeleteGroup (int grp);
	// Delete group 'grp'. Note that the indices of the other
	// groups may change as a result.
	// Return value is false if grp index is out of range

	int GetGroup (int grp, GROUPREQUESTSPEC *grs);
	// retrieve vertex and index data (deep copy)

	int EditGroup (int grp, GROUPEDITSPEC *ges);
	// edit/replace parts of the group

	void AddMesh (Mesh &mesh);
	// Merge "mesh" into "this", by adding all groups of "mesh"
	// Currently this does not use the materials and textures of "mesh"

	inline MATERIAL *GetMaterial (int matidx)
	{ return ((unsigned)matidx < nMtrl ? Mtrl+matidx : 0); }
	// return a material pointer

	int AddMaterial (MATERIAL &mtrl);
	// Add new material to the mesh and return its list index

	bool DeleteMaterial (int matidx);
	// Delete material with index 'matidx' from the list. Any groups
	// using that material are reset to material 0. Any group material
	// indices > matidx are decremented to account for changed list

	int AddTexture (SURFHANDLE tex);
	// Add new texture to the mesh and return its list index
	// If the texture exists already, it is not added again, but the index
	// of the existing texture is returned

	bool SetTexture (int texidx, SURFHANDLE tex, bool release_old = true);
	// replace a texture

	inline SURFHANDLE GetTexture (int texidx)
	{ return (texidx < nTex ? (SURFHANDLE)Tex[texidx] : 0); }
	// return a texture pointer

	void SetTexMixture (int grp, int ntex, float mix);
	void SetTexMixture (int ntex, float mix);

	void ScaleGroup (int grp, float sx, float sy, float sz);
	void Scale (float sx, float sy, float sz);
	// scale an individual group or the whole mesh

	void TranslateGroup (int grp, float dx, float dy, float dz);
	void Translate (float dx, float dy, float dz);
	// translate an individual group or the whole mesh

	enum RotAxis { ROTATE_X, ROTATE_Y, ROTATE_Z };
	void RotateGroup (int grp, RotAxis axis, float angle);
	void Rotate (RotAxis axis, float angle);
	// rotate the mesh 'angle' rad around a coordiate axis

	void TransformGroup (int grp, const glm::fmat4 &mat);
	void Transform (const glm::fmat4 &mat);
	// rotate mesh using the provided rotation matrix

	void TexScaleGroup (int grp, float su, float sv);
	void TexScale (float su, float sv);
	// scale the texture coordinates of an individual group or the whole mesh

	void CalcNormals (int grp, bool missingonly);
	// automatic calculation of vertex normals for group grp
	// if missingonly=true then only normals with zero length are calculated

	void CalcTexCoords (int grp);
	// under construction

	void Clear ();

	static void GlobalEnableSpecular (bool enable);
	void EnableMatAlpha (bool enable);

	friend std::istream &operator>> (std::istream &is, Mesh &mesh);
	// read mesh from file

	friend std::ostream &operator<< (std::ostream &os, const Mesh &mesh);
	// write mesh to file

protected:
	void ReleaseTextures ();
	// Release textures acquired by the mesh

private:
	int nGrp;         // number of groups
	GroupSpec *Grp;     // list of group specs	

	int nMtrl;        // number of materials
	MATERIAL *Mtrl; // list of materials used by the mesh

	int nTex;         // number of textures
	SURFHANDLE *Tex;    // list of textures used by the mesh

	bool GrpSetup;      // true if the following arrays are allocated
	glm::fvec3 *GrpCnt;  // list of barycentres for each group (local coords)
	float *GrpRad;   // list of max. radii for each group
	int *GrpVis;      // visibility flags for each group

	// global mesh flags
	static bool bEnableSpecular;   // enable specular reflection
	bool bModulateMatAlpha;
	// modulate material alpha with texture alpha (if disabled, any groups
	// that use textures ignore the material alpha values)
	int Flags;        // bit | effect
	                    // 0   | set: mesh casts shadow (used for base object meshes)
						// 1   | set: use global shadow flag (bit 0) for all groups; unset: use individual group flags
};

// =======================================================================
// Class MeshManager: globally managed meshes

class MeshManager {
public:
	MeshManager();
	~MeshManager();
	void Flush();

	const Mesh *LoadMesh (const char *fname, bool *firstload = NULL);
	// Load a mesh from file (or just return a handle if loaded already.
	// If firstload is used, it is set to true if the mesh was loaded from
	// file, and false if the mesh was in memory already

private:
	struct MeshBuffer {
		Mesh *mesh;
		uint64_t crc;
		char fname[32];
	} *mlist;
	int nmlist, nmlistbuf;
};

// =======================================================================
// Nonmember functions

bool LoadMesh (const char *meshname, Mesh &mesh);
// Load an unmanaged mesh (caller is responsible for deleting after use)
// meshname is relative to MeshPath directory.
// Returns true if mesh was loaded, false if not found.

void CreateSpherePatch (Mesh &mesh, int nlng, int nlat, int ilat, int res,
	int bseg = -1, bool reduce = true, bool outside = true);
// Create a mesh representing a rectangular patch on a sphere at a given
// position and resolution

#endif // !__MESH_H