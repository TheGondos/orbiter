// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Light.cpp
// class D3D7Light (implementation)
//
// This class represents a light source in terms of DX7 interface
// (D3DLIGHT7)
// ==============================================================

#include "Light.h"
#include "Scene.h"
#include "OGLCamera.h"
#include "Renderer.h"
#include <glm/glm.hpp>
#include <cstring>

OGLLight::OGLLight (OBJHANDLE _hObj, LTYPE _ltype, const Scene *scene, int _idx)
{
	hObj = _hObj;
	rpos = _V(0,0,0);
	ltype = _ltype;
	scn = scene;
	idx = _idx;

	memset (&light, 0, sizeof(Param));

	light.enabled = 1;
	light.dltType = ltype;
	light.dcvDiffuse.r = light.dcvSpecular.r = 1.0f; // generalise (from light source specs)
	light.dcvDiffuse.g = light.dcvSpecular.g = 1.0f;
	light.dcvDiffuse.b = light.dcvSpecular.b = 1.0f;

	light.dvAttenuation0 = 1.0f; 
    //light.dvRange = D3DLIGHT_RANGE_MAX;

	Renderer::LightEnable(idx, true);
}

void OGLLight::Update ()
{
	switch (ltype) {
	case Point:
		// to be done
		break;
	case Spot:
		// to be done
		break;
	case Directional:
		UpdateDirectional();
		break;
	};
}

void OGLLight::UpdateDirectional ()
{
	VECTOR3 rpos;
	oapiGetGlobalPos (hObj, &rpos);
	rpos -= *scn->GetCamera()->GetGPos(); // object position rel. to camera
	rpos /= -length(rpos); // normalise
	light.dvDirection.x = rpos.x;
	light.dvDirection.y = rpos.y;
	light.dvDirection.z = rpos.z;
}
