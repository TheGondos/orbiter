#include "VPlanet.h"
#include "SurfMgr.h"
#include "RingMgr.h"
#include "HazeMgr.h"
#include "CloudMgr.h"
#include "Scene.h"
#include "OGLCamera.h"
#include "surfmgr2.h"
#include "cloudmgr2.h"
#include "VBase.h"
#include <cstring>

static double max_surf_dist = 1e4;

VPlanet::VPlanet (OBJHANDLE handle):VObject(handle)
{
	render_rad = (float)(0.1*mSize);
	dist_scale = 1.0f;
	max_centre_dist = 0.9*g_client->GetScene()->GetCamera()->GetFarlimit();
	maxdist = std::max (max_centre_dist, max_surf_dist + mSize);
	max_patchres = *(int*)oapiGetObjectParam (handle, OBJPRM_PLANET_SURFACEMAXLEVEL);
	max_patchres = std::min ((int)max_patchres, *(int*)g_client->GetConfigParam (CFGPRM_SURFACEMAXLEVEL));
	patchres = 0;

   	int tilever = *(int*)oapiGetObjectParam (handle, OBJPRM_PLANET_TILEENGINE);

	if (tilever < 2) {
		surfmgr = new SurfaceManager (this);
		surfmgr2 = nullptr;
	} else {
		surfmgr = nullptr;
		int patchlvl = 2 << *(int*)g_client->GetConfigParam (CFGPRM_TILEPATCHRES);
		surfmgr2 = new TileManager2<SurfTile> (this, max_patchres, patchlvl);
		prm.horizon_excess = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_HORIZONEXCESS);
		prm.tilebb_excess = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_TILEBBEXCESS);
	}

	prm.horizon_minrad = std::min (1.0 + *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_MINELEVATION)/mSize, 1.0-1e-4);
	prm.bAtm = oapiPlanetHasAtmosphere (handle);
	if (prm.bAtm) {
		const ATMCONST *atmc = oapiGetPlanetAtmConstants(handle);
		prm.atm_href = log(atmc->rho0)*2e4 + 2e4;
		prm.atm_amb0 = std::min (0.7, log (atmc->rho0+1.0)*0.35);
		int amb0 = *(int*)g_client->GetConfigParam (CFGPRM_AMBIENTLEVEL);
		prm.amb0col = 0;
		for (int i = 0; i < 4; i++) prm.amb0col |= amb0 << (i<<3);
	}

	bool hashaze = *(bool*)g_client->GetConfigParam (CFGPRM_ATMHAZE) && prm.bAtm;
	if(hashaze) {
		hazemgr = new HazeManager(this);
	} else {
		hazemgr = nullptr;
	}

	if (*(bool*)oapiGetObjectParam (handle, OBJPRM_PLANET_HASRINGS)) {
		double minrad = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_RINGMINRAD);
		double maxrad = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_RINGMAXRAD);

		ringmgr = new RingManager (this, minrad, maxrad);
		render_rad = (float)(mSize*maxrad);
	} else {
		ringmgr = nullptr;
	}

	memcpy (&fog, oapiGetObjectParam (mHandle, OBJPRM_PLANET_FOGPARAM), sizeof (FogParam));
	prm.bFogEnabled = (fog.dens_0 > 0);

	prm.bCloud = (*(bool*)g_client->GetConfigParam (CFGPRM_CLOUDS) &&
		*(bool*)oapiGetObjectParam (handle, OBJPRM_PLANET_HASCLOUDS));
	if (prm.bCloud) {
			char name[256];
			oapiGetObjectName (mHandle, name, 256);
		int cloudtilever = *(int*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDTILEENGINE);
		prm.cloudalt = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDALT);
		prm.bCloudBrighten = *(bool*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDOVERSATURATE);
		prm.bCloudShadow = *(bool*)g_client->GetConfigParam (CFGPRM_CLOUDSHADOWS);
		prm.shadowalpha = 1.0 - *(float*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDSHADOWCOL);
		if (prm.shadowalpha < 0.01)
			prm.bCloudShadow = false;
		if (cloudtilever == 1) { // legacy cloud engine
			clouddata = new CloudData;
			clouddata->cloudmgr = new CloudManager (this);
			clouddata->cloudshadow = prm.bCloudShadow;
			if (clouddata->cloudshadow) {
				clouddata->shadowalpha = (float)prm.shadowalpha;
			}
			if (*(bool*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDMICROTEX)) {
				clouddata->cloudmgr->SetMicrotexture ("cloud1.dds");
				clouddata->microalt0 = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDMICROALTMIN);
				clouddata->microalt1 = *(double*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDMICROALTMAX);
			}
			cloudmgr2 = nullptr;
		} else { // v2 cloud engine
			clouddata = nullptr;
			int maxlvl = *(int*)oapiGetObjectParam (handle, OBJPRM_PLANET_CLOUDMAXLEVEL);
			maxlvl = std::min ((int)maxlvl, *(int*)g_client->GetConfigParam (CFGPRM_SURFACEMAXLEVEL));
			cloudmgr2 = new TileManager2<CloudTile> (this, maxlvl, 32);
		}
	} else {
		prm.bCloudShadow = false;
	}

	nbase = oapiGetBaseCount (handle);
	vbase = new VBase*[nbase];
	for (uint32_t i = 0; i < nbase; i++)
		vbase[i] = NULL;
}

