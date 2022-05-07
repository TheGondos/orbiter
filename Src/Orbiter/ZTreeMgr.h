// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// ZTreeMgr.h
// Manage compressed and packed tile trees for planetary surface and cloud layers.
// =======================================================================

#ifndef __ZTREEMGR_H
#define __ZTREEMGR_H

#include <iostream>

// =======================================================================
// Tree node structure

struct TreeNode {
	int64_t pos;  // file position of node data
	int32_t size;   // data block size [bytes]
	int32_t child[4]; // array index positions of the children (-1=no child)
//FIXME padding??
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

/*        x   MAGIC x x   size  x  x   flags x x dataofs x
00000000  54 58 01 00 30 00 00 00  01 00 00 00 70 79 03 00  |TX..0.......py..|
          x     datalength      x  xnode cnt x x root1   x
00000010  5d 6c ad 1e 00 00 00 00  ca 1b 00 00 00 00 00 00  |]l..............|
		  x root2   x x  root3  x  x root4 1 x x root4 2 x
00000020  01 00 00 00 02 00 00 00  03 00 00 00 fb 0f 00 00  |................|

struct TreeNode {
	__int64 pos;  // file position of node data
	int32_t size;   // data block size [bytes]
	int32_t child[4]; // array index positions of the children (-1=no child)


00000030  00 00 00 00 00 00 00 00  80 20 00 00 ff ff ff ff  |......... ......|
00000040  ff ff ff ff ff ff ff ff  ff ff ff ff 00 00 00 00  |................|
00000050  03 11 00 00 00 00 00 00  80 80 00 00 ff ff ff ff  |................|
00000060  ff ff ff ff ff ff ff ff  ff ff ff ff 00 00 00 00  |................|
00000070  a4 4a 00 00 00 00 00 00  80 00 02 00 ff ff ff ff  |.J..............|
00000080  ff ff ff ff ff ff ff ff  ff ff ff ff 00 00 00 00  |................|
00000090  86 13 01 00 00 00 00 00  80 00 02 00 04 00 00 00  |................|
000000a0  26 02 00 00 89 0c 00 00  c4 0d 00 00 00 00 00 00  |&...............|
000000b0  bc df 01 00 00 00 00 00  80 00 02 00 05 00 00 00  |................|
000000c0  49 00 00 00 a2 00 00 00  25 01 00 00 00 00 00 00  |I.......%.......|
000000d0  ce cb 02 00 00 00 00 00  80 00 02 00 06 00 00 00  |................|
000000e0  07 00 00 00 08 00 00 00  33 00 00 00 00 00 00 00  |........3.......|
000000f0  37 ad 03 00 00 00 00 00  80 00 02 00 ff ff ff ff  |7...............|
00000100  ff ff ff ff ff ff ff ff  ff ff ff ff 00 00 00 00  |................|
00000110  da 43 04 00 00 00 00 00  80 00 02 00 ff ff ff ff  |.C..............|
00000120  ff ff ff ff ff ff ff ff  ff ff ff ff 00 00 00 00  |................|


*/


// =======================================================================
// Tree table of contents

class TreeTOC {
	friend class ZTreeMgr;

public:
	TreeTOC();
	~TreeTOC();
	size_t fread(int size, FILE *f);
	inline int size() const { return ntree; }
	inline const TreeNode &operator[](int idx) const { return tree[idx]; }

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
