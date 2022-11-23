// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// RingMgr.cpp
// class RingManager (implementation)
// ==============================================================

#include "glad.h"
#include "RingMgr.h"
#include "Texture.h"
#include "OGLCamera.h"
#include "Renderer.h"
#include <cstring>

using namespace oapi;

RingManager::RingManager (const vPlanet *vplanet, double inner_rad, double outer_rad)
{
	vp = vplanet;
	irad = inner_rad;
	orad = outer_rad;
	rres = (unsigned int)-1;
	tres = 0;
	ntex = 0;
	for (unsigned int i = 0; i < MAXRINGRES; i++) {
		mesh[i] = 0;
		tex[i] = 0;
	}
}

RingManager::~RingManager ()
{
	unsigned int i;
	for (i = 0; i < 3; i++)
		if (mesh[i]) delete mesh[i];
	for (i = 0; i < ntex; i++)
		tex[i]->Release();
}

void RingManager::GlobalInit ()
{
	meshShader = Renderer::GetShader("Mesh");
}

void RingManager::SetMeshRes (unsigned int res)
{
	if (res != rres) {
		rres = res;
		if (!mesh[res])
			mesh[res] = CreateRing (irad, orad, 8+res*4);
		if (!ntex)
			ntex = LoadTextures();
		tres = std::min (rres, ntex-1);
	}
}

unsigned int RingManager::LoadTextures ()
{
	char fname[256];
	oapiGetObjectName (vp->Object(), fname, 256);
	strcat (fname, "_ring.tex");
	return g_client->GetTexMgr()->LoadTextures (fname, tex, 0, MAXRINGRES);
}

bool RingManager::Render (OGLCamera *c, glm::mat4 &mWorld, bool front)
{
	MATRIX3 grot;
	glm::vec3 q(mWorld[0][0], mWorld[1][0], mWorld[2][0]);
	float scale = glm::length(q);
	
	oapiGetRotationMatrix(vp->Object(), &grot);
	
	VECTOR3 gdir; oapiCameraGlobalDir(&gdir);

	VECTOR3 yaxis =  mul(grot, _V(0,1,0));
	VECTOR3 xaxis = unit(crossp(gdir, yaxis));
	VECTOR3 zaxis = unit(crossp(xaxis, yaxis));

	if (!front) {
		xaxis = -xaxis;
		zaxis = -zaxis;
	}

	glm::vec3 x(float(xaxis.x), float(xaxis.y), float(xaxis.z));
	glm::vec3 y(float(yaxis.x), float(yaxis.y), float(yaxis.z));
	glm::vec3 z(float(zaxis.x), float(zaxis.y), float(zaxis.z));

	glm::mat4 World = mWorld;

	x*=scale; y*=scale;	z*=scale;

	World[0][0] = x.x;
	World[0][1] = x.y;
	World[0][2] = x.z;

	World[1][0] = y.x;
	World[1][1] = y.y;
	World[1][2] = y.z;

	World[2][0] = z.x;
	World[2][1] = z.y;
	World[2][2] = z.z;

	static OGLMaterial defmat = {
		{0,0,0,1},
		{0,0,0,1},
		{0,0,0,1},
		{0.5,0.5,0.5,1},0
	};

	meshShader->Bind();

	const VECTOR3 &sd = g_client->GetScene()->GetSunDir();
	glm::vec3 sundir;
	sundir.x = sd.x;
	sundir.y = sd.y;
	sundir.z = sd.z;

	auto *vp = c->GetViewProjectionMatrix();
	meshShader->SetMat4("u_ViewProjection",*vp);
	meshShader->SetMat4("u_Model",World);
	meshShader->SetVec3("u_SunDir", sundir);
    meshShader->SetFloat("u_Textured", 1.0);
    meshShader->SetFloat("u_MatAlpha", 1.0);
	meshShader->SetFloat("u_ModulateAlpha", 0.0);

	meshShader->SetVec4("u_Material.ambient", defmat.ambient);
	meshShader->SetVec4("u_Material.diffuse", defmat.diffuse);
	meshShader->SetVec4("u_Material.specular", defmat.specular);
	meshShader->SetVec4("u_Material.emissive", defmat.emissive);
	meshShader->SetFloat("u_Material.specular_power", defmat.specular_power);

	glBindTexture(GL_TEXTURE_2D, tex[tres]->m_TexId);
    
	Renderer::PushBool(Renderer::BLEND, true);

	mesh[rres]->RenderGroup (mesh[rres]->GetGroup(0));

	Renderer::PopBool();

	meshShader->UnBind();
	return true;
}

