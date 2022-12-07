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
#include "OGLMesh.h"
#include "VPlanet.h"
#include "VVessel.h"
#include "VBase.h"
#include "Particle.h"
#include "Renderer.h"
#include <cstring>
#include "Light.h"
//#include "CSphereMgr.h"

static bool Vector3Matrix4Multiply (glm::fvec3 *res, const glm::fvec3 *v, const glm::fmat4 &mat)
{
    float x = v->x* mat[0][0] + v->y*mat[1][0] + v->z* mat[2][0] + mat[3][0];
    float y = v->x* mat[0][1] + v->y*mat[1][1] + v->z* mat[2][1] + mat[3][1];
    float z = v->x* mat[0][2] + v->y*mat[1][2] + v->z* mat[2][2] + mat[3][2];
    float w = v->x* mat[0][3] + v->y*mat[1][3] + v->z* mat[2][3] + mat[3][3];

    if (fabs (w) < 1e-5f) return false;

    res->x = x/w;
    res->y = y/w;
    res->z = z/w;
    return true;
}

using namespace oapi;
/*
static D3DMATRIX ident = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1
};
*/
OGLMaterial def_mat = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{0,0,0,1},0};
const double LABEL_DISTLIMIT = 0.6;

struct PList { // auxiliary structure for object distance sorting
	vPlanet *vo;
	double dist;
};

Scene::Scene (int w, int h)
{
	viewW = w, viewH = h;
	stencilDepth = 8;
//	zclearflag = D3DCLEAR_ZBUFFER;
//	if (stencilDepth) zclearflag |= D3DCLEAR_STENCIL;
	cam = new OGLCamera (w, h);
	csphere = new CelestialSphere ();
	vobjFirst = vobjLast = NULL;
	nstream = 0;
	iVCheck = 0;
	surfLabelsActive = false;
	if (!g_client->clbkGetRenderParam (RP_MAXLIGHTS, &maxlight)) maxlight = 8;
	int maxlight_request = *(int*)g_client->GetConfigParam (CFGPRM_MAXLIGHT);
	if (maxlight_request) maxlight = std::min (maxlight, maxlight_request);
	locallight = *(bool*)g_client->GetConfigParam (CFGPRM_LOCALLIGHT);
	locallight = true;
	if (locallight)
		lightlist = new LIGHTLIST[maxlight];
	memset (&bg_rgba, 0, sizeof (VECTOR3));
	InitResources();
	//cspheremgr = new CSphereManager (_gc, this);
}

Scene::~Scene ()
{
	while (vobjFirst) {
		DelVisualRec (vobjFirst);
	}
	delete cam;
	delete csphere;
	delete light;
	//delete cspheremgr;
	if (nstream) {
		for (int j = 0; j < nstream; j++)
			delete pstream[j];
		delete []pstream;
	}
	if (locallight) delete []lightlist;
	ExitResources();
}

void Scene::Initialise ()
{
	OBJHANDLE hSun = oapiGetGbodyByIndex(0); // generalise later
	light = new OGLLight (hSun, OGLLight::Directional, this, 0);
	light->light.enabled = 1;	

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);

    // Set miscellaneous renderstates
	/*
	dev->SetRenderState (D3DRENDERSTATE_DITHERENABLE, TRUE);
    dev->SetRenderState (D3DRENDERSTATE_ZENABLE, TRUE);
	dev->SetRenderState (D3DRENDERSTATE_FILLMODE, *(bool*)gc->GetConfigParam (CFGPRM_WIREFRAME) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
	dev->SetRenderState (D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
	dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
    dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	dev->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetRenderState (D3DRENDERSTATE_NORMALIZENORMALS, FALSE);
	dev->SetRenderState (D3DRENDERSTATE_ZBIAS, 0);
	dev->SetRenderState (D3DRENDERSTATE_WRAP0, 0);
	dev->SetRenderState (D3DRENDERSTATE_AMBIENT, *(DWORD*)gc->GetConfigParam (CFGPRM_AMBIENTLEVEL) * 0x01010101);

	// Set texture renderstates
    dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState (0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	dev->SetTextureStageState (0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	dev->SetTextureStageState (0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	dev->SetTextureStageState (1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	dev->SetTextureStageState (1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	*/
}

