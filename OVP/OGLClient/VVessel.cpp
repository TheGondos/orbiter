// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VVessel.cpp
// Vessel visualisation
// ==============================================================

#include "glad.h"
#include "VVessel.h"
#include "OGLMesh.h"
#include "OGLCamera.h"
#include "Texture.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

using namespace oapi;

// ==============================================================
// Local prototypes

void TransformPoint (VECTOR3 &p, const glm::mat4 &T);
void TransformDirection (VECTOR3 &a, const glm::mat4 &T, bool normalise);

// ==============================================================
// class vVessel (implementation)
//
// A vVessel is the visual representation of a vessel object.
// ==============================================================

vVessel::vVessel (OBJHANDLE _hObj, const Scene *scene): vObject (_hObj, scene)
{
	vessel = oapiGetVesselInterface (_hObj);
	nmesh = 0;
	nanim = 0;
	bLocalLight = false;
	localLight = *scene->GetLight();
	tCheckLight = oapiGetSimTime()-1.0;
	LoadMeshes ();
	InitAnimations ();
}

vVessel::~vVessel ()
{
	ClearAnimations();
	ClearMeshes();
}

void vVessel::GlobalInit ()
{
	if (defexhausttex) defexhausttex->Release();
	g_client->GetTexMgr()->LoadTexture ("Exhaust.dds", &defexhausttex, 0);

	exhaustShader   = Renderer::GetShader("Exhaust");
	meshUnlitShader = Renderer::GetShader("MeshUnlit");
	shadowShader    = Renderer::GetShader("VesselShadow");

}

void vVessel::GlobalExit ()
{
	if (defexhausttex) {
		defexhausttex->Release();
		defexhausttex = 0;
	}
}

void vVessel::clbkEvent (visevent msg, visevent_data content)
{
	switch (msg) {
	case EVENT_VESSEL_INSMESH:
		InsertMesh (content.meshidx);
		break;
	case EVENT_VESSEL_DELMESH:
		DelMesh (content.meshidx);
		break;
	case EVENT_VESSEL_MESHVISMODE:
		if (content.meshidx < nmesh) {
			meshlist[content.meshidx].vismode = vessel->GetMeshVisibilityMode(content.meshidx);
		}
		break;
	case EVENT_VESSEL_MESHOFS: {
		int idx = content.meshidx;
		if (idx < nmesh) {
			VECTOR3 ofs;
			vessel->GetMeshOffset (idx, ofs);
			if (length(ofs)) {
				if (!meshlist[idx].trans)
					meshlist[idx].trans = new glm::fmat4(1.0f);
				else
					*meshlist[idx].trans = glm::fmat4(1.0f);
                glm::fmat4 *m = meshlist[idx].trans;
                glm::fvec3 offset;
                offset.x = ofs.x;
                offset.y = ofs.y;
                offset.z = ofs.z;
                *m = glm::translate(*m, offset);
			} else {
				if (meshlist[idx].trans) {
					delete meshlist[idx].trans;
					meshlist[idx].trans = nullptr;
				}
			}
		} break;
	}
	case EVENT_VESSEL_MODMESHGROUP:
		//MessageBeep (-1);
		break;
	}
}

MESHHANDLE vVessel::GetMesh (UINT idx)
{
	return (idx < nmesh ? meshlist[idx].mesh : NULL);
}

bool vVessel::Update ()
{
	if (!active) return false;

	vObject::Update ();
	UpdateAnimations ();

	if (oapiGetSimTime() > tCheckLight)
		bLocalLight = ModLighting (&localLight);

	return true;
}

void vVessel::LoadMeshes ()
{
	if (nmesh) ClearMeshes();
	MESHHANDLE hMesh;
	const OGLMesh *mesh;
	VECTOR3 ofs;
	UINT idx;
	OGLMeshManager *mmgr = g_client->GetMeshManager();

	nmesh = vessel->GetMeshCount();
	meshlist = new MESHREC[nmesh];
	memset (meshlist, 0, nmesh*sizeof(MESHREC));

	for (idx = 0; idx < nmesh; idx++) {
		if ((hMesh = vessel->GetMeshTemplate (idx)) && (mesh = mmgr->GetMesh (hMesh))) {
			// copy from preloaded template
			meshlist[idx].mesh = new OGLMesh (*mesh);
		} else if (hMesh = vessel->CopyMeshFromTemplate (idx)) {
			// load on the fly and discard after copying
			meshlist[idx].mesh = new OGLMesh (hMesh);
			oapiDeleteMesh (hMesh);
		}
		if (meshlist[idx].mesh) {
			meshlist[idx].vismode = vessel->GetMeshVisibilityMode (idx);
			vessel->GetMeshOffset (idx, ofs);
			if (length(ofs)) {
				meshlist[idx].trans = new glm::fmat4(1.0f);

                glm::fmat4 *m = meshlist[idx].trans;
                glm::fvec3 offset;
                offset.x = ofs.x;
                offset.y = ofs.y;
                offset.z = ofs.z;
                *m = glm::translate(*m, offset);
				// currently only mesh translations are supported
			}
		}
	}
}

