// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VPlanet.cpp
// class vPlanet (implementation)
//
// A vPlanet is the visual representation of a "planetary" object
// (planet, moon, asteroid).
// Currently this only supports spherical objects, without
// variations in elevation.
// ==============================================================

#include "OGLClient.h"
#include "VPlanet.h"
#include "VBase.h"
#include "OGLCamera.h"
#include "OGLMesh.h"
#include "SurfMgr.h"
#include "surfmgr2.h"
#include "cloudmgr2.h"
#include "CloudMgr.h"
#include "HazeMgr.h"
#include "RingMgr.h"
#include "Renderer.h"
#include <cstring>

using namespace oapi;

// ==============================================================

static double farplane = 1e6;
static double max_surf_dist = 1e4;

extern int SURF_MAX_PATCHLEVEL;

// ==============================================================

vPlanet::vPlanet (OBJHANDLE _hObj, const Scene *scene): vObject (_hObj, scene)
{
	rad = (float)size;
	render_rad = (float)(0.1*rad);
	dist_scale = 1.0f;
	max_centre_dist = 0.9*scene->GetCamera()->GetFarlimit();
	maxdist = std::max (max_centre_dist, max_surf_dist + rad);
	max_patchres = *(int*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_SURFACEMAXLEVEL);
	max_patchres = std::min ((int)max_patchres, *(int*)g_client->GetConfigParam (CFGPRM_SURFACEMAXLEVEL));
	int tilever = *(int*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_TILEENGINE);
	if (tilever < 2) {
		surfmgr = new SurfaceManager (this);
		surfmgr2 = NULL;
	} else {
		surfmgr = NULL;
		int patchlvl = 2 << *(int*)g_client->GetConfigParam (CFGPRM_TILEPATCHRES);
		surfmgr2 = new TileManager2<SurfTile> (this, max_patchres, patchlvl);
		prm.horizon_excess = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_HORIZONEXCESS);
		prm.tilebb_excess = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_TILEBBEXCESS);
	}
	prm.horizon_minrad = std::min (1.0 + *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_MINELEVATION)/size, 1.0-1e-4);
	prm.bAtm = oapiPlanetHasAtmosphere (_hObj);
	if (prm.bAtm) {
		const ATMCONST *atmc = oapiGetPlanetAtmConstants(_hObj);
		prm.atm_href = log(atmc->rho0)*2e4 + 2e4;
		prm.atm_amb0 = std::min (0.7, log (atmc->rho0+1.0)*0.35);
		uint32_t amb0 = *(uint32_t*)g_client->GetConfigParam (CFGPRM_AMBIENTLEVEL);
		prm.amb0col = 0;
		for (int i = 0; i < 4; i++) prm.amb0col |= amb0 << (i<<3);
	}
	hazemgr = 0;
	hashaze = *(bool*)g_client->GetConfigParam (CFGPRM_ATMHAZE) && prm.bAtm;
	bRipple = *(bool*)g_client->GetConfigParam (CFGPRM_SURFACERIPPLE) &&
		*(bool*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_SURFACERIPPLE);
	if (bRipple) {
		if (surfmgr) surfmgr->SetMicrotexture ("waves.dds");
	}

	shadowalpha = (float)(1.0f - *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_SHADOWCOLOUR));
	bVesselShadow = *(bool*)g_client->GetConfigParam (CFGPRM_VESSELSHADOWS) &&
		shadowalpha >= 0.01;

	clouddata = 0;
	cloudmgr2 = 0;
	prm.bCloud = (*(bool*)g_client->GetConfigParam (CFGPRM_CLOUDS) &&
		*(bool*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_HASCLOUDS));
	if (prm.bCloud) {
		int cloudtilever = *(int*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDTILEENGINE);
		prm.cloudalt = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDALT);
		prm.bCloudBrighten = *(bool*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDOVERSATURATE);
		prm.bCloudShadow = *(bool*)g_client->GetConfigParam (CFGPRM_CLOUDSHADOWS);
		prm.shadowalpha = 1.0 - *(float*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDSHADOWCOL);
		if (prm.shadowalpha < 0.01)
			prm.bCloudShadow = false;
		if (cloudtilever == 1) { // legacy cloud engine
			clouddata = new CloudData;
			clouddata->cloudmgr = new CloudManager (this);
			clouddata->cloudshadow = prm.bCloudShadow;
			if (clouddata->cloudshadow) {
				clouddata->shadowalpha = (float)prm.shadowalpha;
			}
			if (*(bool*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDMICROTEX)) {
				clouddata->cloudmgr->SetMicrotexture ("cloud1.dds");
				clouddata->microalt0 = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDMICROALTMIN);
				clouddata->microalt1 = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDMICROALTMAX);
			} else {
				clouddata->microalt0 = 0;
				clouddata->microalt1 = 0;
			}
		} else { // v2 cloud engine
			int maxlvl = *(int*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_CLOUDMAXLEVEL);
			maxlvl = std::min ((int)maxlvl, *(int*)g_client->GetConfigParam (CFGPRM_SURFACEMAXLEVEL));
			cloudmgr2 = new TileManager2<CloudTile> (this, maxlvl, 32);
		}
	} else {
		prm.bCloudShadow = false;
	}

	if (*(bool*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_HASRINGS)) {
		double minrad = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_RINGMINRAD);
		double maxrad = *(double*)oapiGetObjectParam (_hObj, OBJPRM_PLANET_RINGMAXRAD);
		ringmgr = new RingManager (this, minrad, maxrad);
		render_rad = (float)(rad*maxrad);
	} else {
		ringmgr = 0;
	}
	
	memcpy (&fog, oapiGetObjectParam (_hObj, OBJPRM_PLANET_FOGPARAM), sizeof (FogParam));
	prm.bFogEnabled = (fog.dens_0 > 0);

	renderpix = false;
	patchres = 0;
