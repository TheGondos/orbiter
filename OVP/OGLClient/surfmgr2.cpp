// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// surfmgr2.cpp
// class SurfaceManager2 (implementation)
//
// Planetary surface rendering engine v2, including a simple
// LOD (level-of-detail) algorithm for surface patch resolution.
// ==============================================================

#include "surfmgr2.h"
#include "tilelabel.h"
#include "Texture.h"
#include "OGLCamera.h"
#include "Scene.h"
#include "Renderer.h"
#include <cstring>

// =======================================================================

#pragma pack(push,1)

struct ELEVFILEHEADER { // file header for patch elevation data file
	char id[4];            // ID string + version ('E','L','E',1)
	int hdrsize;           // header size (76 expected)
	int dtype;             // data format (0=flat, no data block; 8=uint8; -8=int8; 16=uint16; -16=int16)
	int xgrd,ygrd;         // data grid size (259 x 259 expected)
	int xpad,ypad;         // horizontal, vertical padding width (1, 1 expected)
	double scale;          // data scaling factor (1.0 expected)
	double offset;         // data offset (elevation = raw value * scale + offset)
	double latmin, latmax; // latitude range [rad]
	double lngmin, lngmax; // longitude range [rad]
	double emin, emax, emean; // min, max, mean elevation [m]
};

#pragma pack(pop)

// -----------------------------------------------------------------------

template<>
void TileManager2<SurfTile>::SetSubtreeLabels(QuadTreeNode<SurfTile> *node, bool activate)
{
	if (node->Entry()) {
		if (activate) node->Entry()->CreateLabels();
		else          node->Entry()->DeleteLabels();
	}
	for (int i = 0; i < 4; i++)
		if (node->Child(i))
			SetSubtreeLabels(node->Child(i), activate);
}

// =======================================================================
// Utility functions

static void VtxInterpolate (N2TVERTEX &res, const N2TVERTEX &a, const N2TVERTEX &b, double w)
{
	float w1 = (float)w;
	float w0 = 1.0f-w1;
	res.x = a.x*w0 + b.x*w1;
	res.y = a.y*w0 + b.y*w1;
	res.z = a.z*w0 + b.z*w1;
	res.nx = a.nx*w0 + b.nx*w1;
	res.ny = a.ny*w0 + b.ny*w1;
	res.nz = a.nz*w0 + b.nz*w1;
}

// =======================================================================
// =======================================================================

SurfTile::SurfTile (TileManager2Base *_mgr, int _lvl, int _ilat, int _ilng)
: Tile (_mgr, _lvl, _ilat, _ilng)
{
	smgr = static_cast<TileManager2<SurfTile>* > (_mgr);
	node = 0;
	elev = 0;
	ggelev = 0;
	ltex = 0;
	has_elevfile = false;
	label = 0;
}

// -----------------------------------------------------------------------

SurfTile::~SurfTile ()
{
	if (elev) delete []elev;
	if (ltex && owntex) ltex->Release();
	DeleteLabels();
}

// -----------------------------------------------------------------------

void SurfTile::Load ()
{
	bool bLoadMip = true; // for now
	bool ok;

	int flag = (bLoadMip ? 0:4);
	char path[512];

	// Load surface texture
	ok = false;
	owntex = true;
	if (mgr->Cprm().tileLoadFlags & 0x0001) { // try loading from individual tile file
		sprintf (path, "%s/Surf/%02d/%06d/%06d.dds", mgr->CbodyName(), lvl+4, ilat, ilng);
		ok = (g_client->GetTexMgr()->LoadTextures (path, &tex, flag, 1) != 0);
	}
	if (!ok && smgr->ZTreeManager(0)) { // try loading from compressed archive
		uint8_t *buf;
		int ndata = smgr->ZTreeManager(0)->ReadData(lvl+4, ilat, ilng, &buf);
		if (ndata) {
			ok = g_client->GetTexMgr()->ReadTextureFromMemory (buf, ndata, &tex, flag);
			smgr->ZTreeManager(0)->ReleaseData(buf);
		}
	}
	if (!ok) { // no texture found - interpolate subregion from ancestor
		if (GetParentSubTexRange (&texrange)) {
			tex = getSurfParent()->Tex();
			owntex = false;
		} else {
			tex = 0;
		}
	}

	// Load mask texture
	ltex = NULL;
	if (mgr->Cprm().bSpecular || mgr->Cprm().bLights) {
		if (owntex) {
			ok = false;
			if (mgr->Cprm().tileLoadFlags & 0x0001) { // try loading from individual tile file
				sprintf (path, "%s/Mask/%02d/%06d/%06d.dds", mgr->CbodyName(), lvl+4, ilat, ilng);
				ok = (g_client->GetTexMgr()->LoadTextures (path, &ltex, flag, 1) != 0);
			}
			if (!ok && smgr->ZTreeManager(1)) { // try loading from compressed archive
				uint8_t *buf;
				int ndata = smgr->ZTreeManager(1)->ReadData(lvl+4, ilat, ilng, &buf);
				if (ndata) {
					g_client->GetTexMgr()->ReadTextureFromMemory (buf, ndata, &ltex, flag);
					smgr->ZTreeManager(1)->ReleaseData(buf);
				}
			}
		} else if (node->Parent()) {
			ltex = getSurfParent()->ltex;
		}
	}

	// Load elevation data
	int16_t *elev = ElevationData ();
	bool shift_origin = (lvl >= 4);
	int res = mgr->GridRes();

	if (lvl <= 0) {
		if (!lvl) { // create hemisphere mesh for western or eastern hemispheres
			mesh = CreateMesh_hemisphere (res, elev, 0.0);
		} else {    // create full sphere mesh
			// TODO
		}
	} else {
		// create rectangular patch
		mesh = CreateMesh_quadpatch (res, res, elev, mgr->ElevRes(), 0.0, &texrange, shift_origin, &vtxshift, mgr->GetPlanet()->prm.tilebb_excess);
	}

	static const int label_enable = PLN_ENABLE | PLN_LMARK;
	int plnmode = *(int*)g_client->GetConfigParam (CFGPRM_PLANETARIUMFLAG);
	if ((plnmode & label_enable) == label_enable)
		CreateLabels();
}

