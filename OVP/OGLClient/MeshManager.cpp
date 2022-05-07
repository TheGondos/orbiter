#include "glad.h"
#include "MeshManager.h"
#include "OGLCamera.h"
#include "Scene.h"
#include <stdio.h>
#include <cstring>

static OGLMaterial defmat = {
	{1,1,1,1},
	{1,1,1,1},
	{0,0,0,1},
	{0,0,0,1},0
};

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
        abort();
	}
}

std::unique_ptr<Shader> OGLMesh::mShader;// = std::make_unique<Shader>("Mesh.vs","Mesh.fs");
#define SPEC_DEFAULT ((uint32_t)-1)
#define SPEC_INHERIT ((uint32_t)-2)
OGLMesh::OGLMesh (MESHHANDLE hMesh)
{
	uint32_t nbGroup = oapiMeshGroupCount (hMesh);


    uint32_t nTex = oapiMeshTextureCount (hMesh)+1;
    mTextures.reserve(nTex);
    mTextures.emplace_back(nullptr); // Slot 0 = no texture
    for (uint32_t i = 1; i < nTex; i++) {
        mTextures.emplace_back((OGLTexture *)oapiGetTextureHandle (hMesh, i));
    }
	uint32_t nMtrl = oapiMeshMaterialCount (hMesh);
    if (nMtrl) {
        mMaterials.reserve(nMtrl);
        for (uint32_t i = 0; i < nMtrl; i++) {

            MATERIAL *m = oapiMeshMaterial (hMesh, i);
            OGLMaterial om;
            om.ambient  = glm::vec4(m->ambient.r,  m->ambient.g,  m->ambient.b,  m->ambient.a);
            om.diffuse  = glm::vec4(m->diffuse.r,  m->diffuse.g,  m->diffuse.b,  m->diffuse.a);
            om.specular = glm::vec4(m->specular.r, m->specular.g, m->specular.b, m->specular.a);
            om.emissive = glm::vec4(m->emissive.r, m->emissive.g, m->emissive.b, m->emissive.a);
            om.specular_power = m->power;

            mMaterials.emplace_back(om);
        }
    }

    mMeshGroups.reserve(nbGroup);

    uint32_t lastTexture = 0;
    uint32_t lastMaterial = SPEC_DEFAULT;
	for (uint32_t i = 0; i < nbGroup; i++) {
		MESHGROUPEX *mg = oapiMeshGroupEx (hMesh, i);
        mMeshGroups.emplace_back();

        mMeshGroups[i].flags = mg->Flags;
        mMeshGroups[i].UsrFlag = mg->UsrFlag;
        mMeshGroups[i].m_zBias = mg->zBias;

        mMeshGroups[i].VBA = std::make_unique<VertexArray>();
        mMeshGroups[i].VBA->Bind();
        mMeshGroups[i].VBO = std::make_unique<VertexBuffer>(mg->Vtx, mg->nVtx * sizeof(NTVERTEX));
        mMeshGroups[i].VBO->Bind();
        mMeshGroups[i].nVtx = mg->nVtx;
        if(mg->TexIdx == SPEC_INHERIT) {
            mMeshGroups[i].mTexIdx = lastTexture + 1;
        } else if(mg->TexIdx == SPEC_DEFAULT) {
            mMeshGroups[i].mTexIdx = 0;
        } else {
            mMeshGroups[i].mTexIdx = mg->TexIdx+1;
            lastTexture = mg->TexIdx;
        }
        if(mg->MtrlIdx == SPEC_INHERIT) {
            printf("mtrl SPEC_INHERIT last=%d\n", lastMaterial);
            mg->MtrlIdx = lastMaterial;
        }
        mMeshGroups[i].mTexIdx = mg->TexIdx+1;

        if(mg->MtrlIdx >= mMaterials.size() && mg->MtrlIdx != SPEC_DEFAULT) {
            printf("mg->MtrlIdx >= mMaterials.size() %d <= %ld nMtrl=%d\n", mg->MtrlIdx, mMaterials.size(), nMtrl);
            exit(EXIT_FAILURE);
        }
        mMeshGroups[i].mMatIdx = mg->MtrlIdx;
        lastMaterial = mg->MtrlIdx;

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

        mMeshGroups[i].IBO = std::make_unique<IndexBuffer>(mg->Idx, mg->nIdx);
        mMeshGroups[i].IBO->Bind();

        mMeshGroups[i].VBA->UnBind();
    }

}

