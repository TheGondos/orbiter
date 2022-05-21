// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   OGL Client module
// ==============================================================

// ==============================================================
// MeshMgr.cpp
// class OGLMeshManager (implementation)
//
// Simple management of persistent mesh templates
// ==============================================================
#include "glad.h"
#include "MeshManager.h"
#include "OGLMesh.h"
#include <cstring>

using namespace oapi;

OGLMeshManager::OGLMeshManager ()
{
	nmlist = nmlistbuf = 0;
}

OGLMeshManager::~OGLMeshManager ()
{
	Flush();
}

void OGLMeshManager::Flush ()
{
	int i;
	for (i = 0; i < nmlist; i++)
		delete mlist[i].mesh;
	if (nmlistbuf) {
		delete []mlist;
		nmlist = nmlistbuf = 0;
	}
}

void OGLMeshManager::StoreMesh (MESHHANDLE hMesh)
{
	if (GetMesh (hMesh)) return; // mesh already stored

	if (nmlist == nmlistbuf) { // need to allocate buffer
		MeshBuffer *tmp = new MeshBuffer[nmlistbuf += 32];
		if (nmlist) {
			memcpy (tmp, mlist, nmlist*sizeof(MeshBuffer));
			delete []mlist;
		}
		mlist = tmp;
	}
	mlist[nmlist].hMesh = hMesh;
	mlist[nmlist].mesh = new OGLMesh (hMesh, true);
	nmlist++;
}

const OGLMesh *OGLMeshManager::GetMesh (MESHHANDLE hMesh)
{
	int i;
	for (i = 0; i < nmlist; i++)
		if (mlist[i].hMesh == hMesh) return mlist[i].mesh;

	return NULL;
}