VPlanet::~VPlanet()
{

}


bool VPlanet::Update ()
{
	VObject::Update();

	float rad_scale = mSize;
	bool rescale = false;
	
	if (cdist > maxdist) {
		rescale = true;
		dist_scale = (float)(max_centre_dist/cdist);
	}

	if (rescale) {
		rad_scale *= dist_scale;
		mModel[3][0] *= dist_scale;
		mModel[3][1] *= dist_scale;
		mModel[3][2] *= dist_scale;
	}

	// scale up from template sphere radius 1
	mModel[0][0] *= rad_scale; mModel[0][1] *= rad_scale; mModel[0][2] *= rad_scale;
	mModel[1][0] *= rad_scale; mModel[1][1] *= rad_scale; mModel[1][2] *= rad_scale;
	mModel[2][0] *= rad_scale; mModel[2][1] *= rad_scale; mModel[2][2] *= rad_scale;


	//if (!mVisible) return false;
/*
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
	//	dist_scale = (float)(farplane/(cdist+render_rad));
	//}
	if (rescale) {
		rad_scale *= dist_scale;
		mWorld._41 *= dist_scale;
		mWorld._42 *= dist_scale;
		mWorld._43 *= dist_scale;
	}

	// scale up from template sphere radius 1
	mWorld._11 *= rad_scale; mWorld._12 *= rad_scale; mWorld._13 *= rad_scale;
	mWorld._21 *= rad_scale; mWorld._22 *= rad_scale; mWorld._23 *= rad_scale;
	mWorld._31 *= rad_scale; mWorld._32 *= rad_scale; mWorld._33 *= rad_scale;*/

	// cloud layer world matrix
	if (prm.bCloud) {
		int i,j;
		double cloudrad = mSize + prm.cloudalt;
		prm.cloudrot = *(double*)oapiGetObjectParam (mHandle, OBJPRM_PLANET_CLOUDROTATION);
		prm.cloudvis = (cdist < cloudrad ? 1:0);
		if (cdist > cloudrad*(1.0-1.5e-4)) prm.cloudvis |= 2;
		prm.bCloudFlatShadows = (cdist >= 1.05*mSize);

		if (clouddata && clouddata->cloudmgr) {
			if (prm.cloudvis & 1) {
				clouddata->viewap = acos (mSize/cloudrad);
				if (mSize < cdist) clouddata->viewap += acos (mSize/cdist);
			} else {
				clouddata->viewap = 0;
			}

			float cloudscale = (float)(cloudrad/mSize);

			// world matrix for cloud shadows on the surface
			clouddata->mWorldC0 = mModel;
			/* FIXME: cloud shadows
			if (prm.cloudrot) {
				static D3DMATRIX crot (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
				crot._11 =   crot._33 = (float)cos(prm.cloudrot);
				crot._13 = -(crot._31 = (float)sin(prm.cloudrot));
				D3DMAT_MatrixMultiply (&clouddata->mWorldC0, &clouddata->mWorldC0, &crot);
			}*/

			// world matrix for cloud layer
			clouddata->mWorldC = clouddata->mWorldC0;
			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++) {
					clouddata->mWorldC[i][j] *= cloudscale;
				}

			// set microtexture intensity
			double alt = cdist-mSize;
			double lvl = (clouddata->microalt1-alt)/(clouddata->microalt1-clouddata->microalt0);
			clouddata->cloudmgr->SetMicrolevel (std::max (0.0, std::min (1.0, lvl)));
		}
	}


	// check all base visuals
	if (nbase) {
		VECTOR3 pos, cpos;
		glm::dvec3 tmp = *g_client->GetScene()->GetCamera()->GetGPos();
		cpos.x = tmp.x;
		cpos.y = tmp.y;
		cpos.z = tmp.z;
		double scale = (double)g_client->GetScene()->GetCamera()->GetHeight()/g_client->GetScene()->GetCamera()->GetTanAp();
		for (uint32_t i = 0; i < nbase; i++) {
			OBJHANDLE hBase = oapiGetBaseByIndex (mHandle, i);
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
					vbase[i] = new VBase (hBase);
				}
			}
			if (vbase[i])
				vbase[i]->Update();
		}
	}

	return true;
}
void VPlanet::RenderBaseSurfaces (OGLCamera *c)
{
	for (uint32_t i = 0; i < nbase; i++) {
		if (vbase[i]) {
			//if (!state_check) {
			//	dev->GetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, &alpha);
			//	if (!alpha)
			//		dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			//	state_check = true;
			//}
			vbase[i]->RenderSurface (c);
		}
	}
}