/*
	mipmap_mode = g_client->Cfg()->PlanetMipmapMode;
	aniso_mode = g_client->Cfg()->PlanetAnisoMode;
*/
	nbase = oapiGetBaseCount (_hObj);
	vbase = new vBase*[nbase];
	for (int i = 0; i < nbase; i++)
		vbase[i] = NULL;

	mesh = NULL;
	if (surfmgr && surfmgr->GetMaxLevel() == 0) {
		char cbuf[256];
		oapiGetObjectName (hObj, cbuf, 256);
		MESHHANDLE hMesh = oapiLoadMesh (cbuf);
		if (hMesh) {
			mesh = new OGLMesh (hMesh);
			oapiDeleteMesh (hMesh);
		}
	}
}

// ==============================================================

vPlanet::~vPlanet ()
{
	if (nbase) {
		for (int i = 0; i < nbase; i++)
			if (vbase[i]) delete vbase[i];
		delete []vbase;
	}
	if (surfmgr) delete surfmgr;
	else if (surfmgr2) delete surfmgr2;
	if (cloudmgr2) delete cloudmgr2;

	if (clouddata) {
		delete clouddata->cloudmgr;
		delete clouddata;
	}
	if (hazemgr) delete hazemgr;
	if (ringmgr) delete ringmgr;
	if (mesh)    delete mesh;
}

// ==============================================================

