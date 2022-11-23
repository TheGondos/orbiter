// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VBase.cpp
// class vBase (implementation)
//
// A vBase is the visual representation of a surface base
// object (a "spaceport" on the surface of a planet or moon,
// usually with runways or landing pads where vessels can
// land and take off.
// ==============================================================

#include "glad.h"
#include "VBase.h"
#include "TileMgr.h"
#include "OGLClient.h"
#include "OGLCamera.h"
#include "Renderer.h"
#include <cstring>

struct VERTEX_XYZ   { float x, y, z; };

vBase::vBase (OBJHANDLE _hObj, const Scene *scene): vObject (_hObj, scene)
{
	int i;

	// load surface tiles
	ntile = g_client->GetBaseTileList (_hObj, &tspec);
	if (ntile) {
		tile = new SurfTile[ntile];
		for (i = 0; i < ntile; i++) {
			tile[i].mesh = new OGLMesh (tspec[i].mesh);
		}
	}

	// load meshes for generic structures
	MESHHANDLE *sbs, *sas;
	int nsbs, nsas;
	g_client->GetBaseStructures (_hObj, &sbs, &nsbs, &sas, &nsas);
	if (nstructure_bs = nsbs) {
		structure_bs = new OGLMesh*[nsbs];
		for (i = 0; i < nsbs; i++) structure_bs[i] = new OGLMesh (sbs[i]);
	}
	if (nstructure_as = nsas) {
		structure_as = new OGLMesh*[nsas];
		for (i = 0; i < nsas; i++) structure_as[i] = new OGLMesh (sas[i]);
	}
	SetupShadowMeshes ();
	//localLight = *scene->GetLight();
	bLocalLight = false;
	lights = false;
	Tchk = Tlghtchk = oapiGetSimTime()-1.0;
}

vBase::~vBase ()
{
	int i;

	if (ntile) {
		for (i = 0; i < ntile; i++)
			delete tile[i].mesh;
		delete []tile;
	}
	if (nstructure_bs) {
		for (i = 0; i < nstructure_bs; i++)
			delete structure_bs[i];
		delete []structure_bs;
	}
	if (nstructure_as) {
		for (i = 0; i < nstructure_as; i++)
			delete structure_as[i];
		delete []structure_as;
	}
	if (nshmesh) {
		for (i = 0; i < nshmesh; i++) {
			delete shmesh[i].vbuf;
			delete shmesh[i].idx;
		}
		delete []shmesh;
	}
}

void vBase::GlobalInit()
{
	shadowShader = Renderer::GetShader("GroundShadow");
}