void Scene::CheckVisual (OBJHANDLE hObj)
{
	VECTOR3 pos;
	oapiGetGlobalPos (hObj, &pos);
	double rad = oapiGetSize (hObj);
	double dst = std::max(0.1, dist (pos, *cam->GetGPos()));
	double apprad = (rad*viewH)/(dst*cam->GetTanAp());
	// apparent radius of the object in units of viewport pixels

	VOBJREC *pv = FindVisual (hObj);
	if (!pv) pv = AddVisualRec (hObj);

	if (pv->vobj->IsActive()) {
		if (apprad < 1.0) pv->vobj->Activate (false);
	} else {
		if (apprad > 2.0) pv->vobj->Activate (true);
	}
#ifdef UNDEF
	if (pv) { // object has an associated visual
		if (apprad < 1.0) DelVisualRec (pv); // delete visual
	} else {  // object has not visual
		if (apprad > 2.0) AddVisualRec (hObj); // create visual
	}
#endif
	// the range check has a small hysteresis to avoid continuous
	// creation/deletion for objects at the edge of visibility
}

const OGLLight *Scene::GetLight () const
{
	return light;
}

Scene::VOBJREC *Scene::FindVisual (OBJHANDLE hObj)
{
	VOBJREC *pv;
	for (pv = vobjFirst; pv; pv = pv->next) {
		if (pv->vobj->Object() == hObj) return pv;
	}
	return NULL;
}

void Scene::DelVisualRec (VOBJREC *pv)
{
	// unlink the entry
	if (pv->prev) pv->prev->next = pv->next;
	else          vobjFirst = pv->next;

	if (pv->next) pv->next->prev = pv->prev;
	else          vobjLast = pv->prev;

	// delete the visual, its children and the entry itself
	g_client->UnregisterVisObject (pv->vobj->GetObject());
	delete pv->vobj;
	delete pv;
}

Scene::VOBJREC *Scene::AddVisualRec (OBJHANDLE hObj)
{
	// create the visual and entry
	VOBJREC *pv = new VOBJREC;
	pv->vobj = vObject::Create (hObj, this);
	g_client->RegisterVisObject (hObj, (VISHANDLE)pv->vobj);

	// link entry to end of list
	pv->prev = vobjLast;
	pv->next = NULL;
	if (vobjLast) vobjLast->next = pv;
	else          vobjFirst = pv;
	vobjLast = pv;
	return pv;
}

