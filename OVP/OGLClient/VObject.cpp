#include "VObject.h"
#include "VVessel.h"
#include "VPlanet.h"
#include "VStar.h"
#include "OGLClient.h"
#include "Scene.h"
#include "VBase.h"
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
