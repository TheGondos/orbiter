// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OGL Client module
// ==============================================================

// ==============================================================
// Mesh.cpp
// class OGLMesh (implementation)
//
// This class represents a mesh in terms of DX7 interface elements
// (vertex buffers, index lists, materials, textures) which allow
// it to be rendered to the OpenGL device.
// ==============================================================

#include "glad.h"
#include "OGLMesh.h"
#include "OGLClient.h"
#include "Scene.h"
#include <cstring>

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
        abort();
	}
}

using namespace oapi;

static OGLMaterial defmat = {
	{1,1,1,1},
	{1,1,1,1},
	{0,0,0,1},
	{0,0,0,1},0
};

bool OGLMesh::bEnableSpecular = false;

OGLMesh::OGLMesh ()
{
	bTemplate = false;
	bModulateMatAlpha = false;
	nGrp = 0;
	nTex = 2;
	Tex = new OGLTexture *[nTex];
	Tex[0] = 0;
	nMtrl = 1;
	Mtrl = new OGLMaterial[nMtrl];
}

OGLMesh::OGLMesh (GROUPREC *grp, bool deepcopy)
{
	bTemplate = false;
	bModulateMatAlpha = false;
	nGrp = 0;
	nTex = 1;
	Tex = new OGLTexture *[nTex];
	Tex[0] = 0;
	nMtrl = 0;
	grp->MtrlIdx = SPEC_DEFAULT;
	grp->TexIdx = SPEC_DEFAULT;
	for (int n = 0; n < MAXTEX; n++) {
		grp->TexIdxEx[n] = SPEC_DEFAULT;
		grp->TexMixEx[n] = 0.0;
	}
	AddGroup (grp, deepcopy);
}

OGLMesh::OGLMesh (MESHHANDLE hMesh, bool asTemplate)
{
	int i;
	bTemplate = asTemplate;
	bModulateMatAlpha = false;
	nGrp = oapiMeshGroupCount (hMesh);
	Grp = new GROUPREC*[nGrp];
	for (i = 0; i < nGrp; i++) {
		Grp[i] = new GROUPREC;
		MESHGROUPEX *mg = oapiMeshGroupEx (hMesh, i);
		CopyGroup (Grp[i], mg);
	}
	nTex = oapiMeshTextureCount (hMesh)+1;
	Tex = new OGLTexture *[nTex];
	Tex[0] = 0; // 'no texture'
	for (i = 1; i < nTex; i++) {
		Tex[i] = (OGLTexture *)oapiGetTextureHandle (hMesh, i);
		// no deep copy here - texture templates shouldn't be modified by vessels
	}
	nMtrl = oapiMeshMaterialCount (hMesh);
	if (nMtrl)
		Mtrl = new OGLMaterial[nMtrl];
	for (i = 0; i < nMtrl; i++)
		CopyMaterial (Mtrl+i, oapiMeshMaterial (hMesh, i));
}

OGLMesh::OGLMesh (const OGLMesh &mesh)
{
	// note: 'mesh' must be a template mesh, because we may not be able to
	// access vertex data in video memory
	int i;
	bTemplate = false;
	bModulateMatAlpha = mesh.bModulateMatAlpha;
	nGrp = mesh.nGrp;
	Grp = new GROUPREC*[nGrp];
	for (i = 0; i < nGrp; i++) {
		Grp[i] = new GROUPREC;
		CopyGroup (Grp[i], mesh.Grp[i]);
	}
	nTex = mesh.nTex;
	Tex = new OGLTexture *[nTex];
	for (i = 0; i < nTex; i++) {
		Tex[i] = mesh.Tex[i];
		// no deep copy here - texture templates shouldn't be modified by vessels
	}
	nMtrl = mesh.nMtrl;
	if (nMtrl)
		Mtrl = new OGLMaterial[nMtrl];
	memcpy (Mtrl, mesh.Mtrl, nMtrl*sizeof(OGLMaterial));
}

OGLMesh::~OGLMesh ()
{
	ClearGroups();
	if (nTex) delete []Tex;
	if (nMtrl) delete []Mtrl;
}

void OGLMesh::GlobalEnableSpecular (bool enable)
{
	bEnableSpecular = enable;
}

int OGLMesh::AddGroup (GROUPREC *grp, bool deepcopy)
{
	GROUPREC **tmp = new GROUPREC*[nGrp+1];
	if (nGrp) {
		memcpy (tmp, Grp, nGrp*sizeof(GROUPREC*));
		delete []Grp;
	}
	Grp = tmp;
	if (deepcopy) CopyGroup (Grp[nGrp], grp);
	else          Grp[nGrp] = grp;

	return nGrp++;
}

