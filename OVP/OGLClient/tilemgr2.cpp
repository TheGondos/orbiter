// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// tilemanager2.cpp
// Rendering of planetary surfaces using texture tiles at
// variable resolutions (new version).
// ==============================================================
#include "glad.h"
#include "tilemgr2.h"
#include "Texture.h"
#include "OGLCamera.h"
#include "Scene.h"

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
		exit(-2);
	}
}

// =======================================================================
// Externals

static TEXCRDRANGE2 fullrange = {0,1,0,1};

// =======================================================================

// =======================================================================
// =======================================================================
// Class Tile

Tile::Tile (TileManager2Base *_mgr, int _lvl, int _ilat, int _ilng)
: mgr(_mgr), lvl(_lvl), ilat(_ilat), ilng(_ilng)
{
	mesh = 0;
	tex = 0;
	mean_elev = 0.0;
	texrange = fullrange;
	owntex = true;
	lngnbr_lvl = latnbr_lvl = dianbr_lvl = _lvl;
	state = Invalid;
	cnt = Centre();
}

// -----------------------------------------------------------------------

Tile::~Tile ()
{
	if (mesh) delete mesh;
	if (tex && owntex) tex->Release();
}

// -----------------------------------------------------------------------

bool Tile::PreDelete ()
{
	switch (state) {
	case Loading:
		return false;                // locked
	case InQueue:
		mgr->loader->Unqueue (this); // remove from load queue
		// fall through
	default:
		return true;
	}
}

// -----------------------------------------------------------------------

bool Tile::GetParentSubTexRange (TEXCRDRANGE2 *subrange)
{
	Tile *parent = getParent();
	if (!parent) return false; // haven't got a parent

	if (!(ilat&1)) { // left column
		subrange->tvmin = parent->texrange.tvmin;
		subrange->tvmax = (parent->texrange.tvmin+parent->texrange.tvmax)*0.5f;
	} else {         // right column
		subrange->tvmin = (parent->texrange.tvmin+parent->texrange.tvmax)*0.5f;
		subrange->tvmax = parent->texrange.tvmax;
	}
	if (!(ilng&1)) { // bottom row
		subrange->tumin = parent->texrange.tumin;
		subrange->tumax = (parent->texrange.tumin+parent->texrange.tumax)*0.5f;
	} else {         // top row
		subrange->tumin = (parent->texrange.tumin+parent->texrange.tumax)*0.5f;
		subrange->tumax = parent->texrange.tumax;
	}
	return true;
}

// -----------------------------------------------------------------------
// Check if tile bounding box intersects the viewing frustum
// given the transformation matrix transform

bool Tile::InView (const MATRIX4 &transform)
{
	if (!lvl) return true; // no good check for this yet
	if (!mesh) return true; // DEBUG : TEMPORARY

	bool bx1, bx2, by1, by2, bz1;
	bx1 = bx2 = by1 = by2 = bz1 = false;
	int v;
	double hx, hy;
	for (v = 0; v < 8; v++) {
		VECTOR4 vt = mul (mesh->bbvtx[v], transform);
		hx = vt.x/vt.w, hy = vt.y/vt.w;
		if (vt.z > 0.0) bz1 = true;
		if (vt.w < 0.0) hx = -hx, hy = -hy;
		if (hx > -1.0) bx1 = true;
		if (hx <  1.0) bx2 = true;
		if (hy > -1.0) by1 = true;
		if (hy <  1.0) by2 = true;
		if (bx1 && bx2 && by1 && by2 && bz1) return true;
	}
	return false;
}

// -----------------------------------------------------------------------
// returns the direction of the tile centre from the planet centre in local
// planet coordinates

VECTOR3 Tile::Centre () const
{
	int xxx = lvl;
	if(xxx >= -3 && xxx < 0) xxx = xxx+3;

	int nlat = 1 << xxx;
	int nlng = 2 << xxx;
	double cntlat = PI05 - PI * ((double)ilat+0.5)/(double)nlat, slat = sin(cntlat), clat = cos(cntlat);
	double cntlng = PI2  * ((double)ilng+0.5)/(double)nlng + PI, slng = sin(cntlng), clng = cos(cntlng);
	return _V(clat*clng,  slat,  clat*slng);
}


// -----------------------------------------------------------------------

void Tile::Extents (double *latmin, double *latmax, double *lngmin, double *lngmax) const
{
	int nlat = 1 << lvl;
	int nlng = 2 << lvl;
	*latmin = PI * (0.5 - (double)(ilat+1)/(double)nlat);
	*latmax = PI * (0.5 - (double)ilat/(double)nlat);
	*lngmin = PI2 * (double)(ilng-nlng/2)/(double)nlng;
	*lngmax = PI2 * (double)(ilng-nlng/2+1)/(double)nlng;
}

// -----------------------------------------------------------------------

