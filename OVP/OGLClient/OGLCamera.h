// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// --------------------------------------------------------------
// Camera.h
// Class Camera (interface)
//
// The camera defines the observer position in the 3D world.
// Each scene consists of a camera and a collection of vObjects
// within visual range around it.
// The "render space" (i.e. the coordinate system in which the
// camera and visual objects live) is translated against the
// "global frame" in which orbiter's logical objects live, such
// that the camera is always at the origin. Global and render
// space have however the same orientation.
// --------------------------------------------------------------

#ifndef __OGLCAMERA_H
#define __OGLCAMERA_H

#include "OrbiterAPI.h"
#include "OGLClient.h"
#include <glm/glm.hpp>

class OGLCamera {
public:
	OGLCamera (uint32_t width, uint32_t height);
	void Update ();

	const glm::fmat4 *GetViewProjectionMatrix () const { return &mViewProj; }
	inline const glm::fmat4 *GetViewMatrix () const { return &mView; }
	inline const glm::fmat4 *GetProjectionMatrix () const { return &mProj; }

	void SetAperture (double _ap);
	void SetSize(uint32_t w, uint32_t h);
	inline const VECTOR3 *GetGDir () const { return &gdir; }
	inline const VECTOR3 *GetGPos () const { return &gpos; }
	inline const glm::dmat3 *GetGRot () const { return &grot; }
	float GetFarlimit() { return farplane; }
	float GetNearlimit() { return nearplane; }
	uint32_t GetHeight() const { return height; }
	uint32_t GetWidth() const { return width; }
	float GetTanAp() const { return tanap; }
	bool Direction2Viewport(const VECTOR3 &dir, int &x, int &y);
	inline OBJHANDLE GetProxyBody () const { return hObj_proxy; }

private:
	void UpdateProjectionMatrix();
	uint32_t width, height;

	// camera status parameters from Orbiter API
	VECTOR3 gpos;           // current camera position (global frame)
	VECTOR3 gdir;           // current camera direction (global frame)
	glm::dmat3 grot;           // current camera rotation matrix (global frame)

	// camera elements
	glm::fmat4 mView;        // view matrix for current camera state
	glm::fmat4 mProj;        // projection matrix for current camera state
	glm::fmat4 mViewProj;  // product of view matrix and projection

	float aspect;
	float nearplane;        // fustrum nearplane distance
	float farplane;         // fustrum farplane distance
	float fov;
	float tanap;

	OBJHANDLE hObj_proxy;   // closest celestial body
	double alt_proxy;       // camera distance to surface of hObj_proxy
};

#endif // !__OGLCAMERA_H