void vVessel::InsertMesh (UINT idx)
{
	UINT i;

	if (idx >= nmesh) { // append a new entry to the list
		MESHREC *tmp = new MESHREC[idx+1];
		if (nmesh) {
			memcpy (tmp, meshlist, nmesh*sizeof(MESHREC));
			delete []meshlist;
		}
		meshlist = tmp;
		for (i = nmesh; i < idx; i++) { // zero any intervening entries
			meshlist[i].mesh = 0;
			meshlist[i].trans = 0;
			meshlist[i].vismode = 0;
		}
		nmesh = idx+1;
	} else if (meshlist[idx].mesh) { // replace existing entry
		delete meshlist[idx].mesh;
		if (meshlist[idx].trans) {
			delete meshlist[idx].trans;
			meshlist[idx].trans = 0;
		}
	}

	// now add the new mesh
	MESHHANDLE hMesh;
	const OGLMesh *mesh;
	OGLMeshManager *mmgr = g_client->GetMeshManager();
	VECTOR3 ofs;
	if ((hMesh = vessel->GetMeshTemplate (idx)) && (mesh = mmgr->GetMesh (hMesh))) {
		meshlist[idx].mesh = new OGLMesh (*mesh);
	} else if (hMesh = vessel->CopyMeshFromTemplate (idx)) {
		meshlist[idx].mesh = new OGLMesh (hMesh);
		oapiDeleteMesh (hMesh);
	} else {
		meshlist[idx].mesh = 0;
	}
	if (meshlist[idx].mesh) {
		meshlist[idx].vismode = vessel->GetMeshVisibilityMode (idx);
		vessel->GetMeshOffset (idx, ofs);
		if (length(ofs)) {
            meshlist[idx].trans = new glm::fmat4(1.0f);
            glm::fmat4 *m = meshlist[idx].trans;
            glm::fvec3 offset;
            offset.x = ofs.x;
            offset.y = ofs.y;
            offset.z = ofs.z;
            *m = glm::translate(*m, offset);
			// currently only mesh translations are supported
		} else {
			meshlist[idx].trans = nullptr;
		}
	}
}

void vVessel::ClearMeshes ()
{
	if (nmesh) {
		for (UINT i = 0; i < nmesh; i++) {
			if (meshlist[i].mesh) delete meshlist[i].mesh;
			if (meshlist[i].trans) delete meshlist[i].trans;
		}
		delete []meshlist;
		nmesh = 0;
	}
}

void vVessel::DelMesh (UINT idx)
{
	if (idx >= nmesh) return;
	if (!meshlist[idx].mesh) return;
	delete meshlist[idx].mesh;
	meshlist[idx].mesh = 0;
	if (meshlist[idx].trans) {
		delete meshlist[idx].trans;
		meshlist[idx].trans = 0;
	}
}

void vVessel::InitAnimations ()
{
	if (nanim) ClearAnimations();
	nanim = vessel->GetAnimPtr (&anim);
	if (nanim) {
		UINT i;
		animstate = new double[nanim];
		for (i = 0; i < nanim; i++)
			animstate[i] = anim[i].defstate; // reset to default mesh states
	}
}

void vVessel::ClearAnimations ()
{
	if (nanim) {
		delete []animstate;
		nanim = 0;
	}
}

void vVessel::UpdateAnimations (UINT mshidx)
{
	double newstate;
	for (UINT i = 0; i < nanim; i++) {
		if (!anim[i].ncomp) continue;
		if (animstate[i] != (newstate = anim[i].state)) {
			Animate (i, newstate, mshidx);
			animstate[i] = newstate;
		}
	}
}

bool vVessel::Render ()
{
	if (!active) return false;
	Render (false);
	return true;
}

