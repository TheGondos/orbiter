// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#ifndef __BASEOBJ_H
#define __BASEOBJ_H

#include "D3dmath.h"

struct VERTEX_XYZ   { float x, y, z; };                   // transformed vertex

#define OBJSPEC_EXPORTMESH         0x0001 // object exports mesh
#define OBJSPEC_EXPORTVERTEX       0x0002 // object exports vertex groups
#define OBJSPEC_EXPORTSHADOW       0x0004 // object exports its shadows (or parts of it)
#define OBJSPEC_RENDERBEFORESHADOW 0x0008 // object wants to render itself completely or partly before shadows
#define OBJSPEC_RENDERAFTERSHADOW  0x0010 // object wants to render itself completely or partly after shadows
#define OBJSPEC_RENDERSHADOW       0x0020 // object wants to render its own shadow completely or partly
#define OBJSPEC_UPDATEVERTEX       0x0040 // object needs to update itself dynamically
#define OBJSPEC_LPAD               0x0080 // object is a landing pad
#define OBJSPEC_RWY                0x0100 // object is a runway
#define OBJSPEC_UNDERSHADOW        0x0200 // render under shadows (can be overridden by individual groups)
#define OBJSPEC_EXPORTSHADOWMESH   0x0400
#define OBJSPEC_OWNSHADOW          0x0800 // meshobjects only: use mesh group flags for selecting shadow
#define OBJSPEC_WRAPTOSURFACE      0x1000 // meshobjects only: wrap mesh to elevated surface (e.g. taxiways)

// ======================================================================================
// Atomic objects for surface bases

class Base;
class Mesh;

class BaseObject {
public:
	BaseObject (const Base *_base);

	virtual int Read (std::istream &is);
	// read object description from stream and return error flag (0=ok)

	virtual int ParseLine (const char *label, const char *value) { return 0; }
	// interpret a label/value pair read from the object description

	virtual int GetSpecs() const { return 0; }

	virtual void Setup();
	// Initialisation after reading (but before activation)

	virtual void Activate () {};              // initialise object
	virtual void Deactivate() {};             // hibernate

	int nGroup() { return ngrp; }
	// number of exported vertex groups

	virtual bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow) { return false; }
	// Returns specs for exported group 'grp': length of vertex and index lists,
	// texture id and flag to indicate whether group should be rendered before shadows
	// Only objects which set OBJSPEC_EXPORTVERTEX need to implement this

	virtual void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs) {}
	// Allow the object to export its groups into vertex list vtx and index list idx
	// On call, idx_ofs is the current total length of the index list and should be
	// updated by the function
	// Only objects which set OBJSPEC_EXPORTVERTEX need to implement this

	virtual Mesh *ExportMesh () { return NULL; }
	// Allow the object to export its visual as a mesh.

	virtual Mesh *ExportShadowMesh (double &elev) { elev = 0.0; return NULL; }

	virtual bool GetShadowSpec (int &nvtx, int &nidx)
	{ return false; }
	// Returns specs for exported shadow mesh
	// Only objects which set OBJSPEC_EXPORTSHADOW need to implement this

	virtual void ExportShadow (VERTEX_XYZ *vtx, uint16_t *idx) {}
	// Allow the object to export its shadow (or part of it).
	// Note that vtx need not be initialised at this point, but objects should store
	// the pointer for use in UpdateShadow
	// Only objects which set OBJSPEC_EXPORTSHADOW need to implement this

	virtual void Update () {}
	// allow object to update itself (but not its shadows). Only needs to be implemented
	// for objects which set OBJSPEC_UPDATEVERTEX

	const Vector &EquPos () const { return equpos; }
	// Returns the equatorial position of the reference point
	// The radius value includes ground elevation
	// x=lng [rad], y=lat [rad], z=radius [m]

	double ElevCorrection (double px, double pz);
	// Calculate elevation (y) correction as a result of sphere curvature
	// for position px, pz.

	void MapToCurvature (NTVERTEX *vtx, int nvtx);
	// map the vertices in vtx from a flat plane onto the planet curvature, given their position
	// and the radius of the planet. This maps on a vertex basis, not the whole object.

	void MapToAltitude (NTVERTEX *vtx, int nvtx);
	// map the vertices in vtx from a spherical surface to actual elevated surface, given their
	// position. This maps on a vertex basis, not the object as a whole

	static BaseObject *Create (const Base *_base, std::istream &is);
	// read a new BaseObject from a stream

