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
#include "OGLClient.h"
#include "OGLCamera.h"
#include "OGLMesh.h"
#include "Shader.h"
#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
		abort();
		exit(EXIT_FAILURE);
	}
}

// ==============================================================
// Local prototypes

void TransformPoint (VECTOR3 &p, const glm::mat4 &T);
void TransformDirection (VECTOR3 &a, const glm::mat4 &T, bool normalise);

// ==============================================================
// class VVessel (implementation)
//
// A vVessel is the visual representation of a vessel object.
// ==============================================================

VVessel::VVessel (OBJHANDLE handle): VObject (handle)
{
	vessel = oapiGetVesselInterface (handle);
	nmesh = 0;
	nanim = 0;
	LoadMeshes ();
	InitAnimations();
}

VVessel::~VVessel ()
{
	ClearAnimations();
	ClearMeshes();
}

void VVessel::GlobalInit()
{
	/*
	const DWORD texsize = *(int*)gc->GetConfigParam (CFGPRM_PANELMFDHUDSIZE);

	if (mfdsurf) mfdsurf->Release();
	mfdsurf = (LPDIRECTDRAWSURFACE7)gc->clbkCreateTexture (texsize, texsize);
*/
	if (defexhausttex) defexhausttex->Release();
	defexhausttex = g_client->GetTexMgr()->LoadTexture ("Exhaust.dds", true, 0);
}

void VVessel::GlobalExit ()
{
/*
	if (mfdsurf) {
		mfdsurf->Release();
		mfdsurf = 0;
	}*/
	if (defexhausttex) {
		defexhausttex->Release();
		defexhausttex = 0;
	}
}