bool vVessel::Render (bool internalpass)
{
	if (!active) return false;
	UINT i, mfd;

	bool bCockpit = (oapiCameraInternal() && hObj == oapiGetFocusObject());
	// render cockpit view

	bool bVC = (bCockpit && oapiCockpitMode() == COCKPIT_VIRTUAL);
	// render virtual cockpit

	const VCHUDSPEC *hudspec;
	const VCMFDSPEC *mfdspec[MAXMFD];
	SURFHANDLE sHUD;//, sMFD[MAXMFD];

	OGLLight globalLight;

	if (bLocalLight) {
		Renderer::GetLight (0, &globalLight);
		Renderer::SetLight (&localLight);
	}

	if (bVC) {
		sHUD = g_client->GetVCHUDSurface (&hudspec);
		for (mfd = 0; mfd < MAXMFD; mfd++)
			g_client->GetVCMFDSurface (mfd, &mfdspec[mfd]);
	}

	for (i = 0; i < nmesh; i++) {

		if (!meshlist[i].mesh) continue;

		// check if mesh should be rendered in this pass
		uint16_t vismode = meshlist[i].vismode;
		if (bCockpit) {
			if (internalpass && (vismode & MESHVIS_EXTPASS)) continue;
			if (!(vismode & MESHVIS_COCKPIT)) {
				if ((!bVC) || (!(vismode & MESHVIS_VC))) continue;
			}
		} else {
			if (!(vismode & MESHVIS_EXTERNAL)) continue;
		}

		// transform mesh
		glm::fmat4 mWorldTrans;
		if (meshlist[i].trans) {
			mWorldTrans = mWorld * *(meshlist[i].trans);
		} else {
			mWorldTrans = mWorld;
		}

		if (bVC) { // link MFD textures for rendering
			for (mfd = 0; mfd < MAXMFD; mfd++) {
				if (mfdspec[mfd] && mfdspec[mfd]->nmesh == i) {
					meshlist[i].mesh->GetGroup(mfdspec[mfd]->ngroup)->TexIdx = TEXIDX_MFD0+mfd;
				}
			}
		}

		// render mesh
		meshlist[i].mesh->Render (scn->GetCamera(), mWorldTrans);

		// render VC HUD and MFDs
		if (bVC) {

			// render VC HUD
			if (sHUD && hudspec->nmesh == i) {
				Renderer::Bind(meshUnlitShader);
				auto vp = scn->GetCamera()->GetViewProjectionMatrix();
				meshUnlitShader->SetMat4("u_ViewProjection", *vp);
				meshUnlitShader->SetMat4("u_Model", mWorldTrans);

				glBindTexture(GL_TEXTURE_2D,  ((OGLTexture *)sHUD)->m_TexId);
				Renderer::PushBool(Renderer::DEPTH_TEST, false);
				//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				Renderer::PushBlendFunc(GL_SRC_ALPHA, GL_ONE);
				meshlist[i].mesh->RenderGroup (meshlist[i].mesh->GetGroup(hudspec->ngroup));
//				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				Renderer::PopBlendFunc();
				Renderer::PopBool();
			}
		}
	}

	if (bLocalLight)
		Renderer::SetLight (&globalLight);

	return true;
}

bool vVessel::RenderExhaust ()
{
	if (!active) return false;
	uint32_t i, nexhaust = vessel->GetExhaustCount();
	if (!nexhaust) return true; // nothing to do

	bool need_setup = true;
	double lvl, xsize, zsize;
	VECTOR3 cdir;
	OGLTexture *tex, *ptex = 0;
	EXHAUSTSPEC es;

	static VERTEX_XYZ_TEX ExhaustVtx[8] = {
		{0,0,0, 0.24f,0},
		{0,0,0, 0.24f,1},
		{0,0,0, 0.01f,0},
		{0,0,0, 0.01f,1},
		{0,0,0, 0.50390625f, 0.00390625f},
		{0,0,0, 0.99609375f, 0.00390625f},
		{0,0,0, 0.50390625f, 0.49609375f},
		{0,0,0, 0.99609375f, 0.49609375f}
	};
	static uint16_t ExhaustIdx[12] = {0,1,2, 3,2,1, 4,5,6, 7,6,5};

	static IndexBuffer *IBO;
	static VertexArray *VBA;
	static VertexBuffer *VBO;

	static bool need_setup2 = true;
	if(need_setup2) {
		need_setup2 = false;
		VBA = new VertexArray();
		VBA->Bind();

		VBO = new VertexBuffer(ExhaustVtx, 8*sizeof(VERTEX_XYZ_TEX));
		VBO->Bind();

		IBO = new IndexBuffer(ExhaustIdx, 12);
		IBO->Bind();

		//position
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(VERTEX_XYZ_TEX),                  // stride
		(void*)0            // array buffer offset
		);
		Renderer::CheckError("glVertexAttribPointer0");
		glEnableVertexAttribArray(0);

		//texcoord
		glVertexAttribPointer(
		1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(VERTEX_XYZ_TEX),                  // stride
		(void*)12            // array buffer offset
		);
		Renderer::CheckError("glVertexAttribPointer");
		glEnableVertexAttribArray(1);
		Renderer::CheckError("glEnableVertexAttribArray1");

		VBA->UnBind();
	}

	Renderer::Bind(exhaustShader);
	auto vp = scn->GetCamera()->GetViewProjectionMatrix();
	exhaustShader->SetMat4("u_ViewProjection", *vp);
	exhaustShader->SetMat4("u_Model", mWorld);

	Renderer::PushDepthMask(false);
	for (i = 0; i < nexhaust; i++) {
		if (!(lvl = vessel->GetExhaustLevel (i))) continue;
		vessel->GetExhaustSpec (i, &es);

		if (need_setup) { // initialise render state
			MATRIX3 R;
			vessel->GetRotationMatrix (R);
			cdir = tmul (R, cpos);

			/*
			dev->SetMaterial (&engmat);
			dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &mWorld);
			dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
			*/
			need_setup = false;
		}
		tex = (OGLTexture *)es.tex;
		if (!tex) tex = defexhausttex;

		//if (tex != ptex) dev->SetTexture (0, ptex = tex);

		//xsize = sqrt (zsize = lvl);
		xsize = zsize = 1.0;

		//const double irmax = 0.1/(double)RAND_MAX;
		double alpha = *es.level;
		if (es.modulate) alpha *= ((1.0 - es.modulate)+(double)rand()* es.modulate/(double)RAND_MAX);

		SetExhaustVertices (-(*es.ldir), cdir, *es.lpos, zsize*es.lsize, xsize*es.wsize, ExhaustVtx);

		VBO->Bind();
		VBO->Update(ExhaustVtx, 8 * sizeof(VERTEX_XYZ_TEX));
		VBO->UnBind();
		VBA->Bind();
		glBindTexture(GL_TEXTURE_2D, tex->m_TexId);
		glDrawElements(GL_TRIANGLES, IBO->GetCount(), GL_UNSIGNED_SHORT, 0);

		VBA->UnBind();

		/*
		dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(1,1,1,alpha));

		SetExhaustVertices (-(*es.ldir), cdir, *es.lpos, zsize*es.lsize, xsize*es.wsize, ExhaustVtx);
		dev->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, FVF_XYZ_TEX,
			ExhaustVtx, 8, ExhaustIdx, 12, 0);
		*/
	}

	Renderer::Unbind(exhaustShader);

	Renderer::PopDepthMask();

	return true;
}

