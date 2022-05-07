#include "VObject.h"
#include "VVessel.h"
#include "VPlanet.h"
#include "VStar.h"
#include "OGLClient.h"
#include "Scene.h"
#include "VBase.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

VObject::VObject (OBJHANDLE handle)
{
	mVisible = true;
	mHandle = handle;
	mSize = oapiGetSize(handle);
    mModel = glm::fmat4(1.0f);
}

VObject *VObject::Create (OBJHANDLE handle)
{
	switch (oapiGetObjectType (handle)) {
	case OBJTP_VESSEL:
		return new VVessel (handle);
	case OBJTP_PLANET:
		return new VPlanet (handle);
	case OBJTP_STAR:
		return new VStar (handle);
	case OBJTP_SURFBASE:
	printf("xxx");exit(-1);
		return new VBase (handle);
	default:
		return new VObject (handle);
	}
}


bool VObject::Update ()
{
	oapiGetGlobalPos (mHandle, &cpos);
    VECTOR3 vec;
	oapiCameraGlobalPos (&vec);
	cpos -= vec;

	cdist = length (cpos);

    mVisible = true;
	MATRIX3 grot;
	oapiGetRotationMatrix (mHandle, &grot);

    glm::fvec3 v;
    v.x = cpos.x;
    v.y = cpos.y;
    v.z = cpos.z;

	mModel=glm::mat4(1.0f);
	mModel[0][0] = (float)grot.m11;
	mModel[0][1] = (float)grot.m21;
	mModel[0][2] = (float)grot.m31;
	mModel[1][0] = (float)grot.m12;
	mModel[1][1] = (float)grot.m22;
	mModel[1][2] = (float)grot.m32;
	mModel[2][0] = (float)grot.m13;
	mModel[2][1] = (float)grot.m23;
	mModel[2][2] = (float)grot.m33;

	mModel[3][0] = (float)cpos.x;
	mModel[3][1] = (float)cpos.y;
	mModel[3][2] = (float)cpos.z;

	dmWorld = _M(grot.m11, grot.m21, grot.m31, 0,
		         grot.m12, grot.m22, grot.m32, 0,
				 grot.m13, grot.m23, grot.m33, 0,
				 cpos.x,   cpos.y,   cpos.z,   1);

	CheckResolution();

	return true;
}
