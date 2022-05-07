// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OpenGL Client module
// ==============================================================

// ==============================================================
// TileMgr.h
// class TileManager (interface)
//
// Planetary surface rendering management, including a simple
// LOD (level-of-detail) algorithm for surface patch resolution.
// ==============================================================

#ifndef __TILEMGR_H
#define __TILEMGR_H

#include "MeshManager.h"
#include "SphereMesh.h"
#include <glm/glm.hpp>

#include <mutex>
#include <thread>
#include <condition_variable>

#define MAXQUEUE 10

#pragma pack(push,1)
	struct TILEFILESPEC {
		uint32_t sidx;       // index for surface texture (-1: not present)
		uint32_t midx;       // index for land-water mask texture (-1: not present)
		uint32_t eidx;       // index for elevation data blocks (not used yet; always -1)
		uint32_t flags;      // tile flags: bit 0: has diffuse component; bit 1: has specular component; bit 2: has city lights
		uint32_t subidx[4];  // subtile indices
	};

struct LMASKFILEHEADER { // file header for contents file at level 1-8
	char id[8];          //    ID+version string
	uint32_t hsize;         //    header size
	uint32_t flag;          //    bitflag content information
	uint32_t npatch;        //    number of patches
	uint8_t minres;         //    min. resolution level
	uint8_t maxres;         //    max. resolution level
};
#pragma pack(pop)

struct TILEDESC {
	VertexBuffer *vtx;
	union {
		uint32_t idx;
		OGLTexture *obj;      // diffuse surface texture
	} tex;
	union {
		uint32_t idx;
		OGLTexture *obj;     // landmask texture, if applicable
	} ltex;
	int flag;
	struct TILEDESC *subtile[4];   // sub-tiles for the next resolution level
	int ofs;                     // refers back to the master list entry for the tile
};

typedef struct {
	float tumin, tumax;
	float tvmin, tvmax;
} TEXCRDRANGE;

class VPlanet;
class TileBuffer;

class TileManager {
	friend class TileBuffer;

public:
	TileManager (const VPlanet *vplanet);
	virtual ~TileManager ();

	static void GlobalInit ();
	static void GlobalExit ();
	// One-time global initialisation/exit methods

	inline int GetMaxLevel () const { return maxlvl; }

	virtual void SetMicrotexture (const char *fname);
	virtual void SetMicrolevel (double lvl);

	virtual void Render (glm::mat4 &wmat, double scale, int level, double viewap = 0.0, bool bfog = false);

protected:
	void RenderSimple (int level, TILEDESC *tile);

