// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OpenGL Client module
// ==============================================================

// ==============================================================
// Scene.h
// Class Scene (interface)
//
// A "Scene" represents the 3-D world as seen from a specific
// viewpoint ("camera"). Each scene therefore has a camera object
// associated with it. The Orbiter core supports a single
// camera, but in principle a graphics client could define
// multiple scenes and render them simultaneously into separate
// windows (or into MFD display surfaces, etc.)
// ==============================================================

#ifndef __SCENE_H
#define __SCENE_H

#include "OrbiterAPI.h"
//#include "OGLClient.h"
//#include "GUIManager.h"
#include "CelSphere.h"
#include "OGLCamera.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

class VObject;
class OGLParticleStream;
class Scene {
public:
	Scene (uint32_t width, uint32_t height);
	~Scene ();

	void Update();
	void Render();

	void CheckVisibility(OBJHANDLE);
	OGLCamera *GetCamera() const { return m_camera.get(); }

	void NewVessel(OBJHANDLE hVessel);
	void DeleteVessel(OBJHANDLE hVessel);

	std::unique_ptr<OGLCamera> m_camera;
	std::unique_ptr<CelestialSphere> m_celestialSphere;
	std::unordered_map<OBJHANDLE, VObject *> m_visibleObjects;

	OBJHANDLE hSun;

	glm::vec3 sunDir;

	void Initialise();

	void UpdateLightDir() {
		VECTOR3 rpos;
		oapiGetGlobalPos (hSun, &rpos);
		VECTOR3 gpos;
		glm::dvec3 c = *m_camera->GetGPos();
		gpos.x = c.x;
		gpos.y = c.y;
		gpos.z = c.z;
		rpos -= gpos; // object position rel. to camera
//		rpos -= m_camera->GetGPos(); // object position rel. to camera
		rpos /= -length(rpos); // normalise
		sunDir.x = rpos.x;
		sunDir.y = rpos.y;
		sunDir.z = rpos.z;
	}
	VECTOR3 SkyColour ();

	const glm::vec3 *GetSunDir() const {
		return &sunDir;
	}


	OGLParticleStream **pstream; // list of particle streams
	uint32_t          nstream; // number of streams
	void AddParticleStream (OGLParticleStream *_pstream);
	void DelParticleStream (uint32_t idx);
};

#endif // !__SCENE_H