protected:
	void ParseError (const char *msg) const;
	// Log a parse error

	const Base *base; // pointer to associated surface base
	int ngrp;         // number of mesh groups used by the object
	Vector relpos;    // object position relative to basis frame
	Vector equpos;    // object position in equatorial frame: x=lng [rad], y=lat [rad], z=radius [m]
	Vector scale;     // object scaling factors
	double rot;       // rotation around vertical axis
	double elev;      // elevation of reference point above mean radius [m]
	double yofs;      // vertical translation of reference point against base reference
};

// ======================================================================================
// Generic mesh

class MeshObject: public BaseObject {
public:
	MeshObject (const Base *_base);
	~MeshObject ();
	int ParseLine (const char *label, const char *value);
	int Read (std::istream &is);
	int GetSpecs() const { return specs; }
	void Setup();
	void Activate ();
	void Deactivate();
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportMesh ();
	Mesh *ExportShadowMesh (double &elev);
	
private:
	bool LoadMesh (char *fname); // load mesh into local buffer
	int specs;      // object specs as returned by GetSpecs()
	char *fname;      // mesh file name
	uint64_t texid;   // overall texture
	bool undersh;     // render mesh before shadows?
	bool preload;     // load mesh at program start?
	bool rendershadow;
	bool ownmat;
	float ax, az;
	Mesh *mesh;	
};

// ======================================================================================
// "5-sided block" (no floor) used as simple building etc.

class Block: public BaseObject {
public:
	Block (const Base *_base);
	~Block ();
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOWMESH; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportShadowMesh (double &elev);
	void Activate ();
	void Deactivate ();

private:
	uint64_t  texid[3];   // texture ids
	float  tuscale[3], tvscale[3]; // texture scaling factors
	struct DYNDATA {
		float *databuf;    // some geometry data
		//D3DVERTEX *gv0;       // location of exported vertices for group 0
		//VERTEX_XYZ *shvtx;    // location of exported shadow
	} *dyndata;               // lives only during activation
};                        

// ======================================================================================
// class Hangar: a block with a barrel roof

class Hangar: public BaseObject {
public:
	Hangar (const Base *_base);
	~Hangar ();
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOWMESH; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportShadowMesh (double &elev);
	void Activate ();
	void Deactivate ();

private:
	uint64_t  texid[3];   // texture ids
	float  tuscale[3], tvscale[3]; // texture scaling factors
	struct DYNDATA {
		NTVERTEX *Vtx;       // block vertices
		//VERTEX_XYZ *shvtx;    // location of exported shadow
	} *dyndata;
};

// ======================================================================================
// class Hangar2: a block with a tent-shaped roof

class Hangar2: public BaseObject {
public:
	Hangar2 (const Base *_base);
	~Hangar2 ();
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOWMESH; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportShadowMesh (double &elev);
	void Activate ();
	void Deactivate ();

private:
	float  roofh;      // roof height from base to ridge
	uint64_t  texid[3];   // texture ids
	float  tuscale[3], tvscale[3]; // texture scaling factors
	struct DYNDATA {
		NTVERTEX *Vtx;      // block vertices
		//VERTEX_XYZ *shvtx;   // location of exported shadow
	} *dyndata;
};

// ======================================================================================
// class Hangar3: hangar-type building with barrel roof reaching to the ground
// and recessed entry

class Hangar3: public BaseObject {
public:
	Hangar3 (const Base *_base);
	~Hangar3 ();
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOWMESH; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportShadowMesh (double &elev);
	void Activate ();
	void Deactivate ();

private:
	uint64_t  texid[3];   // texture ids
	float  tuscale[3], tvscale[3]; // texture scaling factors
	struct DYNDATA {
		NTVERTEX *Vtx;      // block vertices
		//VERTEX_XYZ *shvtx;   // location of exported shadow
	} *dyndata;
};

// ======================================================================================
// class Tank: a vertical cylinder without a base face