void vBase::SetupShadowMeshes ()
{
	nshmesh = 0;

	// Get mesh geometries for all base structures
	int i, j, k, m, nmesh, ngrp, nssh;
	MESHHANDLE *ssh;
	double *ecorr;
	g_client->GetBaseShadowGeometry (hObj, &ssh, &ecorr, &nssh);
	if (!nssh) return;

	// Re-assemble meshes according to surface elevation correction heights.
	// All objects with similar corrections can use the same transformation
	// matrix and can therefore be merged for improved performance.
	// This only works for static meshes. Any dynamically animated meshes
	// should be stored separately.
	struct EGROUP {
		MESHHANDLE *mesh;
		int nmesh, nvtx, nidx;
		int bin;
	} *eg;
	const double d_ecorr = 0.2; // correction bin width
	for (i = 0; i < nssh; i++) {
		int bin = (int)(ecorr[i]/d_ecorr);
		for (j = 0; j < nshmesh; j++)
			if (bin == eg[j].bin) break;
		if (j == nshmesh) {   // create new bin
			EGROUP *tmp = new EGROUP[nshmesh+1];
			if (nshmesh) {
				memcpy (tmp, eg, nshmesh*sizeof(EGROUP));
				delete []eg;
			}
			eg = tmp;
			eg[nshmesh].nmesh = eg[nshmesh].nvtx = eg[nshmesh].nidx = 0;
			eg[nshmesh].bin = bin;
			nshmesh++;
		}
		nmesh = eg[j].nmesh;
		MESHHANDLE *tmp = new MESHHANDLE[nmesh+1];
		if (nmesh) {
			memcpy (tmp, eg[j].mesh, nmesh*sizeof(MESHHANDLE));
			delete []eg[j].mesh;
		}
		eg[j].mesh = tmp;
		eg[j].mesh[nmesh] = ssh[i];
		ngrp = oapiMeshGroupCount (ssh[i]);
		for (k = 0; k < ngrp; k++) {
			MESHGROUP *grp = oapiMeshGroup (ssh[i], k);
			if (grp) {
				if (grp->UsrFlag & 1) continue; // "no shadows" flag
				eg[j].nvtx += grp->nVtx;
				eg[j].nidx += grp->nIdx;
			}
		}
		eg[j].nmesh++;
	}

	shmesh = new ShadowMesh[nshmesh];
	VERTEX_XYZ *vtx;
	for (i = 0; i < nshmesh; i++) {
		shmesh[i].va = new VertexArray();
		shmesh[i].va->Bind();
		shmesh[i].vbuf = new VertexBuffer(nullptr, eg[i].nvtx * sizeof(VERTEX_XYZ));
		shmesh[i].vbuf->Bind();
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
		glEnableVertexAttribArray(0);

		vtx = (VERTEX_XYZ *)shmesh[i].vbuf->Map();
		uint16_t *idx = new uint16_t[eg[i].nidx];
		shmesh[i].nvtx = 0;
		shmesh[i].nidx = 0;
		shmesh[i].ecorr = (eg[i].bin-0.5)*d_ecorr;
		for (j = 0; j < eg[i].nmesh; j++) {
			MESHHANDLE mesh = eg[i].mesh[j];
			ngrp = oapiMeshGroupCount (mesh);
			for (k = 0; k < ngrp; k++) {
				MESHGROUP *grp = oapiMeshGroup (mesh, k);
				if (grp->UsrFlag & 1) continue; // "no shadows" flag
				VERTEX_XYZ *vtgt = vtx + shmesh[i].nvtx;
				uint16_t *itgt = idx + shmesh[i].nidx;
				NTVERTEX *vsrc = grp->Vtx;
				uint16_t *isrc = grp->Idx;
				uint16_t iofs = (uint16_t)shmesh[i].nvtx;
				for (m = 0; m < grp->nVtx; m++) {
					vtgt[m].x = vsrc[m].x;
					vtgt[m].y = vsrc[m].y;
					vtgt[m].z = vsrc[m].z;
				}
				for (m = 0; m < grp->nIdx; m++)
					*itgt++ = *isrc++ + iofs;
				shmesh[i].nvtx += grp->nVtx;
				shmesh[i].nidx += grp->nIdx;
			}
		}
		shmesh[i].vbuf->UnMap();
		shmesh[i].vbuf->Bind();
		shmesh[i].idx = new IndexBuffer(idx, eg[i].nidx);
		shmesh[i].idx->Bind();
		delete []idx;
		shmesh[i].va->UnBind();
	}

	for (i = 0; i < nshmesh; i++)
		delete []eg[i].mesh;
	delete []eg;
}

bool vBase::Update ()
{
	if (!vObject::Update()) return false;

	static const double csun_lights = RAD*1.0; // sun elevation at which lights are switched on
	double simt = oapiGetSimTime();

	if (simt > Tlghtchk) {
		double intv;
		//bLocalLight = ModLighting (&localLight, intv);
		Tlghtchk = simt+intv;
	}

	if (simt > Tchk) {
		VECTOR3 pos, sdir;
		MATRIX3 rot;
		oapiGetGlobalPos (hObj, &pos); normalise(pos);
		oapiGetRotationMatrix (hObj, &rot);
		sdir = tmul (rot, -pos);
		double csun = sdir.y;
		bool night = csun < csun_lights;
		if (lights != night) {
			int i;
			for (i = 0; i < nstructure_bs; i++)
				structure_bs[i]->SetTexMixture (1, night ? 1.0f:0.0f);
			for (i = 0; i < nstructure_as; i++)
				structure_as[i]->SetTexMixture (1, night ? 1.0f:0.0f);
			lights = night;
		}
	}
	return true;
}

bool vBase::RenderSurface ()
{
	// note: assumes z-buffer disabled

	if (!active) return false;

	int i;
	bool modlight = false;

	// render tiles
	if (ntile) {
		if (bLocalLight && !modlight) {
			//dev->SetLight (0, &localLight);
			modlight = true;
		}
		//dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
		for (i = 0; i < ntile; i++) {
			glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)tspec[i].tex)->m_TexId);
			tile[i].mesh->Render (scn->GetCamera(), mWorld);
		}
		//dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
	}

	// render generic objects under shadows
	if (nstructure_bs) {
		Renderer::PushBool(Renderer::DEPTH_TEST, true);
		Renderer::PushDepthMask(true);
		for (i = 0; i < nstructure_bs; i++) {
			structure_bs[i]->Render (scn->GetCamera(), mWorld);
		}
		Renderer::PopDepthMask();
		Renderer::PopBool();
	}

	// render surface shadows (TODO)