// -----------------------------------------------------------------------

int16_t *SurfTile::ReadElevationFile (const char *name, int lvl, int ilat, int ilng, double tgt_res, double *mean_elev)
{
	const int ndat = TILE_ELEVSTRIDE*TILE_ELEVSTRIDE;
	ELEVFILEHEADER hdr;
	int16_t *e = NULL;
	//int16_t ofs;
	double scale, offset;
	char path[256], texpath[256];
	FILE *f;
	int i;

	// Elevation data
	if (mgr->Cprm().tileLoadFlags & 0x0001) { // try loading from individual tile file
		sprintf (path, "%s/Elev/%02d/%06d/%06d.elv", name, lvl, ilat, ilng);
		g_client->PlanetTexturePath(path, texpath);
		if ((f = fopen(texpath, "rb"))) {
			e = new int16_t[ndat];
			// read the elevation file header
			size_t ret = fread (&hdr, sizeof(ELEVFILEHEADER), 1, f);
			if(ret != 1) {
				printf("Error reading elevation file\n");
				assert(false);
				exit(EXIT_FAILURE);
			}
			if (hdr.hdrsize != sizeof(ELEVFILEHEADER)) {
				fseek (f, hdr.hdrsize, SEEK_SET);
			}
			scale  = hdr.scale;
			offset = hdr.offset;
			if (mean_elev) *mean_elev = hdr.emean;
			switch (hdr.dtype) {
			case 0: // flat tile, defined by offset
				for (i = 0; i < ndat; i++) e[i] = 0;
				break;
			case 8: {
				uint8_t *tmp = new uint8_t[ndat];
				ret = fread (tmp, sizeof(uint8_t), ndat, f);
				if(ret != ndat) {
					printf("Error reading elevation file\n");
					assert(false);
					exit(EXIT_FAILURE);
				}
				for (i = 0; i < ndat; i++)
					e[i] = (int16_t)tmp[i];
				delete []tmp;
				}
				break;
			case -16:
				ret = fread (e, sizeof(int16_t), ndat, f);
				if(ret != ndat) {
					printf("Error reading elevation file\n");
					assert(false);
					exit(EXIT_FAILURE);
				}
				break;
			}
			fclose (f);
		}
	}
	if (!e && smgr->ZTreeManager(2)) { // try loading from compressed archive
		uint8_t *buf;
		int ndata = smgr->ZTreeManager(2)->ReadData(lvl, ilat, ilng, &buf);
		if (ndata) {
			uint8_t *p = buf;
			e = new int16_t[ndat];
			ELEVFILEHEADER *phdr = (ELEVFILEHEADER*)p;
			p += phdr->hdrsize;
			if (mean_elev) *mean_elev = phdr->emean;
			scale  = phdr->scale;
			offset = phdr->offset;
			switch (phdr->dtype) {
			case 0:
				for (i = 0; i < ndat; i++) e[i] = 0;
				break;
			case 8:
				for (i = 0; i < ndat; i++)
					e[i] = (int16_t)(*p++);
				break;
			case -16:
				memcpy(e, p, ndat*sizeof(int16_t));
				p += ndat*sizeof(int16_t);
				break;
			}
			smgr->ZTreeManager(2)->ReleaseData(buf);
		}
	}
	if (e) {
		if (scale != tgt_res) { // rescale the data
			double rescale = scale/tgt_res;
			for (i = 0; i < ndat; i++)
				e[i] = (int16_t)(e[i]*rescale);
		}
		if (offset) {
			int16_t sofs = (int16_t)(offset/tgt_res);
			for (i = 0; i < ndat; i++)
				e[i] += sofs;
		}
	}

	// Elevation mod data
	if (e) {
		bool ok = false;
		double rescale;
		int16_t offset;
		bool do_rescale, do_shift;
		if (mgr->Cprm().tileLoadFlags & 0x0001) { // try loading from individual tile file
			sprintf (path, "%s/Elev_mod/%02d/%06d/%06d.elv", name, lvl, ilat, ilng);
			g_client->PlanetTexturePath(path, texpath);
			if ((f = fopen(texpath, "rb"))) {
				size_t ret = fread (&hdr, sizeof(ELEVFILEHEADER), 1, f);
				if(ret != 1) {
					printf("Error reading elevation file\n");
					assert(false);
					exit(EXIT_FAILURE);
				}

				if (hdr.hdrsize != sizeof(ELEVFILEHEADER)) {
					fseek (f, hdr.hdrsize, SEEK_SET);
				}
				rescale = (do_rescale = (hdr.scale != tgt_res)) ? hdr.scale/tgt_res : 1.0;
				offset  = (do_shift   = (hdr.offset != 0.0)) ? (int16_t)(hdr.offset/tgt_res) : 0;
				switch (hdr.dtype) {
				case 0: // overwrite the entire tile with a flat offset
					for (i = 0; i < ndat; i++) e[i] = offset;
					break;
				case 8: {
					const uint8_t mask = UCHAR_MAX;
					uint8_t *tmp = new uint8_t[ndat];
					ret = fread (tmp, sizeof(uint8_t), ndat, f);
					if(ret != ndat) {
						printf("Error reading elevation file\n");
						assert(false);
						exit(EXIT_FAILURE);
					}
					for (i = 0; i < ndat; i++) {
						if (tmp[i] != mask) {
							e[i] = (int16_t)(do_rescale ? (int16_t)(tmp[i]*rescale) : (int16_t)tmp[i]);
							if (do_shift) e[i] += offset;
						}
					}
					delete []tmp;
					}
					break;
				case -16: {
					const int16_t mask = SHRT_MAX;
					int16_t *tmp = new int16_t[ndat];
					//int16_t ofs = (int16_t)hdr.offset;
					ret = fread (tmp, sizeof(int16_t), ndat, f);
					if(ret != ndat) {
						printf("Error reading elevation file\n");
						assert(false);
						exit(EXIT_FAILURE);
					}
					for (i = 0; i < ndat; i++) {
						if (tmp[i] != mask) {
							e[i] = (do_rescale ? (int16_t)(tmp[i]*rescale) : tmp[i]);
							if (do_shift) e[i] += offset;
						}
					}
					delete []tmp;
					}
					break;
				}
				fclose(f);
				ok = true;
			}
		}
		if (!ok && smgr->ZTreeManager(3)) { // try loading from compressed archive
			uint8_t *buf;
			int ndata = smgr->ZTreeManager(3)->ReadData(lvl, ilat, ilng, &buf);
			if (ndata) {
				uint8_t *p = buf;
				ELEVFILEHEADER *phdr = (ELEVFILEHEADER*)p;
				p += phdr->hdrsize;
				rescale = (do_rescale = (phdr->scale != tgt_res)) ? phdr->scale/tgt_res : 1.0;
				offset  = (do_shift   = (phdr->offset != 0.0)) ? (int16_t)(phdr->offset/tgt_res) : 0;
				switch(phdr->dtype) {
				case 0:
					for (i = 0; i < ndat; i++) e[i] = offset;
					break;
				case 8: {
					const uint8_t mask = UCHAR_MAX;
					for (i = 0; i < ndat; i++) {
						if (p[i] != mask) {
							e[i] = (int16_t)(do_rescale ? p[i]*rescale : p[i]);
							if (do_shift) e[i] += offset;
						}
					}
					} break;
				case -16: {
					const int16_t mask = SHRT_MAX;
					int16_t *buf16 = (int16_t*)p;
					for (i = 0; i < ndat; i++) {
						if (buf16[i] != mask) {
							e[i] = (do_rescale ? (int16_t)(buf16[i]*rescale) : buf16[i]);
							if (do_shift) e[i] += offset;
						}
					}
					} break;
				}
				smgr->ZTreeManager(3)->ReleaseData(buf);
			}
		}
	}
	return e;
}

