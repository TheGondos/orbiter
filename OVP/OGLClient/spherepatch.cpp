// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// spherepatch.cpp
// Create meshes for spheres and sphere patches
// ==============================================================

#include "glad.h"
#include "spherepatch.h"

static float TEX2_MULTIPLIER = 4.0f; // microtexture multiplier
struct VERTEX_XYZ   { float x, y, z; };

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
	}
}
// ==============================================================
// struct VBMESH

VBMESH::VBMESH ()
{
	vb = 0;
	ib = 0;
	va = 0;
	//bb = 0;
	vtx = 0;
	bbvtx = 0;
	nv = 0;
	idx = 0;
	ni = 0;
	ib = 0;
}

VBMESH::~VBMESH ()
{
	if (vb) delete vb;
	if (ib) delete ib;
	if (va) delete va;
	//if (bb) delete bb;
	if (nv) delete []vtx;
	if (bbvtx) delete []bbvtx;
	if (ni) delete []idx;
	if (ib) delete ib;
}

void VBMESH::MapVertices ()
{
	if (vb) delete vb;

	vb = new VertexBuffer(vtx, nv*sizeof(N2TVERTEX));
}

// ==============================================================
// CreateSphere()
// Create a spherical mesh of radius 1 and resolution defined by nrings
// Below is a list of #vertices and #indices against nrings:
//
// nrings  nvtx   nidx   (nidx = 12 nrings^2)
//   4       38    192
//   6       80    432
//   8      138    768
//  12      302   1728
//  16      530   3072
//  20      822   4800
//  24     1178   6912

