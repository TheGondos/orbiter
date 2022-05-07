// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Scene.cpp
// Class Scene
//
// A "Scene" represents the 3-D world as seen from a specific
// viewpoint ("camera"). Each scene therefore has a camera object
// associated with it. The Orbiter core supports a single
// camera, but in principle a graphics client could define
// multiple scenes and render them simultaneously into separate
// windows (or into MFD display surfaces, etc.)
// ==============================================================

#include "Scene.h"
#include "OGLCamera.h"
#include "VObject.h"
#include "VPlanet.h"
#include "VVessel.h"
#include "Particle.h"
#include <cstring>

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
	}
}

using namespace oapi;

Scene::Scene (uint32_t width, uint32_t height)
{
	m_celestialSphere = std::make_unique<CelestialSphere>();
	m_camera = std::make_unique<OGLCamera>(width, height);
	nstream = 0;
}

Scene::~Scene ()
{
}

void Scene::Initialise()
{
	hSun = oapiGetGbodyByIndex(0); // generalise later
	if(!hSun) {
		printf("Scene::Scene: !hSun\n");
		exit(-1);
	}
}

void Scene::CheckVisibility(OBJHANDLE handle)
{
	//VECTOR3 pos;
	//oapiGetGlobalPos (hObj, &pos);
	//double rad = oapiGetSize (hObj);
	//double dst = dist (pos, *cam->GetGPos());
	//double apprad = (rad*viewH)/(dst*cam->GetTanAp());
	// apparent radius of the object in units of viewport pixels

	auto search = m_visibleObjects.find(handle);
    if (search == m_visibleObjects.end()) {
		VObject *obj = VObject::Create(handle);
		m_visibleObjects[handle] = obj;
		g_client->RegisterVisObject (handle, (VISHANDLE)obj);
	}

/*
	if (pv->vobj->IsActive()) {
		if (apprad < 1.0) pv->vobj->Activate (false);
	} else {
		if (apprad > 2.0) pv->vobj->Activate (true);
	}
*/
}

void Scene::Update ()
{
	m_camera->Update (); // update camera parameters
	UpdateLightDir();

	uint32_t nobj = oapiGetObjectCount();
	for(uint32_t i=0; i<nobj;i++) {

		OBJHANDLE hObj = oapiGetObjectByIndex (i);
		CheckVisibility (hObj);
	}

	for(auto &obj: m_visibleObjects) {
		obj.second->Update();
	}


	// update particle streams - should be skipped when paused
	if (!oapiGetPause()) {
		for (int i = 0; i < nstream;) {
			if (pstream[i]->Expired()) DelParticleStream (i);
			else pstream[i++]->Update();
		}
	}
}

void Scene::NewVessel (OBJHANDLE hVessel)
{
	CheckVisibility(hVessel);
}

void Scene::DeleteVessel (OBJHANDLE hVessel)
{
	auto search = m_visibleObjects.find(hVessel);
    if (search != m_visibleObjects.end()) {
		g_client->UnregisterVisObject (hVessel);
		delete search->second;
		m_visibleObjects.erase(search);
	}
}

void Scene::AddParticleStream (OGLParticleStream *_pstream)
{
	OGLParticleStream **tmp = new OGLParticleStream*[nstream+1];
	if (nstream) {
		memcpy (tmp, pstream, nstream*sizeof(OGLParticleStream*));
		delete []pstream;
	}
	pstream = tmp;
	pstream[nstream++] = _pstream;
}

void Scene::DelParticleStream (uint32_t idx)
{
	OGLParticleStream **tmp;
	if (nstream > 1) {
		uint32_t i, j;
		tmp = new OGLParticleStream*[nstream-1];
		for (i = j = 0; i < nstream; i++)
			if (i != idx) tmp[j++] = pstream[i];
	} else tmp = 0;
	delete pstream[idx];
	delete []pstream;
	pstream = tmp;
	nstream--;
}

VECTOR3 Scene::SkyColour ()
{
	VECTOR3 col = {0,0,0};
	OBJHANDLE hProxy = oapiCameraProxyGbody();
	if (hProxy && oapiPlanetHasAtmosphere (hProxy)) {
		const ATMCONST *atmp = oapiGetPlanetAtmConstants (hProxy);
		VECTOR3 rc, rp, pc;
		oapiCameraGlobalPos (&rc);
		oapiGetGlobalPos (hProxy, &rp);
		pc = rc-rp;
		double cdist = length (pc);
		if (cdist < atmp->radlimit) {
			ATMPARAM prm;
			oapiGetPlanetAtmParams (hProxy, cdist, &prm);
			normalise (rp);
			double coss = dotp (pc, rp) / -cdist;
			double intens = std::min (1.0,(1.0839*coss+0.4581)) * sqrt (prm.rho/atmp->rho0);
			// => intensity=0 at sun zenith distance 115�
			//    intensity=1 at sun zenith distance 60�
			if (intens > 0.0)
				col += _V(atmp->color0.x*intens, atmp->color0.y*intens, atmp->color0.z*intens);
		}
		for (int i = 0; i < 3; i++)
			if (col.data[i] > 1.0) col.data[i] = 1.0;
	}
	return col;
}