// -----------------------------------------------------------------------

bool SurfTile::LoadElevationData ()
{
	// Note: a tile's elevation data are retrieved from its great-grandparent tile. Each tile stores the elevation
	// data for its area at 8x higher resolution than required by itself, so they can be used by the grandchildren.
	// This reduces the number of elevation files, by storing more information in each. It also has a caching effect:
	// Once a grandparent has loaded its elevation data on request of a grandchild, any other grandchildren's requests
	// can be served directly without further disk I/O.

	if (elev) return true; // already present

	has_elevfile = false;
	int mode = mgr->Cprm().elevMode;
	if (!mode) return false;

	int ndat = TILE_ELEVSTRIDE*TILE_ELEVSTRIDE;
	elev = ReadElevationFile (mgr->CbodyName(), lvl+4, ilat, ilng, mgr->ElevRes());
	if (elev) {

		has_elevfile = true;

	} else if (lvl > 0) {

		// construct elevation grid by interpolating ancestor data
		ELEVHANDLE hElev = mgr->ElevMgr();
		if (!hElev) return false;
		int plvl = lvl-1;
		int pilat = ilat >> 1;
		int pilng = ilng >> 1;
		int16_t *pelev = 0;
		QuadTreeNode<SurfTile> *parent = node->Parent();
		for (; plvl >= 0; plvl--) { // find ancestor with elevation data
			if (parent && parent->Entry()->has_elevfile) {
				pelev = parent->Entry()->elev;
				break;
			}
			parent = parent->Parent();
			pilat >>= 1;
			pilng >>= 1;
		}
		if (!pelev) return false;
		elev = new int16_t[ndat];
		// submit ancestor data to elevation manager for interpolation
		g_client->ElevationGrid (hElev, ilat, ilng, lvl, pilat, pilng, plvl, pelev, elev);

	}
	return (elev != 0);
}

// -----------------------------------------------------------------------

int16_t *SurfTile::ElevationData () const
{
	if (!ggelev) {
		int ancestor_dlvl = 3;
		while (1 << (8-ancestor_dlvl) < mgr->GridRes()) ancestor_dlvl--;
		if (lvl >= ancestor_dlvl) { // traverse quadtree back to great-grandparent
			QuadTreeNode<SurfTile> *ancestor = node; // start at my own node
			int blockRes = TILE_FILERES;
			while (blockRes > mgr->GridRes() && ancestor) {
				ancestor = ancestor->Parent();
				blockRes >>= 1;
			}
			if (ancestor && ancestor->Entry()->LoadElevationData()) {
				// compute pixel offset into great-grandparent tile set
				int nblock = TILE_FILERES/blockRes;
				int mask = nblock-1;
				int ofs = (((mask - ilat) & mask) * TILE_ELEVSTRIDE + (ilng & mask)) * blockRes;
				ggelev = ancestor->Entry()->elev + ofs;
			}
		} else {
			SurfTile *ggp = smgr->GlobalTile(lvl-ancestor_dlvl);
			if (ggp && ggp->LoadElevationData ()) {
				int blockRes = mgr->GridRes();
				int nblock = TILE_FILERES/blockRes;
				int mask = nblock-1;
				int ofs = (((mask - ilat) & mask) * TILE_ELEVSTRIDE + (ilng & mask)) * blockRes;
				ggelev = ggp->elev + ofs;
			}
		}
		if (ggelev)
			mean_elev = GetMeanElevation (ggelev);
	}
	return ggelev;
}

