// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// Classes to support meshes under D3D:
// Triangle, Mesh
// =======================================================================

#ifndef __MESH_H
#define __MESH_H

#include <iostream>

typedef char Str256[256];

// =======================================================================
// Class Mesh

class Mesh {
public:
	Mesh ();
	// Create an empty mesh

	~Mesh ();

	inline int nGroup() const { return nGrp; }
	// Number of groups

	inline int nMaterial() const { return nMtrl; }
	// Number of materials

	inline int nTexture() const { return nTex; }
	// Number of textures

	friend std::istream &operator>> (std::istream &is, Mesh &mesh);
	// read mesh from file

private:
	int nGrp;         // number of groups
	int nMtrl;        // number of materials
	int nTex;                // number of textures
};

#endif // !__MESH_H