// =======================================================================
// CreateRing
// Creates mesh for rendering planetary ring system. Creates a ring
// with nsect quadrilaterals. Smoothing the corners of the mesh is
// left to texture transparency. Nsect should be an even number.
// Disc is in xz-plane centered at origin facing up. Size is such that
// a ring of inner radius irad (>=1) and outer radius orad (>irad)
// can be rendered on it.

OGLMesh *RingManager::CreateRing (double irad, double orad, int nsect)
{
	int i, j;
	uint32_t count = nsect/2 + 1;
	OGLMesh::GROUPREC *grp = new OGLMesh::GROUPREC;
	grp->nVtx = 2*count;
	grp->nIdx = 6*(count - 1);
	grp->Idx = new uint16_t[grp->nIdx+12];

	NTVERTEX *Vtx;
	Vtx = grp->Vtx = new NTVERTEX[grp->nVtx+4];
	uint16_t *Idx = grp->Idx;

	double alpha = PI/(double)nsect;
	float nrad = (float)(orad/cos(alpha)); // distance for outer nodes
	float ir = (float)irad;
	float fo = (float)(0.5*(1.0-orad/nrad));
	float fi = (float)(0.5*(1.0-irad/nrad));

	for (i = j = 0; i < count + 2; i++) {
		double phi = i*2.0*alpha;
		float cosp = (float)cos(phi), sinp = (float)sin(phi);
		Vtx[i*2].x = nrad*cosp;  Vtx[i*2+1].x = ir*cosp;
		Vtx[i*2].z = nrad*sinp;  Vtx[i*2+1].z = ir*sinp;
		Vtx[i*2].y = Vtx[i*2+1].y = 0.0;
		Vtx[i*2].nx = Vtx[i*2+1].nx = Vtx[i*2].nz = Vtx[i*2+1].nz = 0.0;
		Vtx[i*2].ny = Vtx[i*2+1].ny = 1.0;
		if (!(i&1)) Vtx[i*2].tu = fo,  Vtx[i*2+1].tu = fi;  //fac;
		else        Vtx[i*2].tu = 1.0f-fo,  Vtx[i*2+1].tu = 1.0f-fi; //1.0f-fac;
		Vtx[i*2].tv = 0.0f, Vtx[i*2+1].tv = 1.0f;

		if (j<=grp->nIdx-6) {
			Idx[j++] = i*2;
			Idx[j++] = i*2+1;
			Idx[j++] = i*2+2;
			Idx[j++] = i*2+3;
			Idx[j++] = i*2+2;
			Idx[j++] = i*2+1;
		}
	}

    grp->VBA = std::make_unique<VertexArray>();
    grp->VBA->Bind();
    grp->VBO = std::make_unique<VertexBuffer>(grp->Vtx, (grp->nVtx+4) * sizeof(NTVERTEX));
    grp->VBO->Bind();

    glVertexAttribPointer(
    0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
    (void*)0            // array buffer offset
    );
    Renderer::CheckError("glVertexAttribPointer0");
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
    1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
    (void*)12            // array buffer offset
    );
    Renderer::CheckError("glVertexAttribPointer");
    glEnableVertexAttribArray(1);
    Renderer::CheckError("glEnableVertexAttribArray1");

    glVertexAttribPointer(
    2,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    2,                  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    sizeof(NTVERTEX),                  // stride
    (void*)24            // array buffer offset
    );
    Renderer::CheckError("glVertexAttribPointer");
    glEnableVertexAttribArray(2);
    Renderer::CheckError("glEnableVertexAttribArray2");

    grp->IBO = std::make_unique<IndexBuffer>(grp->Idx, grp->nIdx + 12);
    grp->IBO->Bind();
    grp->VBA->UnBind();


	return new OGLMesh (grp, false);
}
