// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Texture.h
// Texture loading and management routines for the D3D7 client.
//
// Methods for loading single (.dds) and multi-texture files (.tex)
// stored in DXT? format into DIRECTDRAWSURFACE7 instances.
// ==============================================================

#ifndef __TEXTURE_H
#define __TEXTURE_H

#include <stdio.h>

struct OGLTexture
{
	OGLTexture();
	GLuint m_TexId;
	GLuint m_FBO;
	unsigned int m_RefCnt;
	bool m_InCache;
	int m_Width;
	int m_Height;
	bool Release();
	void AddRef() {m_RefCnt++;}
	void SetAsTarget(bool checkFB = true);
	void InitSize();
	uint32_t m_colorkey;
	std::string m_file;
};

// ==============================================================
// Class TextureManager

class TextureManager {
public:
	TextureManager ();
	~TextureManager ();

	bool LoadTexture (const char *fname, OGLTexture **ppdds, uint32_t flags);
	// Read a texture from file 'fname' into the DX7 surface
	// pointed to by 'ppdds'.
	// flags: passed on to ReadTexture

	bool LoadTexture (const char *fname, long ofs, OGLTexture **ppdds, uint32_t flags = 0);
	// Read a single texture from a multi-texture file at offset ofs
	// Return number of loaded textures (1, or 0 on failure)
	// flags: passed on to ReadTexture

	int LoadTextures (const char *fname, OGLTexture **ppdds, uint32_t flags, int n);
	// Read up to n textures from file 'fname' into the DX7 surface array.
	// Return value is the number of actually loaded textures (<=n)

	bool ReadTexture (FILE *file, OGLTexture **ppdds, uint32_t flags);
	// Read a single texture from open file stream 'file' into the
	// DX7 surface pointed to by 'ppdds'.

	bool ReadTextureFromMemory (const uint8_t *buf, uint32_t nbuf, OGLTexture **ppdds, uint32_t flags);
	// Read a single texture from memory buffer 'buf' into the
	// DX7 surface pointed to by 'ppdds'.

	bool GetTexture (const char *fname, OGLTexture **ppdds, uint32_t flags);
	// Retrieve a texture. First scans the repository of loaded textures.
	// If not found, loads the texture from file and adds it to the repository

	OGLTexture *CreateTextureFromTexId(GLuint texId, int width, int height);
	OGLTexture *GetTextureForRendering(int w, int h, int attrib);

protected:
	//HRESULT LookupPixelFormat (DDPIXELFORMAT ddsdDDSTexture,
	//	DDPIXELFORMAT *pddsdBestMatch);
	// given a texture pixel format 'ddsdDDSTexture', return
	// the best match compatible with the current device in
	// 'pddsdBestMatch'. This is faster than the previous
	// routine because it checks for previously registered
	// matches.

	//HRESULT FindBestPixelFormatMatch (DDPIXELFORMAT ddsdDDSTexture,
	//	DDPIXELFORMAT *pddsdBestMatch);
	// given a texture pixel format 'ddsdDDSTexture', return
	// the best match compatible with the current device in
	// 'pddsdBestMatch'

	//bool ReadDDSSurface (FILE *file,
	//	OGLTexture **ppddsDXT, uint32_t flags = 0);
	// Read a compressed DDS surface from an open stream
	// pddsdComp    : DDS surface description
	// pppddsCompTop: DDS surface
	// flags: bit 0 set: force creation in system memory
	//        bit 1 set: decompress, even if format is supported by device
	//        bit 2 set: don't load mipmaps, even if supported by device

	//bool ReadDDSSurfaceFromMemory (const uint8_t *buf, uint32_t nbuf,
	//	OGLTexture ** ppddsDXT, uint32_t flags);
	// Read a compressed DDS surface from a memory buffer
	// pddsdComp    : DDS surface description
	// pppddsCompTop: DDS surface
	// flags: bit 0 set: force creation in system memory
	//        bit 1 set: decompress, even if format is supported by device
	//        bit 2 set: don't load mipmaps, even if supported by device
/*
	bool BltToUncompressedSurface (DDSURFACEDESC2 ddsd, 
		DDPIXELFORMAT ddpf, OGLTexture * pddsDXT, 
	    OGLTexture ** ppddsNewSurface);*/
	// Creates an uncompressed surface and copies the DXT
	// surface into it

	uint32_t MakeTexId (const char *fname);
	// simple checksum of a string. Used for speeding up texture searches.

private:
	bool bMipmap;
/*
	struct PixelFormatPair {
		DDPIXELFORMAT pixelfmt;
		DDPIXELFORMAT bestmatch;
	} *pfp;        // list of pixel formats and best matches
	int npfp;      // number of valid entries in pfp
*/
	// simple repository of loaded textures: linked list
	struct TexRec {
		OGLTexture *tex;
		char fname[64];
		uint32_t id;
		struct TexRec *next;
	} *firstTex;

	// Some repository management functions below.
	// This could be made more sophisticated (defining a maximum size of
	// the repository, deallocating unused textures as required, etc.)
	// Would also require a reference counter and a size parameter in the
	// TexRec structure.

	TexRec *ScanRepository (const char *fname);
	// Return a matching texture entry from the repository, if found.
	// Otherwise, return NULL.

	void AddToRepository (const char *fname, OGLTexture *pdds);
	// Add a new entry to the repository

	void ClearRepository ();
	// De-allocates the repository and release the DX7 textures
};

// ==============================================================
// Non-member utility functions

#endif // !__TEXTURE_H