bool vPlanet::Update ()
{
	if (!active) return false;

	vObject::Update();

	int i, j;
	float rad_scale = rad;
	bool rescale = false;
	dist_scale = 1.0f;

	if (cdist > maxdist) {
		rescale = true;
		dist_scale = (float)(max_centre_dist/cdist);
	}
	//if (cdist+render_rad > farplane && cdist-rad > 1e4) {
	//	rescale = true;
	//	dist_scale = (FLOAT)(farplane/(cdist+render_rad));
	//}
	if (rescale) {
		rad_scale *= dist_scale;
		mWorld[3][0] *= dist_scale;
		mWorld[3][1] *= dist_scale;
		mWorld[3][2] *= dist_scale;
	}

	// scale up from template sphere radius 1
	mWorld[0][0] *= rad_scale; mWorld[0][1] *= rad_scale; mWorld[0][2] *= rad_scale;
	mWorld[1][0] *= rad_scale; mWorld[1][1] *= rad_scale; mWorld[1][2] *= rad_scale;
	mWorld[2][0] *= rad_scale; mWorld[2][1] *= rad_scale; mWorld[2][2] *= rad_scale;

	// cloud layer world matrix
	if (prm.bCloud) {
		double cloudrad = size + prm.cloudalt;
		prm.cloudrot = *(double*)oapiGetObjectParam (hObj, OBJPRM_PLANET_CLOUDROTATION);
		prm.cloudvis = (cdist < cloudrad ? 1:0);
		if (cdist > cloudrad*(1.0-1.5e-4)) prm.cloudvis |= 2;
		prm.bCloudFlatShadows = (cdist >= 1.05*size);

		if (clouddata) {
			if (prm.cloudvis & 1) {
				clouddata->viewap = acos (size/cloudrad);
				if (size < cdist) clouddata->viewap += acos (size/cdist);
			} else {
				clouddata->viewap = 0;
			}

			float cloudscale = (float)(cloudrad/size);

			// world matrix for cloud shadows on the surface
			memcpy (&clouddata->mWorldC0, &mWorld, sizeof (glm::fmat4));
			/* FIXME: cloud shadows
			if (prm.cloudrot) {
				static D3DMATRIX crot (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
				crot._11 =   crot._33 = (float)cos(prm.cloudrot);
				crot._13 = -(crot._31 = (float)sin(prm.cloudrot));
				D3DMAT_MatrixMultiply (&clouddata->mWorldC0, &clouddata->mWorldC0, &crot);
			}*/

			// world matrix for cloud layer
			memcpy (&clouddata->mWorldC, &clouddata->mWorldC0, sizeof (glm::fmat4));
			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++) {
					clouddata->mWorldC[i][j] *= cloudscale;
				}

			// set microtexture intensity
			if(clouddata->microalt1-clouddata->microalt0 != 0) {
				double alt = cdist-rad;
				double lvl = (clouddata->microalt1-alt)/(clouddata->microalt1-clouddata->microalt0);
				clouddata->cloudmgr->SetMicrolevel (std::max (0.0, std::min (1.0, lvl)));
			}
		}
	}

	// check all base visuals
	if (nbase) {
		VECTOR3 pos, cpos = *scn->GetCamera()->GetGPos();
		double scale = (double)scn->ViewH()/scn->GetCamera()->GetTanAp();
		for (int i = 0; i < nbase; i++) {
			OBJHANDLE hBase = oapiGetBaseByIndex (hObj, i);
			oapiGetGlobalPos (hBase, &pos);
			double rad = oapiGetSize (hBase);
			double dst = dist (pos, cpos);
			double apprad = rad*scale/dst;
			if (vbase[i]) { // base visual exists
				if (apprad < 1.0) { // out of visual range
					delete vbase[i];
					vbase[i] = 0;
				}
			} else {        // base visual doesn't exist
				if (apprad > 2.0) { // within visual range
					vbase[i] = new vBase (hBase, scn);
				}
			}
			if (vbase[i])
				vbase[i]->Update();
		}
	}
	return true;
}

// ==============================================================

