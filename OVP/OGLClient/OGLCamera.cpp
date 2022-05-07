// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Camera.cpp
// Class Camera (implementation)
//
// The camera defines the observer position in the 3D world.
// Each scene consists of a camera and a collection of vObjects
// within visual range around it.
// The "render space" (i.e. the coordinate system in which the
// camera and visual objects live) is translated against the
// "global frame" in which orbiter's logical objects live, such
// that the camera is always at the origin. Global and render
// space have however the same orientation.
// ==============================================================

#include "OGLCamera.h"
#include "OrbiterAPI.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "OGLClient.h"

OGLCamera::OGLCamera (uint32_t w, uint32_t h)
{
	width = w, height = h;
	aspect = (double)w/(double)h;
	SetAperture (RAD*50.0);

	mView = glm::fmat4(1.0f);

	SetFrustumLimits (2.5f, 5e8f); // initial limits
//	SetFrustumLimits (2.5f, 5e9f); // initial limits
}

void OGLCamera::SetSize(uint32_t w, uint32_t h)
{
	width = w, height = h;
	aspect = (double)w/(double)h;
	UpdateProjectionMatrix();
}

void OGLCamera::UpdateProjectionMatrix()
{
	mProj = glm::perspective (fov, aspect, nearplane, farplane);

	//Convert from directx to opengl reference
//	glm::mat4 conv = glm::translate(glm::scale(glm::mat4(1.0f),glm::vec3(0.5f,0.5f,-1.0f)), glm::vec3(1.0f,1.0f,0.0f));
	glm::mat4 conv = glm::translate(glm::scale(glm::mat4(1.0f),glm::vec3(0.5f,0.5f,-1.0f)), glm::vec3(0.0f,0.0f,0.0f));
	mProj = mProj * conv;
	mViewProj = mProj * mView;
}

void OGLCamera::SetFrustumLimits (double nearlimit, double farlimit)
{
	nearplane = (float)nearlimit;
	farplane = (float)farlimit;
	UpdateProjectionMatrix ();
}

void OGLCamera::SetAperture (double _ap)
{
	fov = _ap; // FIXME: FOV == AP??
	tanap = tan(_ap);
	UpdateProjectionMatrix ();
}

void OGLCamera::Update ()
{
	if (fov != oapiCameraAperture())
		SetAperture (oapiCameraAperture());

	VECTOR3 vec;
	oapiCameraGlobalPos (&vec);
	gpos.x = vec.x;
	gpos.y = vec.y;
	gpos.z = vec.z;

	oapiCameraGlobalDir (&vec);
	gdir.x = vec.x;
	gdir.y = vec.y;
	gdir.z = vec.z;
	MATRIX3 mat;
	oapiCameraRotationMatrix (&mat);
	grot = glm::make_mat3((double *)&mat);
	mView = glm::mat4(grot);
	mViewProj = mProj * mView;
}

bool OGLCamera::Direction2Viewport(const VECTOR3 &dir, int &x, int &y)
{
	glm::vec4 homog;
	glm::vec4 idir = {-dir.x, -dir.y, -dir.z, 1.0};
	homog = mViewProj * idir;

	if (homog.x >= -1.0f && homog.y <= 1.0f && homog.z >= 0.0) {
		if (std::hypot(homog.x, homog.y) < 1e-6) {
			x = width/2, y = height/2;
		} else {
			x = (int)(width*0.5f*(1.0f+homog.x));
			y = (int)(height*0.5f*(1.0f-homog.y));
		}
		return true;
	} else
		return false;
}