class Tank: public BaseObject {
public:
	Tank (const Base *_base);
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOWMESH; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	Mesh *ExportShadowMesh (double &elev);
	void Activate ();
	void Deactivate ();

private:
	uint64_t  texid[2];   // texture ids
	float  tuscale[2], tvscale[2]; // texture scaling factors (mantle and top)
	int     nstep;      // segments for circle
	struct DYNDATA {
		NTVERTEX *Vtx;      // block vertices
	} *dyndata;
};

// ======================================================================================
// class Lpad: base class for landing pads

class Lpad: public BaseObject {
public:
	Lpad (const Base *_base);
	void SetPadno (int no) { padno = no; }
	int GetPadno () const { return padno; }
	const Vector &GetPos () const { return relpos; }
	const float GetILSfreq () const { return ILSfreq; }

protected:
	int padno;         // pad number
	float ILSfreq;       // frequency of pad's VTOL ILS signal transmitter [MHz]
};

// ======================================================================================
// class Lpad01: octagonal landing pad

class Lpad01: public Lpad {
public:
	Lpad01 (const Base *_base);
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_LPAD; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);

private:
	uint64_t texid;      // texture id
	static NTVERTEX Vtx[44]; // vertex template
	static uint16_t Idx[144];     // index template
};

// ======================================================================================
// class Lpad02: square landing pad

class Lpad02: public Lpad {
public:
	Lpad02 (const Base *_base);
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_LPAD; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);

private:
	uint64_t texid;      // texture id
	static NTVERTEX Vtx[28]; // vertex template
	static uint16_t Idx[60];      // index template
};

// ======================================================================================
// class Lpad02a: square landing pad (variation)

class Lpad02a: public Lpad {
public:
	Lpad02a (const Base *_base);
	int ParseLine (const char *label, const char *value);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_LPAD; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);

private:
	uint64_t texid;      // texture id
	static NTVERTEX Vtx[37]; // vertex template
	static uint16_t Idx[96];      // index template
};

// ======================================================================================
// class BeaconArray: line of emissive spheres (implemented as billboards) for
// runway lighting etc.

class BeaconArray: public BaseObject {
public:
	BeaconArray (const Base *_base);
	int GetSpecs() const { return OBJSPEC_RENDERAFTERSHADOW | OBJSPEC_UPDATEVERTEX; }
	int Read (std::istream &is);
	void Update ();
	void Activate ();
	void Deactivate();

private:
	glm::fvec3 end1, end2;
	int count;
	double size;
	Vector *Pos;
	POSTEXVERTEX *Vtx;
	uint16_t *Idx;
	int nVtx, nIdx;
	SURFHANDLE tex;  // texture to use
	float col_r, col_g, col_b; // beacon colour
	MATERIAL *lightmat;
};

// ======================================================================================
// class Runway: mesh for runway (no lighting)

class Runway: public BaseObject {
	friend class Base;
public:
	Runway (const Base *_base);
	~Runway ();
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_RWY; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	int Read (std::istream &is);
	void Activate ();
	void Deactivate();
	const float GetILSfreq (int i) const { return ILSfreq[i]; }

private:
	glm::fvec3 end1, end2;
	float ILSfreq[2];
	int nrwseg;      // number of texture segments in runway mesh
	struct RWSEG {                   // runway segment specs
		int subseg;                // number of sub-segments
		float len;                // segment length (1=full runway)
		float tu0, tu1, tv0, tv1; // segment texture coordinates
	} *rwseg;
	float width;    // runway (half-)width [m]
	uint64_t texid[1]; // texture ids
	struct DYNDATA {
		int nRwVtx;        // number of vertices for runway mesh
		int nRwIdx;        // number of indices for runway mesh
		NTVERTEX *RwVtx;    // list of runway vertices
		uint16_t *RwIdx;         // list of runway indices
	} *dyndata;
};

// ==============================================================================
// class RunwayLights: standard lighting for runways (using the same techniques
// as BeaconArray)