void vVessel::RenderBeacons ()
{
	int idx = 0;
	const BEACONLIGHTSPEC *bls = vessel->GetBeacon(idx);
	if (!bls) return; // nothing to do
//	bool need_setup = true;
	double simt = oapiGetSimTime();
	Renderer::PushBool(Renderer::BLEND, true);
	Renderer::PushDepthMask(false);
	for (; bls; bls = vessel->GetBeacon (++idx)) {
		if (bls->active) {
			if (bls->period && (fmod(simt+bls->tofs, bls->period) > bls->duration))
				continue;
			double size = bls->size;
			if (cdist > 50.0)
				size *= pow (cdist/50.0, bls->falloff);
//			if (need_setup) {
//				dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//				dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, FALSE);
//				dev->SetRenderState (D3DRENDERSTATE_ZBIAS, 5);
//				need_setup = false;
//			}
			RenderSpot (bls->pos, (float)size, *bls->col, false, bls->shape);
		}
	}
	// undo device modifications
//	if (!need_setup) {
//		if (!doalpha) dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
//		dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
//		dev->SetRenderState (D3DRENDERSTATE_ZBIAS, 0);
//	}
	Renderer::PopBool(1);
	Renderer::PopDepthMask();
}

void vVessel::RenderGroundShadow (OBJHANDLE hPlanet, float depth)
{
	static const double eps = 1e-2;
	static const double shadow_elev_limit = 0.07;
	double d, alt, R;
	VECTOR3 pp, sd, pvr;
	oapiGetGlobalPos (hPlanet, &pp); // planet global pos
	vessel->GetGlobalPos (sd);       // vessel global pos
	pvr = sd-pp;                     // planet-relative vessel position
	d = length(pvr);                 // vessel-planet distance
	R = oapiGetSize (hPlanet);       // planet mean radius
	R += vessel->GetSurfaceElevation();  // Note: this only works at low vessel altitudes (shadow close to vessel position)
	alt = d-R;                       // altitude above surface
	if (alt*eps > vessel->GetSize()) // too high to cast a shadow
		return;
	normalise (sd);                  // shadow projection direction

	// calculate the intersection of the vessel's shadow with the planet surface
	double fac1 = dotp (sd, pvr);
	if (fac1 > 0.0)                  // shadow doesn't intersect planet surface
		return;
	double csun = -fac1/d;           // sun elevation above horizon
	if (csun < shadow_elev_limit)    // sun too low to cast shadow
		return;
	double arg  = fac1*fac1 - (dotp (pvr, pvr) - R*R);
	if (arg <= 0.0)                  // shadow doesn't intersect with planet surface
		return;
	double a = -fac1 - sqrt(arg);

	MATRIX3 vR;
	vessel->GetRotationMatrix (vR);
	VECTOR3 sdv = tmul (vR, sd);     // projection direction in vessel frame
	VECTOR3 shp = sdv*a;             // projection point
	VECTOR3 hn, hnp = vessel->GetSurfaceNormal();
	vessel->HorizonInvRot (hnp, hn);

	// perform projections
	double nr0 = dotp (hn, shp);
	double nd  = dotp (hn, sdv);
	VECTOR3 sdvs = sdv / nd;

	// build shadow projection matrix
	glm::mat4 mProj, mProjWorld, mProjWorldShift;
	mProj[0][0] = 1.0f - (float)(sdvs.x*hn.x);
	mProj[0][1] =      - (float)(sdvs.y*hn.x);
	mProj[0][2] =      - (float)(sdvs.z*hn.x);
	mProj[0][3] = 0;
	mProj[1][0] =      - (float)(sdvs.x*hn.y);
	mProj[1][1] = 1.0f - (float)(sdvs.y*hn.y);
	mProj[1][2] =      - (float)(sdvs.z*hn.y);
	mProj[1][3] = 0;
	mProj[2][0] =      - (float)(sdvs.x*hn.z);
	mProj[2][1] =      - (float)(sdvs.y*hn.z);
	mProj[2][2] = 1.0f - (float)(sdvs.z*hn.z);
	mProj[2][3] = 0;
	mProj[3][0] =        (float)(sdvs.x*nr0);
	mProj[3][1] =        (float)(sdvs.y*nr0);
	mProj[3][2] =        (float)(sdvs.z*nr0);
	mProj[3][3] = 1;
	mProjWorld = mWorld * mProj;
	
	bool isProjWorld = false;

	// modify depth of shadows at dawn/dusk
	bool resetalpha = false;
	//if (g_client->UseStencilBuffer()) {
		double scale = std::min (1.0, (csun-0.07)/0.015);
		if (scale < 1) {
			depth = scale * depth;
		}
	//}

	Renderer::Bind(shadowShader);
	shadowShader->SetMat4("u_ViewProjection", *scn->GetCamera()->GetViewProjectionMatrix());
	shadowShader->SetFloat("u_ShadowDepth", depth);

	// project all vessel meshes. This should be replaced by a dedicated shadow mesh
	for (unsigned int i = 0; i < nmesh; i++) {
		if (!meshlist[i].mesh) continue;
		if (!(meshlist[i].vismode & MESHVIS_EXTERNAL)) continue; // only render shadows for externally visible meshes
		OGLMesh *mesh = meshlist[i].mesh;
		if (meshlist[i].trans) {
			// add mesh offset to transformation
			mProjWorldShift = mProjWorld * *(meshlist[i].trans);
			shadowShader->SetMat4("u_Model", mProjWorldShift);
		} else {
			shadowShader->SetMat4("u_Model", mProjWorld);
		}

		for (int j = 0; j < mesh->GroupCount(); j++) {
			OGLMesh::GROUPREC *grp = mesh->GetGroup(j);
			if (grp->UsrFlag & 1) continue; // "no shadow" flag
			mesh->RenderGroup (grp);	
		}
	}

	Renderer::Unbind(shadowShader);
}