VBMESH_TS *Tile::CreateMesh_quadpatch (int grdlat, int grdlng, int16_t *elev, double elev_scale, double globelev,
	const TEXCRDRANGE2 *range, bool shift_origin, VECTOR3 *shift, double bb_excess)
{
	int xxx = lvl;
	if(xxx >= -3 && xxx < 0) xxx = xxx+3;

	const float TEX2_MULTIPLIER = 4.0f; // was: 16.0f
	const float c1 = 1.0f, c2 = 0.0f;   // -1.0f/512.0f; // assumes 256x256 texture patches
	int i, j, n, nofs0, nofs1;
	int nlng = 2 << xxx;
	int nlat = 1 << xxx;
	bool north = (ilat < nlat/2);

	double lat, slat, clat, lng, slng, clng, eradius, dx, dy;
	double minlat = PI * (double)(nlat/2-ilat-1)/(double)nlat;
	double maxlat = PI * (double)(nlat/2-ilat)/(double)nlat;
	double minlng = 0;
	double maxlng = PI2/(double)nlng;
	double radius = mgr->obj_size;
	VECTOR3 pos, tpos, nml;
	if (!range) range = &fullrange;
	float turange = range->tumax-range->tumin;
	float tvrange = range->tvmax-range->tvmin;

	int nvtx = (grdlat+1)*(grdlng+1);         // patch mesh node grid
	int nvtxbuf = nvtx + grdlat+1 + grdlng+1; // add buffer for storage of edges (for elevation matching)
	N2TVERTEX *vtx = new N2TVERTEX[nvtxbuf];

	// create transformation for bounding box
	// we define the local coordinates for the patch so that the x-axis points
	// from (minlng,minlat) corner to (maxlng,minlat) corner (origin is halfway between)
	// y-axis points from local origin to middle between (minlng,maxlat) and (maxlng,maxlat)
	// bounding box is created in this system and then transformed back to planet coords.
	double clat0 = cos(minlat), slat0 = sin(minlat);
	double clng0 = cos(minlng), slng0 = sin(minlng);
	double clat1 = cos(maxlat), slat1 = sin(maxlat);
	double clng1 = cos(maxlng), slng1 = sin(maxlng);
	VECTOR3 ex = {clat0*clng1 - clat0*clng0, 0, clat0*slng1 - clat0*slng0}; normalise (ex);
	VECTOR3 ey = {0.5*(clng0+clng1)*(clat1-clat0), slat1-slat0, 0.5*(slng0+slng1)*(clat1-clat0)}; normalise (ey);
	VECTOR3 ez = crossp (ey, ex);
	MATRIX3 R = {ex.x, ex.y, ex.z,  ey.x, ey.y, ey.z,  ez.x, ez.y, ez.z};
	VECTOR3 pref = {radius*clat0*0.5*(clng1+clng0), radius*slat0, radius*clat0*0.5*(slng1+slng0)}; // origin
	VECTOR3 tpmin = {0,0,0}, tpmax = {0,0,0}; 

	// patch translation vector
	if (shift_origin) {
		dx = (north ? clat0:clat1)*radius;
		dy = (north ? slat0:slat1)*radius;
	} else {
		dx = dy = 0.0;
	}
	if (shift) {
		shift->x = dx;
		shift->y = dy;
		shift->z = 0.0;
	}

	// create the vertices
	for (i = n = 0; i <= grdlat; i++) {
		lat = minlat + (maxlat-minlat) * (double)i/(double)grdlat;
		slat = sin(lat), clat = cos(lat);
		for (j = 0; j <= grdlng; j++) {
			lng = minlng + (maxlng-minlng) * (double)j/(double)grdlng;
			slng = sin(lng), clng = cos(lng);

			eradius = radius + globelev; // radius including node elevation
			if (elev) eradius += (double)elev[(i+1)*TILE_ELEVSTRIDE + j+1]*elev_scale;
			nml = _V(clat*clng, slat, clat*slng);
			pos = nml*eradius;
			tpos = mul (R, pos-pref);
			if (!n) {
				tpmin = tpos;
				tpmax = tpos;
			} else {
				if      (tpos.x < tpmin.x) tpmin.x = tpos.x;
			    else if (tpos.x > tpmax.x) tpmax.x = tpos.x;
				if      (tpos.y < tpmin.y) tpmin.y = tpos.y;
				else if (tpos.y > tpmax.y) tpmax.y = tpos.y;
				if      (tpos.z < tpmin.z) tpmin.z = tpos.z;
				else if (tpos.z > tpmax.z) tpmax.z = tpos.z;
			}
			vtx[n].x = (float)(pos.x - dx); vtx[n].nx = (float)(nml.x);
			vtx[n].y = (float)(pos.y - dy); vtx[n].ny = (float)(nml.y);
			vtx[n].z = (float)(pos.z);      vtx[n].nz = (float)(nml.z);

			vtx[n].tu0 = (float)((c1*j)/grdlng+c2); // overlap to avoid seams
			vtx[n].tv0 = (float)(grdlat-i)/(float)(grdlat);
			vtx[n].tu1 = vtx[n].tu0 * TEX2_MULTIPLIER;
			vtx[n].tv1 = vtx[n].tv0 * TEX2_MULTIPLIER;

			// map texture coordinates to subrange
			vtx[n].tu0 = vtx[n].tu0*turange + range->tumin;
			vtx[n].tv0 = vtx[n].tv0*tvrange + range->tvmin;
			n++;
		}
	}

	// create the face indices
	int nidx = 2*grdlat*grdlng * 3;
	uint16_t *idx = new uint16_t[nidx];

	if (elev) { // do adaptive orientation of cell diagonal
		int16_t *elev1, *elev2, *elev1n, *elev2n, err1, err2;
		for (i = n = nofs0 = 0; i < grdlat; i++) {
			nofs1 = nofs0+grdlng+1;
			for (j = 0; j < grdlng; j++) {
				elev1  = elev + TILE_ELEVSTRIDE+2+j+TILE_ELEVSTRIDE*i;
				elev2  = elev1 + TILE_ELEVSTRIDE-1;
				elev1n = elev1 - TILE_ELEVSTRIDE+1;
				elev2n = elev2 + TILE_ELEVSTRIDE-1;
				err1 = abs(*elev1 * 2 - *elev2 - *elev1n) + abs(*elev2 * 2 - *elev1 - *elev2n);
				elev1  = elev + TILE_ELEVSTRIDE+1+j+TILE_ELEVSTRIDE*i;
				elev2  = elev1 + TILE_ELEVSTRIDE+1;
				elev1n = elev1 - TILE_ELEVSTRIDE-1;
				elev2n = elev2 + TILE_ELEVSTRIDE+1;
				err2 = abs(*elev1 * 2 - *elev2 - *elev1n) + abs(*elev2 * 2 - *elev1 - *elev2n);
				if (err1 < err2) {
					idx[n++] = nofs0+j;
					idx[n++] = nofs1+j;
					idx[n++] = nofs0+j+1;
					idx[n++] = nofs0+j+1;
					idx[n++] = nofs1+j;
					idx[n++] = nofs1+j+1;
				} else {
					idx[n++] = nofs0+j;
					idx[n++] = nofs1+j+1;
					idx[n++] = nofs0+j+1;
					idx[n++] = nofs1+j+1;
					idx[n++] = nofs0+j;
					idx[n++] = nofs1+j;
				}
			}
			nofs0 = nofs1;
		}
	} else { // no elevation => no adaptation of cell diagonals necessary
		for (i = n = nofs0 = 0; i < grdlat; i++) {
			nofs1 = nofs0+grdlng+1;
			for (j = 0; j < grdlng; j++) {
				idx[n++] = nofs0+j;
				idx[n++] = nofs1+j;
				idx[n++] = nofs0+j+1;
				idx[n++] = nofs1+j+1;
				idx[n++] = nofs0+j+1;
				idx[n++] = nofs1+j;
			}
			nofs0 = nofs1;
		}
	}

	// regenerate normals for terrain
	if (elev) {
		//const double shade_exaggerate = 1.0; // 1 = normal, <1 = more dramatic landscape shadows
//		double dy, dz, dydz, nz_x, ny_x, nx1, ny1, nz1;
		double dy, dz, dydz, nx1, ny1, nz1;
		int en;
		dy = radius * PI/(nlat*grdlat);  // y-distance between vertices
		//ny_x = shade_exaggerate*dy;
		for (i = n = 0; i <= grdlat; i++) {
			lat = minlat + (maxlat-minlat) * (double)i/(double)grdlat;
			slat = sin(lat), clat = cos(lat);
			dz = radius * PI2*cos(lat) / (nlng*grdlng); // z-distance between vertices on unit sphere
			dydz = dy*dz;
			//nz_x = shade_exaggerate*dz;
			for (j = 0; j <= grdlng; j++) {
				lng = minlng + (maxlng-minlng) * (double)j/(double)grdlng;
				slng = sin(lng), clng = cos(lng);
				en = (i+1)*TILE_ELEVSTRIDE + (j+1);

				// This version avoids the normalisation of the 4 intermediate face normals
				// It's faster and doesn't seem to make much difference
				VECTOR3 nml = {2.0*dydz, dz*elev_scale*(elev[en-TILE_ELEVSTRIDE]-elev[en+TILE_ELEVSTRIDE]), dy*elev_scale*(elev[en-1]-elev[en+1])};
				normalise (nml);
				// rotate into place
				nx1 = nml.x*clat - nml.y*slat;
				ny1 = nml.x*slat + nml.y*clat;
				nz1 = nml.z;
				vtx[n].nx = (float)(nx1*clng - nz1*slng);
				vtx[n].ny = (float)(ny1);
				vtx[n].nz = (float)(nx1*slng + nz1*clng);
				n++;
			}
		}
	}

	// store the adaptable edges in the separate vertex area
	for (i = 0, n = nvtx; i <= grdlat; i++) // store left or right edge
		vtx[n++] = vtx[i*(grdlng+1) + ((ilng&1) ? grdlng:0)];
	for (i = 0; i <= grdlng; i++) // store top or bottom edge
		vtx[n++] = vtx[i + ((ilat&1) ? 0 : (grdlng+1)*grdlat)];

	// create the mesh
	VBMESH_TS *mesh = new VBMESH_TS;

	mesh->nv = nvtx;
	mesh->vtx = vtx;
	mesh->ni = nidx;
	mesh->idx = idx;

	// set bounding box for visibility calculations
	if (bb_excess) {
		double bb_dx = tpmax.x-tpmin.x;
		double bb_dy = tpmax.y-tpmin.y;
		double bb_dz = tpmax.z-tpmin.z;
		double scale = sqrt(bb_dx*bb_dx + bb_dy*bb_dy + bb_dz*bb_dz)/(2.0*sqrt(3.0))*bb_excess;
		tpmin.x -= scale;
		tpmax.x += scale;
		tpmin.y -= scale;
		tpmax.y += scale;
		tpmin.z -= scale;
		tpmax.z += scale;
	}
	pref.x -= dx;
	pref.y -= dy;
	mesh->bbvtx = new VECTOR4[8];
	mesh->bbvtx[0] = _V(tmul (R, _V(tpmin.x, tpmin.y, tpmin.z)) + pref);
	mesh->bbvtx[1] = _V(tmul (R, _V(tpmax.x, tpmin.y, tpmin.z)) + pref);
	mesh->bbvtx[2] = _V(tmul (R, _V(tpmin.x, tpmax.y, tpmin.z)) + pref);
	mesh->bbvtx[3] = _V(tmul (R, _V(tpmax.x, tpmax.y, tpmin.z)) + pref);
	mesh->bbvtx[4] = _V(tmul (R, _V(tpmin.x, tpmin.y, tpmax.z)) + pref);
	mesh->bbvtx[5] = _V(tmul (R, _V(tpmax.x, tpmin.y, tpmax.z)) + pref);
	mesh->bbvtx[6] = _V(tmul (R, _V(tpmin.x, tpmax.y, tpmax.z)) + pref);
	mesh->bbvtx[7] = _V(tmul (R, _V(tpmax.x, tpmax.y, tpmax.z)) + pref);

	return mesh;
}