class RunwayLights: public BaseObject {
public:
	RunwayLights (const Base *_base);
	~RunwayLights ();
	void Setup ();
	int GetSpecs() const { return OBJSPEC_RENDERAFTERSHADOW | OBJSPEC_UPDATEVERTEX; }
	int Read (std::istream &is);
	void Update ();
	void Activate ();
	void Deactivate ();

private:
	void VertexArray (int count, const Vector &cpos, const Vector &pos, const Vector &ofs, double size, POSTEXVERTEX *&Vtx);
	// Calculates the billboard vertices for an array of 'count' beacons, starting at 'pos', each offset from the previous
	// by 'ofs'. cpos is camera position, 'size' is beacon size, 'Vtx' is the result
	// The Vtx pointer is moved past the end of the current list on return

	glm::fvec3 end1, end2;
	int count1;   // number of side line beacons
	float width; // runway (half-)width [m]
	struct VASIDATA {                 // parameters for Visual Approach Slope Indicator (VASI)
		float apprangle;              //    designated approach angle
		float lightsep;               //    separation between red and white indicator lights
		float ofs;                    //    offset of red lights from runway end
	} *vasi;
	struct PAPIDATA {                 // parameters for Precision Approach Path Indicator (PAPI)
		float apprangle;              //    designated approach angle
		float aperture;               //    aperture (angular sensitivity)
		float ofs;                    //    offset from runway end
	} *papi;
	struct DYNDATA {
		int nb_centre, nb_side, nb_approach, nb_end; // number of lights: runway centre, runway side, approach path, runway end
		int nb_vasi_w, nb_vasi_r;                    // number of white and red VASI approach indicators (if any)
		int nb_tot, nb_white_night, nb_white_day, nb_red; // number of lights: total, white, red
		Vector ref1; // reference point 1 (=end1)
		Vector ref2; // reference point 2 (=end2)
		Vector dir;  // unit vector from ref1 to ref2
		Vector nml;  // horizontal unit vector perpendicular to dir
		Vector ofs1; // offset vector between centerline lights
		Vector ofs3; // offset from centerline to runway edge
		Vector ofs4; // offset from centerline to VASI white lights
		Vector ofs5; // offset from centerline to VASI red lights
		double vasi_ry, vasi_wy; // VASI red/white light elevation
		int flashpos; // position of flashing approach light
		double flashtime; // time of next strobe light change
		const double *csun; // pointer to cosine of sun altitude in parent vbase
		bool night;       // skip runway center and side lines during daytime
		POSTEXVERTEX *Vtx, *Vtx_white_night, *Vtx_white_day, *Vtx_red, *Vtx_PAPI;
		uint16_t *Idx, *Idx_white_night, *Idx_white_day, *Idx_red, *Idx_PAPI_w, *Idx_PAPI_r;
		int nVtx, nVtx_white_night, nVtx_white_day, nVtx_red;
		int nIdx, nIdx_white_night, nIdx_white_day, nIdx_red;
		int PAPIwhite;
		SURFHANDLE tex;
	} *dyndata;
};

// ======================================================================================
// class Train: base class for train-type objects (including straight track)

class Train: public BaseObject {
public:
	Train (const Base *_base);

protected:
	void Init (const glm::fvec3 &_end1, const glm::fvec3 &_end2);
	void Setup ();

	void SetCabin (int nvtx, const NTVERTEX *ref, NTVERTEX *res,
		const glm::fvec3 &pos, const glm::fvec3 &ofs);
	// Place cabin, defined by vertices 'ref' at position 'pos' (relative
	// to end1) and return transformed vertices in 'res'
	// 'ofs' is a translation applied up front

	float MoveCabin (float &pos, float &vel, bool &atmin);
	// update cabin position 'pos' and velocity 'vel' and return
	// the displacement. if 'atmin' is true on exit, the cabin is
	// at minpos

	glm::fvec3 end1, end2;    // track terminal points in local base coords
	glm::fvec3 dir;           // direction from end1 to end2
	float length;         // distance between end1 and end2
	float minpos, maxpos; // movement limits for train along track
	float maxspeed;       // max speed [m/s]
	float slowzone;       // distance over which train decelerates at ends

private:
	float speedfac;       // aux for cabin movement
};

// ======================================================================================
// class Train1: monorail-type train