void vVessel::SetExhaustVertices (const VECTOR3 &edir, const VECTOR3 &cdir, const VECTOR3 &ref,
	double lscale, double wscale, VERTEX_XYZ_TEX *ev)
{
	// need to rotate the billboard so it faces the observer
	const float flarescale = 7.0;
	VECTOR3 sdir = crossp (cdir, edir); normalise (sdir);
	VECTOR3 tdir = crossp (cdir, sdir); normalise (tdir);
	float rx = (float)ref.x, ry = (float)ref.y, rz = (float)ref.z;
	float sx = (float)(sdir.x*wscale);
	float sy = (float)(sdir.y*wscale);
	float sz = (float)(sdir.z*wscale);
	float ex = (float)(edir.x*lscale);
	float ey = (float)(edir.y*lscale);
	float ez = (float)(edir.z*lscale);
	ev[1].x = (ev[0].x = rx + sx) + ex;
	ev[1].y = (ev[0].y = ry + sy) + ey;
	ev[1].z = (ev[0].z = rz + sz) + ez;
	ev[3].x = (ev[2].x = rx - sx) + ex;
	ev[3].y = (ev[2].y = ry - sy) + ey;
	ev[3].z = (ev[2].z = rz - sz) + ez;
	wscale *= flarescale, sx *= flarescale, sy *= flarescale, sz *= flarescale;
	float tx = (float)(tdir.x*wscale);
	float ty = (float)(tdir.y*wscale);
	float tz = (float)(tdir.z*wscale);
	ev[4].x = rx - sx + tx;   ev[5].x = rx + sx + tx;
	ev[4].y = ry - sy + ty;   ev[5].y = ry + sy + ty;
	ev[4].z = rz - sz + tz;   ev[5].z = rz + sz + tz;
	ev[6].x = rx - sx - tx;   ev[7].x = rx + sx - tx;
	ev[6].y = ry - sy - ty;   ev[7].y = ry + sy - ty;
	ev[6].z = rz - sz - tz;   ev[7].z = rz + sz - tz;
}