// -----------------------------------------------------------------------

double SurfTile::GetMeanElevation (const int16_t *elev) const
{
	int i, j;
	int res = mgr->GridRes();
	double melev = 0.0;
	for (j = 0; j <= res; j++) {
		for (i = 0; i <= res; i++) {
			melev += elev[i];
		}
		elev += TILE_ELEVSTRIDE;
	}
	return melev / ((res+1)*(res+1));	
}

// -----------------------------------------------------------------------

void SurfTile::Render ()
{
	//bool render_lights = mgr->Cprm().bLights;
	//bool render_shadows = false;//(mgr->GetPlanet()->CloudMgr2() && !mgr->prm.rprm->bCloudFlatShadows);

	//static float spec_base = 0.4f; // 0.95f;
	//static D3DMATERIAL7 pmat, watermat = {{1,1,1,1},{1,1,1,1},{spec_base*1.2f,spec_base*1.0f,spec_base*0.8f,1},{0,0,0,0},50.0f};

	if (!mesh) return; // DEBUG : TEMPORARY
	if(!mesh->VAO) {
		if (state & TILE_VALID) {
			mesh->Upload();
		} else
			return;
	}
//	if(!tex) return;
//	TileManager2Base::Dev()->SetTexture (0, tex);

/*
	// add atmospheric tint to surface textures
	if (mgr->prm.tint) {
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(mgr->prm.atm_tint.x, mgr->prm.atm_tint.y, mgr->prm.atm_tint.z, 1));
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_ADD);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	}
*/
/*
	LPDIRECT3DVERTEXBUFFER7 vb = mesh->vb;        // processed vertex buffer
	TileManager2Base::Dev()->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, vb, 0,
		mesh->nv, mesh->idx, mesh->ni, 0);
*/

	static Shader s("Tile.vs","Tile.fs");
	
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	s.Bind();
	OGLCamera *c = g_client->GetScene()->GetCamera();
	auto *vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection",*vp);
	s.SetMat4("u_Model",smgr->wtrans);
	const VECTOR3 &sd = g_client->GetScene()->GetSunDir();
	glm::vec3 sundir;
	sundir.x = sd.x;
	sundir.y = sd.y;
	sundir.z = sd.z;
	s.SetVec3("u_SunDir", sundir);

	auto *view = c->GetViewMatrix();

	s.SetMat4("u_View", *view);
	if(mgr->prm.fog && g_client->mRenderContext.bFog) {
		glm::vec4 fogColor;
		fogColor.x = g_client->mRenderContext.fogColor.x;
		fogColor.y = g_client->mRenderContext.fogColor.y;
		fogColor.z = g_client->mRenderContext.fogColor.z;
		s.SetVec4("u_FogColor", fogColor);
		s.SetFloat("u_FogDensity", g_client->mRenderContext.fogDensity);
	} else {
		s.SetFloat("u_FogDensity", 0);
	}

	if(mgr->prm.rprm->bAddBkg) {
		VECTOR3 v3bgcol = g_client->GetScene()->SkyColour();
		glm::vec3 bgcol;
		bgcol.x = v3bgcol.x;
		bgcol.y = v3bgcol.y;
		bgcol.z = v3bgcol.z;
		s.SetVec3("u_bgcol", bgcol);
	} else {
		glm::vec3 bgcol(0,0,0);
		s.SetVec3("u_bgcol", bgcol);
	}

    glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);
	Renderer::CheckError("SurfTile::Render glBindTexture");
	mesh->VAO->Bind();

	glDrawElements(GL_TRIANGLES, mesh->IBO->GetCount(), GL_UNSIGNED_SHORT, 0);
	Renderer::CheckError("SurfTile::Render glDrawElements");
	mesh->VAO->UnBind();
	s.UnBind();
    glBindTexture(GL_TEXTURE_2D,  0);
	Renderer::CheckError("SurfTile::Render glBindTexture0");
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	return;


/*
	static const double rad0 = sqrt(2.0)*PI05;
	double sdist, rad;
	bool has_specular = false;
	bool has_shadows = false;
	bool has_lights = false;
	if (ltex || render_shadows) {
		sdist = acos (dotp (mgr->prm.sdir, cnt));
		rad = rad0/(double)(2<<lvl); // tile radius
		has_specular = (ltex && sdist < PI05+rad);
		has_shadows = (render_shadows && sdist < PI05+rad);
		has_lights = (render_lights && ltex && sdist > 1.45);
	}*/