class Train1: public Train {
public:
	Train1 (const Base *_base);
	~Train1 ();
	int Read (std::istream &is);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOW | OBJSPEC_UPDATEVERTEX; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &_texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	bool GetShadowSpec (int &nvtx, int &nidx);
	void ExportShadow (VERTEX_XYZ *vtx, uint16_t *idx);
	void Update ();
	void Activate ();
	void Deactivate ();

private:
	float cpos, cvel;          // cabin position (from p1) and speed
	uint64_t texid;               // texture id for cabin and track
	float tuscale_track;       // texture scaling (for track only)
	NTVERTEX *cabinvtx;           // pointer to exported cabin vertices
	VERTEX_XYZ *shvtx;            // pointer to exported shadow vertices
	struct DYNDATA {
		NTVERTEX *Vtx;            // cabin and track vertices
		SURFHANDLE tex; // texture to use
		int tick;                 // render counter
	} *dyndata;
};

// ======================================================================================
// class Train2: hangrail-type train

class Train2: public Train {
public:
	Train2 (const Base *_base);
	~Train2 ();
	int Read (std::istream &is);
	int GetSpecs() const { return OBJSPEC_EXPORTVERTEX | OBJSPEC_EXPORTSHADOW |
		OBJSPEC_RENDERAFTERSHADOW | OBJSPEC_RENDERSHADOW | OBJSPEC_UPDATEVERTEX; }
	bool GetGroupSpec (int grp, int &nvtx, int &nidx, uint64_t &_texid,
		bool &undershadow, bool &groundshadow);
	void ExportGroup (int grp, NTVERTEX *vtx, uint16_t *idx, int &idx_ofs);
	bool GetShadowSpec (int &nvtx, int &nidx);
	void ExportShadow (VERTEX_XYZ *vtx, uint16_t *idx);
	void Update ();
	void Activate ();
	void Deactivate ();

private:
	float height;            // height of supports over ground
	uint64_t texid;             // texture id for cabin and track
	float tuscale_track;     // texture scaling (for track only)
	NTVERTEX *cabinvtx[2];     // pointers to exported cabin vertices
	VERTEX_XYZ *shvtx;          // pointer to exported shadow vertices
	struct DYNDATA {
		float cpos[2];       // cabin positions (from end1)
		float cvel[2];		// cabin speeds
		NTVERTEX *rail;        // girder and traverse vertices
		VERTEX_XYZ *rshvtx;     // pointer to rail shadow
		SURFHANDLE dtex, ntex; // textures to use
		uint16_t ng;                // number of girders
		int tick;               // render counter
	} *dyndata;
};

// ======================================================================================
// SolarPlant: array of solar panels, aligning themselves with the sun

class SolarPlant: public BaseObject {
public:
	SolarPlant (const Base *_base);
	~SolarPlant ();
	int Read (std::istream &is);
	int GetSpecs() const { return OBJSPEC_RENDERAFTERSHADOW | OBJSPEC_RENDERSHADOW | OBJSPEC_UPDATEVERTEX; }
	void Update ();
	void Activate ();
	void Deactivate();

private:
	glm::fvec3 pos;             // position of footprint centre in local base coords
	float scale;            // scale factor for panels
	float rot;              // rotation around vertical axis
	float sepx, sepz;       // panel spacing in x and z direction [m]
	int nrow, ncol;            // number of rows and columns of panel matrix
	int npanel;                // number of panels in the plant
	uint64_t texid;            // texture id
	float tuscale, tvscale; // texture scaling factors
	glm::fvec3 *ppos;           // reference positions for each panel
	NTVERTEX *Vtx;            // panel vertices
	VERTEX_XYZ *ShVtx;         // shadow vertices
	uint16_t *Idx, *ShIdx;         // panel and shadow mesh indices
	int nVtx, nShVtx;        // number of vertices
	int nIdx, nShIdx;        // number of indices
	bool *flash;               // panel flashing in sunlight?
	SURFHANDLE tex;  // texture to use
	Vector nml;                // pointing direction of panels (towards sun)
	//VBase *vbase;              // associated base visual

	double updT;             // time of next direction update
	bool have_shadows;       // draw panel shadows?
};


#endif // !__BASEOBJ_H