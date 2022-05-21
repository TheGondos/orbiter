// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OGL Client module
// ==============================================================

// ==============================================================
// Particle.h
// Particle system for exhaust contrails
// ==============================================================

#ifndef __PARTICLE_H
#define __PARTICLE_H

#include "OGLClient.h"
#include "VertexBuffer.h"

#define MAXPARTICLE 3000
class OGLCamera;

struct ParticleSpec {
	VECTOR3 pos, vel;
	double size;
	double alpha0;  // alpha value at creation
	double t0;
	int texidx;
	uint32_t flag;
	ParticleSpec *prev, *next;
};

struct NTAVERTEX {
	float x;     ///< vertex x position
	float y;     ///< vertex y position
	float z;     ///< vertex z position
	float nx;    ///< vertex x normal
	float ny;    ///< vertex y normal
	float nz;    ///< vertex z normal
	float tu;    ///< vertex u texture coordinate
	float tv;    ///< vertex v texture coordinate
	float alpha;
};

struct TAVERTEX {
	float x;     ///< vertex x position
	float y;     ///< vertex y position
	float z;     ///< vertex z position
	float tu;    ///< vertex u texture coordinate
	float tv;    ///< vertex v texture coordinate
	float alpha;
};


class OGLParticleStream: public oapi::ParticleStream {
public:
	OGLParticleStream (oapi::GraphicsClient *_gc, PARTICLESTREAMSPEC *pss = 0);
	virtual ~OGLParticleStream();

	static void GlobalInit ();
	static void GlobalExit ();

	static std::unique_ptr<VertexArray> eVBA;
	static std::unique_ptr<VertexBuffer> eVBO;
	static std::unique_ptr<IndexBuffer> IBO;
	static std::unique_ptr<VertexArray> dVBA;
	static std::unique_ptr<VertexBuffer> dVBO;

	void SetObserverRef (const VECTOR3 *cam);
	void SetSourceRef (const VECTOR3 *src);
	void SetSourceOffset (const VECTOR3 &ofs);
	void SetIntensityLevelRef (double *lvl);

	void Activate (bool _active) { active = _active; }
	// activate/deactivate the particle source

	void Timejump ();
	// register a discontinuity

	bool Expired () const { return !level && !np; }
	// stream is dead

	ParticleSpec *CreateParticle (const VECTOR3 &pos, const VECTOR3 &vel, double size, double alpha);
	void DeleteParticle (ParticleSpec *p);
	virtual void Update ();
	void Render (OGLCamera *c);
	//virtual void RenderGroundShadow (LPDIRECT3DDEVICE7 dev, LPDIRECTDRAWSURFACE7 &prevtex) {}

protected:
	void SetSpecs (PARTICLESTREAMSPEC *pss);
	void SetParticleHalflife (double pht);
	double Level2Alpha (double level) const; // map a level (0..1) to alpha (0..1) for given mapping
	double Atm2Alpha (double prm) const; // map atmospheric parameter (e.g. density) to alpha (0..1) for given mapping
	void SetDParticleCoords (const VECTOR3 &ppos, double scale, NTAVERTEX *vtx);
	void SetEParticleCoords (const VECTOR3 &ppos, double scale, NTAVERTEX *vtx);
	void SetShadowCoords (const VECTOR3 &ppos, const VECTOR3 &cdir, double scale, TAVERTEX *vtx);
	void CalcNormals (const VECTOR3 &ppos, NTAVERTEX *vtx);
	virtual void SetMaterial (glm::vec4 &col) { col.r = col.g = col.b = 1; }
	void RenderDiffuse (OGLCamera *c);
	void RenderEmissive (OGLCamera *c);
	VECTOR3 cam_ref;
	const VECTOR3 *src_ref;
	VECTOR3 src_ofs;
	double interval;
	double exp_rate;
	double pdensity;
	double speed; // emission velocity
	double vrand; // velocity randomisation
	double alpha; // particle growth rate
	double beta;  // atmospheric slowdown rate
	double size0;  // particle base size at creation
	double t0; // time of last particle created
	bool active;   // source emitting particles?
	bool diffuse; // particles have diffuse component (need normals)

	PARTICLESTREAMSPEC::LEVELMAP lmap; // level mapping method
	double lmin, lmax;                 // used for level mapping
	PARTICLESTREAMSPEC::ATMSMAP amap;  // atmosphere mapping method
	double amin, afac;                 // used for atmosphere mapping

	ParticleSpec *pfirst, *plast;
	int np; // number of current particles
	int stride; // number of particles rendered simultaneously
	glm::mat4 mModel;
	OGLTexture *tex; // particle texture

//private:
	double ipht2;

protected:
	static OGLTexture *deftex; // default particle texture
	static bool bShadows;               // render particle shadows
};

class ExhaustStream: public OGLParticleStream {
public:
	ExhaustStream (oapi::GraphicsClient *_gc, OBJHANDLE hV,
		const double *srclevel, const VECTOR3 *thref, const VECTOR3 *thdir,
		PARTICLESTREAMSPEC *pss = 0);
	ExhaustStream (oapi::GraphicsClient *_gc, OBJHANDLE hV,
		const double *srclevel, const VECTOR3 &ref, const VECTOR3 &_dir,
		PARTICLESTREAMSPEC *pss = 0);
	//void RenderGroundShadow (LPDIRECT3DDEVICE7 dev, LPDIRECTDRAWSURFACE7 &prevtex);
	void Update ();

private:
	OBJHANDLE hPlanet;
};

class ReentryStream: public OGLParticleStream {
public:
	ReentryStream (oapi::GraphicsClient *_gc, OBJHANDLE hV,
		PARTICLESTREAMSPEC *pss = 0);
	void Update ();

protected:
	void SetMaterial (glm::vec4 &col);

private:
	OBJHANDLE hPlanet;
	double llevel;
};

#endif // !__PARTICLE_H