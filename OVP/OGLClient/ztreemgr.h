// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// --------------------------------------------------------------
// ztreemgr.h
// Class ZTreeMgr (interface)
//
// Manage compressed and packed tile trees for planetary surface
// and cloud layers.
// --------------------------------------------------------------

#ifndef __ZTREEMGR_H
#define __ZTREEMGR_H

#include <iostream>
#include <cstdint>

// =======================================================================
// Tree node structure

struct TreeNode {
	int64_t pos;  // file position of node data
	int32_t size;   // data block size [bytes]
	int32_t child[4]; // array index positions of the children (-1=no child)
	int32_t padding;
	TreeNode() {
		pos = 0;
		size = 0;
		for (int i = 0; i < 4; i++) child[i] = -1;
	}
};

// =======================================================================
// File header for compressed tree files

class TreeFileHeader {
	friend class ZTreeMgr;

public:
	TreeFileHeader();
	size_t fwrite(FILE *f);
	bool fread(FILE *f);

private:
	uint8_t magic[4];      // file ID and version
	int32_t size;         // header size [bytes]
	int32_t flags;        // bit flags
	int32_t dataOfs;      // file offset of start of data block (header + TOC)
	int64_t dataLength; // total length of compressed data block
	int32_t nodeCount;    // total number of tree nodes
	int32_t rootPos1;     // index of level-1 tile (-1 for not present)
	int32_t rootPos2;     // index of level-2 tile (-1 for not present)
	int32_t rootPos3;     // index of level-3 tile (-1 for not present)
	int32_t rootPos4[2];  // index of the level-4 tiles (quadtree roots; -1 for not present)
};

// =======================================================================
// Tree table of contents

class TreeTOC {
	friend class ZTreeMgr;

public:
	TreeTOC();
	~TreeTOC();
	size_t fread(int size, FILE *f);
	int size() const { return ntree; }
	const TreeNode &operator[](int idx) const { return tree[idx]; }

	inline int NodeSizeDeflated(int idx) const
	{ return (int)((idx < ntree-1 ? tree[idx+1].pos : totlength) - tree[idx].pos); }

	inline int NodeSizeInflated(int idx) const
	{ return tree[idx].size; }

private:
	TreeNode *tree;    // array containing all tree node entries
	int ntree;       // number of entries
	int ntreebuf;    // array size
	int64_t totlength; // total data size (deflated)
};

// =======================================================================
// ZTreeMgr class: manage a single layer tree for a planet

class ZTreeMgr {
public:
	enum Layer { LAYER_SURF, LAYER_MASK, LAYER_ELEV, LAYER_ELEVMOD, LAYER_LABEL, LAYER_CLOUD };
	static ZTreeMgr *CreateFromFile(const char *PlanetPath, Layer _layer);
	ZTreeMgr(const char *PlanetPath, Layer _layer);
	~ZTreeMgr();
	const TreeTOC &TOC() const { return toc; }

	int Idx(int lvl, int ilat, int ilng);
	// return the array index of an arbitrary tile (-1: not present)

	int ReadData(int idx, uint8_t **outp);

	inline int ReadData(int lvl, int ilat, int ilng, uint8_t **outp)
	{ return ReadData(Idx(lvl, ilat, ilng), outp); }

	void ReleaseData(uint8_t *data);

	inline int NodeSizeDeflated(int idx) const { return toc.NodeSizeDeflated(idx); }
	inline int NodeSizeInflated(int idx) const { return toc.NodeSizeInflated(idx); }

protected:
	bool OpenArchive();
	int Inflate(const uint8_t *inp, int ninp, uint8_t *outp, int noutp);

private:
	char *path;
	Layer layer;
	FILE *treef;
	TreeTOC toc;
	int32_t rootPos1;    // index of level-1 tile (-1 for not present)
	int32_t rootPos2;    // index of level-2 tile (-1 for not present)
	int32_t rootPos3;    // index of level-3 tile (-1 for not present)
	int32_t rootPos4[2]; // index of the level-4 tiles (quadtree roots; -1 for not present)
	int64_t dofs;
};

#endif // !__ZTREEMGR_H