void vPlanet::CheckResolution ()
{
	double alt = std::max (1.0, cdist-rad);
	double apr = rad * scn->ViewH()*0.5 / (alt * scn->GetCamera()->GetTanAp());
	// apparent planet radius in units of screen pixels

	int new_patchres;
	double ntx;

	if (apr < 2.5) { // render planet as 2x2 pixels
		renderpix = true;
		new_patchres = 0;
		ntx = 0;
	} else {
		renderpix = false;
		ntx = PI*2.0 * apr;

		static const double scal2 = 1.0/log(2.0);
		const double shift = (surfmgr2 ? 6.0 : 5.0); // reduce level for tile mgr v2, because of increased patch size
		new_patchres = std::min (std::max ((int)(scal2*log(ntx)-shift),1), max_patchres);
	}
	if (new_patchres != patchres) {
		if (hashaze) {
			if (new_patchres < 1) {
				if (hazemgr) { delete hazemgr; hazemgr = 0; }
			} else {
				if (!hazemgr) { hazemgr = new HazeManager (this); }
			}
		}
		if (ringmgr) {
			int ringres = (new_patchres <= 3 ? 0 : new_patchres <= 4 ? 1:2);
			ringmgr->SetMeshRes (ringres);
		}
		patchres = new_patchres;
	}
}

// ==============================================================

void vPlanet::RenderZRange (double *nplane, double *fplane)
{
	double d = dotp (*scn->GetCamera()->GetGDir(), cpos);
	*fplane = std::max (1e3, d+rad*1.2);
	*nplane = std::max (1e0, d-rad*1.2);
	*fplane = std::min (*fplane, *nplane*1e5);
}

// ==============================================================

