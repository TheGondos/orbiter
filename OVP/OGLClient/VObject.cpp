#include "glad.h"
#include "VObject.h"
#include "VVessel.h"
#include "VPlanet.h"
#include "VStar.h"
#include "OGLClient.h"
#include "Scene.h"
#include "VBase.h"
#include "OGLCamera.h"
#include "Renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

OGLTexture *vObject::blobtex[3] = {0,0,0};

vObject::vObject (OBJHANDLE _hObj, const Scene *scene): VisObject (_hObj)
{
	active = true;
	hObj = _hObj;
	scn  = scene;
	size = oapiGetSize(hObj);
	cdist = 0.0;
	dmWorld = identity4();
    mWorld = glm::fmat4(1.0f);
}

vObject *vObject::Create (OBJHANDLE handle, const Scene *scene)
{
	switch (oapiGetObjectType (handle)) {
	case OBJTP_VESSEL:
		return new vVessel (handle, scene);
	case OBJTP_PLANET:
		return new vPlanet (handle, scene);
	case OBJTP_STAR:
		return new vStar (handle, scene);
	case OBJTP_SURFBASE:
		printf("xxx");exit(-1);
		return new vBase (handle, scene);
	default:
		return new vObject (handle, scene);
	}
}

void vObject::GlobalInit ()
{
	for (int i = 0; i < 3; i++) {
		static const char *fname[3] = {"Ball.dds","Ball2.dds","Ball3.dds"};
		g_client->GetTexMgr()->LoadTexture (fname[i], blobtex+i, 0);
	}
	spotShader = Renderer::GetShader("Spot");
	
	struct TVERTEX {
		float x;     ///< vertex x position
		float y;     ///< vertex y position
		float z;     ///< vertex z position
		float tu;    ///< vertex u texture coordinate
		float tv;    ///< vertex v texture coordinate
	};

	uint16_t idx[6] = {0,1,2, 3,2,1};
	TVERTEX vtx[4] = {
		{0,-1, 1,  0,0},
		{0, 1, 1,  0,1},
		{0,-1,-1,  1,0},
		{0, 1,-1,  1,1}
	};

	spotVBA = new VertexArray();
	spotVBA->Bind();
	spotVBO = new VertexBuffer(vtx, 4*sizeof(TVERTEX));
	spotVBO->Bind();
	//position
	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(TVERTEX),    // stride
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
	sizeof(TVERTEX),    // stride
	(void*)12           // array buffer offset
	);
	Renderer::CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(1);
	Renderer::CheckError("glEnableVertexAttribArray1");

	spotIBO = new IndexBuffer(idx, 6);
	spotIBO->Bind();
	spotVBA->UnBind();

}

void vObject::GlobalExit ()
{
	for (int i = 0; i < 3; i++) {
		if (blobtex[i]) blobtex[i]->Release();
	}
}

void vObject::Activate (bool isactive)
{
	active = isactive;
}


bool vObject::Update ()
{
	if (!active) return false;

	oapiGetGlobalPos (hObj, &cpos);
    VECTOR3 vec;
	oapiCameraGlobalPos (&vec);
	cpos -= vec;

	cdist = length (cpos);

	MATRIX3 grot;
	oapiGetRotationMatrix (hObj, &grot);

    glm::fvec3 v;
    v.x = cpos.x;
    v.y = cpos.y;
    v.z = cpos.z;

	mWorld=glm::mat4(1.0f);
	mWorld[0][0] = (float)grot.m11;
	mWorld[0][1] = (float)grot.m21;
	mWorld[0][2] = (float)grot.m31;
	mWorld[1][0] = (float)grot.m12;
	mWorld[1][1] = (float)grot.m22;
	mWorld[1][2] = (float)grot.m32;
	mWorld[2][0] = (float)grot.m13;
	mWorld[2][1] = (float)grot.m23;
	mWorld[2][2] = (float)grot.m33;

	mWorld[3][0] = (float)cpos.x;
	mWorld[3][1] = (float)cpos.y;
	mWorld[3][2] = (float)cpos.z;

	dmWorld = _M(grot.m11, grot.m21, grot.m31, 0,
		         grot.m12, grot.m22, grot.m32, 0,
				 grot.m13, grot.m23, grot.m33, 0,
				 cpos.x,   cpos.y,   cpos.z,   1);

	CheckResolution();

	return true;
}

void vObject::RenderSpot (const VECTOR3 *ofs, float size, const VECTOR3 &col, bool lighting, int shape)
{
	static glm::fmat4 W = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
	MATRIX3 grot;
	oapiGetRotationMatrix (hObj, &grot);
	VECTOR3 gpos;
	oapiGetGlobalPos (hObj, &gpos);
	VECTOR3 pos (cpos);
	if (ofs) pos += mul (grot, *ofs);
	double dist = length (pos);
	VECTOR3 bdir (pos/dist);
	const VECTOR3 &camp = *scn->GetCamera()->GetGPos();
	double hz = std::hypot (bdir.x, bdir.z);
	double phi = atan2 (bdir.z, bdir.x);
	float sphi = (float)sin(phi), cphi = (float)cos(phi);

	const double ambient = 0.2;
	double cosa = dotp (unit(gpos), unit(gpos - camp));
	double intens = (lighting ? 0.5 * ((1.0-ambient)*cosa + 1.0+ambient) : 1.0);

	W[0][0] =  (float)bdir.x;
	W[0][1] =  (float)bdir.y;
	W[0][2] =  (float)bdir.z;
	W[2][0] =  (-(float)(bdir.z/hz));
	/*W._32 =  0;*/
	W[2][2] =  (float)(bdir.x/hz);
	W[1][0] =  (/*W._32*W._13*/ - W[0][1]*W[2][2]);
	W[1][1] =  (W[2][2]*W[0][0] - W[0][2]*W[2][0]);
	W[1][2] =  (W[2][0]*W[0][1] /*- W._11*W._32*/);
	W[3][0] =  (float)pos.x;
	W[3][1] =  (float)pos.y;
	W[3][2] =  (float)pos.z;

	W[0][0] *= size; W[0][1] *= size; W[0][2] *= size;
	W[1][0] *= size; W[1][1] *= size; W[1][2] *= size;
	W[2][0] *= size; /*W._32 *= size;*/ W[2][2] *= size;

	Renderer::Bind(spotShader);

	Renderer::PushBool(Renderer::DEPTH_TEST, true);
	//Renderer::PushBool(Renderer::CULL_FACE, false);

	spotShader->SetMat4("u_ViewProjection", *scn->GetCamera()->GetViewProjectionMatrix());
	spotShader->SetMat4("u_Model", W);

//	dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &W);
	glBindTexture (GL_TEXTURE_2D, blobtex[shape]->m_TexId);
//	dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGB(col.x*intens, col.y*intens, col.z*intens));
	glm::vec3 spotcolor{col.x*intens, col.y*intens, col.z*intens};
	spotShader->SetVec3("u_SpotColor", spotcolor);

	spotVBA->Bind();
	Renderer::CheckError("spotVBA->Bind");
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	Renderer::CheckError("glDrawElements");
	spotVBA->UnBind();
	Renderer::CheckError("spotVBA->UnBind");

	Renderer::PopBool(1);
//	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
//	dev->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, D3DFVF_VERTEX, vtx, 4, idx, 6, 0);
//	dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
//	dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Renderer::Unbind(spotShader);
}