void Scene::AddLocalLight (const LightEmitter *le, const vObject *vo, int idx)
{
	OGLLight lght;
	switch (le->GetType()) {
	case LightEmitter::LT_POINT: {
		lght.light.dltType = OGLLight::Point;
		lght.light.dvRange = (float)((PointLight*)le)->GetRange();
		const double *att = ((PointLight*)le)->GetAttenuation();
		lght.light.dvAttenuation0 = (float)att[0];
		lght.light.dvAttenuation1 = (float)att[1];
		lght.light.dvAttenuation2 = (float)att[2];
		} break;
	case LightEmitter::LT_SPOT: {
		lght.light.dltType = OGLLight::Spot;
		lght.light.dvRange = (float)((SpotLight*)le)->GetRange();
		const double *att = ((SpotLight*)le)->GetAttenuation();
		lght.light.dvAttenuation0 = (float)att[0];
		lght.light.dvAttenuation1 = (float)att[1];
		lght.light.dvAttenuation2 = (float)att[2];


		float P = float(((SpotLight*)le)->GetPenumbra());
		float U = float(((SpotLight*)le)->GetUmbra());
		if (P > 3.05f) P = 3.05f;
		if (U > 2.96f) U = 2.96f;
		float cosp = cos(P * 0.5f);
		float cosu = cos(U * 0.5f);
		float tanp = tan(P * 0.5f);

		lght.light.dvFalloff = 1.0f;
		lght.light.dvTheta = 1.0f / (cosu - cosp);
		lght.light.dvPhi = cosp;
		} break;
	}
	double intens = le->GetIntensity();
	const COLOUR4 &col_d = le->GetDiffuseColour();
	lght.light.dcvDiffuse = { (float)(col_d.r*intens),
						(float)(col_d.g*intens),
						(float)(col_d.b*intens),
						(float)(col_d.a*intens)};
	const COLOUR4 &col_s = le->GetSpecularColour();
	lght.light.dcvSpecular = {(float)(col_s.r*intens),
						(float)(col_s.g*intens),
						(float)(col_s.b*intens),
						(float)(col_s.a*intens)};
	const COLOUR4 &col_a = le->GetAmbientColour();
	lght.light.dcvAmbient = { (float)(col_a.r*intens),
						(float)(col_a.g*intens),
						(float)(col_a.b*intens),
						(float)(col_a.a*intens)};
	if (lght.light.dltType != OGLLight::Directional) {
		const VECTOR3 pos = le->GetPosition();
		glm::vec3 p = { (float)pos.x, (float)pos.y, (float)pos.z }; 
		Vector3Matrix4Multiply (&lght.light.dvPosition, &p, vo->MWorld());
	}
	if (lght.light.dltType != OGLLight::Point) {
		MATRIX3 grot;
		oapiGetRotationMatrix (vo->Object(), &grot);
		VECTOR3 d = mul (grot, le->GetDirection());
		lght.light.dvDirection = {(float)d.x, (float)d.y, (float)d.z};
	}
	lght.idx = idx;
	Renderer::SetLight (&lght);
	Renderer::LightEnable (idx, true);
}

