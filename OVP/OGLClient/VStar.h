// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// VStar.h
// class vStar (interface)
//
// Renders the central star as a billboard mesh.
// ==============================================================

#ifndef __VSTAR_H
#define __VSTAR_H

#include "VObject.h"

class OGLMesh;
class OGLTexture;
class OGLCamera;
class Scene;

class VStar: public VObject {
public:
	VStar (OBJHANDLE _hObj);
	~VStar ();

	static void GlobalInit ();
	static void GlobalExit ();

	bool Update () override;
	bool Render (OGLCamera *c) override;

private:
	double size;       ///< physical size of central body
	double maxdist;    ///< max render distance

	static OGLMesh *billboard;         ///< visual
	static OGLTexture *deftex; ///< default texture
};

#endif // !__VSTAR_H