/*
	// render specular water reflections
	if (has_specular) {
		TileManager2Base::Dev()->GetMaterial (&pmat);
		TileManager2Base::Dev()->SetMaterial (&watermat);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, TRUE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

		int dns;
		if (mgr->prm.fog) {
			// increase fog density to simulate sky reflection on water surface
			TileManager2Base::Dev()->GetRenderState (D3DRENDERSTATE_FOGDENSITY, &dns);
			float fFogDns = *((float*)&dns) * (float)(1.0 + 3.0*exp(-4e2*(mgr->prm.cdist-1.0)));
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGDENSITY, *((int*) (&fFogDns)));
		}

		// use water mask to limit specular reflection to water surfaces
		TileManager2Base::Dev()->SetTexture (1, ltex);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE|D3DTA_COMPLEMENT); // need to invert alpha channel of mask
		if (!mgr->prm.tint) { // just pass colour through this stage
			TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_CURRENT);
		}
		TileManager2Base::Dev()->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, vb, 0,
			mesh->nv, mesh->idx, mesh->ni, 0);

		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		TileManager2Base::Dev()->SetTexture (1, 0);
		if (mgr->prm.fog)
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGDENSITY, dns);
		TileManager2Base::Dev()->SetMaterial (&pmat);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	}*/
/*
	if (mgr->prm.tint) {
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	}*/
/*
	// add cloud shadows
	if (has_shadows) {
		const TileManager2<CloudTile> *cmgr = mgr->GetPlanet()->CloudMgr2();
		int nlng = 2 << lvl;
		int nlat = 1 << lvl;
		double minlat = PI * (double)(nlat/2-ilat-1)/(double)nlat;
		double maxlat = PI * (double)(nlat/2-ilat)/(double)nlat;
		double minlng = PI2 * (double)(ilng-nlng/2)/(double)nlng;
		double maxlng = PI2 * (double)(ilng-nlng/2+1)/(double)nlng;

		const Tile *tbuf[2];
		int maxlvl = std::min(lvl,9);
		int ncloud = cmgr->Coverage(minlat, maxlat, minlng, maxlng, maxlvl, tbuf, 2);
		if (ncloud > 0) {
			float alpha = (float)mgr->prm.rprm->shadowalpha;
			if (alpha >= 0.01f) { // otherwise don't render cloud shadows for this planet

				if (ncloud == 1) { // other cases still to be done
					mgr->Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

					// set the shadow colour and opacity
					mgr->Dev()->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, mgr->prm.tint ?
						D3DRGBA(mgr->prm.atm_tint.x, mgr->prm.atm_tint.y, mgr->prm.atm_tint.z, alpha) :
					    D3DRGBA(0, 0, 0, alpha));

					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
					TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

					for (int i = 0; i < ncloud; i++) {
						mgr->Dev()->SetTexture (0, tbuf[i]->Tex());

						// to be completed: create new mesh with modified texture coordinates
						const TEXCRDRANGE2 *ctexrange = tbuf[i]->GetTexRange();
						double cminlat, cmaxlat, cminlng, cmaxlng;
						float tu0, tu1, tv0, tv1, tuscale, tvscale;
						tbuf[i]->Extents (&cminlat, &cmaxlat, &cminlng, &cmaxlng);

						cminlng -= mgr->prm.rprm->cloudrot;
						cmaxlng -= mgr->prm.rprm->cloudrot;
						if (cmaxlng < -PI) {
							cminlng += PI2;
							cmaxlng += PI2;
						}

						static int ncvtx = (mgr->GridRes()+3)*(mgr->GridRes()+3);
						static N2TVERTEX *cvtx = new N2TVERTEX[ncvtx];
						if (mesh->nv > ncvtx) {
							delete []cvtx;
							cvtx = new N2TVERTEX[ncvtx=mesh->nv];
						}
						memcpy(cvtx, mesh->vtx, mesh->nv*sizeof(N2TVERTEX));
						tu0 = ctexrange->tumin + (ctexrange->tumax-ctexrange->tumin)*(float)((minlng-cminlng)/(cmaxlng-cminlng));
						tu1 = ctexrange->tumin + (ctexrange->tumax-ctexrange->tumin)*(float)((maxlng-cminlng)/(cmaxlng-cminlng));
						tv0 = ctexrange->tvmin + (ctexrange->tvmax-ctexrange->tvmin)*(float)((maxlat-cmaxlat)/(cminlat-cmaxlat));
						tv1	= ctexrange->tvmin + (ctexrange->tvmax-ctexrange->tvmin)*(float)((minlat-cmaxlat)/(cminlat-cmaxlat));
						tuscale = (tu1-tu0)/(texrange.tumax-texrange.tumin);
						tvscale = (tv1-tv0)/(texrange.tvmax-texrange.tvmin);
						for (int j = 0; j < mesh->nv; j++) {
							cvtx[j].tu0 = tu0 + tuscale * (cvtx[j].tu0-texrange.tumin);
							cvtx[j].tv0 = tv0 + tvscale * (cvtx[j].tv0-texrange.tvmin);
						}

						mgr->Dev()->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, FVF_2TEX, cvtx, mesh->nv, mesh->idx, mesh->ni, 0);
					}

					mgr->Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
					mgr->Dev()->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					mgr->Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_CURRENT);
					mgr->Dev()->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					mgr->Dev()->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
				}

			}
		}
	}
	*/
/*
	// add city lights
	if (has_lights) {
		// disable fog
		if (mgr->prm.fog) {
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGENABLE, FALSE);
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
		}

		double fac = mgr->Cprm().lightfac;
		if (sdist < 1.9) fac *= (sdist-1.45)/(1.9-1.45);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(fac,fac,fac,1));
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		TileManager2Base::Dev()->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		TileManager2Base::Dev()->SetTexture (0, ltex);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
		TileManager2Base::Dev()->DrawIndexedPrimitiveVB (D3DPT_TRIANGLELIST, vb, 0,
			mesh->nv, mesh->idx, mesh->ni, 0);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

		// turn fog back on
		if (mgr->prm.fog) {
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGENABLE, TRUE);
			TileManager2Base::Dev()->SetRenderState (D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP);
		}
	}

	// reset the atmospheric tint
	TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	TileManager2Base::Dev()->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_CURRENT);*/
}