void VPlanet::RenderBaseStructures (OGLCamera *c)
{
	bool zmod = false, zcheck = false;
	int bz = false, bzw = false;

	for (uint32_t i = 0; i < nbase; i++) {
		if (vbase[i]) {
			if (!zcheck) { // enable zbuffer
				//dev->GetRenderState (D3DRENDERSTATE_ZENABLE, &bz);
				//dev->GetRenderState (D3DRENDERSTATE_ZWRITEENABLE, &bzw);
				if (!bz || !bzw) {
					//dev->SetRenderState (D3DRENDERSTATE_ZENABLE, TRUE);
					//dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
					//scn->GetCamera()->SetFustrumLimits (1, 1e5);
					zmod = true;
				}
				zcheck = true;
			}
			vbase[i]->RenderStructures (c);
		}
	}
	if (zmod) {
		//dev->SetRenderState (D3DRENDERSTATE_ZENABLE, bz);
		//dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, bzw);
		//scn->GetCamera()->SetFustrumLimits (10, 1e6);
	}
}

bool VPlanet::ModLighting (uint32_t &ambient)
{
	// modify ambient light level inside atmospheres as a function of sun elevation
	if (!prm.bAtm) return false;
	if (cdist >= mSize+prm.atm_href) return false;

	VECTOR3 cpos;
	glm::dvec3 tmp = *g_client->GetScene()->GetCamera()->GetGPos();
	cpos.x = tmp.x;
	cpos.y = tmp.y;
	cpos.z = tmp.z;

	double alpha = acos (dotp (unit(cpos), -unit(cpos)));
	// angular distance between sun and planet as seen from camera

	double sunelev = alpha - PI05; // elevation of sun above horizon (assuming camera on ground)
	if (sunelev < -14.0*RAD) return false;  // total darkness

	double rscale = (mSize-cdist)/prm.atm_href + 1.0;    // effect altitude scale (1 on ground, 0 at reference alt)
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

bool VPlanet::Render (OGLCamera *c)
{
	if (!mVisible) return false;

	glDisable(GL_DEPTH_TEST);
	if(ringmgr) {
		glDisable(GL_CULL_FACE);
		ringmgr->Render(mModel, c, false);
		glEnable(GL_CULL_FACE);
	}
	if (prm.bCloud && (prm.cloudvis & 1))
		RenderCloudLayer ( true, prm);      // render clouds from below

	glDisable(GL_CULL_FACE);
	if (hazemgr) hazemgr->Render (c, mModel);       // horizon ring
		glEnable(GL_CULL_FACE);

	VECTOR3 skybg = g_client->GetScene()->SkyColour();
	double sum = skybg.x + skybg.y + skybg.z;
	prm.bAddBkg = sum > 0.0 && mHandle != oapiCameraProxyGbody();
	prm.bFog = prm.bFogEnabled;

	if (prm.bFog) { // set up distance fog
		double h = std::max (1.0, cdist-mSize);

		VECTOR3 fogcol = fog.col;
		double h_ref = fog.alt_ref;   // 3e3;
		double fog_0 = fog.dens_0;    // 5e-5;
		double fog_ref = fog.dens_ref; // 3e-5;
		double h_max = mSize*1.5; // At this altitude, fog effect drops to zero
		double scl = h_ref*fog_ref;
		float fogfactor;
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
			oapiGetGlobalPos (mHandle, &ppos);
			double cosa = dotp (unit(ppos), unit(cpos));
			double bright = 1.0 * std::max (0.0, std::min (1.0, cosa + 0.3));
			float rfog = (float)(bright*(std::min(1.0,fogcol.x)+0.0)); // "whiten" the fog colour
			float gfog = (float)(bright*(std::min(1.0,fogcol.y)+0.0));
			float bfog = (float)(bright*(std::min(1.0,fogcol.z)+0.0));

			prm.mFogColor = glm::vec4(rfog, gfog, bfog, 1.0);
			prm.mFogDensity = fogfactor;
		}
	}


    RenderSphere();

	RenderBaseSurfaces(c);
	RenderBaseStructures(c);


	// TODO: disable fog



	if (prm.bCloudShadow) {
		RenderCloudShadows (c, prm);                // cloud shadows
	}

	if (prm.bCloud && (prm.cloudvis & 2))
		RenderCloudLayer ( false, prm);	  // render clouds from above

	glDisable(GL_CULL_FACE);
	if (hazemgr) hazemgr->Render (c, mModel, true);       // horizon ring
	glEnable(GL_CULL_FACE);

	if(ringmgr) {
		glDisable(GL_CULL_FACE);
			ringmgr->Render(mModel, c, true);
		glEnable(GL_CULL_FACE);
	}
/*
	if (renderpix) { // render as 2x2 pixel block

		RenderDot (dev);

	} else {             // render as sphere

		int nmlnml = TRUE;
		if (mesh || !surfmgr2) {
			// old-style and mesh-based planet surfaces use a rescaled world matrix,
			// so we need to make sure that normals are renormalised
			dev->GetRenderState (D3DRENDERSTATE_NORMALIZENORMALS, &nmlnml);
			if (!nmlnml) dev->SetRenderState (D3DRENDERSTATE_NORMALIZENORMALS, TRUE);
		}

		int amb = prm.amb0col;
		bool ringpostrender = false;
		bool clear_zbuf = false;
		float fogfactor;

		prm.bFog = prm.bFogEnabled;
		prm.bTint = prm.bFogEnabled;

		D3DCOLOR skybg = scn->GetBgColour();
		prm.bAddBkg = ((skybg & 0xFFFFFF) && (hObj != scn->GetCamera()->GetProxyBody()));

		if (ringmgr) {
			if (cdist < rad*ringmgr->InnerRad()) { // camera inside inner ring edge
				ringmgr->Render (dev, mWorld);
			} else {
				// if the planet has a ring system we update the z-buffer
				// but don't do z-checking for the planet surface
				// This strategy could do with some reconsideration
				dev->SetRenderState (D3DRENDERSTATE_ZENABLE, TRUE);
				dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
				dev->SetRenderState (D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
				ringpostrender = true;
				clear_zbuf = true;
			}
		}
		if (prm.bCloud && (prm.cloudvis & 1))
			RenderCloudLayer (dev, D3DCULL_CW, prm);      // render clouds from below
		if (hazemgr) hazemgr->Render (dev, mWorld);       // horizon ring

		if (prm.bAtm) {
			if (ModLighting (amb))
				dev->SetRenderState (D3DRENDERSTATE_AMBIENT, amb);
		}

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
				dev->SetRenderState (D3DRENDERSTATE_FOGENABLE, TRUE);
				dev->SetRenderState (D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
				dev->SetRenderState (D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP);
				dev->SetRenderState (D3DRENDERSTATE_FOGCOLOR, D3DRGB(rfog,gfog,bfog));
				dev->SetRenderState (D3DRENDERSTATE_FOGDENSITY, *((int*)(&fogfactor)));
			}
		}

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
			dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &mWorld);
			mesh->Render (dev);
		} else {
			bool using_zbuf;
			RenderSphere (dev, prm, using_zbuf);            // planet surface
			if (using_zbuf) clear_zbuf = false;
		}

		if (nbase) RenderBaseStructures (dev);

		if (prm.bAtm) {
			if (amb != prm.amb0col)
				dev->SetRenderState (D3DRENDERSTATE_AMBIENT, prm.amb0col);
		}

		if (prm.bFog) { // turn off fog
			dev->SetRenderState (D3DRENDERSTATE_FOGENABLE, FALSE);
			dev->SetRenderState (D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
		}

		if (ringpostrender) {
			// reset z-comparison function and disable z-buffer
			dev->SetRenderState (D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
			dev->SetRenderState (D3DRENDERSTATE_ZENABLE, FALSE);
		}
		if (prm.bCloud && (prm.cloudvis & 2))
			RenderCloudLayer (dev, D3DCULL_CCW, prm);	  // render clouds from above
		if (hazemgr) hazemgr->Render (dev, mWorld, true); // haze across planet disc
		if (ringpostrender) {
			// turn z-buffer on for ring system
			dev->SetRenderState (D3DRENDERSTATE_ZENABLE, TRUE);
			ringmgr->Render (dev, mWorld);
			dev->SetRenderState (D3DRENDERSTATE_ZENABLE, FALSE);
			dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, FALSE);
		}
		if (clear_zbuf)
			dev->Clear (0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0L);

		if (!nmlnml) dev->SetRenderState (D3DRENDERSTATE_NORMALIZENORMALS, FALSE);
	}
    */
	return true;
}