int OGLMesh::AddGroup (const MESHGROUPEX *mg)
{
	GROUPREC *tmp = new GROUPREC;
	CopyGroup (tmp, mg);
	return AddGroup (tmp, false);
}

bool OGLMesh::CopyGroup (GROUPREC *tgt, const GROUPREC *src)
{
	tgt->nVtx = src->nVtx;
	tgt->Vtx = new NTVERTEX[tgt->nVtx];
    memcpy (tgt->Vtx, src->Vtx, src->nVtx*sizeof(NTVERTEX));
	tgt->nIdx = src->nIdx;
	tgt->Idx = new uint16_t[tgt->nIdx];
	memcpy (tgt->Idx, src->Idx, tgt->nIdx*sizeof(uint16_t));
	tgt->TexIdx = src->TexIdx;
	memcpy (tgt->TexIdxEx, src->TexIdxEx, MAXTEX*sizeof(int));
	memcpy (tgt->TexMixEx, src->TexMixEx, MAXTEX*sizeof(float));
	tgt->MtrlIdx = src->MtrlIdx;
	tgt->UsrFlag = src->UsrFlag;
	tgt->zBias   = src->zBias;
	tgt->IntFlag = src->IntFlag;

    tgt->VBA = std::make_unique<VertexArray>();
    tgt->VBA->Bind();
    tgt->VBO = std::make_unique<VertexBuffer>(tgt->Vtx, tgt->nVtx * sizeof(NTVERTEX));
    tgt->VBO->Bind();

    glVertexAttribPointer(
    0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
    (void*)0            // array buffer offset
    );
    CheckError("glVertexAttribPointer0");
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
    1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
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
    sizeof(NTVERTEX),                  // stride
    (void*)24            // array buffer offset
    );
    CheckError("glVertexAttribPointer");
    glEnableVertexAttribArray(2);
    CheckError("glEnableVertexAttribArray2");

    tgt->IBO = std::make_unique<IndexBuffer>(tgt->Idx, tgt->nIdx);
    tgt->IBO->Bind();
    tgt->VBA->UnBind();

	return true;
}

bool OGLMesh::CopyGroup (GROUPREC *grp, const MESHGROUPEX *mg)
{
	grp->nVtx = mg->nVtx;
    grp->Vtx = new NTVERTEX[mg->nVtx];
    memcpy (grp->Vtx, mg->Vtx, grp->nVtx*sizeof(NTVERTEX));
	grp->nIdx = mg->nIdx;
	grp->Idx = new uint16_t[grp->nIdx];
	memcpy (grp->Idx, mg->Idx, grp->nIdx*sizeof(uint16_t));
	grp->TexIdx = mg->TexIdx;
	memcpy (grp->TexIdxEx, mg->TexIdxEx, MAXTEX*sizeof(int));
	memcpy (grp->TexMixEx, mg->TexMixEx, MAXTEX*sizeof(float));
	if (grp->TexIdx != SPEC_DEFAULT && grp->TexIdx != SPEC_INHERIT) grp->TexIdx++;
	for (int n = 0; n < MAXTEX; n++)
		if (grp->TexIdxEx[n] != SPEC_DEFAULT) grp->TexIdxEx[n]++;
	grp->MtrlIdx = mg->MtrlIdx;
	grp->UsrFlag = mg->UsrFlag;
	grp->zBias   = mg->zBias;
	grp->IntFlag = mg->Flags;

    grp->VBA = std::make_unique<VertexArray>();
    grp->VBA->Bind();
    grp->VBO = std::make_unique<VertexBuffer>(mg->Vtx, mg->nVtx * sizeof(NTVERTEX));
    grp->VBO->Bind();

    glVertexAttribPointer(
    0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
    (void*)0            // array buffer offset
    );
    CheckError("glVertexAttribPointer0");
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
    1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
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
    sizeof(NTVERTEX),                  // stride
    (void*)24            // array buffer offset
    );
    CheckError("glVertexAttribPointer");
    glEnableVertexAttribArray(2);
    CheckError("glEnableVertexAttribArray2");

    grp->IBO = std::make_unique<IndexBuffer>(mg->Idx, mg->nIdx);
    grp->IBO->Bind();
    grp->VBA->UnBind();

	return true;
}

bool OGLMesh::CopyMaterial (OGLMaterial *mat7, MATERIAL *mat)
{
	memcpy (mat7, mat, sizeof (OGLMaterial));
	return true;
	// exploits the fact that OGLMaterial and MATERIAL are identical
	// structures. In general, this needs to be converted from one
	// structure to the other.
}