/*
	if (modlight) // restore lighting
		dev->SetLight (0, (LPD3DLIGHT7)scn->GetLight());
*/
	return true;
}

bool vBase::RenderStructures ()
{
	// note: assumes z-buffer enabled

	if (!active) return false;

	//if (bLocalLight) // modify lighting
	//	dev->SetLight (0, &localLight);

	// render generic objects above shadows
	for (int i = 0; i < nstructure_as; i++)
		structure_as[i]->Render (scn->GetCamera(), mWorld);
/*
	if (bLocalLight) { // restore lighting
		dev->SetLight (0, (LPD3DLIGHT7)scn->GetLight());
	}*/

	return true;
}

void vBase::RenderGroundShadow (float depth)
{
	if (!nshmesh) return; // nothing to do

	shadowShader->Bind();
	shadowShader->SetMat4("u_ViewProjection", *scn->GetCamera()->GetViewProjectionMatrix());

	static const double shadow_elev_limit = 0.07;
	double d, csun, nr0;
	VECTOR3 pp, sd, pvr;
	OBJHANDLE hPlanet = oapiGetBasePlanet (hObj); // planet handle
	oapiGetGlobalPos (hPlanet, &pp);              // planet global pos
	oapiGetGlobalPos (hObj, &sd);                 // base global pos
	pvr = sd-pp;                                  // planet-relative base position
	d = length (pvr);                             // planet radius at base location
	normalise (sd);                               // shadow projection direction

	double fac1 = dotp (sd, pvr);
	if (fac1 > 0.0)                               // base is on planet night-side
		return;
	csun = -fac1/d;                               // sun elevation above horizon
	if (csun < shadow_elev_limit)                 // sun too low to cast shadow
		return;

	MATRIX3 vR;
	oapiGetRotationMatrix (hObj, &vR);
	VECTOR3 sdv = tmul (vR, sd);     // projection direction in base frame
	VECTOR3 hnp = pvr; normalise(hnp);
	VECTOR3 hn = tmul (vR, hnp);     // horizon normal in vessel frame

	// perform projections
	double nd = dotp (hn, sdv);
	VECTOR3 sdvs = sdv / nd;
	if (!sdvs.y) return; // required for plane offset correction

	// build shadow projection matrix
	glm::mat4 mProj, mProjWorld, mProjWorldShift;
	mProj[0][0] = (float)(1.0 - sdvs.x*hn.x);
	mProj[0][1] = (float)(    - sdvs.y*hn.x);
	mProj[0][2] = (float)(    - sdvs.z*hn.x);
	mProj[0][3] = 0;
	mProj[1][0] = (float)(    - sdvs.x*hn.y);
	mProj[1][1] = (float)(1.0 - sdvs.y*hn.y);
	mProj[1][2] = (float)(    - sdvs.z*hn.y);
	mProj[1][3] = 0;
	mProj[2][0] = (float)(    - sdvs.x*hn.z);
	mProj[2][1] = (float)(    - sdvs.y*hn.z);
	mProj[2][2] = (float)(1.0 - sdvs.z*hn.z);
	mProj[2][3] = 0;
	mProj[3][0] = 0;
	mProj[3][1] = 0;
	mProj[3][2] = 0;
	mProj[3][3] = 1.0f;
	mProjWorld = mWorld * mProj;
	mProjWorldShift = mProjWorld;

	// modify depth of shadows at dawn/dusk
//	if (g_client->UseStencilBuffer()) {
		double scale = std::min (1.0, (csun-0.07)/0.015);
		if (scale < 1) {
			depth = scale * depth;
		}
//	}

	shadowShader->SetFloat("u_ShadowDepth", depth);

	for (int i = 0; i < nshmesh; i++) {

		// add shadow plane offset to transformation
		nr0 = shmesh[i].ecorr/sdvs.y;
		mProjWorldShift[3][0] = mProjWorld[3][0] + (float)(nr0*(sdvs.x*mWorld[0][0] + sdvs.y*mWorld[1][0] + sdvs.z*mWorld[2][0]));
		mProjWorldShift[3][1] = mProjWorld[3][1] + (float)(nr0*(sdvs.x*mWorld[0][1] + sdvs.y*mWorld[1][1] + sdvs.z*mWorld[2][1]));
		mProjWorldShift[3][2] = mProjWorld[3][2] + (float)(nr0*(sdvs.x*mWorld[0][2] + sdvs.y*mWorld[1][2] + sdvs.z*mWorld[2][2]));

		shadowShader->SetMat4("u_Model", mProjWorldShift);

		shmesh[i].va->Bind();

        glDrawElements(GL_TRIANGLES, shmesh[i].idx->GetCount(), GL_UNSIGNED_SHORT, 0);

		shmesh[i].va->UnBind();
	}

	shadowShader->UnBind();
}