void VPlanet::RenderCloudLayer (int cullmode, const RenderPrm &prm)
{
//	if (cullmode != D3DCULL_CCW) dev->SetRenderState (D3DRENDERSTATE_CULLMODE, cullmode);
//	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//	if (cloudmgr2)
//		cloudmgr2->Render (dmWorld, false, prm);
//	else

   // double dist_scale = 1.0f;
	if(clouddata && clouddata->cloudmgr)
		clouddata->cloudmgr->Render (clouddata->mWorldC, dist_scale, std::min(patchres,8), clouddata->viewap); // clouds
	else if (cloudmgr2)
		cloudmgr2->Render (dmWorld, false, prm);

//	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
//	if (cullmode != D3DCULL_CCW) dev->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
}

void VPlanet::RenderCloudShadows (OGLCamera *c, const RenderPrm &prm)
{
	if (cloudmgr2) {
		if (prm.bCloudFlatShadows)
			cloudmgr2->RenderFlatCloudShadows (dmWorld, prm);
	} else if (clouddata) { // legacy method
//		D3DMATERIAL7 pmat;
//		static D3DMATERIAL7 cloudmat = {{0,0,0,1},{0,0,0,1},{0,0,0,0},{0,0,0,0},0};

		//float alpha = clouddata->shadowalpha;
		//cloudmat.diffuse.a = cloudmat.ambient.a = alpha;

//		dev->GetMaterial (&pmat);
//		dev->SetMaterial (&cloudmat);

		//int ablend;
//		dev->GetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, &ablend);
//		if (!ablend)
//			dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		clouddata->cloudmgr->Render (clouddata->mWorldC0, std::min(patchres,8), (int)clouddata->viewap);

//		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
//		if (!ablend)
//			dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
//		dev->SetMaterial (&pmat);
	}
}