bool vPlanet::Render ()
{
	Renderer::CheckError("Render");
	if (!active) return false;

	if (renderpix) { // render as 2x2 pixel block

		RenderDot ();
		Renderer::CheckError("Render");

	} else {             // render as sphere

		bool nmlnml = true;
		if (mesh || !surfmgr2) {
			// old-style and mesh-based planet surfaces use a rescaled world matrix,
			// so we need to make sure that normals are renormalised
			nmlnml = g_client->mRenderContext.normalizeNormals;
			if (!nmlnml) g_client->mRenderContext.normalizeNormals = true;
		}

		uint32_t amb = prm.amb0col;
		//bool ringpostrender = false;
		float fogfactor;

		prm.bFog = prm.bFogEnabled;
		prm.bTint = prm.bFogEnabled;

		VECTOR3 skybg = scn->GetBgColour();
		prm.bAddBkg = ((skybg.x + skybg.y + skybg.z > 0) && (hObj != scn->GetCamera()->GetProxyBody()));

		if (ringmgr) {
			Renderer::PushBool(Renderer::CULL_FACE, false);
			ringmgr->Render(scn->GetCamera(), mWorld, false);
			Renderer::PopBool();
		}
		Renderer::CheckError("Render");

		if (prm.bCloud && (prm.cloudvis & 1))
			RenderCloudLayer (GL_CCW, prm);      // render clouds from below
		Renderer::CheckError("Render");

		if (hazemgr) hazemgr->Render (scn->GetCamera(), mWorld);       // horizon ring

		if (prm.bAtm) {
			if (ModLighting (amb))
				g_client->mRenderContext.ambient = amb;
		}
		Renderer::CheckError("Render");

		if (prm.bFog) { // set up distance fog
			double h = std::max (1.0, cdist-size);

			VECTOR3 fogcol = fog.col;
			double h_ref = fog.alt_ref;   // 3e3;
			double fog_0 = fog.dens_0;    // 5e-5;
			double fog_ref = fog.dens_ref; // 3e-5;
			double h_max = size*1.5; // At this altitude, fog effect drops to zero
			double scl = h_ref*fog_ref;

			if (h < h_ref) {
				// linear zone
				fogfactor = (float)(h/h_ref * (fog_ref-fog_0) + fog_0);
			} else {
				// hyperbolic zone: fogfactor = a/(h+b) + c
				// a, b and c are designed such that
				// * fogfactor(h) is continuous at h = h_ref
				// * d fogfactor / dh is continuous at h = h_ref
				// * fogfactor(h_max) = 0
				double b = - (fog_ref*h_max + (fog_ref-fog_0)*(h_max-h_ref)) / (fog_ref + (fog_ref-fog_0)/h_ref * (h_max-h_ref));
				double a = fog_ref*(h_ref+b)*(h_max+b)/(h_max-h_ref);
				double c = -a/(h_max+b);
				fogfactor = (float)(a/(h+b)+c);
			}

			if (fogfactor < 0.0) prm.bFog = false;
			else {
				// day/nighttime fog lighting
				VECTOR3 ppos;
				oapiGetGlobalPos (hObj, &ppos);
				double cosa = dotp (unit(ppos), unit(cpos));
				double bright = 1.0 * std::max (0.0, std::min (1.0, cosa + 0.3));
				float rfog = (float)(bright*(std::min(1.0,fogcol.x)+0.0)); // "whiten" the fog colour
				float gfog = (float)(bright*(std::min(1.0,fogcol.y)+0.0));
				float bfog = (float)(bright*(std::min(1.0,fogcol.z)+0.0));

				g_client->mRenderContext.bFog = true;
				g_client->mRenderContext.fogColor = {rfog, gfog, bfog};
				g_client->mRenderContext.fogDensity = fogfactor;
			}
		}
		Renderer::CheckError("Render");

		if (prm.bTint) {
			prm.rgbTint = *(VECTOR3*)oapiGetObjectParam (hObj, OBJPRM_PLANET_ATMTINTCOLOUR);
			double R = oapiGetSize (hObj);
			double alt = cdist - R;
			double alt_ref1 = fog.alt_ref*5.0;
			double alt_ref2 = alt_ref1 * 0.1;
			if (alt < alt_ref1) {
				double scale = (alt-alt_ref2)/(alt_ref1-alt_ref2);
				if (scale <= 0.0) prm.bTint = false;
				else prm.rgbTint *= scale;
			}
		}

		if (mesh) {
			Renderer::CheckError("Render");
			mesh->Render (scn->GetCamera(), mWorld);
			Renderer::CheckError("Render");
		} else {
			Renderer::CheckError("Render");
			bool using_zbuf;
			RenderSphere (prm, using_zbuf);            // planet surface
			Renderer::CheckError("Render");
		}

		if (nbase) RenderBaseStructures ();
		Renderer::CheckError("Render");

		if (prm.bAtm) {
			if (amb != prm.amb0col)
				g_client->mRenderContext.ambient = prm.amb0col;
		}

		if (prm.bFog) { // turn off fog
			g_client->mRenderContext.bFog = false;
		}

		if (prm.bCloud && (prm.cloudvis & 2)) {
			RenderCloudLayer (GL_CW, prm);	  // render clouds from above
			Renderer::CheckError("Render");
		}
		if (hazemgr) hazemgr->Render (scn->GetCamera(), mWorld, true); // haze across planet disc
		Renderer::CheckError("Render");

		if (ringmgr) {
			Renderer::PushBool(Renderer::CULL_FACE, false);
			ringmgr->Render (scn->GetCamera(), mWorld, true);
			Renderer::PopBool();
			Renderer::CheckError("Render");
		}

		if (!nmlnml) g_client->mRenderContext.normalizeNormals = false;
	}
	return true;
}

// ==============================================================

void vPlanet::ActivateLabels(bool activate)
{
	if (surfmgr2 && *(int*)oapiGetObjectParam(hObj, OBJPRM_PLANET_LABELENGINE) == 2) {
		if (activate) surfmgr2->CreateLabels();
		else          surfmgr2->DeleteLabels();
	}
}

// ==============================================================

void vPlanet::RenderLabels(oapi::Sketchpad *skp, oapi::Font **labelfont, int *fontidx)
{
	if (surfmgr2 && *(int*)oapiGetObjectParam(hObj, OBJPRM_PLANET_LABELENGINE) == 2)
		surfmgr2->RenderLabels(skp, labelfont, fontidx);
}

// ==============================================================