bool vVessel::ModLighting (OGLLight *light)
{
	VECTOR3 GV, GS, GP, S, P;

	// we only test the closest celestial body for shadowing
	OBJHANDLE hP = vessel->GetSurfaceRef();
	OBJHANDLE hS = oapiGetGbodyByIndex(0); // the central star
	CELBODY *cb = oapiGetCelbodyInterface(hP);
	CELBODY2 *cb2 = (cb->Version() >= 2 ? (CELBODY2*)cb : NULL);
	vessel->GetGlobalPos(GV);
	oapiGetGlobalPos (hS, &GS);
	S = GS-GV; // sun's position from vessel
	double s = length(S);
	double as = asin(oapiGetSize(hS)/s);
	VECTOR3 lcol = {1,1,1};
	double amb = 0;
	double dt = 1.0;
	bool lightmod = false;
	int i, j;

	// calculate shadowing by planet

	for (i = 0;; i++) {
		oapiGetGlobalPos (hP, &GP);
		P = GP-GV;
		double p = length(P);
		if (p < s) {                                      // shadow only if planet closer than sun
			double psize = oapiGetSize(hP);
			double phi = acos (dotp(S,P)/(s*p));          // angular distance between sun and planet
			double ap = (psize < p ? asin(psize / p) : PI05);  // apparent size of planet disc [rad]

			const ATMCONST *atm = (oapiGetObjectType(hP)==OBJTP_PLANET ? oapiGetPlanetAtmConstants (hP) : NULL);
			if (atm) {  // case 1: planet has atmosphere
				double alt = p-psize;                // vessel altitude
				double altlimit = *(double*)oapiGetObjectParam (hP, OBJPRM_PLANET_ATTENUATIONALT);
				double ap1 = ap * (altlimit+psize)/psize; 
				if (alt < altlimit) {
					static const double delta0 = RAD*100.0;
					// This is the angular separation between planet centre and star below which
					// the atmosphere affects lighting when on the planet surface. (100: when sun
					// is 10 deg above horizon). Should possibly be made atmosphere-specific.
					ap1 = delta0 / (1.0 + alt*(delta0-ap1)/(altlimit*ap1));
				}

				if (as+ap1 >= phi && ap/as > 0.1) {       // overlap and significant planet size
					double dap = ap1-ap;
					VECTOR3 plight = {1,1,1};
					if (as < ap) {                        // planet disc larger than sun disc
						if (phi < ap-as) {                // totality (sun below horizon)
							plight.x = plight.y = plight.z = 0.0;
						} else {
							double dispersion = std::max (0.02, std::min (0.9, log (atm->rho0+1.0)));
							double r0 = 1.0-0.35*dispersion;
							double g0 = 1.0-0.75*dispersion;
							double b0 = 1.0-1.0 *dispersion;
							if (phi > as+ap) {            // sun above horizon
								double f = (phi-as-ap)/dap;
								plight.x = f*(1.0-r0) + r0;
								plight.y = f*(1.0-g0) + g0;
								plight.z = f*(1.0-b0) + b0;
							} else {                      // sun partially below horizon
								double f = (phi-ap+as)/(2.0*as);
								plight.x = f*r0;
								plight.y = f*g0;
								plight.z = f*b0;
							}
							dt = 0.1;
						}
					} else {  // planet disc smaller than sun disc
						double maxcover = ap*ap / (as*as);
						if (phi < as-ap)
							plight.x = plight.y = plight.z = 1.0-maxcover; // annularity
						else {
							double frac = 1.0 - 0.5*maxcover * (1.0 + (as-phi)/ap); // partial cover
							plight.x = plight.y = plight.z = frac;
							dt = 0.1;
						}
					}
					for	(j = 0; j < 3; j++) lcol.data[j] = std::min (lcol.data[j], plight.data[j]);
					lightmod = true;
				}

				// modification of ambient lighting
				if (!i && vessel->GetAtmRef()) {
					double sunelev = phi-ap;
					if (sunelev > - 14.0*RAD) {
						double amb0 = std::min (0.7, log (atm->rho0+1.0)*0.4);
						double alt = p-psize;
						amb = amb0 / (alt*0.5e-4 + 1.0);
						amb *= std::min (1.0, (sunelev+14.0*RAD)/(20.0*RAD));
						if (!lightmod) lightmod = (amb > 0.05);
						amb = std::max (0.0, amb-0.05);
						// reduce direct light component to avoid overexposure
						lcol *= 1.0-amb*0.5;
					}
				}

			} else {  // case 2: planet has no atmosphere

				if (phi < as+ap && ap/as > 0.1) {         // overlap and significant planet size
					double lfrac;
					if (as < ap) {                        // planet disc larger than sun disc
						if (phi <= ap-as)                 // totality
							lfrac = 0.0;
						else {                            // partial cover
							lfrac = (phi+as-ap)/(2.0*as);
							dt = 0.1;
						}
					} else {                              // sun disc larger than planet disc
						double maxcover = ap*ap / (as*as);
						if (phi < as-ap) {                // annularity
							lfrac = 1.0-maxcover;
						} else {                          // partial cover
							lfrac = 1.0 - 0.5*maxcover * (1.0 + (as-phi)/ap);
							dt = 0.1;
						}
					}
					for (j = 0; j < 3; j++) lcol.data[j] = std::min (lcol.data[j], lfrac);
					lightmod = true;
				}
			}
		}
		if (!cb2 || (oapiGetObjectType (hP = cb2->GetParent()) != OBJTP_PLANET))
			break;
			// if this is a moon, also check the parent planet
			// warning: currently this only works for moons defined via
			// CELBODY2 interface
		cb = oapiGetCelbodyInterface(hP);
		cb2 = (cb->Version() >= 2 ? (CELBODY2*)cb : NULL);
	}

	if (lightmod) {
		//D3DCOLORVALUE starcol = sun->GetLightColor();
		glm::vec4 starcol = {1,1,1,1}; // for now
		light->light.dcvDiffuse.r = light->light.dcvSpecular.r = starcol.r * (float)lcol.x;
		light->light.dcvDiffuse.g = light->light.dcvSpecular.g = starcol.g * (float)lcol.y;
		light->light.dcvDiffuse.b = light->light.dcvSpecular.b = starcol.b * (float)lcol.z;
		light->light.dcvAmbient.r = (float)amb;
		light->light.dcvAmbient.g = (float)amb;
		light->light.dcvAmbient.b = (float)amb;
		S /= s;
		light->light.dvDirection.x = -(float)S.x;
		light->light.dvDirection.y = -(float)S.y;
		light->light.dvDirection.z = -(float)S.z;
	}

	tCheckLight = oapiGetSimTime() + dt;
	// we might be able to increase the interval when far from the
	// boundary, but then need to force a check in the case of sudden
	// movement (e.g. editor)

	return lightmod;
}

