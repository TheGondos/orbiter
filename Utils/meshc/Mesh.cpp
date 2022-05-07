// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "Mesh.h"
#include <stdio.h>
#include "D3dmath.h"

using namespace std;

#define _strnicmp strncasecmp

// =======================================================================
// Class Mesh

Mesh::Mesh ()
{
	nGrp = nMtrl = nTex = 0;
}

Mesh::~Mesh ()
{
}

istream &operator>> (istream &is, Mesh &mesh)
{
	printf("1\n");
	char cbuf[256], comment[256];
	int i, j, g, ngrp, nvtx, ntri, nidx, nmtrl, mtrl_idx, ntex, tex_idx, flag, res;
	bool term;

	if (!is.getline (cbuf, 256)) return is;
	if (strcmp (cbuf, "MSHX1")) return is;
	printf("2\n");

	if (!is.getline (cbuf, 256)) return is;
	if (strncmp (cbuf, "GROUPS", 6) || sscanf (cbuf+6, "%d", &ngrp) != 1) return is;
	mesh.nGrp = ngrp;
	printf("3\n");

	for (g = 0, term = false; g < ngrp && !term; g++) {
		// set defaults
		for (;;) {
			if (!is.getline (cbuf, 256)) { term = true; break; }
			printf("-- %s\n", cbuf);
			if (!_strnicmp (cbuf, "MATERIAL", 8)) {       // read material index
			} else if (!_strnicmp (cbuf, "TEXTURE", 7)) { // read texture index
			} else if (!_strnicmp (cbuf, "ZBIAS", 5)) {   // read z-bias
			} else if (!_strnicmp (cbuf, "TEXWRAP", 7)) { // read wrap flags
			} else if (!_strnicmp (cbuf, "NONORMAL", 8)) {
			} else if (!_strnicmp (cbuf, "GEOM", 4)) {    // read geometry
				if (sscanf (cbuf+4, "%d%d", &nvtx, &ntri) != 2) break; // parse error - skip group
				for (i = 4; cbuf[i]; i++)           // read comment (preceeded by ';')
						if (cbuf[i] == ';') { strcpy (comment, cbuf+i+1); break; }
				nidx = ntri*3;
				for (i = 0; i < nvtx; i++) {
						if (!is.getline (cbuf, 256)) {
								break;
						}
				}
				for (i = j = 0; i < ntri; i++) {
						if (!is.getline (cbuf, 256)) {
								break;
						}
						j += 3;
				}
				break;
			}
		}
	}
	printf("4\n");

	// read material list
	if (is.getline (cbuf, 256) && !strncmp (cbuf, "MATERIALS", 9) && (sscanf (cbuf+9, "%d", &nmtrl) == 1)) {
		mesh.nMtrl = nmtrl;
		for (i = 0; i < nmtrl; i++) {
			is.getline (cbuf, 256);
		}
		for (i = 0; i < nmtrl; i++) {
			is.getline (cbuf, 256);
			is.getline (cbuf, 256);
			is.getline (cbuf, 256);
			is.getline (cbuf, 256);
			is.getline (cbuf, 256);
		}
	}
	printf("->%s\n", cbuf);
	printf("5\n");

	if (is.getline (cbuf, 256) && !strncmp (cbuf, "TEXTURES", 8) && (sscanf (cbuf+8, "%d", &ntex) == 1)) {
		mesh.nTex = ntex;
		for (i = 0; i < ntex; i++) {
			is.getline (cbuf, 256);
		}
	}
	printf("6\n");
	return is;
}
