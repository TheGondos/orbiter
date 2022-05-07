// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "elevmgr.h"
#include "Celbody.h"
#include "Planet.h"
#include "Orbiter.h"

static int elev_grid = 256;
static int elev_stride = elev_grid+3;
static int MAXLVL_LIMIT = 11;

extern Orbiter *g_pOrbiter;
extern TimeData td;
extern char DBG_MSG[256];

#pragma pack(push,1)

struct ELEVFILEHEADER { // file header for patch elevation data file
	char id[4];            // ID string + version ('E','L','E',1)
	int hdrsize;           // header size (100 expected)
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

ElevationManager::ElevationManager (const CelestialBody *_cbody)
: cbody(_cbody)
{
	mode = g_pOrbiter->Cfg()->CfgVisualPrm.ElevMode;
	tilesource = g_pOrbiter->Cfg()->CfgPRenderPrm.TileLoadFlags;
	maxlvl = MAXLVL_LIMIT;
	elev_res = 1.0;
	if (cbody->Type() == OBJTP_PLANET) {
		maxlvl = std::min (maxlvl, (int)(((Planet*)cbody)->MaxPatchLevel()-7));
		// -7: -4 for level offset of quadtree root, -3 for great-grandfather elevation access mode
		elev_res = ((Planet*)cbody)->ElevationResolution();
	}
	if (tilesource & 0x0002) {
		char cbuf[256];
		g_pOrbiter->Cfg()->PTexPath (cbuf, cbody->Name());
		treeMgr[0] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_ELEV);
		treeMgr[1] = ZTreeMgr::CreateFromFile(cbuf, ZTreeMgr::LAYER_ELEVMOD);
	} else {
		for (int i = 0; i < 2; i++)
			treeMgr[i] = 0;
	}
}

ElevationManager::~ElevationManager ()
{
	for (int i = 0; i < 2; i++)
		if (treeMgr[i])
			delete treeMgr[i];
}

bool ElevationManager::TileIdx (double lat, double lng, int lvl, int *ilat, int *ilng) const
{
	int nlat = 1 << lvl;
	int nlng = 2 << lvl;

	*ilat = (int)((Pi05-lat)/Pi * nlat);
	*ilng = (int)((lng+Pi)/Pi2 * nlng);
	return true;
}

int16_t *ElevationManager::LoadElevationTile (int lvl, int ilat, int ilng, double tgt_res) const
{
	int16_t *elev = 0;
	//int16_t ofs;

	if (mode) {
		int i;
		const int ndat = elev_stride*elev_stride;
		double scale, offset;
		if (tilesource & 0x0001) {
			FILE *f;
			char fname[256], path[256];
			sprintf (fname, "%s\\Elev\\%02d\\%06d\\%06d.elv", cbody->Name(), lvl, ilat, ilng);
			g_pOrbiter->Cfg()->PTexPath(path, fname);
			if ((f = fopen(path, "rb"))) {
				elev = new int16_t[ndat];
				ELEVFILEHEADER hdr;
				size_t ret = fread (&hdr, sizeof(ELEVFILEHEADER), 1, f);
				if(ret != sizeof(ELEVFILEHEADER)) {
					printf("Error reading elevation tile\n");
					exit(EXIT_FAILURE);
				}
				if (hdr.hdrsize != sizeof(ELEVFILEHEADER)) {
					fseek (f, hdr.hdrsize, SEEK_SET);
				}
				scale  = hdr.scale;
				offset = hdr.offset;
				switch (hdr.dtype) {
				case 0: // flat tile, defined by offset
					for (i = 0; i < ndat; i++) elev[i] = 0;
					break;
				case 8: {
					uint8_t *tmp = new uint8_t[ndat];
					ret = fread (tmp, sizeof(uint8_t), ndat, f);
					if(ret != sizeof(uint8_t) * ndat) {
						printf("Error reading elevation tile\n");
						exit(EXIT_FAILURE);
					}
					for (i = 0; i < ndat; i++)
						elev[i] = (int16_t)tmp[i];
					delete []tmp;
					}
					break;
				case -16:
					ret = fread (elev, sizeof(int16_t), ndat, f);
					if(ret != sizeof(int16_t) * ndat) {
						printf("Error reading elevation tile\n");
						exit(EXIT_FAILURE);
					}
					break;
				}
				fclose(f);
			}
		}
		if (!elev && treeMgr[0]) {
			uint8_t *buf;
			int ndata = treeMgr[0]->ReadData(lvl, ilat, ilng, &buf);
			if (ndata) {
				uint8_t *p = buf;
				elev = new int16_t[ndat];
				ELEVFILEHEADER *phdr = (ELEVFILEHEADER*)p;
				p += phdr->hdrsize;
				scale  = phdr->scale;
				offset = phdr->offset;
				switch (phdr->dtype) {
				case 0: // flat tile, defined by offset
					for (i = 0; i < ndat; i++) elev[i] = 0;
					break;
				case 8:
					for (i = 0; i < ndat; i++)
						elev[i] = (int16_t)(*p++);
					break;
				case -16:
					memcpy(elev, p, ndat*sizeof(int16_t));
					p += ndat*sizeof(int16_t);
					break;
				}
				treeMgr[0]->ReleaseData(buf);
			}
		}
		if (elev) {
			if (scale != tgt_res) { // rescale the data
				double rescale = scale/tgt_res;
				for (i = 0; i < ndat; i++)
					elev[i] = (int16_t)(elev[i]*rescale);
			}
			if (offset) {
				int16_t sofs = (int16_t)(offset/tgt_res);
				for (i = 0; i < ndat; i++)
					elev[i] += sofs;
			}
		}
	}
	return elev;
}