void Scene::Update ()
{
	cam->Update (); // update camera parameters
	Renderer::SetViewPort(cam->GetWidth(),cam->GetHeight());

	light->Update (); // update light sources

	// check object visibility (one object per frame in the interest
	// of scalability)
	int nobj = oapiGetObjectCount();
	if (iVCheck >= nobj) iVCheck = 0;
	OBJHANDLE hObj = oapiGetObjectByIndex (iVCheck++);
	CheckVisual (hObj);

	// update all existing visuals
	for (VOBJREC *pv = vobjFirst; pv; pv = pv->next) {
		vObject *vo = pv->vobj;
		//OBJHANDLE hObj = vo->Object();
		vo->Update();
	}

	// update particle streams - should be skipped when paused
	if (!oapiGetPause()) {
		for (int i = 0; i < nstream;) {
			if (pstream[i]->Expired()) DelParticleStream (i);
			else pstream[i++]->Update();
		}
	}
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

void Scene::Render ()
{
	int i, j;
	int n;
	VOBJREC *pv;

	Update (); // update camera and visuals

	VECTOR3 bgcol = SkyColour();
	double skybrt = (bgcol.x+bgcol.y+bgcol.z)/3.0;
	bg_rgba = bgcol;
	int bglvl = 0;
	//if (bg_rgba) { // suppress stars darker than the background
		bglvl = 255.0*(bg_rgba.x + bg_rgba.y + bg_rgba.z);
		bglvl = std::min (bglvl/2, 255);
	//}

	// Clear the viewport
    glClearColor(bgcol.x, bgcol.y, bgcol.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//if (FAILED (dev->BeginScene ())) return;

	Renderer::SetLight(light);
	int nlight = 1;
	
	if (locallight) {
		int j, k;
		for (pv = vobjFirst; pv; pv = pv->next) {
			if (!pv->vobj->IsActive()) continue;
			OBJHANDLE hObj = pv->vobj->Object();
			if (oapiGetObjectType (hObj) == OBJTP_VESSEL) {
				VESSEL *vessel = oapiGetVesselInterface (hObj);
				int nemitter = vessel->LightEmitterCount();
				for (j = 0; j < nemitter; j++) {
					const LightEmitter *em = vessel->GetLightEmitter(j);
					if (!em->IsActive() || !em->GetIntensity()) continue;
					if (oapiCameraInternal()) {
						if (em->GetVisibility() == LightEmitter::VIS_EXTERNAL)
							continue;
					}
					else {
						if (em->GetVisibility() == LightEmitter::VIS_COCKPIT)
							continue;
					}
					const VECTOR3 *pos = em->GetPositionRef();
					glm::fvec3 q, p = {(float)pos->x, (float)pos->y, (float)pos->z};
					Vector3Matrix4Multiply(&q, &p, pv->vobj->MWorld());
					double dst2 = q.x*q.x + q.y*q.y + q.z*q.z;
					for (k = nlight-1; k >= 1; k--) {
						if (lightlist[k].camdist2 < dst2) {
							break;
						} else if (k < maxlight-1) {
							lightlist[k+1] = lightlist[k]; // shift entries to make space
						} else
							nlight--;
					}
					if (k == maxlight-1) continue;
					lightlist[k+1].plight = em;
					lightlist[k+1].vobj = pv->vobj;
					lightlist[k+1].camdist2 = dst2;
					nlight++;
				}
			}
		}
		for (i = 1; i < nlight; i++)
			if (lightlist[i].plight->GetVisibility() & LightEmitter::VIS_EXTERNAL)
				AddLocalLight (lightlist[i].plight, lightlist[i].vobj, i);
	}

	//dev->SetMaterial (&def_mat);
	glBindTexture(GL_TEXTURE_2D, 0);

	// planetarium mode flags
	int plnmode = *(int*)g_client->GetConfigParam (CFGPRM_PLANETARIUMFLAG);

	// render celestial sphere (without z-buffer)
	//dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &ident);
	//dev->SetTexture (0,0);
	Renderer::PushBool(Renderer::DEPTH_TEST, false);
	Renderer::PushDepthMask(false);
	Renderer::EnableLighting(false);

	// use explicit colours
	//dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	//dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);

	// planetarium mode (celestial sphere elements)
	if (plnmode & PLN_ENABLE) {
		/*
		DWORD dstblend;
		dev->GetRenderState (D3DRENDERSTATE_DESTBLEND, &dstblend);
		dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		*/
		double linebrt = 1.0-skybrt;

		// render ecliptic grid
		if (plnmode & PLN_EGRID) {
			auto tmp = _V(0,0,0.4)*linebrt;
			//csphere->RenderGrid (dev, tmp, !(plnmode & PLN_ECL));
		}
		if (plnmode & PLN_ECL) {
			auto tmp = _V(0,0,0.8)*linebrt;
			//csphere->RenderGreatCircle (dev, tmp);
		}

		// render celestial grid
		if (plnmode & (PLN_CGRID|PLN_EQU)) {
			/*
			static double obliquity = 0.4092797095927;
			static double coso = cos(obliquity), sino = sin(obliquity);
			static D3DMATRIX rot = {1.0f,0.0f,0.0f,0.0f,  0.0f,(float)coso,(float)sino,0.0f,  0.0f,-(float)sino,(float)coso,0.0f,  0.0f,0.0f,0.0f,1.0f};
			dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &rot);
			if (plnmode & PLN_CGRID) {
				auto tmp = _V(0.35,0,0.35)*linebrt;
				csphere->RenderGrid (dev, tmp, !(plnmode & PLN_EQU));
			}
			if (plnmode & PLN_EQU) {
				auto tmp = _V(0.7,0,0.7)*linebrt;
				csphere->RenderGreatCircle (dev, tmp);
			}
			dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &ident);
			*/
		}

		// render constellation lines
		if (plnmode & PLN_CONST) {
			auto tmp = _V(0.4,0.3,0.2)*linebrt;
			csphere->RenderConstellations(tmp, cam);
		}
/*
		dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, dstblend);
		dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
*/
		if (plnmode & PLN_CCMARK) {
			const GraphicsClient::LABELLIST *list;
			int n, nlist;
			oapi::Sketchpad *skp= NULL;
			nlist = g_client->GetCelestialMarkers (&list);
			for (n = 0; n < nlist; n++) {
				if (list[n].active) {
					if(!skp) skp = oapiGetSketchpad(nullptr);
					skp->SetFont(hLabelFont[0]);
					skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
					skp->SetBackgroundMode (oapi::Sketchpad::BK_TRANSPARENT);
					int size = (int)(viewH/80.0*list[n].size+0.5);
					int col = list[n].colour;
					skp->SetPen (hLabelPen[col]);
					skp->SetTextColor (labelCol[col]);
					const GraphicsClient::LABELSPEC *ls = list[n].list;
					for (i = 0; i < list[n].length; i++) {
						RenderDirectionMarker (skp, ls[i].pos, ls[i].label[0], ls[i].label[1], list[n].shape, size);
					}
				}
			}
			if (skp) g_client->clbkReleaseSketchpad (skp);
		}
	}

	// revert to standard colour selection
	//dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	//dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);

	csphere->RenderStars (cam, bgcol);
	//cspheremgr->Render (dev, 8, bglvl);

	// turn on lighting
	Renderer::EnableLighting(true);

	// render solar system celestial objects (planets and moons)
	// we render without z-buffer, so need to distance-sort the objects
	int np;
	const int MAXPLANET = 512; // hard limit; should be fixed
	static PList plist[MAXPLANET];
	for (pv = vobjFirst, np = 0; pv && np < MAXPLANET; pv = pv->next) {
		if (!pv->vobj->IsActive()) continue;
		OBJHANDLE hObj = pv->vobj->Object();
		int objtp = oapiGetObjectType (hObj);
		if (objtp == OBJTP_PLANET || objtp == OBJTP_STAR) {
			plist[np].vo = (vPlanet*)pv->vobj;
			plist[np].dist = pv->vobj->CamDist();
			np++;
		}
	}
	int distcomp (const void *arg1, const void *arg2);
	qsort ((void*)plist, np, sizeof(PList), distcomp);
	for (i = 0; i < np; i++) {
		OBJHANDLE hObj = plist[i].vo->Object();
		plist[i].vo->Render ();
		
		if (plnmode & PLN_ENABLE) {
			if (plnmode & PLN_CMARK) {
				VECTOR3 pp;
				char name[256];
				oapiGetObjectName (hObj, name, 256);
				oapiGetGlobalPos (hObj, &pp);
				RenderObjectMarker (0, pp, name, 0, 0, viewH/80);
			}
			
			if ((plnmode & PLN_SURFMARK) && (oapiGetObjectType (hObj) == OBJTP_PLANET)) {
				int label_format = *(int*)oapiGetObjectParam(hObj, OBJPRM_PLANET_LABELENGINE);
				if (label_format < 2 && (plnmode & PLN_LMARK)) { // user-defined planetary surface labels
					double rad = oapiGetSize (hObj);
					double apprad = rad/(plist[i].dist * cam->GetTanAp());
					const GraphicsClient::LABELLIST *list;
					int n, nlist;
					oapi::Sketchpad *skp= NULL;
					MATRIX3 prot;
					VECTOR3 ppos, cpos;
					nlist = g_client->GetSurfaceMarkers (hObj, &list);
					for (n = 0; n < nlist; n++) {
						if (list[n].active && apprad*list[n].distfac > LABEL_DISTLIMIT) {
							if (!skp) {
								skp = oapiGetSketchpad(nullptr);
								skp->SetFont(hLabelFont[0]);
								skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
								skp->SetBackgroundMode (oapi::Sketchpad::BK_TRANSPARENT);
								oapiGetRotationMatrix (hObj, &prot);
								oapiGetGlobalPos (hObj, &ppos);
								const VECTOR3 *cp = cam->GetGPos();
								cpos = tmul (prot, *cp-ppos); // camera in local planet coords
							}
							int size = (int)(viewH/80.0*list[n].size+0.5);
							int col = list[n].colour;
							skp->SetPen (hLabelPen[col]);
							skp->SetTextColor (labelCol[col]);
							const GraphicsClient::LABELSPEC *ls = list[n].list;
							VECTOR3 sp;
							for (j = 0; j < list[n].length; j++) {
								if (dotp (ls[j].pos, cpos-ls[j].pos) >= 0.0) { // surface point visible?
									sp = mul (prot, ls[j].pos) + ppos;
									RenderObjectMarker (skp, sp, ls[j].label[0], ls[j].label[1], list[n].shape, size);
								}
							}
						}
					}
					if (skp) g_client->clbkReleaseSketchpad (skp);
				}
			}
		}
	}

	// render new-style surface markers
	if ((plnmode & PLN_ENABLE) && (plnmode & PLN_LMARK)) {
		oapi::Sketchpad *skp = 0;
		int fontidx = -1;
		for (i = 0; i < np; i++) {
			OBJHANDLE hObj = plist[i].vo->Object();
			if (oapiGetObjectType(hObj) != OBJTP_PLANET) continue;
			if (!surfLabelsActive)
				plist[i].vo->ActivateLabels(true);
			int label_format = *(int*)oapiGetObjectParam(hObj, OBJPRM_PLANET_LABELENGINE);
			if (label_format == 2) {
				if (!skp) {
					skp = g_client->clbkGetSketchpad(0);
					skp->SetPen(label_pen);
				}
				((vPlanet*)plist[i].vo)->RenderLabels(skp, label_font, &fontidx);
			}
		}
		surfLabelsActive = true;
		if (skp)
			g_client->clbkReleaseSketchpad(skp);
	} else {
		if (surfLabelsActive)
			surfLabelsActive = false;
	}


	// turn z-buffer back on
	Renderer::PopDepthMask();
	Renderer::PopBool();

	// render the vessel objects
	//cam->SetFustrumLimits (1, 1e5);
	OBJHANDLE hFocus = oapiGetFocusObject();
	vVessel *vFocus = NULL;
	for (pv = vobjFirst; pv; pv = pv->next) {
		if (!pv->vobj->IsActive()) continue;
		OBJHANDLE hObj = pv->vobj->Object();
		if (oapiGetObjectType (hObj) == OBJTP_VESSEL) {
			pv->vobj->Render ();
			if (hObj == hFocus) vFocus = (vVessel*)pv->vobj; // remember focus visual
			
			if ((plnmode & (PLN_ENABLE|PLN_VMARK)) == (PLN_ENABLE|PLN_VMARK)) {
				VECTOR3 gpos;
				char name[256];
				oapiGetGlobalPos (hObj, &gpos);
				oapiGetObjectName (hObj, name, 256);
				RenderObjectMarker (0, gpos, name, 0, 0, viewH/80);
			}
		}
	}

	// render static engine exhaust
	Renderer::PushBool(Renderer::BLEND, true);

	for (pv = vobjFirst; pv; pv = pv->next) {
		if (!pv->vobj->IsActive()) continue;
		pv->vobj->RenderBeacons ();
		if (oapiGetObjectType (pv->vobj->Object()) == OBJTP_VESSEL) {
			((vVessel*)(pv->vobj))->RenderExhaust ();
		}
	}

	// render exhaust particle system

		if (locallight) {
			for (i = 1; i < nlight; i++) {
				switch (lightlist[i].plight->GetVisibility()) {
				case LightEmitter::VIS_EXTERNAL:
					Renderer::LightEnable (i, true);
					break;
				case LightEmitter::VIS_COCKPIT:
					AddLocalLight (lightlist[i].plight, lightlist[i].vobj, i);
					break;
				}
			}
		}



	for (n = 0; n < nstream; n++)
		pstream[n]->Render (cam);


	if (locallight)
		for (i = 1; i < nlight; i++)
			Renderer::LightEnable (i, false);

	glBindTexture(GL_TEXTURE_2D, 0);

	Renderer::PopBool();

	// render the internal parts of the focus object in a separate render pass
	if (oapiCameraInternal() && vFocus) {
		// switch cockpit lights on, external-only lights off
		
		if (locallight) {
			for (i = 1; i < nlight; i++) {
				switch (lightlist[i].plight->GetVisibility()) {
				case LightEmitter::VIS_EXTERNAL:
					Renderer::LightEnable (i, false);
					break;
				case LightEmitter::VIS_COCKPIT:
					AddLocalLight (lightlist[i].plight, lightlist[i].vobj, i);
					break;
				}
			}
		}
		// should also check for internal meshes
		glClear(GL_DEPTH_BUFFER_BIT); // clear z-buffer
		double nearp = cam->GetNearlimit();
		double farp  = cam->GetFarlimit ();
		cam->SetFrustumLimits (0.1, oapiGetSize (hFocus));
		vFocus->Render (true);
		cam->SetFrustumLimits (nearp, farp);
	}

	if (locallight)
		for (i = 1; i < nlight; i++)
			Renderer::LightEnable (i, false);

	// render 2D panel and HUD
	g_client->Render2DOverlay();

	//dev->EndScene();
}

// ==============================================================

void Scene::RenderVesselShadows (OBJHANDLE hPlanet, float depth) const
{
	// performance note: the device parameters should only be set if
	// any vessels actually want to render their shadows

	// set device parameters
	Renderer::PushBool(Renderer::BLEND, true);
	Renderer::PushBool(Renderer::STENCIL_TEST, true);
	glStencilMask(1);
	glStencilFunc(GL_NOTEQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
	Renderer::CheckError("RenderVesselShadows");
/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(0,0,0,depth));
	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
*/
	// render vessel shadows
	VOBJREC *pv;
	for (pv = vobjFirst; pv; pv = pv->next) {
		if (!pv->vobj->IsActive()) continue;
		if (oapiGetObjectType (pv->vobj->Object()) == OBJTP_VESSEL) {
			((vVessel*)(pv->vobj))->RenderGroundShadow (hPlanet, depth);
			Renderer::CheckError("RenderVesselShadows");
		}
	}

	Renderer::PopBool(2);
	Renderer::CheckError("RenderVesselShadows");

	// render particle shadows
	/*
	LPDIRECTDRAWSURFACE7 tex = 0;
	for (DWORD j = 0; j < nstream; j++) {
		pstream[j]->RenderGroundShadow (dev, tex);
	}
*/
//	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
//	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
}

// ==============================================================

oapi::Sketchpad *Scene::GetLabelSkp (int mode)
{
	oapi::Sketchpad *skp = oapiGetSketchpad(nullptr);
	skp->SetPen(hLabelPen[mode]);
	skp->SetFont(hLabelFont[0]);
	skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
	skp->SetTextColor (labelCol[mode]);
	skp->SetBackgroundMode (oapi::Sketchpad::BK_TRANSPARENT);
	return skp;
}

// ==============================================================

void Scene::RenderDirectionMarker (oapi::Sketchpad *skp, const VECTOR3 &rdir, const char *label1, const char *label2, int mode, int scale)
{
	bool local_skp = (skp == nullptr);
	int x, y;
	
	if(cam->Direction2Viewport(rdir, x, y)) {
		if (local_skp) skp = GetLabelSkp (mode);

		switch (mode) {
		case 0: // box
			skp->Rectangle ( x-scale, y-scale, x+scale+1, y+scale+1);
			break;
		case 1: // circle
			skp->Ellipse ( x-scale, y-scale, x+scale+1, y+scale+1);
			break;
		case 2: // diamond
			skp->MoveTo ( x, y-scale);
			skp->LineTo ( x+scale, y); skp->LineTo ( x, y+scale);
			skp->LineTo ( x-scale, y); skp->LineTo ( x, y-scale);
			break;
		case 3: { // delta
			int scl1 = (int)(scale*1.1547);
			skp->MoveTo ( x, y-scale );
			skp->LineTo ( x+scl1, y+scale); skp->LineTo (x-scl1, y+scale); skp->LineTo (x, y-scale);
			} break;
		case 4: { // nabla
			int scl1 = (int)(scale*1.1547);
			skp->MoveTo ( x, y+scale);
			skp->LineTo ( x+scl1, y-scale); skp->LineTo ( x-scl1, y-scale); skp->LineTo ( x, y+scale);
			} break;
		case 5: { // cross
			int scl1 = scale/4;
			skp->MoveTo ( x, y-scale ); skp->LineTo ( x, y-scl1);
			skp->MoveTo ( x, y+scale ); skp->LineTo ( x, y+scl1);
			skp->MoveTo ( x-scale, y ); skp->LineTo ( x-scl1, y);
			skp->MoveTo ( x+scale, y ); skp->LineTo ( x+scl1, y);
			} break;
		case 6: { // X
			int scl1 = scale/4;
			skp->MoveTo ( x-scale, y-scale ); skp->LineTo ( x-scl1, y-scl1);
			skp->MoveTo ( x-scale, y+scale ); skp->LineTo ( x-scl1, y+scl1);
			skp->MoveTo ( x+scale, y-scale ); skp->LineTo ( x+scl1, y-scl1);
			skp->MoveTo ( x+scale, y+scale ); skp->LineTo ( x+scl1, y+scl1);
			} break;
		}
		if (label1) skp->Text (x, y-scale, label1, strlen (label1));
		if (label2) skp->Text (x, y+scale+labelSize[0], label2, strlen (label2));
		if (local_skp) oapiReleaseSketchpad(skp);
	}
}

void Scene::RenderObjectMarker (oapi::Sketchpad *skp, const VECTOR3 &gpos, const char *label1, const char *label2, int mode, int scale)
{
	VECTOR3 dp (gpos - *cam->GetGPos());
	normalise (dp);
	RenderDirectionMarker (skp, dp, label1, label2, mode, scale);
}

void Scene::NewVessel (OBJHANDLE hVessel)
{
	CheckVisual (hVessel);
}

void Scene::DeleteVessel (OBJHANDLE hVessel)
{
	VOBJREC *pv = FindVisual (hVessel);
	if (pv) DelVisualRec (pv);
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

void Scene::DelParticleStream (int idx)
{
	OGLParticleStream **tmp;
	if (nstream > 1) {
		int i, j;
		tmp = new OGLParticleStream*[nstream-1];
		for (i = j = 0; i < nstream; i++)
			if (i != idx) tmp[j++] = pstream[i];
	} else tmp = 0;
	delete pstream[idx];
	delete []pstream;
	pstream = tmp;
	nstream--;
}

void Scene::InitResources ()
{
	for (int i = 0; i < 6; i++)
		hLabelPen[i] = oapiCreatePen (1, 0, labelCol[i]);
	labelSize[0] = std::max (viewH/60, 14);
//	hLabelFont[0] = oapiCreateFont (labelSize[0], 0, 0, 0, 400, TRUE, 0, 0, 0, 3, 2, 1, 49, "Arial");
	hLabelFont[0] = oapiCreateFont (labelSize[0], true, "Arial");

	const int fsize[4] = {12, 16, 20, 26};
	for (int i = 0; i < 4; i++)
		label_font[i] = oapiCreateFont(fsize[i], true, "Arial", FONT_BOLD);
	label_pen = oapiCreatePen(1, 0, oapiGetColour(255,255,255));
}

void Scene::ExitResources ()
{
	int i;
	for (i = 0; i < 6; i++)
		oapiReleasePen (hLabelPen[i]);
	for (i = 0; i < 1; i++)
		oapiReleaseFont (hLabelFont[i]);

	for (int i = 0; i < 4; i++)
		oapiReleaseFont(label_font[i]);
	oapiReleasePen(label_pen);
}

int distcomp (const void *arg1, const void *arg2)
{
	double d1 = ((PList*)arg1)->dist;
	double d2 = ((PList*)arg2)->dist;
	return (d1 > d2 ? -1 : d1 < d2 ? 1 : 0);
}

COLORREF Scene::labelCol[6] = {0x00FFFF, 0xFFFF00, 0x4040FF, 0xFF00FF, 0x40FF40, 0xFF8080};