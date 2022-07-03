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
#include "OGLMesh.h"

class OGLTexture;
class Scene;

class vStar: public vObject {
public:
	vStar (OBJHANDLE _hObj, const Scene *scene);
	~vStar ();

	static void GlobalInit ();
	static void GlobalExit ();

	bool Update () override;
	bool Render () override;

private:
	double size;       ///< physical size of central body
	double maxdist;    ///< max render distance

	static OGLMesh *billboard;         ///< visual
	static OGLTexture *deftex; ///< default texture
};

#endif // !__VSTAR_H