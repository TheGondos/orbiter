// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OGL Client module
// ==============================================================

// ==============================================================
// Light.h
// class OGLLight (interface)
//
// This class represents a light source
// ==============================================================

#ifndef __LIGHT_H
#define __LIGHT_H

#include "Orbitersdk.h"
#include <glm/glm.hpp>

#define MAX_LIGHTS 8

class Scene;
class OGLLight {
public:
	enum LTYPE { Point, Spot, Directional };
	struct Param {
		int enabled;
		LTYPE dltType;
		glm::vec4 dcvDiffuse;
		glm::vec4 dcvSpecular;
		glm::vec4 dcvAmbient;
		glm::vec3 dvPosition;
		glm::vec3 dvDirection;
		float dvAttenuation0;
		float dvAttenuation1;
		float dvAttenuation2;
		float dvRange;
		float dvFalloff;
		float dvTheta;
		float dvPhi;
	};

	OGLLight () { light.enabled = 0; }
	OGLLight (OBJHANDLE _hObj, LTYPE _ltype, const Scene *scene, int _idx);

	void Update ();

	inline const Param *GetLight() const { return &light; }
	int idx;          // light index

	void UpdateDirectional ();

	OBJHANDLE hObj;   // object to which the light source is attached
	VECTOR3 rpos;     // light source position in the object frame
	LTYPE ltype;      // light type
	const Scene *scn; // The scene to which the light source belongs

	Param light;
};

#endif // !__LIGHT_H