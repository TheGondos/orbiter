// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// HazeMgr.cpp
// class HazeManager (implementation)
//
// Planetary atmospheric haze rendering
// Implemented as transparent overlay on planetary disc
// ==============================================================
#include "glad.h"
#include "HazeMgr.h"
#include "VPlanet.h"
#include "Texture.h"
#include "OGLCamera.h"
#include <glm/gtc/matrix_inverse.hpp>

using namespace oapi;

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
	}
}

HazeManager::HazeManager (const VPlanet *vplanet)
{
	vp = vplanet;
	obj = vp->GetObject();
	rad = oapiGetSize (obj);
	const ATMCONST *atmc = oapiGetPlanetAtmConstants (obj);
	if (atmc) {
		basecol = *(VECTOR3*)oapiGetObjectParam (obj, OBJPRM_PLANET_HAZECOLOUR);
		hralt = (float)(atmc->horizonalt / rad);
		dens0 = (float)(std::min (1.0, atmc->horizonalt/64e3 *
			*(double*)oapiGetObjectParam (obj, OBJPRM_PLANET_HAZEDENSITY)));
	} else {
		basecol = _V(1,1,1);
		hralt = 0.01f;
		dens0 = 1.0f;
	}
	if (*(bool*)oapiGetObjectParam (obj, OBJPRM_PLANET_HASCLOUDS)) {
		hshift = *(double*)oapiGetObjectParam (obj, OBJPRM_PLANET_HAZESHIFT);
		cloudalt = *(double*)oapiGetObjectParam (obj, OBJPRM_PLANET_CLOUDALT);
	} else
		hshift = 0;

	hscale = (float)(1.0 - *(double*)oapiGetObjectParam (obj, OBJPRM_PLANET_HAZEEXTENT));
}

void HazeManager::GlobalInit ()
{
	int i;
	for (i = 0; i < HORIZON_NSEG; i++)
		Idx[i*2] = i*2+1, Idx[i*2+1] = i*2;
	Idx[i*2] = 1, Idx[i*2+1] = 0;

	for (i = 0; i < HORIZON_NSEG; i++) {
		Vtx[i*2].tu = Vtx[i*2+1].tu = (float)(i%2);
		Vtx[i*2].tv = 1.0f;
		Vtx[i*2+1].tv = 0.0f;
		double phi = (double)i/(double)HORIZON_NSEG * PI*2.0;
		CosP[i] = (float)cos(phi), SinP[i] = (float)sin(phi);
	}
	horizon = g_client->GetTexMgr()->LoadTexture ("Horizon.dds", true, 0);

	VBA = std::make_unique<VertexArray>();
	VBA->Bind();
	VBO = std::make_unique<VertexBuffer>(Vtx, sizeof(Vtx));
	VBO->Bind();
	IBO = std::make_unique<IndexBuffer>(Idx, nIdx);
	IBO->Bind();

        glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(HVERTEX),                  // stride
        (void*)0            // array buffer offset
        );
        CheckError("glVertexAttribPointer0");
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
        1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        4,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(HVERTEX),                  // stride
        (void*)12            // array buffer offset
        );
        CheckError("glVertexAttribPointer");
        glEnableVertexAttribArray(1);
        CheckError("glEnableVertexAttribArray1");

        glVertexAttribPointer(
        2,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(HVERTEX),                  // stride
        (void*)28            // array buffer offset
        );
        CheckError("glVertexAttribPointer");
        glEnableVertexAttribArray(2);
        CheckError("glEnableVertexAttribArray2");

	VBA->UnBind();
}