// -----------------------------------------------------------------------

void SurfTile::MatchEdges ()
{
	if (edgeok) return; // done already
	edgeok = true;
	if (!mesh) return;  // sanity check

	QuadTreeNode<SurfTile> *lngnbr = smgr->FindNode (lvl, ilng + (ilng & 1 ? 1 : -1), ilat);
	QuadTreeNode<SurfTile> *latnbr = smgr->FindNode (lvl, ilng, ilat + (ilat & 1 ? 1 : -1));
	QuadTreeNode<SurfTile> *dianbr = smgr->FindNode (lvl, ilng + (ilng & 1 ? 1 : -1), ilat + (ilat & 1 ? 1 : -1));

	if (lngnbr && !(lngnbr->Entry()->state & TILE_VALID)) lngnbr = 0;
	if (latnbr && !(latnbr->Entry()->state & TILE_VALID)) latnbr = 0;
	if (dianbr && !(dianbr->Entry()->state & TILE_VALID)) dianbr = 0;

	int new_lngnbr_lvl = (lngnbr ? lngnbr->Entry()->lvl : lvl);
	int new_latnbr_lvl = (latnbr ? latnbr->Entry()->lvl : lvl);
	int new_dianbr_lvl = (dianbr ? dianbr->Entry()->lvl : lvl);

	// fix lower-level neighbours first
	bool nbr_updated = false;
	if (new_lngnbr_lvl < lvl) {
		lngnbr->Entry()->MatchEdges ();
		nbr_updated = true;
	}
	if (new_latnbr_lvl < lvl) {
		latnbr->Entry()->MatchEdges ();
		nbr_updated = true;
	}
	if (!nbr_updated && new_dianbr_lvl < lvl) {
		// now we need to check the diagonal neighbour, since this could be lower resolution
		// and thus responsible for the corner node elevation
		dianbr->Entry()->MatchEdges ();
	}

	bool lngedge_changed = (new_lngnbr_lvl != lngnbr_lvl);
	bool latedge_changed = (new_latnbr_lvl != latnbr_lvl);
	bool diaedge_changed = (new_dianbr_lvl != dianbr_lvl);

	if (lngedge_changed || latedge_changed || diaedge_changed) {
		if (new_dianbr_lvl < new_lngnbr_lvl && new_dianbr_lvl < new_latnbr_lvl) {
			FixCorner (dianbr ? dianbr->Entry() : 0);
			FixLatitudeBoundary (latnbr ? latnbr->Entry() : 0, true);
			FixLongitudeBoundary (lngnbr ? lngnbr->Entry() : 0, true);
		} else if (new_latnbr_lvl < new_lngnbr_lvl) {
			FixLatitudeBoundary (latnbr ? latnbr->Entry() : 0);
			FixLongitudeBoundary (lngnbr ? lngnbr->Entry() : 0, true);
		} else {
			FixLongitudeBoundary (lngnbr ? lngnbr->Entry() : 0);
			FixLatitudeBoundary (latnbr ? latnbr->Entry() : 0, true);
		}
		lngnbr_lvl = new_lngnbr_lvl;
		latnbr_lvl = new_latnbr_lvl;
		dianbr_lvl = new_dianbr_lvl;
	}
}

// -----------------------------------------------------------------------

void SurfTile::FixCorner (const SurfTile *nbr)
{
	if (!mesh) return; // sanity check

	int res = mgr->GridRes();
	int vtx_idx = (ilat & 1 ? 0 : (res+1)*res) + (ilng & 1 ? res : 0);
	N2TVERTEX &vtx_store = mesh->vtx[mesh->nv + (ilat & 1 ? 0 : res)];

	int16_t *elev = ElevationData();
	if (!elev) return;

	if (nbr) {
		int16_t *nbr_elev = nbr->ElevationData();
		if (!nbr_elev) return;

		int16_t corner_elev = elev[TILE_ELEVSTRIDE+1 + (ilat & 1 ? 0 : TILE_ELEVSTRIDE*res) + (ilng & 1 ? res : 0)];
		int16_t nbr_corner_elev = nbr_elev[TILE_ELEVSTRIDE+1 + (ilat & 1 ? TILE_ELEVSTRIDE*res : 0) + (ilng & 1 ? 0 : res)];
	
		double rad = mgr->CbodySize();
		double radfac = (rad+nbr_corner_elev)/(rad+corner_elev);
		mesh->vtx[vtx_idx].x = (float)(vtx_store.x*radfac + vtxshift.x*(radfac-1.0));
		mesh->vtx[vtx_idx].y = (float)(vtx_store.y*radfac + vtxshift.y*(radfac-1.0));
		mesh->vtx[vtx_idx].z = (float)(vtx_store.z*radfac + vtxshift.z*(radfac-1.0));
		mesh->Upload();
	}
}

// -----------------------------------------------------------------------