void OGLMesh::DeleteGroup (GROUPREC *grp)
{
	delete []grp->Idx;
	if (grp->nVtx) delete[] grp->Vtx;
	delete grp;
}

int OGLMesh::GetGroup (int grp, GROUPREQUESTSPEC *grs)
{
	static NTVERTEX zero = {0,0,0, 0,0,0, 0,0};
	if (grp >= nGrp) return 1;
	GROUPREC *g = Grp[grp];
	int nv = g->nVtx;
	uint32_t ni = g->nIdx;
	int i, vi;
	int ret = 0;

	if (grs->nVtx && grs->Vtx) { // vertex data requested
        if (grs->VtxPerm) { // random access data request
            for (i = 0; i < grs->nVtx; i++) {
                vi = grs->VtxPerm[i];
                if (vi < nv) {
                    grs->Vtx[i] = g->Vtx[vi];
                } else {
                    grs->Vtx[i] = zero;
                    ret = 1;
                }
            }
        } else {
            if (grs->nVtx > nv) grs->nVtx = nv;
            memcpy (grs->Vtx, g->Vtx, grs->nVtx * sizeof(NTVERTEX));
        }
	}

	if (grs->nIdx && grs->Idx) { // index data requested
		if (grs->IdxPerm) { // random access data request
			for (i = 0; i < grs->nIdx; i++) {
				vi = grs->IdxPerm[i];
				if (vi < ni) {
					grs->Idx[i] = g->Idx[vi];
				} else {
					grs->Idx[i] = 0;
					ret = 1;
				}
			}
		} else {
			if (grs->nIdx > ni) grs->nIdx = ni;
			memcpy (grs->Idx, g->Idx, grs->nIdx * sizeof(uint16_t));
		}
	}

	grs->MtrlIdx = g->MtrlIdx;
	grs->TexIdx = g->TexIdx;
	return ret;
}

int OGLMesh::EditGroup (int grp, GROUPEDITSPEC *ges)
{
	if (grp >= nGrp) return 1;
	GROUPREC *g = Grp[grp];
	int i, vi;

	int flag = ges->flags;
	if (flag & GRPEDIT_SETUSERFLAG)
		g->UsrFlag = ges->UsrFlag;
	else if (flag & GRPEDIT_ADDUSERFLAG)
		g->UsrFlag |= ges->UsrFlag;
	else if (flag & GRPEDIT_DELUSERFLAG)
		g->UsrFlag &= ~ges->UsrFlag;

	if (flag & GRPEDIT_VTXMOD) {
		NTVERTEX *vtx = g->Vtx;
        for (i = 0; i < ges->nVtx; i++) {
            vi = (ges->vIdx ? ges->vIdx[i] : i);
            if (vi < g->nVtx) {
                if      (flag & GRPEDIT_VTXCRDX)    vtx[vi].x   = ges->Vtx[i].x;
                else if (flag & GRPEDIT_VTXCRDADDX) vtx[vi].x  += ges->Vtx[i].x;
                if      (flag & GRPEDIT_VTXCRDY)    vtx[vi].y   = ges->Vtx[i].y;
                else if (flag & GRPEDIT_VTXCRDADDY) vtx[vi].y  += ges->Vtx[i].y;
                if      (flag & GRPEDIT_VTXCRDZ)    vtx[vi].z   = ges->Vtx[i].z;
                else if (flag & GRPEDIT_VTXCRDADDZ) vtx[vi].z  += ges->Vtx[i].z;
                if      (flag & GRPEDIT_VTXNMLX)    vtx[vi].nx  = ges->Vtx[i].nx;
                else if (flag & GRPEDIT_VTXNMLADDX) vtx[vi].nx += ges->Vtx[i].nx;
                if      (flag & GRPEDIT_VTXNMLY)    vtx[vi].ny  = ges->Vtx[i].ny;
                else if (flag & GRPEDIT_VTXNMLADDY) vtx[vi].ny += ges->Vtx[i].ny;
                if      (flag & GRPEDIT_VTXNMLZ)    vtx[vi].nz  = ges->Vtx[i].nz;
                else if (flag & GRPEDIT_VTXNMLADDZ) vtx[vi].nz += ges->Vtx[i].nz;
                if      (flag & GRPEDIT_VTXTEXU)    vtx[vi].tu  = ges->Vtx[i].tu;
                else if (flag & GRPEDIT_VTXTEXADDU) vtx[vi].tu += ges->Vtx[i].tu;
                if      (flag & GRPEDIT_VTXTEXV)    vtx[vi].tv  = ges->Vtx[i].tv;
                else if (flag & GRPEDIT_VTXTEXADDV) vtx[vi].tv += ges->Vtx[i].tv;
            }
        }
		g->VBO->Bind();
        g->VBO->Update(g->Vtx, g->nVtx * sizeof(NTVERTEX));
		g->VBO->UnBind();
	}
	return 0;
}