void vPlanet::RenderDot ()
{
	// to do
}

// ==============================================================

void vPlanet::RenderSphere (const RenderPrm &prm, bool &using_zbuf)
{
	using_zbuf = false;
/*
	if (mipmap_mode) {
		float fBias = (float)g_client->Cfg()->PlanetMipmapBias;
		dev->SetTextureStageState (0, D3DTSS_MIPFILTER, mipmap_mode == 1 ? D3DTFP_POINT:D3DTFP_LINEAR);
		dev->SetTextureStageState (0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&fBias)) );
	}
	if (aniso_mode > 1) {
		dev->SetTextureStageState (0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC);
		dev->SetTextureStageState (0, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC);
		dev->SetTextureStageState (0, D3DTSS_MAXANISOTROPY, aniso_mode);
	}
*/
	// for planets seen through an atmospheric layer from the surface of
	// another planet, add the ambient atmosphere colour to the rendering
	if (prm.bAddBkg) {
//		dev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_ADD);
//		dev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_CURRENT);
//		dev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		g_client->mRenderContext.backgroungColor = g_client->GetScene()->GetBgColour();
//		dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, bgc);
	}
	Renderer::CheckError("RenderSphere");

	//Renderer::PushFrontFace(Renderer::FrontFace::CW);
	if (surfmgr2) {
		if (cdist >= 1.3*rad && cdist > 3e6) {
			surfmgr2->Render (dmWorld, false, prm);
			Renderer::CheckError("RenderSphere");
		} else {
			Renderer::PushBool(Renderer::DEPTH_TEST, true);
			Renderer::PushDepthMask(true);
			surfmgr2->Render (dmWorld, true, prm);
			Renderer::PopDepthMask();
			Renderer::PopBool();
			//using_zbuf = true;
			Renderer::CheckError("RenderSphere");
		}
	} else {
		//mercury, venus, saturn, titan, hyperion, uranus, miranda, ariel, umbriel, titan, oberon, neptune, triton, proteus, nereide 
		surfmgr->Render (mWorld, dist_scale, patchres, 0.0, prm.bFog); // surface
		Renderer::CheckError("RenderSphere");
	}
	//Renderer::PopFrontFace();

	if (prm.bAddBkg) {
//		dev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
//		dev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
//		dev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		g_client->mRenderContext.backgroungColor = {0,0,0};
	}

	if (nbase) {
		RenderBaseSurfaces ();                     // base surfaces
		Renderer::CheckError("RenderSphere");
		RenderBaseShadows (shadowalpha);         // base 
		Renderer::CheckError("RenderSphere");
	}
/*
	if (mipmap_mode) {
		float fBias = 0.0f;
		dev->SetTextureStageState (0, D3DTSS_MIPFILTER, D3DTFP_NONE);
		dev->SetTextureStageState (0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&fBias)) );
	}
	if (aniso_mode > 1) {
		dev->SetTextureStageState (0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
		dev->SetTextureStageState (0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		dev->SetTextureStageState (0, D3DTSS_MAXANISOTROPY, 1);
	}
	*/
	if (prm.bCloudShadow) {
		RenderCloudShadows (prm);                // cloud shadows
		Renderer::CheckError("RenderSphere");
	}

	if (bVesselShadow && hObj == oapiCameraProxyGbody()) {
	// cast shadows only on planet closest to camera
		scn->RenderVesselShadows (hObj, shadowalpha); // vessel shadows
		Renderer::CheckError("RenderSphere");
	}

}

// ==============================================================

void vPlanet::RenderCloudLayer (GLenum cullmode, const RenderPrm &prm)
{
	Renderer::CheckError("RenderCloudLayer");
	Renderer::PushFrontFace((Renderer::FrontFace)cullmode);
	Renderer::PushBool(Renderer::BLEND, true);
	if (cloudmgr2)
		cloudmgr2->Render (dmWorld, false, prm);
	else
		clouddata->cloudmgr->Render (clouddata->mWorldC, dist_scale, std::min(patchres,8), clouddata->viewap); // clouds
	Renderer::PopBool();
	Renderer::PopFrontFace();
	Renderer::CheckError("RenderCloudLayer");
}