void CreateSphere (VBMESH &mesh, uint32_t nrings, bool hemisphere, int which_half, int texres)
{
	// Allocate memory for the vertices and indices
	uint32_t       nVtx = hemisphere ? nrings*(nrings+1)+2 : nrings*(2*nrings+1)+2;
	uint32_t       nIdx = hemisphere ? 6*nrings*nrings : 12*nrings*nrings;
	N2TVERTEX* Vtx = new N2TVERTEX[nVtx];
	uint16_t*        Idx = new uint16_t[nIdx];

	// Counters
    uint16_t x, y, nvtx = 0, nidx = 0;
	N2TVERTEX *vtx = Vtx;
	uint16_t *idx = Idx;

	// Angle deltas for constructing the sphere's vertices
    float fDAng   = (float)PI / nrings;
    float fDAngY0 = fDAng;
	uint32_t x1 = (hemisphere ? nrings : nrings*2);
	uint32_t x2 = x1+1;
	float du = 0.5f/(float)texres;
	float a  = (1.0f-2.0f*du)/(float)x1;

    // Make the middle of the sphere
    for (y = 0; y < nrings; y++) {
        float y0 = (float)cos(fDAngY0);
        float r0 = (float)sin(fDAngY0);
		float tv = fDAngY0/(float)PI;

        for (x = 0; x < x2; x++) {
            float fDAngX0 = x*fDAng - (float)PI;  // subtract Pi to wrap at +-180�
			if (hemisphere && which_half) fDAngX0 += (float)PI;

			glm::vec3 v = {r0*(float)cos(fDAngX0), y0, r0*(float)sin(fDAngX0)};
			float tu = a*(float)x + du;
			//float tu = x/(float)x1;

            *vtx++ = N2TVERTEX (v, v, tu, tv, tu, tv);
			nvtx++;
        }
        fDAngY0 += fDAng;
    }

    for (y = 0; y < nrings-1; y++) {
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
	glm::vec3 pvy = {0, 1, 0}, nvy = {0,-1,0};
	uint16_t wNorthVtx = nvtx;
    *vtx++ = N2TVERTEX (pvy, pvy, 0.5f, 0.0f, 0.5f, 0.0f);
    nvtx++;
	uint16_t wSouthVtx = nvtx;
    *vtx++ = N2TVERTEX (nvy, nvy, 0.5f, 1.0f, 0.5f, 1.0f);
    nvtx++;

    for (x = 0; x < x1; x++) {
		uint16_t p1 = wSouthVtx;
		uint16_t p2 = (uint16_t)( (y)*x2 + (x+0) );
		uint16_t p3 = (uint16_t)( (y)*x2 + (x+1) );

        *idx++ = p1;
        *idx++ = p3;
        *idx++ = p2;
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


	mesh.vb = new VertexBuffer(Vtx, nVtx*sizeof(N2TVERTEX));

	delete []Vtx;
	mesh.nv  = nVtx;
	mesh.idx = Idx;
	mesh.ib = new IndexBuffer(Idx, nIdx);
	mesh.ni  = nIdx;
	//mesh.bb = 0;
	mesh.vtx = 0;

	mesh.va = new VertexArray();
	mesh.va->Bind();
	mesh.vb->Bind();

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

	mesh.ib->Bind();

	mesh.va->UnBind();
}

// ==============================================================

void CreateSpherePatch (VBMESH &mesh, int nlng, int nlat, int ilat, int res, int bseg,
	bool reduce, bool outside, bool store_vtx, bool shift_origin)
{
	const float c1 = 1.0f, c2 = 0.0f;
	int i, j, nVtx, nIdx, nseg, n, nofs0, nofs1;
	double minlat, maxlat, lat, minlng, maxlng, lng;
	double slat, clat, slng, clng;
	uint16_t tmp;
	VECTOR3 pos, tpos;

	minlat = PI*0.5 * (double)ilat/(double)nlat;
	maxlat = PI*0.5 * (double)(ilat+1)/(double)nlat;
	minlng = 0;
	maxlng = PI*2.0/(double)nlng;
	if (bseg < 0 || ilat == nlat-1) bseg = (nlat-ilat)*res;

	// generate nodes
	nVtx = (bseg+1)*(res+1);
	if (reduce) nVtx -= ((res+1)*res)/2;
	N2TVERTEX *Vtx = new N2TVERTEX[nVtx];

	// create transformation for bounding box
	// we define the local coordinates for the patch so that the x-axis points
	// from (minlng,minlat) corner to (maxlng,minlat) corner (origin is halfway between)
	// y-axis points from local origin to middle between (minlng,maxlat) and (maxlng,maxlat)
	// bounding box is created in this system and then transformed back to planet coords.
	double clat0 = cos(minlat), slat0 = sin(minlat);
	double clng0 = cos(minlng), slng0 = sin(minlng);
	double clat1 = cos(maxlat), slat1 = sin(maxlat);
	double clng1 = cos(maxlng), slng1 = sin(maxlng);
	VECTOR3 ex = {clat0*clng1 - clat0*clng0, 0, clat0*slng1 - clat0*slng0}; normalise(ex);
	VECTOR3 ey = {0.5*(clng0+clng1)*(clat1-clat0), slat1-slat0, 0.5*(slng0+slng1)*(clat1-clat0)}; normalise(ey);
	VECTOR3 ez = crossp (ey, ex);
	MATRIX3 R = {ex.x, ex.y, ex.z,  ey.x, ey.y, ey.z,  ez.x, ez.y, ez.z};
	VECTOR3 pref = {0.5*(clat0*clng1 + clat0*clng0), slat0, 0.5*(clat0*slng1 + clat0*slng0)}; // origin
	VECTOR3 tpmin, tpmax; 

	float dx, dy;
	if (shift_origin) {
		dx = (float)clat0;
		dy = (float)slat0;
	}

	for (i = n = 0; i <= res; i++) {  // loop over longitudinal strips
		lat = minlat + (maxlat-minlat) * (double)i/(double)res;
		slat = sin(lat), clat = cos(lat);
		nseg = (reduce ? bseg-i : bseg);
		for (j = 0; j <= nseg; j++) {
			lng = (nseg ? minlng + (maxlng-minlng) * (double)j/(double)nseg : 0.0);
			slng = sin(lng), clng = cos(lng);
			pos = _V(clat*clng, slat, clat*slng);
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

			Vtx[n].x = Vtx[n].nx = (float)(pos.x);
			Vtx[n].y = Vtx[n].ny = (float)(pos.y);
			Vtx[n].z = Vtx[n].nz = (float)(pos.z);
			if (shift_origin)
				Vtx[n].x -= dx, Vtx[n].y -= dy;

			Vtx[n].tu0 = (float)(nseg ? (c1*j)/nseg+c2 : 0.5f); // overlap to avoid seams
			Vtx[n].tv0 = (float)((c1*(res-i))/res+c2);
			Vtx[n].tu1 = (nseg ? Vtx[n].tu0 * TEX2_MULTIPLIER : 0.5f);
			Vtx[n].tv1 = Vtx[n].tv0 * TEX2_MULTIPLIER;
			if (!outside) {
				Vtx[n].nx = -Vtx[n].nx;
				Vtx[n].ny = -Vtx[n].ny;
				Vtx[n].nz = -Vtx[n].nz;
			}
			n++;
		}
	}

	// generate faces
	nIdx = (reduce ? res * (2*bseg-res) : 2*res*bseg) * 3;
	uint16_t *Idx = new uint16_t[nIdx];

	for (i = n = nofs0 = 0; i < res; i++) {
		nseg = (reduce ? bseg-i : bseg);
		nofs1 = nofs0+nseg+1;
		for (j = 0; j < nseg; j++) {
			Idx[n++] = nofs0+j;
			Idx[n++] = nofs1+j;
			Idx[n++] = nofs0+j+1;
			if (reduce && j == nseg-1) break;
			Idx[n++] = nofs0+j+1;
			Idx[n++] = nofs1+j;
			Idx[n++] = nofs1+j+1;
		}
		nofs0 = nofs1;
	}
	if (!outside)
		for (i = 0; i < nIdx; i += 3) {
			tmp = Idx[i+1];
			Idx[i+1] = Idx[i+2];
			Idx[i+2] = tmp;
		}

	mesh.vb = new VertexBuffer(Vtx, nVtx*sizeof(N2TVERTEX));

	if (store_vtx) {
		mesh.vtx = Vtx;
	} else {
		delete []Vtx;
		mesh.vtx = 0;
	}
	mesh.nv  = nVtx;
	mesh.idx = Idx;
	mesh.ni  = nIdx;
	mesh.ib = new IndexBuffer(Idx, nIdx);



	mesh.va = new VertexArray();
	mesh.va->Bind();
	mesh.vb->Bind();

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

	mesh.ib->Bind();
	mesh.va->UnBind();	
/*
	// set bounding box
	mesh.bb = new VertexBuffer(nullptr, 8 * sizeof(VERTEX_XYZ));
	VERTEX_XYZ *V = (VERTEX_XYZ *)mesh.bb->Map();

	if (shift_origin) {
		pref.x -= dx;
		pref.y -= dy;
	}

	// transform bounding box back to patch coordinates
	pos = tmul (R, _V(tpmin.x, tpmin.y, tpmin.z)) + pref;
	V[0].x = (float)(pos.x); V[0].y = (float)(pos.y); V[0].z = (float)(pos.z);
	pos = tmul (R, _V(tpmax.x, tpmin.y, tpmin.z)) + pref;
	V[1].x = (float)(pos.x); V[1].y = (float)(pos.y); V[1].z = (float)(pos.z);
	pos = tmul (R, _V(tpmin.x, tpmax.y, tpmin.z)) + pref;
	V[2].x = (float)(pos.x); V[2].y = (float)(pos.y); V[2].z = (float)(pos.z);
	pos = tmul (R, _V(tpmax.x, tpmax.y, tpmin.z)) + pref;
	V[3].x = (float)(pos.x); V[3].y = (float)(pos.y); V[3].z = (float)(pos.z);
	pos = tmul (R, _V(tpmin.x, tpmin.y, tpmax.z)) + pref;
	V[4].x = (float)(pos.x); V[4].y = (float)(pos.y); V[4].z = (float)(pos.z);
	pos = tmul (R, _V(tpmax.x, tpmin.y, tpmax.z)) + pref;
	V[5].x = (float)(pos.x); V[5].y = (float)(pos.y); V[5].z = (float)(pos.z);
	pos = tmul (R, _V(tpmin.x, tpmax.y, tpmax.z)) + pref;
	V[6].x = (float)(pos.x); V[6].y = (float)(pos.y); V[6].z = (float)(pos.z);
	pos = tmul (R, _V(tpmax.x, tpmax.y, tpmax.z)) + pref;
	V[7].x = (float)(pos.x); V[7].y = (float)(pos.y); V[7].z = (float)(pos.z);
	mesh.bb->UnMap ();
*/
}

// ==============================================================

void DestroyVBMesh (VBMESH &mesh)
{
	delete mesh.vb;
	delete mesh.va;
	mesh.vb  = 0;
	mesh.va  = 0;
	mesh.nv  = 0;
//	if (mesh.bb) {
//		delete mesh.bb;
//		mesh.bb  = 0;
//	}
	delete []mesh.idx;
	mesh.idx = 0;
	if(mesh.ib)
		delete mesh.ib;
	if (mesh.vtx) {
		delete []mesh.vtx;
		mesh.vtx = 0;
	}
	mesh.ni  = 0;
}