int OGLMesh::SetMaterial (int matidx, const MATERIAL *m)
{
    OGLMaterial &om = mMaterials[matidx];

    om.ambient  = glm::vec4(m->ambient.r,  m->ambient.g,  m->ambient.b,  m->ambient.a);
    om.diffuse  = glm::vec4(m->diffuse.r,  m->diffuse.g,  m->diffuse.b,  m->diffuse.a);
    om.specular = glm::vec4(m->specular.r, m->specular.g, m->specular.b, m->specular.a);
    om.emissive = glm::vec4(m->emissive.r, m->emissive.g, m->emissive.b, m->emissive.a);
    om.specular_power = m->power;

    return 0;
}

OGLMesh::OGLMesh (MESHGROUPEX *mg, OGLMaterial *mat, OGLTexture *t)
{
    mMeshGroups.reserve(1);

    mMeshGroups.emplace_back();

    mMeshGroups[0].flags = mg->Flags;
    mMeshGroups[0].UsrFlag = mg->UsrFlag;
    mMeshGroups[0].m_zBias = mg->zBias;

    mMeshGroups[0].VBA = std::make_unique<VertexArray>();
    mMeshGroups[0].VBA->Bind();
    mMeshGroups[0].VBO = std::make_unique<VertexBuffer>(mg->Vtx, mg->nVtx * sizeof(NTVERTEX));
    mMeshGroups[0].VBO->Bind();
    mMeshGroups[0].nVtx = mg->nVtx;

    if(mg->TexIdx == SPEC_DEFAULT) {
        printf("inherited OGLMesh::OGLMesh (MESHGROUPEX *mg)\n");
        exit(-1);
    }
    mMeshGroups[0].mTexIdx = 1;
    mMeshGroups[0].mMatIdx = 0;
    mTextures.reserve(2);
    mTextures.emplace_back(nullptr); // Slot 0 = no texture
    mTextures.emplace_back(t);

    mMaterials.reserve(1);
    mMaterials.emplace_back(*mat);
    //OGLMaterial *m=mat;

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

    mMeshGroups[0].IBO = std::make_unique<IndexBuffer>(mg->Idx, mg->nIdx);
    mMeshGroups[0].IBO->Bind();

    mMeshGroups[0].VBA->UnBind();
}

OGLMesh::~OGLMesh ()
{

}

bool OGLMesh::SetTexture (int texidx, SURFHANDLE tex)
{
	if ((size_t)texidx >= mTextures.size()) return false;
    if(tex == nullptr) {
        printf("settexture 0\n");
    }
	mTextures[texidx] = (OGLTexture *)tex;
	return true;
}

void OGLMesh::RenderGroup(OGLCamera *c, glm::fmat4 &model, MeshGroup *mg, OGLTexture *tex)
{
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
glDisable(GL_DEPTH_TEST);
    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();

    mShader->Bind();
	auto vp = c->GetViewProjectionMatrix();
	mShader->SetMat4("u_ViewProjection", *vp);
	mShader->SetMat4("u_Model", model);
	mShader->SetVec3("u_SunDir", sundir);

   // int i=0;
    //int vt=0;
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    	//uint32_t uflag = mg->UsrFlag;

//        if(mTextures[mg->mTexIdx + 1] != nullptr)
  //       glBindTexture(GL_TEXTURE_2D,  mTextures[mg->mTexIdx + 1]->m_TexId);
    //    else
      //   glBindTexture(GL_TEXTURE_2D,  0);

       //  glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);


        CheckError("glBindTexture");

        OGLMaterial *material = &defmat;
        if(mg->mMatIdx != SPEC_DEFAULT) {//SPEC_DEFAULT) {
            material = &mMaterials[mg->mMatIdx];
        } else {
            //printf("defaultmat on RenderGroup\n");
            //exit(-1);
        }

        glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);
        mShader->SetFloat("u_Textured", 1.0);
        mShader->SetFloat("u_MatAlpha", 1.0);
        mShader->SetVec4("u_Material.ambient", material->ambient);
        mShader->SetVec4("u_Material.diffuse", material->diffuse);
        mShader->SetVec4("u_Material.specular", material->specular);
        mShader->SetVec4("u_Material.emissive", material->emissive);
        mShader->SetFloat("u_Material.specular_power", material->specular_power);

        mg->VBA->Bind();
        mg->IBO->GetCount();
        glDrawElements(GL_TRIANGLES, mg->IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
        mg->VBA->UnBind();
        
    //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    mShader->UnBind();
    glDisable(GL_DEPTH_TEST);
}