void OGLMesh::ClearGroups ()
{
	if (nGrp) {
		for (int g = 0; g < nGrp; g++)
			DeleteGroup (Grp[g]);
		delete []Grp;
		nGrp = 0;
	}
}

bool OGLMesh::SetTexture (int texidx, SURFHANDLE tex)
{
	if (texidx >= nTex) return false;
	// do we need to release the previous texture here?
	Tex[texidx] = (OGLTexture *)tex;
	return true;
}

void OGLMesh::SetTexMixture (int ntex, float mix)
{
	ntex--;
	for (int g = 0; g < nGrp; g++) {
		if (Grp[g]->TexIdxEx[ntex] != SPEC_DEFAULT)
			Grp[g]->TexMixEx[ntex] = mix;
	}
}

void OGLMesh::RenderGroup (GROUPREC *grp)
{
	if (grp->nVtx && grp->nIdx) {
        grp->VBA->Bind();
        glDrawElements(GL_TRIANGLES, grp->IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
        grp->VBA->UnBind();
	}
}

void OGLMesh::Render (OGLCamera *c, glm::fmat4 &model)
{
    glEnable(GL_DEPTH_TEST);  
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();

    static Shader mShader("Mesh.vs", "Mesh.fs");

    mShader.Bind();

	auto vp = c->GetViewProjectionMatrix();
	mShader.SetMat4("u_ViewProjection", *vp);
	mShader.SetMat4("u_Model", model);
	mShader.SetVec3("u_SunDir", sundir);

//	dev->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
//	if (bModulateMatAlpha)
//		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	if (bModulateMatAlpha) {
		mShader.SetFloat("u_ModulateAlpha", 1.0);
	} else {
		mShader.SetFloat("u_ModulateAlpha", 0.0);
	}

	int g, j, n, mi, pmi, ti, pti, uflag, wrap, zb = 0, owrap = 0;
	bool skipped = false;
	bool texstage[MAXTEX] = {false};
//	bool specular = false;
//	bool lighting = true;

	OGLMaterial *mat = &defmat;
	for (g = 0; g < nGrp; g++) {

		uflag = Grp[g]->UsrFlag;
		if (uflag & 2) { // user skip
			skipped = true;
			continue;
		}

		// set material
		if ((mi = Grp[g]->MtrlIdx) == SPEC_INHERIT && skipped) // find last valid material
			for (j = g-1; j >= 0; j--)
				if ((mi = Grp[j]->MtrlIdx) != SPEC_INHERIT) break;

		
		if (mi != SPEC_INHERIT && (!g || mi != pmi)) {
			mat = (mi != SPEC_DEFAULT ? Mtrl+mi : &defmat);

/*
			if (bEnableSpecular) {
				if (mat->specular_power) {
					if (!specular) dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, specular = TRUE);
				} else {
					if (specular) dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, specular = FALSE);
				}
			}*/
			pmi = mi;
		} 

		mShader.SetVec4("u_Material.ambient", mat->ambient);
		mShader.SetVec4("u_Material.diffuse", mat->diffuse);
		mShader.SetVec4("u_Material.specular", mat->specular);
		mShader.SetVec4("u_Material.emissive", mat->emissive);
		mShader.SetFloat("u_Material.specular_power", mat->specular_power);
		mShader.SetFloat("u_MatAlpha", mat->diffuse.a);

		// set primary texture
		if ((ti = Grp[g]->TexIdx) == SPEC_INHERIT && skipped) // find last valid texture
			for (j = g-1; j >= 0; j--)
				if ((ti = Grp[j]->TexIdx) != SPEC_INHERIT) break;
		if (ti != SPEC_INHERIT && (!g || (ti != pti))) {
			OGLTexture * tx = nullptr;
			if (ti != SPEC_DEFAULT) {
				tx = (ti < TEXIDX_MFD0 ? Tex[ti] : (OGLTexture *)g_client->GetMFDSurface(ti-TEXIDX_MFD0));
			} else tx = nullptr;
            if(tx == nullptr) {
                glBindTexture(GL_TEXTURE_2D,  0);
                mShader.SetFloat("u_Textured", 0.0);
            } else {
                glBindTexture(GL_TEXTURE_2D,  tx->m_TexId);
                mShader.SetFloat("u_Textured", 1.0);
                mShader.SetFloat("u_MatAlpha", 1.0);
            }
			pti = ti;
		}

		// set additional textures
        /*
		for (n = 0; n < MAXTEX; n++) {
			if (Grp[g]->TexMixEx[n] && (ti = Grp[g]->TexIdxEx[n]) != SPEC_DEFAULT) {
				dev->SetTexture (n+1, Tex[ti]);
				dev->SetTextureStageState (n+1, D3DTSS_COLOROP, D3DTOP_ADD);
				dev->SetTextureStageState (n+1, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
				texstage[n] = true;
			} else if (texstage[n]) {
				dev->SetTextureStageState (n+1, D3DTSS_COLOROP, D3DTOP_DISABLE);
				texstage[n] = false;
			}
		}

		if (zb != Grp[g]->zBias)
			dev->SetRenderState (D3DRENDERSTATE_ZBIAS, zb = Grp[g]->zBias);

		wrap = 0;
		if (Grp[g]->IntFlag & 0x03) { // wrap flag
			if (Grp[g]->IntFlag & 0x01) wrap |= D3DWRAP_U;
			if (Grp[g]->IntFlag & 0x02) wrap |= D3DWRAP_V;
		}
		if (wrap != owrap)
			dev->SetRenderState (D3DRENDERSTATE_WRAP0, owrap = wrap);

		if (!(uflag & 0x4) != lighting)
			dev->SetRenderState (D3DRENDERSTATE_LIGHTING, lighting = !lighting);
*/
/*
		if(Grp[g]->zBias) {
			glEnable( GL_POLYGON_OFFSET_FILL );
			printf("%f\n", Grp[g]->zBias);
			glPolygonOffset( Grp[g]->zBias, Grp[g]->zBias );
		}
*/
		if (uflag & 0x8) { // brighten
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		if (uflag &0x10) { // skip texture color information
		//printf("skip\n");
		//	mShader.SetFloat("u_Textured", 0.0);
//			dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		}

		RenderGroup (Grp[g]);
		/*
		if(Grp[g]->zBias) {
			glDisable( GL_POLYGON_OFFSET_FILL );
		}*/

		if (uflag & 0x8) { // reset brighten
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		if (uflag & 0x10) {
//			dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		}

		skipped = false;
	}
/*
	if (owrap)     dev->SetRenderState (D3DRENDERSTATE_WRAP0, 0);
	if (zb)        dev->SetRenderState (D3DRENDERSTATE_ZBIAS, 0);
	if (specular)  dev->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
	if (!lighting) dev->SetRenderState (D3DRENDERSTATE_LIGHTING, TRUE);
	for (n = 0; n < MAXTEX; n++) {
		if (texstage[n]) 
			dev->SetTextureStageState (n+1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	}
	if (bModulateMatAlpha)
		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        */
}

void OGLMesh::TransformGroup (int n, const glm::fmat4 *mm)
{
	GROUPREC *grp = Grp[n];
	int i, nv = grp->nVtx;
	float x, y, z, w;
    const glm::fmat4 &m=*mm;

	for (i = 0; i < nv; i++) {
		NTVERTEX &v = grp->Vtx[i];
		x = v.x*m[0][0] + v.y*m[1][0] + v.z* m[2][0] + m[3][0];
		y = v.x*m[0][1] + v.y*m[1][1] + v.z* m[2][1] + m[3][1];
		z = v.x*m[0][2] + v.y*m[1][2] + v.z* m[2][2] + m[3][2];
		w = v.x*m[0][3] + v.y*m[1][3] + v.z* m[2][3] + m[3][3];
    	v.x = x/w;
		v.y = y/w;
		v.z = z/w;

		x = v.nx*m[0][0] + v.ny*m[1][0] + v.nz* m[2][0];
		y = v.nx*m[0][1] + v.ny*m[1][1] + v.nz* m[2][1];
		z = v.nx*m[0][2] + v.ny*m[1][2] + v.nz* m[2][2];
		float len = (float)sqrt (x*x + y*y + z*z);
		if(len != 0.0)
			w = 1.0f/len;
		else
			w = 1.0;

		v.nx = x*w;
		v.ny = y*w;
		v.nz = z*w;
	}

	grp->VBO->Bind();
    grp->VBO->Update(grp->Vtx, nv * sizeof(NTVERTEX));
	grp->VBO->UnBind();
	//if (GrpSetup) SetupGroup (grp);
}