// ==============================================================

void vPlanet::RenderCloudShadows (const RenderPrm &prm)
{
	if (cloudmgr2) {
		if (prm.bCloudFlatShadows)
			cloudmgr2->RenderFlatCloudShadows (dmWorld, prm);
	} else if (clouddata) { // legacy method
		OGLMaterial pmat;
		static OGLMaterial cloudmat = {{0,0,0,1},{0,0,0,1},{0,0,0,0},{0,0,0,0},0};

		float alpha = clouddata->shadowalpha;
		cloudmat.diffuse.a = cloudmat.ambient.a = alpha;

		pmat = g_client->mRenderContext.material;
		g_client->mRenderContext.material = cloudmat;

		Renderer::PushBool(Renderer::BLEND, true);
		//dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		clouddata->cloudmgr->Render (clouddata->mWorldC0, std::min(patchres,8), (int)clouddata->viewap);

		//dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		Renderer::PopBool();
		g_client->mRenderContext.material = pmat;
	}
}

// ==============================================================

void vPlanet::RenderBaseSurfaces ()
{
	Renderer::PushBool(Renderer::BLEND, true);
	for (int i = 0; i < nbase; i++) {
		if (vbase[i]) {
			vbase[i]->RenderSurface ();
		}
	}

	// restore render state
	Renderer::PopBool();
}

// ==============================================================

void vPlanet::RenderBaseShadows (float depth)
{
	// set device parameters
	Renderer::PushBool(Renderer::BLEND, true);
	Renderer::PushBool(Renderer::DEPTH_TEST, true);
	glStencilMask(1);
	glStencilFunc(GL_NOTEQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(0,0,0,depth));
	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
*/
	for (int i = 0; i < nbase; i++)
		if (vbase[i])
			vbase[i]->RenderGroundShadow (depth);

	// reset device parameters
	Renderer::PopBool(2);
/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
*/
}

// ==============================================================

void vPlanet::RenderBaseStructures ()
{
	Renderer::PushBool(Renderer::DEPTH_TEST, true);
	Renderer::PushDepthMask(true);

	for (int i = 0; i < nbase; i++) {
		if (vbase[i]) {
			vbase[i]->RenderStructures ();
		}
	}
	Renderer::PopDepthMask();
	Renderer::PopBool();
}

// ==============================================================

bool vPlanet::ModLighting (uint32_t &ambient)
{
	// modify ambient light level inside atmospheres as a function of sun elevation
	if (!prm.bAtm) return false;
	if (cdist >= size+prm.atm_href) return false;

	double alpha = acos (dotp (unit(*scn->GetCamera()->GetGPos()), -unit(cpos)));
	// angular distance between sun and planet as seen from camera

	double sunelev = alpha - PI05; // elevation of sun above horizon (assuming camera on ground)
	if (sunelev < -14.0*RAD) return false;  // total darkness

	double rscale = (size-cdist)/prm.atm_href + 1.0;    // effect altitude scale (1 on ground, 0 at reference alt)
	double amb = prm.atm_amb0 * std::min (1.0, (sunelev+14.0*RAD)/(20.0*RAD)); // effect magnitude (dependent on sun elevation)
	if (amb < 0.05) return false;
	amb = std::max (0.0, amb-0.05);

	uint32_t addamb = (uint32_t)(amb*rscale*256.0);
	uint32_t newamb = *(uint32_t*)g_client->GetConfigParam (CFGPRM_AMBIENTLEVEL) + addamb;
	ambient = 0;
	for (int i = 0; i < 4; i++)
		ambient |= std::min ((uint32_t)255, newamb) << (i<<3);
	return true;
}
