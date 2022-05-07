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
#include "Shader.h"
#include "Scene.h"
#include <cstring>

using namespace oapi;

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
	}
}

RingManager::RingManager (const VPlanet *vplanet, double inner_rad, double outer_rad)
{
	vp = vplanet;
	irad = inner_rad;
	orad = outer_rad;
	rres = -1;
	tres = 0;
	ntex = 0;
	
	ntex = LoadTextures(tex);
	tres=ntex - 1;
	for (uint32_t res = 0; res < ntex; res++) {
		CreateRing (mesh[res], irad, orad, 8+res*4, tex[res]);
	}
}

RingManager::~RingManager ()
{
	for (uint32_t i = 0; i < ntex; i++)
		tex[i]->Release();
}

void RingManager::SetMeshRes (int res)
{
	if ((uint32_t)res != rres) {
		rres = res;
		tres = std::min (rres, ntex-1);
	}
}

uint32_t RingManager::LoadTextures (OGLTexture **tex)
{
	char fname[256];
	oapiGetObjectName (vp->GetObject(), fname, 256);
	strcat (fname, "_ring.tex");
	return g_client->GetTexMgr()->LoadTextures (fname, tex, 0, MAXRINGRES);
}

bool RingManager::Render (glm::mat4 &mWorld, OGLCamera *c, bool front)
{
	tres=1;
	MATRIX3 grot;
	//static glm::mat4 imat;
	glm::vec3 q(mWorld[0][0], mWorld[1][0], mWorld[2][0]);
	float scale = glm::length(q);
	
	oapiGetRotationMatrix(vp->GetObject(), &grot);
	
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

//	D3DMAT_FromAxisT(&World, &x, &y, &z);

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

static Shader s("Mesh.vs","Mesh.fs");
	s.Bind();


    glm::vec3 sundir = *g_client->GetScene()->GetSunDir();

	auto *vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection",*vp);
	s.SetMat4("u_Model",World);
	s.SetVec3("u_SunDir", sundir);
    s.SetFloat("u_Textured", 1.0);
    s.SetFloat("u_MatAlpha", 1.0);

	s.SetVec4("u_Material.ambient", defmat.ambient);
	s.SetVec4("u_Material.diffuse", defmat.diffuse);
	s.SetVec4("u_Material.specular", defmat.specular);
	s.SetVec4("u_Material.emissive", defmat.emissive);
	s.SetFloat("u_Material.specular_power", defmat.specular_power);

	glBindTexture(GL_TEXTURE_2D, mesh[tres].texture->m_TexId);
	CheckError("RingManager::Render glBindTexture");
	mesh[tres].VAO->Bind();
	glDrawElements(GL_TRIANGLES, mesh[tres].IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
	CheckError("RingManager::Render glDrawElements");
	mesh[tres].VAO->UnBind();
	glBindTexture(GL_TEXTURE_2D,  0);
	CheckError("RingManager::Render glBindTexture0");
	s.UnBind();
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

void RingManager::CreateRing (RINGMESH &mesh, double irad, double orad, int nsect, OGLTexture *tex)
{
	uint32_t i, j;
	uint32_t count = nsect/2 + 1;

	uint32_t nVtx = 2*count;
	uint32_t nIdx = 6*(count - 1);
	uint16_t *Idx = new uint16_t[nIdx+12];
	NTVERTEX *Vtx = new NTVERTEX[nVtx+4];
	 
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

		if (j<=nIdx-6) {
			Idx[j++] = i*2;
			Idx[j++] = i*2+1;
			Idx[j++] = i*2+2;
			Idx[j++] = i*2+3;
			Idx[j++] = i*2+2;
			Idx[j++] = i*2+1;
		}
	}
	mesh.VAO = std::make_unique<VertexArray>();
	mesh.VAO->Bind();
	mesh.VBO = std::make_unique<VertexBuffer>(Vtx, (nVtx+4)*sizeof(NTVERTEX));
	mesh.VBO->Bind();
	mesh.texture = tex;

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

	mesh.IBO = std::make_unique<IndexBuffer>(Idx, nIdx +12);
	mesh.IBO->Bind();

	mesh.VAO->UnBind();

	delete []Vtx;
	delete []Idx;
}