void OGLMesh::Render(OGLCamera *c, glm::fmat4 &model)
{
    glEnable(GL_DEPTH_TEST);  
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();

    mShader->Bind();

	auto vp = c->GetViewProjectionMatrix();
	mShader->SetMat4("u_ViewProjection", *vp);
	mShader->SetMat4("u_Model", model);
	mShader->SetVec3("u_SunDir", sundir);

    int i=0;
    int vt=0;
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
//glEnable(GL_POLYGON_OFFSET_FILL);
    for (auto &mg : mMeshGroups) {
    	uint32_t uflag = mg.UsrFlag;
		if (uflag & 2) { // user skip
			//skipped = true;
			continue;
		}
		if (uflag & 0x8) { // brighten
            printf("brighten\n");//exit(-1);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

//			dev->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
//			dev->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		}

		if (uflag &0x10) { // skip texture color information
       // printf("skip texture\n");exit(-1);
//			dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		}

        OGLTexture *tex;

        //glPolygonOffset(1, mg.m_zBias);

        if((unsigned)mg.mTexIdx < (unsigned)TEXIDX_MFD0) {
            tex = mTextures[mg.mTexIdx];// + 1];
        } else {
            tex = (OGLTexture *)g_client->GetMFDSurface(mg.mTexIdx-TEXIDX_MFD0);
        // printf("m %p %d %d\n",tex, mg.mTexIdx, mg.mTexIdx-TEXIDX_MFD0);
        }

        OGLMaterial *material = &defmat;
        assert(mg.mMatIdx!=SPEC_INHERIT);
//        printf("mg.mMatIdx = %d %ld\n",mg.mMatIdx, mMaterials.size());
        if(mg.mMatIdx != SPEC_DEFAULT /*&& mg.mMatIdx < mMaterials.size()*/) {//SPEC_DEFAULT) {
            material = &mMaterials[mg.mMatIdx];
        } else {
  //                      mShader->UnBind();continue;
    //  printf("mg.mMatIdx == SPEC_DEFAULT\n");                  
//exit(-1);
        }
        if(tex != nullptr && mg.mTexIdx!=0) {
            glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);
            CheckError("glBindTexture(GL_TEXTURE_2D,  tex->m_TexId)");
            mShader->SetFloat("u_Textured", 1.0);
            mShader->SetFloat("u_MatAlpha", 1.0);
        } else {
            glBindTexture(GL_TEXTURE_2D,  0);
            mShader->SetFloat("u_Textured", 0.0);
            mShader->SetFloat("u_MatAlpha", material->diffuse.a);
        }

        CheckError("glBindTexture");

        mShader->SetVec4("u_Material.ambient", material->ambient);
        mShader->SetVec4("u_Material.diffuse", material->diffuse);
        mShader->SetVec4("u_Material.specular", material->specular);
        mShader->SetVec4("u_Material.emissive", material->emissive);
        mShader->SetFloat("u_Material.specular_power", material->specular_power);

        mg.VBA->Bind();
        vt+=mg.IBO->GetCount();
        glDrawElements(GL_TRIANGLES, mg.IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
        mg.VBA->UnBind();
        i++;

        		if (uflag & 0x8) { // brighten
  //          printf("brighten\n");//exit(-1);
//            glEnable(GL_BLEND);   
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
    }
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    mShader->UnBind();
    glDisable(GL_DEPTH_TEST);
//glDisable(GL_POLYGON_OFFSET_FILL);
}

int OGLMesh::EditGroup (int grp, GROUPEDITSPEC *ges)
{
	if ((size_t)grp >= mMeshGroups.size()) return 1;
	MeshGroup *mg = &mMeshGroups[grp];

	int flag = ges->flags;
	if (flag & GRPEDIT_SETUSERFLAG)
		mg->UsrFlag = ges->UsrFlag;
	else if (flag & GRPEDIT_ADDUSERFLAG)
		mg->UsrFlag |= ges->UsrFlag;
	else if (flag & GRPEDIT_DELUSERFLAG)
		mg->UsrFlag &= ~ges->UsrFlag;


	if (flag & GRPEDIT_VTXMOD) {
        NTVERTEX *vtx = (NTVERTEX *)mg->VBO->Map();
        for (int i = 0; i < ges->nVtx; i++) {
            int vi = (ges->vIdx ? ges->vIdx[i] : i);
            if (true) {
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
        mg->VBO->UnMap();
	}

    return 0;
}

void OGLMesh::TransformGroup (int n, const glm::fmat4 *mm)
{
    MeshGroup *mg = &mMeshGroups[n];
    NTVERTEX *vtx = (NTVERTEX *)mg->VBO->Map();
    glm::fmat4 m=*mm;
	int i, nv = mg->nVtx;
	float x, y, z, w;

	for (i = 0; i < nv; i++) {
		NTVERTEX &v = vtx[i];
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
		w = 1.0f/(float)sqrt (x*x + y*y + z*z);
		v.nx = x*w;
		v.ny = y*w;
		v.nz = z*w;
	}

    mg->VBO->UnMap();
}


int OGLMesh::GetGroup (int grp, GROUPREQUESTSPEC *grs)
{
	static NTVERTEX zero = {0,0,0, 0,0,0, 0,0};
	if ((size_t)grp >= mMeshGroups.size()) return 1;
	MeshGroup *mg = &mMeshGroups[grp];

	int nv = mg->IBO->GetCount();// g->nVtx; //FIXME
	//int ni = g->nIdx;
	int ret = 0;

	if (grs->nVtx && grs->Vtx) { // vertex data requested
        NTVERTEX *vtx = (NTVERTEX *)mg->VBO->Map();

        if (grs->VtxPerm) { // random access data request
            for (int i = 0; i < grs->nVtx; i++) {
                int vi = grs->VtxPerm[i];
                if (vi < nv) {
                    grs->Vtx[i] = vtx[vi];
                } else {
                    grs->Vtx[i] = zero;
                    ret = 1;
                }
            }
        } else {
            if (grs->nVtx > nv) grs->nVtx = nv;
            memcpy (grs->Vtx, vtx, grs->nVtx * sizeof(NTVERTEX));
        }

        mg->VBO->UnMap();
	}

	if (grs->nIdx && grs->Idx) { // index data requested
    printf("grs->nIdx && grs->Idx\n");
    exit(-1);
    /*
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
		}*/
	}

	//grs->MtrlIdx = g->MtrlIdx;
	grs->TexIdx = mg->mTexIdx;
	return ret;
}

OGLMeshManager::OGLMeshManager ()
{
    OGLMesh::mShader = std::make_unique<Shader>("Mesh.vs","Mesh.fs");
}

OGLMeshManager::~OGLMeshManager ()
{
}

void OGLMeshManager::Flush ()
{
    mMeshMap.clear();
}

void OGLMeshManager::StoreMesh (MESHHANDLE hMesh)
{
    if(mMeshMap.find(hMesh) != mMeshMap.end())
        return;

    mMeshMap.emplace(hMesh, std::make_unique<OGLMesh>(hMesh));
}

OGLMesh *OGLMeshManager::GetMesh (MESHHANDLE hMesh)
{
    const auto &it = mMeshMap.find(hMesh);
    if(it == mMeshMap.end())
        return NULL;
    
    return mMeshMap[hMesh].get();
}