void vVessel::Animate (UINT an, double state, UINT mshidx)
{
	double s0, s1, ds;
	unsigned int i, ii;
	glm::mat4 T;
	ANIMATION *A = anim+an;
	for (ii = 0; ii < A->ncomp; ii++) {
		i = (state > animstate[an] ? ii : A->ncomp-ii-1);
		ANIMATIONCOMP *AC = A->comp[i];
		if (mshidx != (unsigned int)-1 && mshidx != AC->trans->mesh) continue;
		s0 = animstate[an]; // current animation state in the visual
		if      (s0 < AC->state0) s0 = AC->state0;
		else if (s0 > AC->state1) s0 = AC->state1;
		s1 = state;           // required animation state
		if      (s1 < AC->state0) s1 = AC->state0;
		else if (s1 > AC->state1) s1 = AC->state1;
		if ((ds = (s1-s0)) == 0) continue; // nothing to do for this component
		ds /= (AC->state1 - AC->state0);   // stretch to range 0..1

		// Build transformation matrix
		switch (AC->trans->Type()) {
		case MGROUP_TRANSFORM::NULLTRANSFORM:
			T = glm::mat4(1.0);
			AnimateComponent (AC, T);
			break;
		case MGROUP_TRANSFORM::ROTATE: {
			MGROUP_ROTATE *rot = (MGROUP_ROTATE*)AC->trans;
			glm::vec3 ax = {(float)(rot->axis.x), (float)(rot->axis.y), (float)(rot->axis.z)};

//			D3DMAT_RotationFromAxis (ax, (float)ds*rot->angle, &T);

			// Calculate quaternion
			float angle = 0.5f * (float)ds*rot->angle;
			float w = cosf(angle), sina = sinf(angle);
			float x = sina * ax.x;
			float y = sina * ax.y;
			float z = sina * ax.z;

			// Rotation matrix
			float xx = x*x, yy = y*y, zz = z*z;
			float xy = x*y, xz = x*z, yz = y*z;
			float wx = w*x, wy = w*y, wz = w*z;

			T[0][0] = 1.0f - 2.0f * (yy+zz);
			T[0][1] =        2.0f * (xy+wz);
			T[0][2] =        2.0f * (xz-wy);
			T[1][0] =        2.0f * (xy-wz);
			T[1][1] = 1.0f - 2.0f * (xx+zz);
			T[1][2] =        2.0f * (yz+wx);
			T[2][0] =        2.0f * (xz+wy);
			T[2][1] =        2.0f * (yz-wx);
			T[2][2] = 1.0f - 2.0f * (xx+yy);

			T[0][3] = T[1][3] = T[2][3] = T[3][0] = T[3][1] = T[3][2] = 0.0f;
			T[3][3] = 1.0f;

			float dx = (float)(rot->ref.x), dy = (float)(rot->ref.y), dz = (float)(rot->ref.z);
			T[3][0] = dx - T[0][0]*dx - T[1][0]*dy - T[2][0]*dz;
			T[3][1] = dy - T[0][1]*dx - T[1][1]*dy - T[2][1]*dz;
			T[3][2] = dz - T[0][2]*dx - T[1][2]*dy - T[2][2]*dz;
			AnimateComponent (AC, T);
			} break;
		case MGROUP_TRANSFORM::TRANSLATE: {
			MGROUP_TRANSLATE *lin = (MGROUP_TRANSLATE*)AC->trans;
			T = glm::mat4(1.0);
			T[3][0] = (float)(ds*lin->shift.x);
			T[3][1] = (float)(ds*lin->shift.y);
			T[3][2] = (float)(ds*lin->shift.z);
			AnimateComponent (AC, T);
			} break;
		case MGROUP_TRANSFORM::SCALE: {
			MGROUP_SCALE *scl = (MGROUP_SCALE*)AC->trans;
			s0 = (s0-AC->state0)/(AC->state1-AC->state0);
			s1 = (s1-AC->state0)/(AC->state1-AC->state0);
			T = glm::mat4(1.0);
			T[0][0] = (float)((s1*(scl->scale.x-1)+1)/(s0*(scl->scale.x-1)+1));
			T[1][1] = (float)((s1*(scl->scale.y-1)+1)/(s0*(scl->scale.y-1)+1));
			T[2][2] = (float)((s1*(scl->scale.z-1)+1)/(s0*(scl->scale.z-1)+1));
			T[3][0] = (float)scl->ref.x * (1.0f-T[0][0]);
			T[3][1] = (float)scl->ref.y * (1.0f-T[1][1]);
			T[3][2] = (float)scl->ref.z * (1.0f-T[2][2]);
			AnimateComponent (AC, T);
			} break;
		}
	}
}