bool ElevationManager::LoadElevationTile_mod (int lvl, int ilat, int ilng, double tgt_res, int16_t *elev) const
{
	if (mode) {
		int i;
		const int ndat = elev_stride*elev_stride;
		double rescale;
		int16_t offset;
		bool do_shift, do_rescale;
		if (tilesource & 0x0001) {
			FILE *f;
			char fname[256], path[256];
			sprintf (fname, "%s\\Elev_mod\\%02d\\%06d\\%06d.elv", cbody->Name(), lvl, ilat, ilng);
			g_pOrbiter->Cfg()->PTexPath(path, fname);
			if ((f = fopen(path, "rb"))) {
				ELEVFILEHEADER hdr;
				size_t ret = fread (&hdr, sizeof(ELEVFILEHEADER), 1, f);
				if(ret != sizeof(ELEVFILEHEADER)) {
					printf("Error reading elevation tile_mod\n");
					exit(EXIT_FAILURE);
				}
				if (hdr.hdrsize != sizeof(ELEVFILEHEADER)) {
					fseek (f, hdr.hdrsize, SEEK_SET);
				}
				rescale = (do_rescale = (hdr.scale != tgt_res)) ? hdr.scale/tgt_res : 1.0;
				offset  = (do_shift   = (hdr.offset != 0.0)) ? (int16_t)(hdr.offset/tgt_res) : 0;
				switch (hdr.dtype) {
				case 0: // overwrite the entire tile with a flat offset
					for (i = 0; i < ndat; i++) elev[i] = offset;
					break;
				case 8: {
					const uint8_t mask = UCHAR_MAX;
					uint8_t *tmp = new uint8_t[ndat];
					ret = fread (tmp, sizeof(uint8_t), ndat, f);
					if(ret != (size_t)ndat) {
						printf("Error reading elevation tile_mod\n");
						exit(EXIT_FAILURE);
					}
					for (i = 0; i < ndat; i++) {
						if (tmp[i] != mask) {
							elev[i] = (int16_t)(do_rescale ? (int16_t)(tmp[i]*rescale) : (int16_t)tmp[i]);
							if (do_shift) elev[i] += offset;
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
					if(ret != sizeof(int16_t) * ndat) {
						printf("Error reading elevation tile_mod\n");
						exit(EXIT_FAILURE);
					}
					for (i = 0; i < ndat; i++) {
						if (tmp[i] != mask) {
							elev[i] = (do_rescale ? (int16_t)(tmp[i]*rescale) : tmp[i]);
							if (do_shift) elev[i] += offset;
						}
					}
					delete []tmp;
					}
					break;
				}
				fclose(f);
				return true;
			}
		}
		if (treeMgr[1]) {
			uint8_t *buf;
			int ndata = treeMgr[1]->ReadData(lvl, ilat, ilng, &buf);
			if (ndata) {
				uint8_t *p = buf;
				ELEVFILEHEADER *phdr = (ELEVFILEHEADER*)p;
				p += phdr->hdrsize;
				//int16_t ofs = (int16_t)phdr->offset;
				rescale = (do_rescale = (phdr->scale != tgt_res)) ? phdr->scale/tgt_res : 1.0;
				offset  = (do_shift   = (phdr->offset != 0.0)) ? (int16_t)(phdr->offset/tgt_res) : 0;
				switch (phdr->dtype) {
				case 0:
					for (i = 0; i < ndat; i++) elev[i] = offset;
					break;
				case 8: {
					const uint8_t mask = UCHAR_MAX;
					for (i = 0; i < ndat; i++) {
						if (p[i] != mask) {
							elev[i] = (int16_t)(do_rescale ? p[i]*rescale : p[i]);
							if (do_shift) elev[i] += offset;
						}
					}
					} break;
				case -16: {
					const int16_t mask = SHRT_MAX;
					int16_t *buf16 = (int16_t*)p;
					for (i = 0; i < ndat; i++) {
						if (buf16[i] != mask) {
							elev[i] = (do_rescale ? (int16_t)(buf16[i]*rescale) : buf16[i]);
							if (do_shift) elev[i] += offset;
						}
					}
					} break;
				}
				treeMgr[1]->ReleaseData(buf);
				return true;
			}
		}
	}
	return false;
}

double ElevationManager::Elevation (double lat, double lng, int reqlvl, std::vector<ElevationTile> *tilecache, Vector *normal, int *reslvl) const
{
	double e = 0.0;
	if (reslvl) *reslvl = 0;
	reqlvl = (reqlvl ? std::min (std::max(0,reqlvl-7), maxlvl) : maxlvl);

	if (mode) {
		static ElevationTile local_tile;

		ElevationTile *tile;
		int ntile = 0;
		if (tilecache) {
			tile = tilecache->data();
			ntile = tilecache->size();
		}

		if (!ntile) {
			tile = &local_tile;
			ntile = 1;
		}

		int i, lvl, ilat, ilng;
		ElevationTile *t = 0;

		for (i = 0; i < ntile; i++) {
			if (tile[i].data &&
				reqlvl == tile[i].tgtlvl &&
				lat >= tile[i].latmin && lat <= tile[i].latmax &&
				lng >= tile[i].lngmin && lng <= tile[i].lngmax) {
					t = tile+i;
					break;
				}
		}
		if (!t) { // correct tile not in list - need to load from file
			t = tile;  // find oldest tile
			for (i = 1; i < ntile; i++) 
				if (tile[i].last_access < t->last_access)
					t = tile+i;

			if (t->data) {
				delete []t->data;
				t->data = 0;
			}
			for (lvl = reqlvl; lvl >= 0; lvl--) {
				TileIdx (lat, lng, lvl, &ilat, &ilng);
				t->data = LoadElevationTile (lvl+4, ilat, ilng, elev_res);
				if (t->data) {
					LoadElevationTile_mod (lvl+4, ilat, ilng, elev_res, t->data); // load modifications
					int nlat = 1 << lvl;
					int nlng = 2 << lvl;
					t->lvl = lvl;
					t->tgtlvl = reqlvl;
					t->latmin = (0.5-(double)(ilat+1)/double(nlat))*Pi;
					t->latmax = (0.5-(double)ilat/double(nlat))*Pi;
					t->lngmin = (double)ilng/(double)nlng*Pi2 - Pi;
					t->lngmax = (double)(ilng+1)/(double)nlng*Pi2 - Pi;
					// still need to store emin and emax
					auto gc = g_pOrbiter->GetGraphicsClient();
					if (gc) gc->clbkFilterElevation((OBJHANDLE)cbody, ilat, ilng, lvl, elev_res, t->data);
					break;
				}
			}
			t->lat0 = t->lng0 = t->nmlidx = -1;
		}

		if (t->data) {
			int16_t *elev_base = t->data+elev_stride+1; // strip padding
			double latidx = (lat-t->latmin) * elev_grid/(t->latmax-t->latmin);
			double lngidx = (lng-t->lngmin) * elev_grid/(t->lngmax-t->lngmin);
			int lat0 = (int)latidx;
			int lng0 = (int)lngidx;
			int16_t *eptr = elev_base + lat0*elev_stride + lng0;
			if (mode == 1) { // linear interpolation
				//bool tri;
				double w_lat = latidx-lat0;
				double w_lng = lngidx-lng0;

				double e01 = eptr[0]*(1.0-w_lng) + eptr[1]*w_lng;
				double e02 = eptr[elev_stride]*(1.0-w_lng) + eptr[elev_stride+1]*w_lng;
				e = e01*(1.0-w_lat) + e02*w_lat;

				if (normal) {
					double dlat = (t->latmax-t->latmin)/elev_grid;
					double dlng = (t->lngmax-t->lngmin)/elev_grid;
					double dz = dlat * cbody->Size();
					double dx = dlng * cbody->Size() * cos(lat);
					double nx01 = eptr[1]-eptr[0];
					double nx02 = eptr[elev_stride+1]-eptr[elev_stride];
					double nx = w_lat*nx02 + (1.0-w_lat)*nx01;
					Vector vnx(dx,nx,0);
					double nz01 = eptr[elev_stride]-eptr[0];
					double nz02 = eptr[elev_stride+1]-eptr[1];
					double nz = w_lng*nz02 + (1.0-w_lng)*nz01;
					Vector vnz(0,nz,dz);
					*normal = crossp(vnz,vnx).unit();
				}
			} else { // cubic spline interpolation
				double a_m1, a_0, a_p1, a_p2, b_m1, b_0, b_p1, b_p2;
				double tlat = latidx-lat0;
				double tlng = lngidx-lng0;
				a_m1 = eptr[-elev_stride-1];
				a_0  = eptr[-elev_stride];
				a_p1 = eptr[-elev_stride+1];
				a_p2 = eptr[-elev_stride+2];
				b_m1 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[-1];
				a_0  = eptr[0];
				a_p1 = eptr[1];
				a_p2 = eptr[2];
				b_0 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[elev_stride-1];
				a_0  = eptr[elev_stride];
				a_p1 = eptr[elev_stride+1];
				a_p2 = eptr[elev_stride+2];
				b_p1 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[2*elev_stride-1];
				a_0  = eptr[2*elev_stride];
				a_p1 = eptr[2*elev_stride+1];
				a_p2 = eptr[2*elev_stride+2];
				b_p2 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				e =	0.5 * (2.0*b_0 + tlat*(-b_m1+b_p1) +
					tlat*tlat*(2.0*b_m1-5.0*b_0+4.0*b_p1-b_p2) +
					tlat*tlat*tlat*(-b_m1+3.0*b_0-3.0*b_p1+b_p2));
				if (normal) {
					double dlat = (t->latmax-t->latmin)/elev_grid;
					double dlng = (t->lngmax-t->lngmin)/elev_grid;
					double dz = dlat * cbody->Size();
					double dx = dlng * cbody->Size() * cos(lat);
					double dex00 = 0.5*(eptr[1]-eptr[-1]);
					double dex01 = 0.5*(eptr[2]-eptr[0]);
					double dex10 = 0.5*(eptr[elev_stride+1]-eptr[elev_stride-1]);
					double dex11 = 0.5*(eptr[elev_stride+2]-eptr[elev_stride]);
					double dez00 = 0.5*(eptr[elev_stride]-eptr[-elev_stride]);
					double dez01 = 0.5*(eptr[elev_stride*2]-eptr[0]);
					double dez10 = 0.5*(eptr[elev_stride+1]-eptr[-elev_stride+1]);
					double dez11 = 0.5*(eptr[elev_stride*2+1]-eptr[1]);
					double w1_lat = latidx - lat0;
					double w0_lat = 1.0-w1_lat;
					double w1_lng = lngidx - lng0;
					double w0_lng = 1.0-w1_lng;
					double dex = (dex00+dex10)*0.5*w0_lng + (dex01+dex11)*0.5*w1_lng;
					double dez = (dez00+dez10)*0.5*w0_lat + (dez01+dez11)*0.5*w1_lat;
					normal->x = -dex;
					normal->z = -dez;
					normal->y = 0.5*(dx+dz);
					normal->unify();
				}
			}
			t->last_access = td.SysT0;
			t->lat0 = lat0;
			t->lng0 = lng0;
			if (reslvl) *reslvl = t->lvl+7;
		}
	}
	return e*elev_res;
}

void ElevationManager::ElevationGrid (int ilat, int ilng, int lvl, int pilat, int pilng, int plvl, int16_t* pelev, int16_t *elev, double *emean) const
{
	int i, j, nmean;
	int nlng = 2 << lvl;
	int nlat = 1 << lvl;
	double lng, lat, e;
	double latmin = Pi05 * (double)(nlat-2*ilat-2)/(double)nlat;
	double latmax = Pi05 * (double)(nlat-2*ilat)/(double)nlat;
	double lngmin = Pi * (double)(2*ilng-nlng)/(double)nlng;
	double lngmax = Pi * (double)(2*ilng-nlng+2)/(double)nlng;
	double dlat = (latmax-latmin)/elev_grid;
	double dlng = (lngmax-lngmin)/elev_grid;

	int pnlng = 2 << plvl;
	int pnlat = 1 << plvl;
	double platmin = Pi05 * (double)(pnlat-2*pilat-2)/(double)pnlat;
	double platmax = Pi05 * (double)(pnlat-2*pilat)/(double)pnlat;
	double plngmin = Pi * (double)(2*pilng-pnlng)/(double)pnlng;
	double plngmax = Pi * (double)(2*pilng-pnlng+2)/(double)pnlng;
	//int plat0 = -1000, plng0 = -1000;
	//bool pcelldiag = false;

	int16_t *elev_base = elev + elev_stride+1;
	int16_t *pelev_base = pelev + elev_stride+1;
	if (emean) {
		*emean = 0.0;
		nmean = 0;
	}
	for (i = -1; i <= elev_grid+1; i++) {
		lat = latmin + i*dlat;
		double latidx = (lat-platmin) * elev_grid/(platmax-platmin);
		int lat0 = (int)floor(latidx);
		for (j = -1; j <= elev_grid+1; j++) {
			lng = lngmin + j*dlng;
			double lngidx = (lng-plngmin) * elev_grid/(plngmax-plngmin);
			int lng0 = (int)floor(lngidx);

			int16_t *eptr = pelev_base + lat0*elev_stride + lng0;
			if (mode == 1) { // linear interpolation

				double w_lat = latidx-lat0;
				double w_lng = lngidx-lng0;
				e = (1.0-w_lat)*(eptr[0]*(1.0-w_lng) + eptr[1]*w_lng) + w_lat*(eptr[elev_stride]*(1.0-w_lng) + eptr[elev_stride+1]*w_lng);

			} else { // cubic spline interpolation

				// take care of boundaries
				int latstencil[4] = {-elev_stride,0,elev_stride,2*elev_stride};
				int lngstencil[4] = {-1,0,1,2};
				if (lat0 < 0) latstencil[0] = 0;
				else if (lat0 >= elev_grid) latstencil[3] = elev_stride;
				if (lng0 < 0) lngstencil[0] = 0;
				else if (lng0 >= elev_grid) lngstencil[3] = 1;

				double a_m1, a_0, a_p1, a_p2, b_m1, b_0, b_p1, b_p2;
				double tlat = latidx-lat0;
				double tlng = lngidx-lng0;
				a_m1 = eptr[latstencil[0]+lngstencil[0]];
				a_0  = eptr[latstencil[0]+lngstencil[1]];
				a_p1 = eptr[latstencil[0]+lngstencil[2]];
				a_p2 = eptr[latstencil[0]+lngstencil[3]];
				b_m1 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[latstencil[1]+lngstencil[0]];
				a_0  = eptr[latstencil[1]+lngstencil[1]];
				a_p1 = eptr[latstencil[1]+lngstencil[2]];
				a_p2 = eptr[latstencil[1]+lngstencil[3]];
				b_0 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[latstencil[2]+lngstencil[0]];
				a_0  = eptr[latstencil[2]+lngstencil[1]];
				a_p1 = eptr[latstencil[2]+lngstencil[2]];
				a_p2 = eptr[latstencil[2]+lngstencil[3]];
				b_p1 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				a_m1 = eptr[latstencil[3]+lngstencil[0]];
				a_0  = eptr[latstencil[3]+lngstencil[1]];
				a_p1 = eptr[latstencil[3]+lngstencil[2]];
				a_p2 = eptr[latstencil[3]+lngstencil[3]];
				b_p2 = 0.5 * (2.0*a_0 + tlng*(-a_m1+a_p1) +
					tlng*tlng*(2.0*a_m1-5.0*a_0+4.0*a_p1-a_p2) +
					tlng*tlng*tlng*(-a_m1+3.0*a_0-3.0*a_p1+a_p2));
				e =	0.5 * (2.0*b_0 + tlat*(-b_m1+b_p1) +
					tlat*tlat*(2.0*b_m1-5.0*b_0+4.0*b_p1-b_p2) +
					tlat*tlat*tlat*(-b_m1+3.0*b_0-3.0*b_p1+b_p2));
				e = std::max(-32767.0, std::min (32766.0, e));
			}
			//plat0 = lat0;
			//plng0 = lng0;
			elev_base[i*elev_stride+j] = (int16_t)e;
			if (emean && i >= 0 && j >= 0 && i <= elev_grid && j <= elev_grid) {
				*emean += e;
				nmean++;
			}
		}
	}
	if (emean && nmean)
		*emean /= nmean;
}