/*
bool vBase::ModLighting (LPD3DLIGHT7 light, double &nextcheck)
{
	VECTOR3 GB, GS, GP, S, P;
	VECTOR3 lcol = {1,1,1};
	double s, p, rs, as, phi;
	int j;

	OBJHANDLE hP = oapiGetBasePlanet (hObj);
	OBJHANDLE hS = oapiGetGbodyByIndex(0); // the central star
	oapiGetGlobalPos (hObj, &GB);          // base position
	oapiGetGlobalPos (hS, &GS);            // sun position
	oapiGetGlobalPos (hP, &GP);            // planet position
	S = GS-GB;                             // sun's position from base
	s = length(S);                         // sun's distance
	rs = oapiGetSize (hS);
	as = asin (rs/s);                      // apparent radius of sun's disc [rad]
	double amb = 0;
	bool lightmod = false;

	// Calculate shadowing by planet
	P = GP-GB;
	p = length(P);
	phi = acos (dotp(S,P)/(s*p));    // angular distance between sun and planet
	static const double ap = PI05;   // apparent size of planet disc [rad]

	const ATMCONST *atm = (oapiGetObjectType(hP)==OBJTP_PLANET ? oapiGetPlanetAtmConstants (hP) : NULL);
	if (atm) { // case 1: planet has atmosphere

		double ap1 = RAD*100.0;
		// This is the angular separation between planet centre and star below which
		// the atmosphere affects lighting when on the planet surface. (100: when sun
		// is 10 deg above horizon). Should possibly be made atmosphere-specific.
		if (as+ap1 >= phi) {         // overlap
			double dap = ap1-ap;
			VECTOR3 plight = {1,1,1};
			if (phi < ap-as) {       // totality (sun below horizon)
				plight.x = plight.y = plight.z = 0.0;
			} else {
				double dispersion = std::max (0.02, std::min (0.9, log (atm->rho0+1.0)));
				double r0 = 1.0-0.40*dispersion;
				double g0 = 1.0-0.65*dispersion;
				double b0 = 1.0-1.0 *dispersion;
				if (phi > as+ap) {   // sun above horizon
					double f = (phi-as-ap)/dap;
					plight.x = f*(1.0-r0) + r0;
					plight.y = f*(1.0-g0) + g0;
					plight.z = f*(1.0-b0) + b0;
				} else {             // sun partially below horizon
					double f = (phi-ap+as)/(2.0*as);
					plight.x = f*r0;
					plight.y = f*g0;
					plight.z = f*b0;
				}
			}
			for	(j = 0; j < 3; j++) lcol.data[j] = std::min (lcol.data[j], plight.data[j]);
			lightmod = true;
		}

		// modification of ambient lighting
		double sunelev = phi-ap;
		if (sunelev > - 14.0*RAD) {
			double amb0 = std::min (0.7, log (atm->rho0+1.0)*0.4);
			amb = amb0 * std::min (1.0, (sunelev+14.0*RAD)/(20.0*RAD));
			if (!lightmod) lightmod = (amb > 0.05);
			amb = std::max (0.0, amb-0.05);
			// reduce direct light component to avoid overexposure
			lcol *= 1.0-amb*0.5;
		}

	} else {   // case 2: planet has no atmosphere

		if (phi < as+ap) {       // overlap
			double lfrac = (phi <= ap-as ? 0.0 : (phi+as-ap)/(2.0*as));
			for (j = 0; j < 3; j++) lcol.data[j] = std::min (lcol.data[j], lfrac);
			lightmod = true;
		}

	}

	if (lightmod) {
		D3DCOLORVALUE starcol = {1,1,1,1}; // for now
		light->dcvDiffuse.r = light->dcvSpecular.r = starcol.r * (float)lcol.x;
		light->dcvDiffuse.g = light->dcvSpecular.g = starcol.g * (float)lcol.y;
		light->dcvDiffuse.b = light->dcvSpecular.b = starcol.b * (float)lcol.z;
		light->dcvAmbient.r = (float)amb;
		light->dcvAmbient.g = (float)amb;
		light->dcvAmbient.b = (float)amb;
		S /= s;
		light->dvDirection.x = -(float)S.x;
		light->dvDirection.y = -(float)S.y;
		light->dvDirection.z = -(float)S.z;
	}

	nextcheck = std::max (1.0, 50.0*(sqrt(fabs(phi-PI05)))); // next test
	return lightmod;
}
*/