void HazeManager::Render (OGLCamera *c, glm::mat4 &wmat, bool dual)
{
	glm::mat4 imat(0);

	VECTOR3 psun;
	int i, j;
	double phi, csun, alpha, colofs;
	float cosp, sinp, cost, sint, h1, h2, r1, r2, intr, intg, intb;
glDisable(GL_DEPTH_TEST);
//	D3DMAT_MatrixInvert (&imat, &wmat);
	imat = glm::affineInverse(wmat);
	VECTOR3 rpos = {imat[3][0], imat[3][1], imat[3][2]};   // camera in local coords (planet radius = 1)
	double cdist = length (rpos);

	alpha = dens0 * std::min (1.0, (cdist-1.0)*200.0);
	if (!dual) alpha = 1.0-alpha;
	if (alpha <= 0.0) return;  // nothing to do
	alpha = std::min(alpha,1.0);

	VECTOR3 cpos = {0,cdist,0};
	double id = 1.0 / std::max (cdist, 1.002);  // inverse camera distance; 1.001: hack to avoid horizon to creep too close
	double visrad = acos (id);             // aperture of visibility sector
	double sinv = sin(visrad);
	h1 = (float)id, h2 = h1 + (float)(hralt*id);
	r1 = (float)sinv, r2 = (1.0f+hralt)*r1;
	
	if (!dual) { // pull lower horizon edge below surface to avoid problems with elevations < 0
		h1 *= vp->prm.horizon_minrad;
		r1 *= vp->prm.horizon_minrad;
	}

	if (hshift) {
		if (cdist-1.0 > cloudalt/rad) {
			float dr = (float)(hshift*sinv);
			float dh = (float)(hshift*id);
			h1 += dh, h2 += dh;
			r1 += dr, r2 += dr;
		}
	}

	float dens = (float)std::max (1.0, 1.4 - 0.3/hralt*(cdist-1.0)); // saturate haze colour at low altitudes
	if (dual) dens *= (float)(0.5 + 0.5/cdist);                 // scale down intensity at large distances

	normalise (rpos);
	cost = (float)rpos.y, sint = (float)sqrt (1.0-cost*cost);
	phi = atan2 (rpos.z, rpos.x), cosp = (float)cos(phi), sinp = (float)sin(phi);
	glm::mat4 rmat = {cost*cosp, -sint, cost*sinp, 0,
		              sint*cosp,  cost, sint*sinp, 0,
					  -sinp,      0,    cosp,      0,
					  0,          0,    0,         1};

	//D3DMAT_MatrixMultiply (&transm, &wmat, &rmat);
	glm::mat4 transm = wmat * rmat;

	MATRIX3 rrmat = {cost*cosp, -sint, cost*sinp,
		             sint*cosp,  cost, sint*sinp,
					 -sinp,      0,    cosp     };

	MATRIX3 grot;
	VECTOR3 gpos;
	oapiGetRotationMatrix (obj, &grot);
	oapiGetGlobalPos (obj, &gpos);
	psun = tmul (grot, -gpos); // sun in planet coords
	psun = mul (rrmat, psun);  // sun in camera-relative horizon coords
	VECTOR3 cs = psun-cpos; normalise(cs); // camera->sun
	normalise (psun);
	//float psunx = (float)psun.x, psuny = (float)psun.y, psunz = (float)psun.z;

	colofs = (dual ? 0.4 : 0.3);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
//	dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &transm);
//	dev->SetTexture (0, horizon);
	glBindTexture(GL_TEXTURE_2D, horizon->m_TexId);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
//	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//	dev->SetRenderState (D3DRENDERSTATE_LIGHTING, FALSE);
//	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	HVERTEX *v = (HVERTEX *)VBO->Map();

	for (i = j = 0; i < HORIZON_NSEG; i++) {
		VECTOR3 hp = {v[j].x = r1*CosP[i], v[j].y = h1, v[j].z = r1*SinP[i]};
		csun = dotp (hp, psun);
		VECTOR3 cp = hp-cpos; normalise(cp);
		double colsh = 0.5*(dotp (cp,cs) + 1.0);

		// compose a colourful sunset
		double maxred   = colofs-0.18*colsh,  minred   = maxred-0.4;
		double maxgreen = colofs-0.1*colsh,  mingreen = maxgreen-0.4;
		double maxblue  = colofs/*+0.0*colsh*/,  minblue  = maxblue-0.4;
		if      (csun > maxred) intr = 1.0f;
		else if (csun < minred) intr = 0.0f;
		else                    intr = (float)((csun-minred)*2.5);
		if      (csun > maxgreen) intg = 1.0f;
		else if (csun < mingreen) intg = 0.0f;
		else                      intg = (float)((csun-mingreen)*2.5);
		if      (csun > maxblue) intb = 1.0f;
		else if (csun < minblue) intb = 0.0f;
		else                     intb = (float)((csun-minblue)*2.5);
		glm::vec4 col = {intr*std::min(1.0,dens*basecol.x), intg*std::min(1.0,dens*basecol.y), intb*std::min(1.0,dens*basecol.z), alpha};

		v[j].dcol = col;
		j++;
		v[j].x = r2*CosP[i];
		v[j].y = h2;
		v[j].z = r2*SinP[i];
		v[j].dcol = col;
		j++;
	}
	VBO->UnMap();

static Shader s("Haze.vs","Haze.fs");
	s.Bind();
	auto *vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection",*vp);
//	s.SetMat4("u_Model",*ringmat);
	s.SetMat4("u_Model",transm);
//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	VBA->Bind();
	glDrawElements(GL_TRIANGLE_STRIP, IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
	VBA->UnBind();

//	dev->DrawIndexedPrimitive (D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE |
//		D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0), Vtx, 2*HORIZON_NSEG, Idx, nIdx, 0);

	if (dual) {
		h2 = h1;
		r2 = hscale * r1*r1; 
		HVERTEX *v = (HVERTEX *)VBO->Map();
		for (i = j = 0; i < HORIZON_NSEG; i++) {
			j++;
			v[j].x = r2*CosP[i];
			v[j].y = h2;
			v[j].z = r2*SinP[i];
			j++;
		}
		VBO->UnMap();
//		dev->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
//		dev->DrawIndexedPrimitive (D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE |
//			D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0), Vtx, 2*HORIZON_NSEG, Idx, nIdx, 0);
//		dev->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
		VBA->Bind();
	glFrontFace(GL_CCW);
		glDrawElements(GL_TRIANGLE_STRIP, IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
	glFrontFace(GL_CW);

		VBA->UnBind();
	}

//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


	s.UnBind();

//	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
//	dev->SetRenderState (D3DRENDERSTATE_LIGHTING, TRUE);
//	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
//	dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);

	glBindTexture(GL_TEXTURE_2D, 0);
}

// ==============================================================
// static member initialisation

uint16_t     HazeManager::Idx[HORIZON_NSEG*2+2];
struct   HazeManager::HVERTEX HazeManager::Vtx[HORIZON_NSEG*2];
int    HazeManager::nIdx = HORIZON_NSEG*2+2;
float HazeManager::CosP[HORIZON_NSEG];
float HazeManager::SinP[HORIZON_NSEG];
OGLTexture *HazeManager::horizon = 0;
std::unique_ptr<VertexBuffer> HazeManager::VBO;
std::unique_ptr<IndexBuffer> HazeManager::IBO;
std::unique_ptr<VertexArray> HazeManager::VBA;