void VVessel::clbkEvent (visevent msg, visevent_data content)
{
	switch (msg) {
	case EVENT_VESSEL_INSMESH:
		InsertMesh (content.meshidx);
		break;
	case EVENT_VESSEL_DELMESH:
		DelMesh (content.meshidx);
		break;
	case EVENT_VESSEL_MESHVISMODE:
		// todo
		break;
	case EVENT_VESSEL_MESHOFS: {
		int idx = content.meshidx;
		if (idx < nmesh) {
			VECTOR3 ofs;
			vessel->GetMeshOffset (idx, ofs);
			if (length(ofs)) {
				if (!meshlist[idx].trans)
					meshlist[idx].trans = new glm::fmat4(1.0f);

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

MESHHANDLE VVessel::GetMesh (unsigned int idx)
{
	return (idx < nmesh ? meshlist[idx].mesh : NULL);
}

bool VVessel::Update ()
{
	VObject::Update ();
	if (!mVisible) return false;
	UpdateAnimations ();

	return true;
}

void VVessel::LoadMeshes ()
{
	if (nmesh) ClearMeshes();
	MESHHANDLE hMesh;
	const OGLMesh *mesh;
	VECTOR3 ofs;
	int idx;

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

void VVessel::InsertMesh (unsigned int idx)
{
	int i;

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
			meshlist[idx].trans = nullptr;
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

void VVessel::ClearMeshes ()
{
	if (nmesh) {
		for (int i = 0; i < nmesh; i++) {
			if (meshlist[i].mesh) delete meshlist[i].mesh;
		}
		delete []meshlist;
		nmesh = 0;
	}
}

void VVessel::DelMesh (unsigned int idx)
{
	if (idx >= nmesh) return;
	if (!meshlist[idx].mesh) return;
	delete meshlist[idx].mesh;
	meshlist[idx].mesh = 0;
	if (meshlist[idx].trans) {
		delete meshlist[idx].trans;
		meshlist[idx].trans = nullptr;
	}
}

void VVessel::InitAnimations ()
{
	if (nanim) ClearAnimations();
	nanim = vessel->GetAnimPtr (&anim);
	if (nanim) {
		unsigned int i;
		animstate = new double[nanim];
		for (i = 0; i < nanim; i++)
			animstate[i] = anim[i].defstate; // reset to default mesh states
	}
}

void VVessel::ClearAnimations ()
{
	if (nanim) {
		delete []animstate;
		nanim = 0;
	}
}

void VVessel::UpdateAnimations (unsigned int mshidx)
{
	double newstate;
	for (unsigned int i = 0; i < nanim; i++) {
		if (!anim[i].ncomp) continue;
		if (animstate[i] != (newstate = anim[i].state)) {
			Animate (i, newstate, mshidx);
			animstate[i] = newstate;
		}
	}
}

bool VVessel::Render (OGLCamera *c, bool internalpass)
{
	if (!mVisible) return false;
	unsigned int mfd;
	//bool bWorldValid = false;

	bool bCockpit = (oapiCameraInternal() && mHandle == oapiGetFocusObject());
	// render cockpit view

	bool bVC = (bCockpit && oapiCockpitMode() == COCKPIT_VIRTUAL);
	// render virtual cockpit
	const VCHUDSPEC *hudspec;
	const VCMFDSPEC *mfdspec[MAXMFD];
	SURFHANDLE sHUD = nullptr;//, sMFD[MAXMFD];

	if (bVC) {
		sHUD = g_client->GetVCHUDSurface (&hudspec);
		
		for (mfd = 0; mfd < MAXMFD; mfd++) {
//			sMFD[mfd] = g_client->GetVCMFDSurface (mfd, &mfdspec[mfd]);
			g_client->GetVCMFDSurface (mfd, &mfdspec[mfd]);
		}		
	}

//	for (auto &kw : mMeshes) {
  //      auto &mr = kw.second;

	for (int i = 0; i < nmesh; i++) {
		if (!meshlist[i].mesh) continue;
		auto &mr = meshlist[i];

		uint16_t vismode = mr.vismode;
/*
#define MESHVIS_NEVER          0x00  ///< Mesh is never visible
#define MESHVIS_EXTERNAL       0x01  ///< Mesh is visible in external views
#define MESHVIS_COCKPIT        0x02  ///< Mesh is visible in all internal (cockpit) views
#define MESHVIS_ALWAYS         (MESHVIS_EXTERNAL|MESHVIS_COCKPIT) ///< Mesh is always visible
#define MESHVIS_VC             0x04  ///< Mesh is only visible in virtual cockpit internal views
#define MESHVIS_EXTPASS        0x10  ///< Visibility modifier: render mesh during external pass, even for internal views*/

		if (bCockpit) {
			if (internalpass && (vismode & MESHVIS_EXTPASS)) continue;
			if (!(vismode & MESHVIS_COCKPIT)) {
				if ((!bVC) || (!(vismode & MESHVIS_VC))) continue;
			}
		} else {
			if (!(vismode & MESHVIS_EXTERNAL)) continue;
		}

		glm::fmat4 mWorldTrans = mModel;
		// transform mesh
		if (mr.trans) {
            mWorldTrans = mModel * *(mr.trans);
			//printf("VVessel::Render mr.trans\n");
            //exit(-1);
        }

		if (bVC) { // link MFD textures for rendering
			for (mfd = 0; mfd < MAXMFD; mfd++) {
				if (mfdspec[mfd] && mfdspec[mfd]->nmesh == (int)i) {
					mr.mesh->GetGroup(mfdspec[mfd]->ngroup)->TexIdx = TEXIDX_MFD0+mfd;
				}
			}
		}

		// render mesh
		mr.mesh->Render (c, mWorldTrans);

		// render VC HUD and MFDs
		if (bVC) {

			// render VC HUD
			if (sHUD && hudspec->nmesh == (int)i) {

				glDisable(GL_DEPTH_TEST);  
				glEnable(GL_BLEND);

				glm::vec3 sundir = *g_client->GetScene()->GetSunDir();

				static Shader mShader("Mesh.vs", "Mesh.fs");

				mShader.Bind();

				auto vp = c->GetViewProjectionMatrix();
				mShader.SetMat4("u_ViewProjection", *vp);
				mShader.SetMat4("u_Model", mWorldTrans);
				mShader.SetVec3("u_SunDir", sundir);
				glBindTexture(GL_TEXTURE_2D,  ((OGLTexture *)sHUD)->m_TexId);
				mShader.SetFloat("u_Textured", 1.0);
				mShader.SetFloat("u_ModulateAlpha", 0.0);
				OGLMesh::GROUPREC *g = mr.mesh->GetGroup(hudspec->ngroup);

				static OGLMaterial defmat = {
					{1,1,1,1},
					{1,1,1,1},
					{0,0,0,1},
					{0,0,0,1},0
				};

				OGLMaterial *mat = (g->MtrlIdx != SPEC_DEFAULT ? mr.mesh->GetMaterial(g->MtrlIdx) : &defmat);
				mShader.SetVec4("u_Material.ambient", mat->ambient);
				mShader.SetVec4("u_Material.diffuse", mat->diffuse);
				mShader.SetVec4("u_Material.specular", mat->specular);
				mShader.SetVec4("u_Material.emissive", mat->emissive);
				mShader.SetFloat("u_Material.specular_power", mat->specular_power);
				mShader.SetFloat("u_MatAlpha", 1.0);

				//dev->SetTexture (0, mfdsurf);
				//dev->SetRenderState (D3DRENDERSTATE_LIGHTING, FALSE);
				//dev->SetRenderState (D3DRENDERSTATE_ZENABLE, FALSE);
				//dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//				mr.mesh->RenderGroup (c, mWorldTrans, mr.mesh->GetGroup(hudspec->ngroup), (OGLTexture *)sHUD);
				mr.mesh->RenderGroup (mr.mesh->GetGroup(hudspec->ngroup));
				mShader.UnBind();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				//dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
				//dev->SetRenderState (D3DRENDERSTATE_LIGHTING, TRUE);
				//dev->SetRenderState (D3DRENDERSTATE_ZENABLE, TRUE);
			}
		}
	}

	return true;
}

bool VVessel::RenderExhaust (OGLCamera *c)
{
	if (!mVisible) return false;
	uint32_t i, nexhaust = vessel->GetExhaustCount();
	if (!nexhaust) return true; // nothing to do

	bool need_setup = true;
	double lvl, xsize, zsize;
	VECTOR3 cdir;
	OGLTexture *tex, *ptex = 0;
	EXHAUSTSPEC es;

	static TVERTEX ExhaustVtx[8] = {
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

		VBO = new VertexBuffer(ExhaustVtx, 8*sizeof(TVERTEX));
		VBO->Bind();

		IBO = new IndexBuffer(ExhaustIdx, 12);
		IBO->Bind();

		//position
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(TVERTEX),                  // stride
		(void*)0            // array buffer offset
		);
		CheckError("glVertexAttribPointer0");
		glEnableVertexAttribArray(0);

		//texcoord
		glVertexAttribPointer(
		1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(TVERTEX),                  // stride
		(void*)12            // array buffer offset
		);
		CheckError("glVertexAttribPointer");
		glEnableVertexAttribArray(1);
		CheckError("glEnableVertexAttribArray1");

		VBA->UnBind();
	}
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	static Shader s("Exhaust.vs","Exhaust.fs");

	s.Bind();
	auto vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection", *vp);
	s.SetMat4("u_Model", mModel);

	for (i = 0; i < nexhaust; i++) {
		if (!(lvl = vessel->GetExhaustLevel (i))) continue;
		vessel->GetExhaustSpec (i, &es);

		if (need_setup) { // initialise render state
			MATRIX3 R;
			vessel->GetRotationMatrix (R);
			cdir = tmul (R, cpos);
			/*
			dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, FALSE);
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
		VBO->Update(ExhaustVtx, 8 * sizeof(TVERTEX));
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

	s.UnBind();
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);
	/*
	if (!need_setup) { // reset render state
		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	}*/
	return true;
}

void VVessel::SetExhaustVertices (const VECTOR3 &edir, const VECTOR3 &cdir, const VECTOR3 &ref,
	double lscale, double wscale, TVERTEX *ev)
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

void VVessel::Animate (unsigned int an, double state, unsigned int mshidx)
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

void VVessel::AnimateComponent (ANIMATIONCOMP *comp, const glm::mat4 &T)
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

OGLTexture *VVessel::defexhausttex = nullptr;

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