struct PList {
	VObject *obj;
	double cdist;
};
int distcomp (const void *arg1, const void *arg2);

void Scene::Render ()
{
	Update();

	VECTOR3 bgcol = SkyColour();
	//double skybrt = (bgcol.x+bgcol.y+bgcol.z)/3.0;

	glClearColor(bgcol.x, bgcol.y, bgcol.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0,0,m_camera->GetWidth(),m_camera->GetHeight());
/*
	VECTOR3 col;
	col.x = 1.0;
	col.y = 1.0;
	col.z = 1.0;
*/
	OGLCamera *c = m_camera.get();
//	m_celestialSphere->RenderConstellations(col, c);
	m_celestialSphere->RenderStars(c, bgcol);
//	m_celestialSphere->RenderEcliptic(c);

	/*
	for(auto &obj: m_visibleObjects) {
		obj.second->Render(c);
	}*/


	// render solar system celestial objects (planets and moons)
	// we render without z-buffer, so need to distance-sort the objects
	int np = 0;
	const int MAXPLANET = 512; // hard limit; should be fixed

	static PList plist[MAXPLANET];

//	c->SetFrustumLimits (2.5f, 5e6f);

	for(auto &obj: m_visibleObjects) {
		OBJHANDLE hObj = obj.second->GetObject();
		int objtp = oapiGetObjectType (hObj);
		if (objtp == OBJTP_PLANET || objtp == OBJTP_STAR) {
			plist[np].obj = obj.second;
			plist[np].cdist = obj.second->CamDist();
			np++;
		}
	}

	int distcomp (const void *arg1, const void *arg2);
	qsort ((void*)plist, np, sizeof(PList), distcomp);

//	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glEnable(GL_CULL_FACE);
	CheckError("Scene: glEnable(GL_CULL_FACE)");
	glFrontFace(GL_CW);
	CheckError("Scene: glFrontFace(GL_CW)");
	glDisable(GL_DEPTH_TEST);
	CheckError("Scene: glDisable(GL_DEPTH_TEST)");
	for (int i = 0; i < np; i++) {
		plist[i].obj->Render(c);
	}

	glEnable(GL_DEPTH_TEST);
	CheckError("Scene: glEnable(GL_DEPTH_TEST)");

//	c->SetFrustumLimits (0.05f, 5e6f);

	// render the vessel objects
	OBJHANDLE hFocus = oapiGetFocusObject();
	VVessel *vFocus = NULL;
	for(auto &obj: m_visibleObjects) {
		OBJHANDLE hObj = obj.second->GetObject();
		int objtp = oapiGetObjectType (hObj);
		if (objtp == OBJTP_VESSEL) {
			if (hObj == hFocus)
				vFocus = (VVessel *)obj.second; // remember focus visual
			obj.second->Render(c);
		}
	}

	CheckError("Scene: after vessels draw");

	// render static engine exhaust
//	DWORD alpha;
//	dev->GetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, &alpha);
//	if (!alpha) dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//	for (pv = vobjFirst; pv; pv = pv->next) {
	for(auto &obj: m_visibleObjects) {
//		if (!pv->vobj->IsActive()) continue;
//		pv->vobj->RenderBeacons (dev);
		if (oapiGetObjectType (obj.second->GetObject()) == OBJTP_VESSEL) {
			((VVessel *)obj.second)->RenderExhaust (c);
		}
	}

	// render exhaust particle system
	for (uint32_t n = 0; n < nstream; n++)
		pstream[n]->Render (c);

	// render the internal parts of the focus object in a separate render pass
	if (oapiCameraInternal() && vFocus) {
		/*
		// switch cockpit lights on, external-only lights off
		if (locallight) {
			for (i = 1; i < nlight; i++) {
				switch (lightlist[i].plight->GetVisibility()) {
				case LightEmitter::VIS_EXTERNAL:
					dev->LightEnable (i, FALSE);
					break;
				case LightEmitter::VIS_COCKPIT:
					AddLocalLight (lightlist[i].plight, lightlist[i].vobj, i);
					break;
				}
			}
		}*/
		// should also check for internal meshes
		//dev->Clear (0, NULL, zclearflag,  0, 1.0f, 0L); // clear z-buffer
		glClear(GL_DEPTH_BUFFER_BIT);
		double nearp = c->GetNearlimit();
		double farp  = c->GetFarlimit ();
		
		c->SetFrustumLimits (0.05, oapiGetSize (hFocus));
		//c->SetFrustumLimits (0.05f, 5e9f);
		vFocus->Render (c, true);
		c->SetFrustumLimits (nearp, farp);
	}

	// render 2D panel and HUD
	g_client->Render2DOverlay();
}

int distcomp (const void *arg1, const void *arg2)
{
	double d1 = ((struct PList*)arg1)->cdist;
	double d2 = ((struct PList*)arg2)->cdist;
	return (d1 > d2 ? -1 : d1 < d2 ? 1 : 0);
}
