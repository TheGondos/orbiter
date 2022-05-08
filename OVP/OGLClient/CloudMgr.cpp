// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// CloudMgr.cpp
// class CloudManager (implementation)
//
// Planetary rendering management for cloud layers, including a simple
// LOD (level-of-detail) algorithm for patch resolution.
// ==============================================================

#include "CloudMgr.h"
#include "VPlanet.h"
#include "Texture.h"
#include "OGLCamera.h"
#include "Scene.h"
#include <cstring>

using namespace oapi;

// =======================================================================

CloudManager::CloudManager (const VPlanet *vplanet)
: TileManager (vplanet)
{
	char *texname = new char[strlen(objname)+7];
	strcpy (texname, objname);
	strcat (texname, "_cloud");
	delete []objname;
	objname = texname;

	maxlvl = std::min (*(int*)g_client->GetConfigParam (CFGPRM_SURFACEMAXLEVEL),        // global setting
	              *(int*)oapiGetObjectParam (obj, OBJPRM_PLANET_SURFACEMAXLEVEL)); // planet-specific setting
	maxbaselvl = std::min (8, maxlvl);
	pcdir = _V(1,0,0);
	lightfac = *(double*)g_client->GetConfigParam (CFGPRM_SURFACELIGHTBRT);
	nmask = 0;
	nhitex = nhispec = 0;
	cloudtexidx = 0;

	atmc = oapiGetPlanetAtmConstants (obj);

	int maxidx = patchidx[maxbaselvl];
	tiledesc = new TILEDESC[maxidx];
	memset (tiledesc, 0, maxidx*sizeof(TILEDESC));

	for (int i = 0; i < patchidx[maxbaselvl]; i++)
		tiledesc[i].flag = 1;
	LoadTextures ();
}

// =======================================================================

void CloudManager::Render (glm::mat4 &wmat, double scale, int level, double viewap)
{
	/*
	int pAlpha;
	dev->GetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, &pAlpha);
	if (!pAlpha)
		dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	bool do_micro = (microtex && microlvl > 0.01);
	if (do_micro) {
		dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		dev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		dev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);

		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_ADDSMOOTH);
		dev->SetTextureStageState (0, D3DTSS_TEXCOORDINDEX, 1);
		dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
		dev->SetTexture (0, microtex);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
		dev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		dev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		dev->SetTextureStageState (1, D3DTSS_TEXCOORDINDEX, 0);
		dev->SetTextureStageState (1, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
		double alpha = 1.0-microlvl;
		dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(1, 1, 1, alpha));
		cloudtexidx = 1;
	} else {
		cloudtexidx = 0;
	}
*/
	TileManager::Render (wmat, scale, level, viewap);
/*
	if (do_micro) {
		dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);

		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		dev->SetTextureStageState (0, D3DTSS_TEXCOORDINDEX, 0);
		dev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		dev->SetTexture (1, 0);
	}

	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
	if (!pAlpha)
		dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);*/
}

// =======================================================================

void CloudManager::RenderTile (int lvl, int hemisp, int ilat, int nlat, int ilng, int nlng, double sdist,
	TILEDESC *tile, const TEXCRDRANGE &range, OGLTexture *tex, OGLTexture *ltex, int flag)
{
	VBMESH &mesh = PATCH_TPL[lvl][ilat]; // patch template

if (range.tumin != 0 && range.tumax != 1) {
	printf("tumin=%f tumax=%f tvmin=%f tvmax=%f\n",range.tumin,range.tumax,range.tvmin,range.tvmax);
	exit(-1);
}
	
	static Shader s("Tile.vs","Tile.fs");
	
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	s.Bind();
	OGLCamera *c = g_client->GetScene()->GetCamera();
	auto *vpm = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection",*vpm);
	s.SetMat4("u_Model",RenderParam.wtrans);
    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();
	s.SetVec3("u_SunDir", sundir);

	if(vp->prm.bAddBkg) {
		VECTOR3 v3bgcol = g_client->GetScene()->SkyColour();
		glm::vec3 bgcol;
		bgcol.x = v3bgcol.x;
		bgcol.y = v3bgcol.y;
		bgcol.z = v3bgcol.z;
		s.SetVec3("u_bgcol", bgcol);
	} else {
		glm::vec3 bgcol(0,0,0);
		s.SetVec3("u_bgcol", bgcol);
	}
	if(vp->prm.bFog) {
		s.SetVec4("u_FogColor", vp->prm.mFogColor);
		s.SetFloat("u_FogDensity", vp->prm.mFogDensity);
	} else {
		s.SetFloat("u_FogDensity", 0);
	}

    glBindTexture(GL_TEXTURE_2D, tex->m_TexId);
	mesh.VAO->Bind();
	glDrawElements(GL_TRIANGLES, mesh.IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
	mesh.VAO->UnBind();
	s.UnBind();
    glBindTexture(GL_TEXTURE_2D,  0);
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	return;

/*

	LPDIRECT3DVERTEXBUFFER7 vb;        // processed vertex buffer
	VBMESH &mesh = PATCH_TPL[lvl][ilat]; // patch template

	if (range.tumin == 0 && range.tumax == 1) {
		vb = mesh.vb; // use vertex buffer directly
	} else {
		if (!tile->vtx) {
			D3DVERTEXBUFFERDESC vbd = 
				{ sizeof(D3DVERTEXBUFFERDESC), vbMemCaps | D3DVBCAPS_WRITEONLY, FVF_2TEX, mesh.nv };
			gc->GetDirect3D7()->CreateVertexBuffer (&vbd, &tile->vtx, 0);
			ApplyPatchTextureCoordinates (mesh, tile->vtx, range);
			tile->vtx->Optimize (RenderParam.dev, 0); // no more change, so we can optimize
		}
		vb = tile->vtx; // use buffer with transformed texture coords
	}

	// step 1: render full patch, either completely diffuse or completely specular
	RenderParam.dev->SetTexture (cloudtexidx, tex);
	RenderParam.dev->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, vb, 0,
		mesh.nv, mesh.idx, mesh.ni, 0);*/
}