// -----------------------------------------------------------------------

VBMESH_TS *Tile::CreateMesh_hemisphere (int grd, int16_t *elev, double globelev)
{
	const int texres = 512;
	double radius = mgr->obj_size + globelev;
	double eradius, slat, clat, slng, clng;
	VECTOR3 pos, nml;

	// Allocate memory for the vertices and indices
	int         nVtx = (grd-1)*(grd+1)+2;
	int         nIdx = 6*(grd*(grd-2)+grd);
	N2TVERTEX *Vtx = new N2TVERTEX[nVtx];
	uint16_t*       Idx = new uint16_t[nIdx];

	// Counters
    uint16_t x, y, nvtx = 0, nidx = 0;
	N2TVERTEX *vtx = Vtx;
	uint16_t *idx = Idx;

	// Angle deltas for constructing the sphere's vertices
    double fDAng   = PI / grd;
    double lng, lat = fDAng;
	int x1 = grd;
	int x2 = x1+1;
	float du = 0.5f/(float)texres;
	float a  = (1.0f-2.0f*du)/(float)x1;

    // Make the middle of the sphere
    for (y = 1; y < grd; y++) {
		slat = sin(lat), clat = cos(lat);
		float tv = (float)(lat/PI);

        for (x = 0; x < x2; x++) {
            lng = x*fDAng - PI;  // subtract Pi to wrap at +-180�
			if (ilng) lng += PI;
			slng = sin(lng), clng = cos(lng);
			eradius = radius + globelev; // radius including node elevation
			if (elev) eradius += (double)elev[(grd+1-y)*TILE_ELEVSTRIDE + x+1];
			nml = _V(slat*clng, clat, slat*slng);
			pos = nml*eradius;
			vtx->x = (float)(pos.x);  vtx->nx = (float)(nml.x);
			vtx->y = (float)(pos.y);  vtx->ny = (float)(nml.y);
			vtx->z = (float)(pos.z);  vtx->nz = (float)(nml.z);
			float tu = a*(float)x + du;
			vtx->tu0 = vtx->tu1 = tu;
			vtx->tv0 = vtx->tv1 = tv;
			vtx++;
			nvtx++;
        }
        lat += fDAng;
    }

    for (y = 0; y < grd-2; y++) {
        for (x = 0; x < x1; x++) {
            *idx++ = (uint16_t)( (y+0)*x2 + (x+0) );
            *idx++ = (uint16_t)( (y+0)*x2 + (x+1) );
            *idx++ = (uint16_t)( (y+1)*x2 + (x+0) );
            *idx++ = (uint16_t)( (y+0)*x2 + (x+1) );
            *idx++ = (uint16_t)( (y+1)*x2 + (x+1) );
            *idx++ = (uint16_t)( (y+1)*x2 + (x+0) ); 
			nidx += 6;
        }
    }
    // Make top and bottom
	uint16_t wNorthVtx = nvtx;
	eradius = radius + globelev;
	if (elev) {
		double mn = 0.0;
		for (x = 0; x < x2; x++) mn += (double)elev[TILE_ELEVSTRIDE*(grd+1) + x+1];
		eradius += mn/x2;
	}
	nml = _V(0,1,0);
	pos = nml*eradius;
	vtx->x = (float)(pos.x);  vtx->nx = (float)(nml.x);
	vtx->y = (float)(pos.y);  vtx->ny = (float)(nml.y);
	vtx->z = (float)(pos.z);  vtx->nz = (float)(nml.z);
	vtx->tu0 = vtx->tu1 = 0.5f;
	vtx->tv0 = vtx->tv1 = 0.0f;
	vtx++;
    nvtx++;
	uint16_t wSouthVtx = nvtx;

	
	eradius = radius + globelev;
	if (elev) {
		double mn = 0.0;
		for (x = 0; x < x2; x++) mn += (double)elev[TILE_ELEVSTRIDE + x+1];
		eradius += mn/x2;
	}
	nml = _V(0,-1,0);
	pos = nml*eradius;
	vtx->x = (float)(pos.x);  vtx->nx = (float)(nml.x);
	vtx->y = (float)(pos.y);  vtx->ny = (float)(nml.y);
	vtx->z = (float)(pos.z);  vtx->nz = (float)(nml.z);
	vtx->tu0 = vtx->tu1 = 0.5f;
	vtx->tv0 = vtx->tv1 = 1.0f;
	vtx++;
    nvtx++;

    for (x = 0; x < x1; x++) {
		uint16_t p1 = wSouthVtx;
		uint16_t p2 = (uint16_t)( (grd-2)*x2 + (x+0) );
		uint16_t p3 = (uint16_t)( (grd-2)*x2 + (x+1) );

        *idx++ = p1;
        *idx++ = p2;
        *idx++ = p3;
		nidx += 3;
    }

    for (x = 0; x < x1; x++) {
		uint16_t p1 = wNorthVtx;
		uint16_t p2 = (uint16_t)( (0)*x2 + (x+0) );
		uint16_t p3 = (uint16_t)( (0)*x2 + (x+1) );

        *idx++ = p1;
        *idx++ = p3;
        *idx++ = p2;
		nidx += 3;
    }

	// regenerate normals for terrain
	if (elev) {
		double dy, dz, dydz, nx1, ny1, nz1;
		int en;
		dy = radius * PI/grd;  // y-distance between vertices
		vtx = Vtx;
		for (y = 1; y < grd; y++) {
			lat = PI05-y*fDAng;
			slat = sin(lat), clat = cos(lat);
			dz = radius * PI*cos(lat) / grd; // z-distance between vertices on unit sphere
			dydz = dy*dz;
			for (x = 0; x < x2; x++) {
				lng = x*fDAng;
				if (!ilng) lng -= PI;
				slng = sin(lng), clng = cos(lng);
				en = (grd+1-y)*TILE_ELEVSTRIDE + x+1;
				VECTOR3 nml = {2.0*dydz, dz*(elev[en-TILE_ELEVSTRIDE]-elev[en+TILE_ELEVSTRIDE]), dy*(elev[en-1]-elev[en+1])};
				normalise(nml);
				// rotate into place
				nx1 = nml.x*clat - nml.y*slat;
				ny1 = nml.x*slat + nml.y*clat;
				nz1 = nml.z;
				vtx->nx = (float)(nx1*clng - nz1*slng);
				vtx->ny = (float)(ny1);
				vtx->nz = (float)(nx1*slng + nz1*clng);
				vtx++;
			}
		}
	}

	// create the mesh
	VBMESH_TS *mesh = new VBMESH_TS;

	mesh->nv = nVtx;
	mesh->vtx = Vtx;
	mesh->ni = nIdx;
	mesh->idx = Idx;

	return mesh;
}
void VBMESH_TS::Upload()
{
	VAO = std::make_unique<VertexArray>();
	VAO->Bind();
	VBO = std::make_unique<VertexBuffer>(vtx, nv*sizeof(N2TVERTEX));
	VBO->Bind();

	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(N2TVERTEX),                  // stride
	(void*)0            // array buffer offset
	);
	CheckError("glVertexAttribPointer0");
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
	1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(N2TVERTEX),                  // stride
	(void*)12            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(1);
	CheckError("glEnableVertexAttribArray1");

	glVertexAttribPointer(
	2,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	2,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(N2TVERTEX),                  // stride
	(void*)24            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(2);
	CheckError("glEnableVertexAttribArray2");

	glVertexAttribPointer(
	3,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	2,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(N2TVERTEX),                  // stride
	(void*)32            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(3);
	CheckError("glEnableVertexAttribArray3");

	IBO = std::make_unique<IndexBuffer>(idx, ni);
	IBO->Bind();

	VAO->UnBind();
}
// =======================================================================
// =======================================================================

bool TileLoader::bRunThread = true;
int TileLoader::nqueue = 0;
int TileLoader::queue_in = 0;
int TileLoader::queue_out = 0;
std::mutex TileLoader::hLoadMutex;
struct TileLoader::QUEUEDESC TileLoader::queue[MAXQUEUE2] = {0};

TileLoader::TileLoader ()
{
	bRunThread = true;
	nqueue = queue_in = queue_out = 0;
	load_frequency = 10;//g_client->Cfg()->PlanetLoadFrequency;
	hLoadThread = std::thread (Load_ThreadProc, this);
}

// -----------------------------------------------------------------------

TileLoader::~TileLoader ()
{
	bRunThread = false;
	hLoadThread.join();
/*
	if (WaitForSingleObject (hLoadThread, 1000) == WAIT_TIMEOUT) {
		TerminateThread (hLoadThread, 0);
	}
	CloseHandle (hLoadThread);
	CloseHandle (hLoadMutex);
	*/
}

// -----------------------------------------------------------------------

bool TileLoader::LoadTileAsync (Tile *tile)
{
	// Note: the queue should probably be sorted according to tile level

	int i, j;

	// Check if request is already present
	for (i = 0; i < nqueue; i++) {
		j = (i+queue_out) % MAXQUEUE2;
		if (queue[j].tile == tile)
			return false;
	}

	if (nqueue == MAXQUEUE2) { // queue full
		int maxlvl, maxlvl_idx = 0;
		for (i = 1, maxlvl = queue[0].tile->lvl; i < nqueue; i++)
			if (queue[i].tile->lvl > maxlvl)
				maxlvl = queue[i].tile->lvl, maxlvl_idx = i;
		if (maxlvl > tile->lvl) { // replace the highest level queued tile (load from lowest to highest)
			queue[maxlvl_idx].tile->state = Tile::Invalid;
			queue[maxlvl_idx].tile = tile;
			tile->state = Tile::InQueue;
		} else {
			tile->state = Tile::Invalid;
		}
		return false;
	}

	// add tile to load queue
	QUEUEDESC *qd = queue+queue_in;
	qd->tile = tile;
	tile->state = Tile::InQueue;
	nqueue++;
	queue_in = (queue_in+1) % MAXQUEUE2;
	return true;
}

// -----------------------------------------------------------------------

bool TileLoader::Unqueue (Tile *tile)
{
	if (tile->state != Tile::InQueue) return false;

	int i, j;
	for (i = 0; i < nqueue; i++) {
		j = (queue_out+i) % MAXQUEUE2;
		if (queue[j].tile == tile) {
			if (i) queue[j].tile = queue[queue_out].tile;
			queue_out = (queue_out+1) % MAXQUEUE2;
			nqueue--;
			return true;
		}
	}
	return false;
}

// -----------------------------------------------------------------------
#ifdef UNDEF
int WINAPI TileLoader::Load_ThreadProc (void *data)
{
	TileLoader *loader = (TileLoader*)data;
	int idle = 1000/loader->load_frequency;
	Tile *tile;
	bool load;

	while (bRunThread) {
		WaitForMutex ();
		if (load = (nqueue > 0)) {
			tile = queue[queue_out].tile;
			tile->state = Tile::Loading; // lock tile and its ancestor tree
			queue_out = (queue_out+1) % MAXQUEUE2; // remove from queue
			nqueue--;
		}
		ReleaseMutex ();

		if (load) {
			tile->Load(); // load/create the tile

			WaitForMutex ();
			tile->state = Tile::Inactive; // unlock tile
			ReleaseMutex ();
		} else {
			Sleep (idle);
		}
	}
	return 0;
}
#endif
// -----------------------------------------------------------------------
#include <unistd.h>
int TileLoader::Load_ThreadProc (TileLoader *loader)
{
	const int tile_packet_size = 8; // max number of tiles to process from queue
	//int idle = 1000/loader->load_frequency;
	Tile *tile[tile_packet_size];
	int nload, i;

sleep(6000);
	while (bRunThread) {
		WaitForMutex ();
		for (nload = 0; nqueue > 0 && nload < tile_packet_size; nload++) {
			tile[nload] = queue[queue_out].tile;
			tile[nload]->state = Tile::Loading; // lock tile and its ancestor tree
			queue_out = (queue_out+1) % MAXQUEUE2; // remove from queue
			nqueue--;
		}
		ReleaseMutex ();

		if (nload) {
			for (i = 0; i < nload; i++) {
				tile[i]->Load(); // load/create the tile
			}

			WaitForMutex ();
			for (i = 0; i < nload; i++)
				tile[i]->state = Tile::Inactive; // unlock tile
			ReleaseMutex ();
		} else {
			sleep (1);
		}
	}
	return 0;
}

// =======================================================================
// =======================================================================

TileManager2Base::configPrm TileManager2Base::cprm = {
	2,                  // elevInterpol
	false,				// bSpecular
	false,				// bLights
	false,              // bCloudShadow
	0.5,                // lightfac
	0x0003              // tileLoadFlags
};
TileLoader *TileManager2Base::loader = NULL;
double TileManager2Base::resolutionBias = 4.0;
double TileManager2Base::resolutionScale = 1.0;
bool TileManager2Base::bTileLoadThread = false;

// -----------------------------------------------------------------------

TileManager2Base::TileManager2Base (const vPlanet *vplanet, int _maxres, int _gridres)
: vp(vplanet)
{
	// set persistent parameters
	prm.maxlvl = std::max (0, _maxres-4);
	gridRes = _gridres;

	obj = vp->Object();
	obj_size = oapiGetSize (obj);
	oapiGetObjectName (obj, cbody_name, 256);
	camera = g_client->GetScene()->GetCamera();
	emgr = oapiElevationManager(obj);
	elevRes = *(double*)oapiGetObjectParam (obj, OBJPRM_PLANET_ELEVRESOLUTION);
}

// -----------------------------------------------------------------------

void TileManager2Base::GlobalInit()
{
	resolutionBias = 4.0 + *(double*)g_client->GetConfigParam (CFGPRM_RESOLUTIONBIAS);
	int w, h;
	g_client->clbkGetViewportSize (&w, &h);
	resolutionScale = 1400.0 / (double)h;
	cprm.bSpecular = *(bool*)g_client->GetConfigParam (CFGPRM_SURFACEREFLECT);
	cprm.bLights = *(bool*)g_client->GetConfigParam (CFGPRM_SURFACELIGHTS);
	cprm.bCloudShadow = *(bool*)g_client->GetConfigParam (CFGPRM_CLOUDSHADOWS);
	cprm.lightfac = *(double*)g_client->GetConfigParam (CFGPRM_SURFACELIGHTBRT);
	cprm.elevMode = *(int*)g_client->GetConfigParam (CFGPRM_ELEVATIONMODE);
	cprm.tileLoadFlags = 0x0003;//g_client->Cfg()->PlanetTileLoadFlags; //*(int*)gclient->GetConfigParam (CFGPRM_TILELOADFLAGS);
	bTileLoadThread = false; // TODO: g_pOrbiter->Cfg()->CfgPRenderPrm.bLoadOnThread;
	loader = new TileLoader ();
}

// -----------------------------------------------------------------------

void TileManager2Base::GlobalExit ()
{
	delete loader;
}

// -----------------------------------------------------------------------

void TileManager2Base::SetRenderPrm (MATRIX4 &dwmat, double prerot, bool use_zbuf, const vPlanet::RenderPrm &rprm)
{
	const double minalt = std::max(0.002,rprm.horizon_excess);
	VECTOR3 obj_pos, cam_pos;
	//OGLCamera *camera = g_client->GetScene()->GetCamera();

	prm.rprm = &rprm;

	// set up the parameter structure
	oapiGetRotationMatrix(obj, &prm.grot); // planet's rotation matrix
	oapiGetGlobalPos (obj, &obj_pos);      // planet position
	oapiCameraGlobalPos (&cam_pos);        // camera position
	if (prerot) {
		double srot = sin(prerot), crot = cos(prerot);
		MATRIX4 RM4 = { crot,0,-srot,0,  0,1,0,0,  srot,0,crot,0,  0,0,0,1 };
		prm.dwmat = mul(RM4,dwmat);
		prm.grot = mul (prm.grot, _M(crot,0,srot, 0,1,0, -srot,0,crot));
	} else {
		prm.dwmat = dwmat;
	}
	prm.dwmat_tmp = prm.dwmat;
	prm.cpos = obj_pos-cam_pos;
	prm.cdir = tmul (prm.grot, -prm.cpos); // camera's direction in planet frame
	double cdist = length(prm.cdir);
	prm.cdist = cdist / obj_size;          // camera's distance in units of planet radius
	normalise (prm.cdir);
	prm.sdir = tmul (prm.grot, -obj_pos);  // sun's direction in planet frame
	normalise (prm.sdir);
	prm.viewap = acos (1.0/(std::max (prm.cdist, 1.0+minalt)));
	prm.scale = 1.0;
	prm.fog = rprm.bFog;
	prm.tint = rprm.bTint;

	if (prm.tint)
		prm.atm_tint = rprm.rgbTint;
}

// -----------------------------------------------------------------------

#ifdef UNDEF
void TileManager2::Render (MATRIX4 &dwmat, bool use_zbuf, bool addfog)
{
	double np, fp;
	bool reset_clipping = false;
	float fogfactor;
	int i;
	VECTOR3 obj_pos, cam_pos;
	Camera *camera = gc->GetScene()->GetCamera();

	// set up the parameter structure
	prm.dwmat = dwmat;
	prm.dwmat_tmp = dwmat;
	oapiGetRotationMatrix(obj, &prm.grot); // planet's rotation matrix
	oapiGetGlobalPos (obj, &obj_pos);      // planet position
	oapiCameraGlobalPos (&cam_pos);        // camera position
	prm.cpos = obj_pos-cam_pos;
	prm.cdir = tmul (prm.grot, -prm.cpos); // camera's direction in planet frame
	double cdist = length(prm.cdir);
	prm.cdist = cdist / obj_size;          // camera's distance in units of planet radius
	normalise (prm.cdir);
	prm.sdir = tmul (prm.grot, -obj_pos);  // sun's direction in planet frame
	normalise (prm.sdir);
	prm.viewap = acos (1.0/(std::max (prm.cdist, 1.0)));
	prm.scale = 1.0;
	prm.fog = addfog;

	// adjust scaling parameters (can only be done if no z-buffering is in use)
	if (!use_zbuf) {
		double R = obj_size;
		double D = cdist;
		double zmax = (D - R*R/D) * 1.5;
		double zmin = (D-R) * 0.9;
		double zscale = 1.0;

		np = camera->GetNearlimit();
		fp = camera->GetFarlimit();

		// rescale planet and pull closer to camera
		if (fp < zmax) {
			zscale = fp/zmax;
			for (int i = 0; i < 16; i++) prm.dwmat_tmp.data[i] = prm.dwmat.data[i] *= zscale;
			prm.dwmat.m44 = prm.dwmat_tmp.m44 = 1.0;
			reset_clipping = true;

			if (addfog) { // need to scale the fog density accordingly
				dev->GetRenderState (D3DRENDERSTATE_FOGDENSITY, ((int*)(&fogfactor)));
				float fogfactor_scaled = (float)(fogfactor/zscale);
				dev->SetRenderState (D3DRENDERSTATE_FOGDENSITY, *((int*)(&fogfactor_scaled)));
			}
			prm.scale = zscale;
		}
	}

	// build a transformation matrix for frustum testing
	MATRIX4 Mproj = camera->ProjectionMatrix();
	Mproj.m33 = 1.0; Mproj.m43 = -1.0;  // adjust near plane to 1, far plane to infinity
	MATRIX4 Mview = camera->ViewMatrix();
	prm.dviewproj = mul(Mview,Mproj);

	WaitForSingleObject (loader->hLoadMutex, INFINITE);

	// update the tree
	for (i = 0; i < 2; i++)
		ProcessNode (tiletree+i);

	// render the tree
	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	for (i = 0; i < 2; i++)
		RenderNode (tiletree+i);
	dev->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	ReleaseMutex (loader->hLoadMutex);

	if (reset_clipping) {
		if (addfog)
			dev->SetRenderState (D3DRENDERSTATE_FOGDENSITY, *((int*)(&fogfactor)));
	}
}
#endif

// -----------------------------------------------------------------------

MATRIX4 TileManager2Base::WorldMatrix (int ilng, int nlng, int ilat, int nlat)
{
	if (nlat < 2) {  // render full sphere
		return prm.dwmat;
	}

	double lat, lng = PI2 * (double)ilng/(double)nlng + PI; // add pi so texture wraps at +-180�
	double slng = sin(lng), clng = cos(lng);
	MATRIX4 lrot = {clng,0,slng,0,  0,1.0,0,0,  -slng,0,clng,0,  0,0,0,1.0};

	if (nlat <= 8) {  // rotate patch into place
		return mul(lrot,prm.dwmat);
	} else {          // shift, scale and rotate
		bool north = (ilat < nlat/2);
		if (north) lat = PI * (double)(nlat/2-ilat-1)/(double)nlat;
		else       lat = PI * (double)(nlat/2-ilat)/(double)nlat;
		double s = obj_size;
		double dx = s*cos(lng)*cos(lat); // the offsets between sphere centre and tile corner
		double dy = s*sin(lat);
		double dz = s*sin(lng)*cos(lat);
		prm.dwmat_tmp.m41 = (dx*prm.grot.m11 + dy*prm.grot.m12 + dz*prm.grot.m13 + prm.cpos.x) * (float)prm.scale;
		prm.dwmat_tmp.m42 = (dx*prm.grot.m21 + dy*prm.grot.m22 + dz*prm.grot.m23 + prm.cpos.y) * (float)prm.scale;
		prm.dwmat_tmp.m43 = (dx*prm.grot.m31 + dy*prm.grot.m32 + dz*prm.grot.m33 + prm.cpos.z) * (float)prm.scale;
		return mul(lrot,prm.dwmat_tmp);
	}
}

// -----------------------------------------------------------------------

void TileManager2Base::SetWorldMatrix (const MATRIX4 &W)
{
	wtrans[0][0] = W.m11;
	wtrans[0][1] = W.m12;
	wtrans[0][2] = W.m13;
	wtrans[0][3] = W.m14;

	wtrans[1][0] = W.m21;
	wtrans[1][1] = W.m22;
	wtrans[1][2] = W.m23;
	wtrans[1][3] = W.m24;

	wtrans[2][0] = W.m31;
	wtrans[2][1] = W.m32;
	wtrans[2][2] = W.m33;
	wtrans[2][3] = W.m34;

	wtrans[3][0] = W.m41;
	wtrans[3][1] = W.m42;
	wtrans[3][2] = W.m43;
	wtrans[3][3] = W.m44;
}
