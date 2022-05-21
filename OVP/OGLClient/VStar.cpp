// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VStar.cpp
// class vStar (interface)
//
// Renders the central star as a billboard mesh.
// ==============================================================

#include "MeshManager.h"
#include "Texture.h"
#include "VStar.h"
#include "Scene.h"
#include "OGLMesh.h"
#include <cstring>

OGLMesh *VStar::billboard = 0;
OGLTexture *VStar::deftex = 0;

VStar::VStar (OBJHANDLE _hObj): VObject (_hObj)
{
	size = oapiGetSize (_hObj);
	maxdist = 0.1*g_client->GetScene()->GetCamera()->GetFarlimit();

}

VStar::~VStar ()
{
}

void VStar::GlobalInit ()
{
	// create billboard mesh for sprite representation
	billboard = new OGLMesh ();
	static uint16_t idx[6] = {0,1,2, 3,2,1};
	MESHGROUPEX mg;
	memset (&mg, 0, sizeof(MESHGROUPEX));
	mg.MtrlIdx = 0;
	mg.TexIdx = 0;
	mg.nVtx = 4;
	mg.nIdx = 6;
	mg.Idx = idx;
	mg.Vtx = new NTVERTEX[mg.nVtx];
	int i;
	for (i = 0; i < mg.nVtx; i++) {
		mg.Vtx[i].x = 0;
		mg.Vtx[i].y = (i%2 ? 1.0f:-1.0f);
		mg.Vtx[i].z = (i<2 ? 1.0f:-1.0f);
		mg.Vtx[i].nx = -1.0f;
		mg.Vtx[i].ny = 0;
		mg.Vtx[i].nz = 0;
		mg.Vtx[i].tu = (i<2 ? 0:1.0f);
		mg.Vtx[i].tv = (i%2 ? 1.0f:0);
	}
	billboard->AddGroup (&mg);
	delete []mg.Vtx;

	// load the default texture
	deftex = g_client->GetTexMgr()->LoadTexture ("star.dds", true, 0);
	billboard->SetTexture(1, deftex);


	// set material definition
	OGLMaterial mtrl;
    memset (&mtrl, 0, sizeof(OGLMaterial));
    mtrl.emissive.r = 1.0f;
    mtrl.emissive.g = 1.0f;
    mtrl.emissive.b = 1.0f;

	*billboard->GetMaterial(0) = mtrl;
}

void VStar::GlobalExit ()
{
	if (billboard) delete billboard;
	if (deftex)    deftex->Release();
}

bool VStar::Update ()
{
//	if (!mVisible) return false;
	VObject::Update ();

	float dist_scale;
	float rad_scale = (float)size;
	float size_hack; // make star look bigger at distance
	VECTOR3 bdir (unit(cpos));
	double hz = std::hypot (bdir.x, bdir.z);
	//double phi = atan2 (bdir.z, bdir.x);
	//float sphi = (float)sin(phi), cphi = (float)cos(phi);
	//float tx = (float)cpos.x, ty = (float)cpos.y, tz = (float)cpos.z;
	/*
	mWorld._11 =  (float)bdir.x; //cphi;
	mWorld._12 =  (float)bdir.y; //0;
	mWorld._13 =  (float)bdir.z; //sphi;
	mWorld._31 = -(float)(bdir.z/hz); //-sphi;
	mWorld._32 =  0;
	mWorld._33 =  (float)(bdir.x/hz); //cphi;
	mWorld._21 =  -(mWorld._12*mWorld._33 - mWorld._32*mWorld._13); //0;
	mWorld._22 =  -(mWorld._13*mWorld._31 - mWorld._33*mWorld._11); //1;
	mWorld._23 =  -(mWorld._11*mWorld._32 - mWorld._31*mWorld._12); // 0;
	mWorld._41 =  (float)cpos.x;
	mWorld._42 =  (float)cpos.y;
	mWorld._43 =  (float)cpos.z;
*/
	mModel[0][0] =  (float)bdir.x; //cphi;
	mModel[0][1] =  (float)bdir.y; //0;
	mModel[0][2] =  (float)bdir.z; //sphi;
	mModel[2][0] = -(float)(bdir.z/hz); //-sphi;
	mModel[2][1] =  0;
	mModel[2][2] =  (float)(bdir.x/hz); //cphi;
	mModel[1][0] =  -(mModel[0][1]*mModel[2][2] - mModel[2][1]*mModel[0][2]); //0;
	mModel[1][1] =  -(mModel[0][2]*mModel[2][0] - mModel[2][2]*mModel[0][0]); //1;
	mModel[1][2] =  -(mModel[0][0]*mModel[2][1] - mModel[2][0]*mModel[0][1]); // 0;
	mModel[3][0] =  (float)cpos.x;
	mModel[3][1] =  (float)cpos.y;
	mModel[3][2] =  (float)cpos.z;


	// artificially reduce size reduction with distance
	// to make star appear larger
	//size_hack = (float)(1.0+2e-8*pow(cdist,0.7));
	size_hack = 7.5;
	rad_scale *= size_hack;

	if (cdist > maxdist) {
		dist_scale = (float)(maxdist/cdist);
		rad_scale *= dist_scale;
		mModel[3][0] *= dist_scale;
		mModel[3][1] *= dist_scale;
		mModel[3][2] *= dist_scale;
	}

	// scale up sphere radius from 1 to planet radius
	mModel[0][0] *= rad_scale; mModel[0][1] *= rad_scale; mModel[0][2] *= rad_scale;
	mModel[1][0] *= rad_scale; mModel[1][1] *= rad_scale; mModel[1][2] *= rad_scale;
	mModel[2][0] *= rad_scale; mModel[2][1] *= rad_scale; mModel[2][2] *= rad_scale;

	return true;
}

bool VStar::Render (OGLCamera *c)
{
	// Render states expected on entry:
	//    D3DRENDERSTATE_ZENABLE=FALSE
	//    D3DRENDERSTATE_ZWRITEENABLE=FALSE
	//    D3DRENDERSTATE_ALPHABLENDENABLE=TRUE
	// Render states modified on exit:
	//    D3DTRANSFORMSTATE_WORLD
	//    Texture

//	D3DMATERIAL7 pmtrl;

//	dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &mWorld);
//	dev->SetTexture (0, deftex);
//	dev->GetMaterial (&pmtrl);
//	dev->SetMaterial (&mtrl);

	billboard->Render (c, mModel);
//	dev->SetMaterial (&pmtrl);

	return true;
}