	void ProcessTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, TILEDESC *tile,
		const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, int flag,
		const TEXCRDRANGE &bkp_range, OGLTexture *bkp_tex, OGLTexture *bkp_ltex, int bkp_flag);

	virtual void RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, double sdist, TILEDESC *tile,
		const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, int flag) = 0;

	bool LoadPatchData ();
	// load binary definition file for LOD levels 1-8

	bool LoadTileData ();
	// load binary definition file for LOD levels > 8

	bool AddSubtileData (TILEDESC &td, TILEFILESPEC *tfs, int idx, int sub, int lvl);
	// add a high-resolution subtile specification to the tree

	void LoadTextures (char *modstr = 0);
	// load patch textures for all LOD levels

	void PreloadTileTextures (TILEDESC *tile8, int ntex, int nmask);
	// Pre-load high-resolution tile textures for the planet (level >= 9)

	void AddSubtileTextures (TILEDESC *td, OGLTexture **tbuf, int nt, OGLTexture **mbuf, int nm);
	// add a high-resolution subtile texture to the tree

	void LoadSpecularMasks ();
	// load specular and night light textures

	VECTOR3 TileCentre (int hemisp, int ilat, int nlat, int ilng, int nlng);
	// direction to tile centre from planet centre in planet frame

	void TileExtents (int hemisp, int ilat, int nlat, int ilg, int nlng, double &lat1, double &lat2, double &lng1, double &lng2);

	bool TileInView (int lvl, int ilat);
	// checks if a given tile is observable from camera position

	void SetWorldMatrix (int ilng, int nlng, int ilat, int nlat);
	// set the world transformation for a particular tile

	bool SpecularColour (glm::vec4 *col);
	// adjust specular reflection through atmosphere

	const VPlanet *vp;               // the planet visual
	OBJHANDLE obj;                   // the planet object
	char *objname;                   // the name of the planet (for identifying texture files)
	int tilever;                   // file version for tile textures
	int maxlvl;                      // max LOD level
	int maxbaselvl;                  // max LOD level, capped at 8
	int ntex;                      // total number of loaded textures for levels <= 8
	int nhitex;                    // number of textures for levels > 8
	int nhispec;                   // number of specular reflection masks (level > 8)
	double lightfac;                 // city light intensity factor
	double microlvl;                 // intensity of microtexture
	int nmask;                     // number of specular reflection masks/light maps (level <= 8)
	VECTOR3 pcdir;                   // previous camera direction
	static glm::mat4 Rsouth;         // rotation matrix for mapping tiles to southern hemisphere
	float spec_base;                 // base intensity for specular reflections
	const ATMCONST *atmc;            // atmospheric parameters (used for specular colour modification)
	int bPreloadTile;                // pre-load surface tile textures

	TILEDESC *tiledesc;              // tile descriptors for levels 1-8
	static TileBuffer *tilebuf;      // subtile manager

	OGLTexture **texbuf;     // texture buffer for surface textures (level <= 8)
	OGLTexture **specbuf;    // texture buffer for specular masks (level <= 8);
	OGLTexture *microtex;    // microtexture overlay

	// object-independent configuration data
	static bool bGlobalSpecular;     // user wants specular reflections
	static bool bGlobalRipple;       // user wants specular microtextures
	static bool bGlobalLights;       // user wants planet city lights

	// tile patch templates
	static VBMESH PATCH_TPL_1;
	static VBMESH PATCH_TPL_2;
	static VBMESH PATCH_TPL_3;
	static VBMESH PATCH_TPL_4[2];
	static VBMESH PATCH_TPL_5;
	static VBMESH PATCH_TPL_6[2];
	static VBMESH PATCH_TPL_7[4];
	static VBMESH PATCH_TPL_8[8];
	static VBMESH PATCH_TPL_9[16];
	static VBMESH PATCH_TPL_10[32];
	static VBMESH PATCH_TPL_11[64];
	static VBMESH PATCH_TPL_12[128];
	static VBMESH PATCH_TPL_13[256];
	static VBMESH PATCH_TPL_14[512];
	static VBMESH *PATCH_TPL[15];
	static int patchidx[9];          // texture offsets for different LOD levels
	static int NLAT[9];
	static int NLNG5[1], NLNG6[2], NLNG7[4], NLNG8[8], *NLNG[9];
	static int vpX0, vpX1, vpY0, vpY1; // viewport boundaries

	static int vbMemCaps;          // video/system memory flag for vertex buffers

	struct RENDERPARAM {
		glm::mat4 wmat;              // world matrix
		glm::mat4 wmat_tmp;          // copy of world matrix used as work buffer
		glm::mat4 wtrans;
		int tgtlvl;                  // target resolution level
		MATRIX3 grot;                // planet rotation matrix
		VECTOR3 cpos;                // planet offset vector (in global frame)
		VECTOR3 sdir;                // sun direction from planet centre (in planet frame)
		VECTOR3 cdir;                // camera direction from planet centre (in planet frame)
		double cdist;                // camera distance from planet centre (in units of planet radii)
		double viewap;               // aperture of surface cap visible from camera pos
		double objsize;              // planet radius
		bool bfog;                   // distance fog flag
	} RenderParam;

	friend void ApplyPatchTextureCoordinates (VBMESH &mesh, VertexBuffer *vtx, const TEXCRDRANGE &range);
};
void ApplyPatchTextureCoordinates (VBMESH &mesh, VertexBuffer *vtx, const TEXCRDRANGE &range);

// =======================================================================
// Class TileBuffer: Global resource; holds a collection of
// tile specifications across all planets

class TileBuffer {
public:
	TileBuffer ();
	~TileBuffer ();
	TILEDESC *AddTile ();
	void DeleteSubTiles (TILEDESC *tile);

	friend void ClearVertexBuffers (TILEDESC *td);
	// Recursively remove subrange vertex buffers from a tile tree with
	// root td. This is necessary when a new tile has been loaded, because
	// this can change the subrange extents for child tiles.

	bool LoadTileAsync (const char *name, TILEDESC *tile);
	// load the textures for a tile for planet 'name', given by descriptor
	// 'tile', using a separate thread.
	// Returns false if request can't be entered (queue full, or request
	// already present)

	static std::mutex hQueueMutex;
	static std::condition_variable hCV;

private:
	bool DeleteTile (TILEDESC *tile);

	static int LoadTile_ThreadProc (TileBuffer *);
	// the thread function loading tile textures on demand

	bool bLoadMip;  // load mipmaps for tiles if available
	std::thread hLoadThread;
	static bool bRunThread;
	static int nqueue, queue_in, queue_out;
	int nbuf;     // buffer size;
	int nused;    // number of active entries
	int last;     // index of last activated entry
	TILEDESC **buf; // tile buffer

	static struct QUEUEDESC {
		const char *name;
		TILEDESC *td;
	} loadqueue[MAXQUEUE];
};

#endif // !__TILEMGR_H