void VPlanet::RenderSphere ()
{
	if (surfmgr2) {
		if (cdist >= 1.3*mSize && cdist > 3e6) {
			surfmgr2->Render (dmWorld, false, prm);
		} else {
			glEnable(GL_DEPTH_TEST);
			surfmgr2->Render (dmWorld, true, prm);
			glDisable(GL_DEPTH_TEST);
		}
	} else {
		surfmgr->Render (mModel, dist_scale, patchres, 0.0, prm.bFog); // surface
	}
}

void VPlanet::CheckResolution ()
{
	double alt = std::max (1.0, cdist-mSize);
	double apr = mSize * g_client->GetScene()->GetCamera()->GetHeight()*0.5 / (alt * g_client->GetScene()->GetCamera()->GetTanAp());
	// apparent planet radius in units of screen pixels

	int new_patchres;
	double ntx;

	if (apr < 2.5) { // render planet as 2x2 pixels
		//renderpix = true;
		new_patchres = 0;
		ntx = 0;
	} else {
		//renderpix = false;
		ntx = PI*2.0 * apr;

		static const double scal2 = 1.0/log(2.0);
		const double shift = (surfmgr2 ? 6.0 : 5.0); // reduce level for tile mgr v2, because of increased patch size
		new_patchres = std::min (std::max ((int)(scal2*log(ntx)-shift),1), max_patchres);
	}
	if (new_patchres != patchres) {
		/*
		if (hashaze) {
			if (new_patchres < 1) {
				if (hazemgr) { delete hazemgr; hazemgr = 0; }
			} else {
				if (!hazemgr) { hazemgr = new HazeManager (this); }
			}
		}*/
		if (ringmgr) {
			int ringres = (new_patchres <= 3 ? 0 : new_patchres <= 4 ? 1:2);
			ringmgr->SetMeshRes (ringres);
		}
		patchres = new_patchres;
	}
}