void SurfTile::FixLongitudeBoundary (const SurfTile *nbr, bool keep_corner)
{
	// Fix the left or right edge
	int i, i0, i1, j, vtx_ofs, nbrlvl, dlvl;
	int res = mgr->GridRes();
	N2TVERTEX *vtx_store;

	vtx_ofs = (ilng & 1 ? res : 0); // check if neighbour is at left or right edge
	if (nbr && nbr->mesh && nbr->mesh->vtx) {
		nbrlvl = std::min(nbr->lvl, lvl); // we don't need to worry about neigbour levels higher than ours
		vtx_store = mesh->vtx + mesh->nv;
		if (nbrlvl == lvl) { // put my own edge back
			i0 = 0;
			i1 = res;
			if (keep_corner) {
				if (ilat & 1)
					i0++;
				else
					i1--;
			}
			for (i = i0; i <= i1; i++)
				mesh->vtx[vtx_ofs+i*(res+1)] = vtx_store[i];
		} else {  // interpolate to neighbour's left edge
			dlvl = lvl-nbrlvl;
			if (dlvl <= 5) { // for larger tile level differences the interleaved sampling method doesn't work
				int16_t *elev = ElevationData();
				int16_t *nbr_elev = nbr->ElevationData();
				if (elev && nbr_elev) {
					elev += TILE_ELEVSTRIDE+1 + vtx_ofs; // add offset
					int nsub = 1 << dlvl;    // number of tiles fitting alongside the lowres neighbour
					int vtx_skip = nsub*(res+1);  // interleave factor for nodes attaching to neigbour nodes
					int nbr_range = res/nsub; // number of neighbour vertices attaching to us - 1
					int subidxmask = (1<<dlvl)-1;
					int subidx = ilat & subidxmask;
					int nbr_ofs = (subidxmask-subidx) * nbr_range * TILE_ELEVSTRIDE;
					nbr_elev += TILE_ELEVSTRIDE+1 + nbr_ofs + (res-vtx_ofs);
					double rad = mgr->CbodySize();
					// match nodes to neighbour elevations
					i0 = 0;
					i1 = nbr_range;
					if (keep_corner) {
						if (ilat & 1)
							i0++;
						else
							i1--;
					}
					for (i = i0; i <= i1; i++) {
						double radfac = (rad+nbr_elev[i*TILE_ELEVSTRIDE])/(rad+elev[i*TILE_ELEVSTRIDE*nsub]);
						mesh->vtx[vtx_ofs+i*vtx_skip].x = (float)(vtx_store[i*nsub].x*radfac + vtxshift.x*(radfac-1.0));
						mesh->vtx[vtx_ofs+i*vtx_skip].y = (float)(vtx_store[i*nsub].y*radfac + vtxshift.y*(radfac-1.0));
						mesh->vtx[vtx_ofs+i*vtx_skip].z = (float)(vtx_store[i*nsub].z*radfac + vtxshift.z*(radfac-1.0));
					}
					// interpolate the nodes that fall between neighbour nodes
					for (i = 0; i < nbr_range; i++)
						for (j = 1; j < nsub; j++)
							VtxInterpolate (mesh->vtx[vtx_ofs+j*(res+1)+i*vtx_skip], mesh->vtx[vtx_ofs+i*vtx_skip], mesh->vtx[vtx_ofs+(i+1)*vtx_skip], (double)j/double(nsub));
				}
			} else {
				// problems
			}
		}
		mesh->Upload();
	}
}

// -----------------------------------------------------------------------

void SurfTile::FixLatitudeBoundary (const SurfTile *nbr, bool keep_corner)
{
	// Fix the top or bottom edge
	int i, i0, i1, j, nbrlvl, dlvl;
	int res = mgr->GridRes();
	N2TVERTEX *vtx_store;

	int line = (ilat & 1 ? 0 : res);
	int vtx_ofs = (ilat & 1 ? 0 : line*(res+1));
	if (nbr && nbr->mesh && nbr->mesh->vtx) {
		nbrlvl = std::min(nbr->lvl, lvl); // we don't need to worry about neigbour levels higher than ours
		vtx_store = mesh->vtx + mesh->nv + res + 1;
		if (nbrlvl == lvl) { // put my own edge back
			i0 = 0;
			i1 = res;
			if (keep_corner) {
				if (ilng & 1)
					i1--;
				else
					i0++;
			}
			for (i = i0; i <= i1; i++)
				mesh->vtx[vtx_ofs+i] = vtx_store[i];
		} else {
			dlvl = lvl-nbrlvl;
			if (dlvl <= 5) { // for larger tile level differences the interleaved sampling method doesn't work
				int16_t *elev = ElevationData();
				int16_t *nbr_elev = nbr->ElevationData();
				if (elev && nbr_elev) {
					elev += (line+1)*TILE_ELEVSTRIDE + 1; // add offset
					int nsub = 1 << dlvl;    // number of tiles fitting alongside the lowres neighbour
					int vtx_skip = nsub;     // interleave factor for nodes attaching to neigbour nodes
					int nbr_range = res/nsub; // number of neighbour vertices attaching to us - 1
					int subidxmask = (1<<dlvl)-1;
					int subidx = ilng & subidxmask;
					int nbr_ofs = subidx * nbr_range;
					nbr_elev += TILE_ELEVSTRIDE+1 + nbr_ofs + (res-line)*TILE_ELEVSTRIDE;
					double rad = mgr->CbodySize();
					// match nodes to neighbour elevations
					i0 = 0;
					i1 = nbr_range;
					if (keep_corner) {
						if (ilng & 1)
							i1--;
						else
							i0++;
					}
					for (i = i0; i <= i1; i++) {
						double radfac = (rad+nbr_elev[i])/(rad+elev[i*nsub]);
						mesh->vtx[vtx_ofs+i*vtx_skip].x = (float)(vtx_store[i*nsub].x*radfac + vtxshift.x*(radfac-1.0));
						mesh->vtx[vtx_ofs+i*vtx_skip].y = (float)(vtx_store[i*nsub].y*radfac + vtxshift.y*(radfac-1.0));
						mesh->vtx[vtx_ofs+i*vtx_skip].z = (float)(vtx_store[i*nsub].z*radfac + vtxshift.z*(radfac-1.0));
					}
					// interpolate the nodes that fall between neighbour nodes
					for (i = 0; i < nbr_range; i++)
						for (j = 1; j < nsub; j++)
							VtxInterpolate (mesh->vtx[vtx_ofs+i*vtx_skip+j], mesh->vtx[vtx_ofs+i*vtx_skip], mesh->vtx[vtx_ofs+(i+1)*vtx_skip], (double)j/double(nsub));
				}
			} else {
				// problems
			}
		}
		mesh->Upload();
	}
}