void vVessel::AnimateComponent (ANIMATIONCOMP *comp, const glm::mat4 &T)
{
	unsigned int i;
	MGROUP_TRANSFORM *trans = comp->trans;

	if (trans->mesh == LOCALVERTEXLIST) { // transform a list of individual vertices

		VECTOR3 *vtx = (VECTOR3*)trans->grp;
		for (i = 0; i < trans->ngrp; i++)
			TransformPoint (vtx[i], T);

	} else {                              // transform mesh groups

		if (trans->mesh >= nmesh) return; // mesh index out of range
		OGLMesh *mesh = meshlist[trans->mesh].mesh;
		if (!mesh) return;

		if (trans->grp) { // animate individual mesh groups
			for (i = 0; i < trans->ngrp; i++)
				mesh->TransformGroup (trans->grp[i], &T);
		} else {          // animate complete mesh
//			mesh->Transform (T);
		}
	}

	// recursively transform all child animations
	for (i = 0; i < comp->nchildren; i++) {
		ANIMATIONCOMP *child = comp->children[i];
		AnimateComponent (child, T);
		switch (child->trans->Type()) {
		case MGROUP_TRANSFORM::NULLTRANSFORM:
			break;
		case MGROUP_TRANSFORM::ROTATE: {
			MGROUP_ROTATE *rot = (MGROUP_ROTATE*)child->trans;
			TransformPoint (rot->ref, T);
			TransformDirection (rot->axis, T, true);
			} break;
		case MGROUP_TRANSFORM::TRANSLATE: {
			MGROUP_TRANSLATE *lin = (MGROUP_TRANSLATE*)child->trans;
			TransformDirection (lin->shift, T, false);
			} break;
		case MGROUP_TRANSFORM::SCALE: {
			MGROUP_SCALE *scl = (MGROUP_SCALE*)child->trans;
			TransformPoint (scl->ref, T);
			// we can't transform anisotropic scaling vector
			} break;
		}
	}
}

OGLTexture *vVessel::defexhausttex = nullptr;

void TransformPoint (VECTOR3 &p, const glm::mat4 &T)
{
	double x = p.x*T[0][0] + p.y*T[1][0] + p.z*T[2][0] + T[3][0];
	double y = p.x*T[0][1] + p.y*T[1][1] + p.z*T[2][1] + T[3][1];
	double z = p.x*T[0][2] + p.y*T[1][2] + p.z*T[2][2] + T[3][2];
	double w = p.x*T[0][3] + p.y*T[1][3] + p.z*T[2][3] + T[3][3];
    p.x = x/w;
	p.y = y/w;
	p.z = z/w;
}

void TransformDirection (VECTOR3 &a, const glm::mat4 &T, bool normalise)
{
	double x = a.x*T[0][0] + a.y*T[1][0] + a.z*T[2][0];
	double y = a.x*T[0][1] + a.y*T[1][1] + a.z*T[2][1];
	double z = a.x*T[0][2] + a.y*T[1][2] + a.z*T[2][2];
	a.x = x, a.y = y, a.z = z;
	if (normalise) {
		double len = sqrt (x*x + y*y + z*z);
		a.x /= len;
		a.y /= len;
		a.z /= len;
	}
}