// -----------------------------------------------------------------------

void SurfTile::CreateLabels()
{
	if (!label) label = TileLabel::Create(this);
}

// -----------------------------------------------------------------------

void SurfTile::RenderLabels (oapi::Sketchpad *skp, oapi::Font **labelfont, int *fontidx)
{
	if (!label) return;
	label->Render(skp, labelfont, fontidx);
}

// =======================================================================
// =======================================================================

template<>
void TileManager2<SurfTile>::Render (MATRIX4 &dwmat, bool use_zbuf, const vPlanet::RenderPrm &rprm)
{
	// set generic parameters
	SetRenderPrm (dwmat, 0, use_zbuf, rprm);

	double np = 0.0, fp = 0.0;
	int i;
	OGLCamera *camera = g_client->GetScene()->GetCamera();

	// adjust scaling parameters (can only be done if no z-buffering is in use)
	if (!use_zbuf) {
		double R = obj_size;
		double D = prm.cdist*R;
		double zmax = (D - R*R/D) * 1.5;
		double zmin = std::max (2.0, std::min (zmax*1e-4, (D-R) * 0.8));

//	zmax = zmax * 2.0 - 1.0;
//	zmin = zmin * 2.0 - 1.0;

		//double zscale = 1.0;

		np = camera->GetNearlimit();
		fp = camera->GetFarlimit();
		camera->SetFrustumLimits (zmin, zmax);
	}

	if (rprm.bAddBkg)
		prm.tint = false; // the two effects are not currently compatible

	// build a transformation matrix for frustum testing
	/*
	MATRIX4 Mproj = camera->ProjectionMatrix();
	Mproj.m33 = 1.0; Mproj.m43 = -1.0;  // adjust near plane to 1, far plane to infinity
	MATRIX4 Mview = camera->ViewMatrix();
	prm.dviewproj = mul(Mview,Mproj);
*/

	glm::mat4 Mproj = *camera->GetProjectionMatrix();
	Mproj[2][2] = 1.0; Mproj[3][2] = -1.0;  // adjust near plane to 1, far plane to infinity
	glm::mat4 Mview = *camera->GetViewMatrix();
	//prm.dviewproj = mul(Mview,Mproj);

	glm::mat4 vpf = Mview * Mproj ;
	vpf = *camera->GetViewProjectionMatrix();
	MATRIX4 vp;

	vp.m11 = vpf[0][0];
	vp.m12 = vpf[0][1];
	vp.m13 = vpf[0][2];
	vp.m14 = vpf[0][3];
	vp.m21 = vpf[1][0];
	vp.m22 = vpf[1][1];
	vp.m23 = vpf[1][2];
	vp.m24 = vpf[1][3];
	vp.m31 = vpf[2][0];
	vp.m32 = vpf[2][1];
	vp.m33 = vpf[2][2];
	vp.m34 = vpf[2][3];
	vp.m41 = vpf[3][0];
	vp.m42 = vpf[3][1];
	vp.m43 = vpf[3][2];
	vp.m44 = vpf[3][3];


	prm.dviewproj = vp;

	// TODO: render full sphere for levels < 4

	//loader->WaitForMutex();

	// update the tree
	for (i = 0; i < 2; i++)
		ProcessNode (tiletree+i);

	// render the tree
	//for (i = 0; i < 2; i++)
	//	Dev()->SetTextureStageState (i, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	for (i = 0; i < 2; i++)
		RenderNode (tiletree+i);

	//for (i = 0; i < 2; i++)
	//	Dev()->SetTextureStageState (i, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	//loader->ReleaseMutex ();

	if (!use_zbuf)
		camera->SetFrustumLimits(np,fp);
}

// -----------------------------------------------------------------------

template<>
void TileManager2<SurfTile>::RenderLabels (oapi::Sketchpad *skp, oapi::Font **labelfont, int *fontidx)
{
	for (int i = 0; i < 2; i++)
		RenderNodeLabels (tiletree+i, skp, labelfont, fontidx);
}

// -----------------------------------------------------------------------

template<>
void TileManager2<SurfTile>::CreateLabels()
{
	loader->WaitForMutex();

	for (int i = 0; i < 2; i++)
		SetSubtreeLabels(tiletree+i, true);

	loader->ReleaseMutex();
}

// -----------------------------------------------------------------------

template<>
void TileManager2<SurfTile>::DeleteLabels()
{
	loader->WaitForMutex();

	for (int i = 0; i < 2; i++)
		SetSubtreeLabels(tiletree+i, false);

	loader->ReleaseMutex();
}

// -----------------------------------------------------------------------

template<>
void TileManager2<SurfTile>::LoadZTrees()
{
	treeMgr = new ZTreeMgr*[ntreeMgr = 5];
	if (cprm.tileLoadFlags & 0x0002) {
		char cbuf[256];
		g_client->PlanetTexturePath (CbodyName(), cbuf);
		treeMgr[0] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_SURF);
		treeMgr[1] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_MASK);
		treeMgr[2] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_ELEV);
		treeMgr[3] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_ELEVMOD);
		treeMgr[4] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_LABEL);
	} else {
		for (int i = 0; i < ntreeMgr; i++)
			treeMgr[i] = 0;
	}
}
