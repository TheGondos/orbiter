// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Particle.cpp
// Particle system for exhaust contrails
// ==============================================================

#include "glad.h"
#include "Particle.h"
#include "Scene.h"
//#include "Camera.h"
#include "Texture.h"
#include <stdio.h>
#include "VertexBuffer.h"

std::unique_ptr<VertexArray> OGLParticleStream::eVBA;
std::unique_ptr<VertexBuffer> OGLParticleStream::eVBO;
std::unique_ptr<IndexBuffer> OGLParticleStream::IBO;
std::unique_ptr<VertexArray> OGLParticleStream::dVBA;
std::unique_ptr<VertexBuffer> OGLParticleStream::dVBO;

static NTAVERTEX evtx[MAXPARTICLE*4]; // vertex list for emissive trail (no normals)
static NTAVERTEX dvtx[MAXPARTICLE*4]; // vertex list for diffusive trail
static uint16_t idx[MAXPARTICLE*6];  // index list

static float tu[8*4] = {0.0,0.5,0.5,0.0, 0.5,1.0,1.0,0.5, 0.0,0.5,0.5,0.0, 0.5,1.0,1.0,0.5,
						0.5,0.5,0.0,0.0, 1.0,1.0,0.5,0.5, 0.5,0.5,0.0,0.0, 1.0,1.0,0.5,0.5};
static float tv[8*4] = {0.0,0.0,0.5,0.5, 0.0,0.0,0.5,0.5, 0.5,0.5,1.0,1.0, 0.5,0.5,1.0,1.0,
						0.0,0.5,0.5,0.0, 0.0,0.5,0.5,0.0, 0.5,1.0,1.0,0.5, 0.5,1.0,1.0,0.5};

using namespace oapi;

static PARTICLESTREAMSPEC DefaultParticleStreamSpec = {
	0,                            // flags
	8.0,                          // creation size
	0.5,                          // creation rate
	100,                          // emission velocity
	0.3,                          // velocity randomisation
	8.0,                          // lifetime
	0.5,                          // growth rate
	3.0,                          // atmospheric slowdown
	PARTICLESTREAMSPEC::DIFFUSE,  // render lighting method
	PARTICLESTREAMSPEC::LVL_SQRT, // mapping from level to alpha
	0, 1,						  // lmin and lmax levels for mapping
	PARTICLESTREAMSPEC::ATM_PLOG, // mapping from atmosphere to alpha
	1e-4, 1						  // amin and amax densities for mapping
};
static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
		abort();
		exit(EXIT_FAILURE);
	}
}

OGLTexture *OGLParticleStream::deftex = nullptr;
bool OGLParticleStream::bShadows = false;

OGLParticleStream::OGLParticleStream (GraphicsClient *_gc, PARTICLESTREAMSPEC *pss): ParticleStream (_gc, pss)
{
	glm::dvec3 tmp = *g_client->GetScene()->GetCamera()->GetGPos();
	cam_ref.x = tmp.x;
	cam_ref.y = tmp.y;
	cam_ref.z = tmp.z;

	src_ref = 0;
	src_ofs = _V(0,0,0);
	interval = 0.02;
	SetSpecs (pss ? pss : &DefaultParticleStreamSpec);
	t0 = oapiGetSimTime();
	active = false;
	pfirst = NULL;
	plast = NULL;
	np = 0;
	mModel =  glm::mat4(1.0f);
}

OGLParticleStream::~OGLParticleStream()
{
	while (pfirst) {
		ParticleSpec *tmp = pfirst;
		pfirst = pfirst->next;
		delete tmp;
	}
}

void OGLParticleStream::GlobalInit ()
{
	deftex = g_client->GetTexMgr()->LoadTexture ("Contrail1.dds", true, 0);
	bShadows = *(bool*)g_client->GetConfigParam (CFGPRM_VESSELSHADOWS);

	int i, j, k, r, ofs;
	for (i = j = 0; i < MAXPARTICLE; i++) {
		ofs = i*4;
		idx[j++] = ofs;
		idx[j++] = ofs+2;
		idx[j++] = ofs+1;
		idx[j++] = ofs+2;
		idx[j++] = ofs;
		idx[j++] = ofs+3;
		r = rand() & 7;
		for (k = 0; k < 4; k++) {
			evtx[ofs+k].tu = dvtx[ofs+k].tu = tu[r*4+k];
			evtx[ofs+k].tv = dvtx[ofs+k].tv = tv[r*4+k];
		}
	}

	eVBA = std::make_unique<VertexArray>();
	eVBA->Bind();
	eVBO = std::make_unique<VertexBuffer>(evtx, MAXPARTICLE*4*sizeof(NTAVERTEX));
	eVBO->Bind();
	//position
	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)0            // array buffer offset
	);
	CheckError("glVertexAttribPointer0");
	glEnableVertexAttribArray(0);

	//normal
	glVertexAttribPointer(
	1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)12            // array buffer offset
	);
	CheckError("glVertexAttribPointer0");
	glEnableVertexAttribArray(1);


	//texcoord
	glVertexAttribPointer(
	2,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	2,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)24            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(2);
	CheckError("glEnableVertexAttribArray1");

	//alpha
	glVertexAttribPointer(
	3,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	1,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)32            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(3);
	CheckError("glEnableVertexAttribArray2");

	IBO = std::make_unique<IndexBuffer>(idx, MAXPARTICLE*6);
	IBO->Bind();
	eVBA->UnBind();

	dVBA = std::make_unique<VertexArray>();
	dVBA->Bind();
	dVBO = std::make_unique<VertexBuffer>(dvtx, MAXPARTICLE*4*sizeof(NTAVERTEX));
	dVBO->Bind();
	//position
	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)0            // array buffer offset
	);
	CheckError("glVertexAttribPointer0");
	glEnableVertexAttribArray(0);

	//normal
	glVertexAttribPointer(
	1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)12            // array buffer offset
	);
	CheckError("glVertexAttribPointer0");
	glEnableVertexAttribArray(1);

	//texcoord
	glVertexAttribPointer(
	2,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	2,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)24            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(2);
	CheckError("glEnableVertexAttribArray1");

	//alpha
	glVertexAttribPointer(
	3,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	1,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	sizeof(NTAVERTEX),                  // stride
	(void*)32            // array buffer offset
	);
	CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(3);
	CheckError("glEnableVertexAttribArray2");
	IBO->Bind();
	dVBA->UnBind();
}

void OGLParticleStream::GlobalExit ()
{
	if (deftex) {
		deftex->Release();
		deftex = nullptr;
	}
}

void OGLParticleStream::SetSpecs (PARTICLESTREAMSPEC *pss)
{
	SetParticleHalflife (pss->lifetime);
	size0 = pss->srcsize;
	speed = pss->v0;
	vrand = pss->srcspread;
	alpha = pss->growthrate;
	beta  = pss->atmslowdown;
	pdensity = pss->srcrate;
	diffuse = (pss->ltype == PARTICLESTREAMSPEC::DIFFUSE);
	lmap  = pss->levelmap;
	lmin  = pss->lmin, lmax = pss->lmax;
	amap  = pss->atmsmap;
	amin  = pss->amin;
	switch (amap) {
	case PARTICLESTREAMSPEC::ATM_PLIN: afac = 1.0/(pss->amax-amin); break;
	case PARTICLESTREAMSPEC::ATM_PLOG: afac = 1.0/log(pss->amax/amin); break;
	case PARTICLESTREAMSPEC::ATM_FLAT: break;
	}
	tex = (pss->tex ? (OGLTexture *)pss->tex : deftex);
}

void OGLParticleStream::SetParticleHalflife (double pht)
{
	exp_rate = RAND_MAX/pht;
	stride = std::max (1, std::min (20,(int)pht));
	ipht2 = 0.5/pht;
}

void OGLParticleStream::SetObserverRef (const VECTOR3 *cam)
{
	cam_ref = *cam;
}

void OGLParticleStream::SetSourceRef (const VECTOR3 *src)
{
	src_ref = src;
}

void OGLParticleStream::SetSourceOffset (const VECTOR3 &ofs)
{
	src_ofs = ofs;
}

void OGLParticleStream::SetIntensityLevelRef (double *lvl)
{
	level = lvl;
}

double OGLParticleStream::Level2Alpha (double level) const
{
	switch (lmap) {
	case PARTICLESTREAMSPEC::LVL_FLAT:
		return lmin;
	case PARTICLESTREAMSPEC::LVL_LIN:
		return level;
	case PARTICLESTREAMSPEC::LVL_SQRT:
		return sqrt (level);
	case PARTICLESTREAMSPEC::LVL_PLIN:
		return std::max (0.0, std::min (1.0, (level-lmin)/(lmax-lmin)));
	case PARTICLESTREAMSPEC::LVL_PSQRT:
		return (level <= lmin ? 0 : level >= lmax ? 1 : sqrt ((level-lmin)/(lmax-lmin)));
	}
	return 0; // should not happen
}

double OGLParticleStream::Atm2Alpha (double prm) const
{
	switch (amap) {
	case PARTICLESTREAMSPEC::ATM_FLAT:
		return amin;
	case PARTICLESTREAMSPEC::ATM_PLIN:
		return std::max (0.0, std::min (1.0, (prm-amin)*afac));
	case PARTICLESTREAMSPEC::ATM_PLOG:
		return std::max (0.0, std::min (1.0, log(prm/amin)*afac));
	}
	return 0; // should not happen
}

ParticleSpec *OGLParticleStream::CreateParticle (const VECTOR3 &pos, const VECTOR3 &vel, double size,
	double alpha)
{
	ParticleSpec *p = new ParticleSpec;
	p->pos = pos;
	p->vel = vel;
	p->size = size;
	p->alpha0 = alpha;
	p->t0 = oapiGetSimTime();
	p->texidx = (rand() & 7) * 4;
	p->flag = 0;
	p->next = NULL;
	p->prev = plast;
	if (plast) plast->next = p;
	else       pfirst = p;
	plast = p;
	np++;

	if (np > MAXPARTICLE)
		DeleteParticle (pfirst);

	return p;
}

void OGLParticleStream::DeleteParticle (ParticleSpec *p)
{
	if (p->prev) p->prev->next = p->next;
	else         pfirst = p->next;
	if (p->next) p->next->prev = p->prev;
	else         plast = p->prev;
	delete p;
	np--;
}

void OGLParticleStream::Update ()
{
	ParticleSpec *p, *tmp;
	double dt = oapiGetSimStep();

	for (p = pfirst; p;) {
		if (dt * exp_rate > rand()) {
			tmp = p;
			p = p->next;
			DeleteParticle (tmp);
		} else {
			p->pos += p->vel*dt;
			p = p->next;
		}
	}
}

void OGLParticleStream::Timejump ()
{
	while (pfirst) {
		ParticleSpec *tmp = pfirst;
		pfirst = pfirst->next;
		delete tmp;
	}
	pfirst = NULL;
	plast = NULL;
	np = 0;
	t0 = oapiGetSimTime();
}

void OGLParticleStream::SetDParticleCoords (const VECTOR3 &ppos, double scale, NTAVERTEX *vtx)
{
	VECTOR3 cdir = ppos;
	double ux, uy, uz, vx, vy, vz, len;
	if (cdir.y || cdir.z) {
		ux =  0;
		uy =  cdir.z;
		uz = -cdir.y;
		len = scale / sqrt (uy*uy + uz*uz);
		uy *= len;
		uz *= len;
		vx = cdir.y*cdir.y + cdir.z*cdir.z;
		vy = -cdir.x*cdir.y;
		vz = -cdir.x*cdir.z;
		len = scale / sqrt(vx*vx + vy*vy + vz*vz);
		vx *= len;
		vy *= len;
		vz *= len;
	} else {
		ux = 0;
		uy = scale;
		uz = 0;
		vx = 0;
		vy = 0;
		vz = scale;
	}
	vtx[0].x = (float)(ppos.x-ux-vx);
	vtx[0].y = (float)(ppos.y-uy-vy);
	vtx[0].z = (float)(ppos.z-uz-vz);
	vtx[1].x = (float)(ppos.x-ux+vx);
	vtx[1].y = (float)(ppos.y-uy+vy);
	vtx[1].z = (float)(ppos.z-uz+vz);
	vtx[2].x = (float)(ppos.x+ux+vx);
	vtx[2].y = (float)(ppos.y+uy+vy);
	vtx[2].z = (float)(ppos.z+uz+vz);
	vtx[3].x = (float)(ppos.x+ux-vx);
	vtx[3].y = (float)(ppos.y+uy-vy);
	vtx[3].z = (float)(ppos.z+uz-vz);
}

void OGLParticleStream::SetEParticleCoords (const VECTOR3 &ppos, double scale, NTAVERTEX *vtx)
{
	VECTOR3 cdir = ppos;
	double ux, uy, uz, vx, vy, vz, len;
	if (cdir.y || cdir.z) {
		ux =  0;
		uy =  cdir.z;
		uz = -cdir.y;
		len = scale / sqrt (uy*uy + uz*uz);
		uy *= len;
		uz *= len;
		vx = cdir.y*cdir.y + cdir.z*cdir.z;
		vy = -cdir.x*cdir.y;
		vz = -cdir.x*cdir.z;
		len = scale / sqrt(vx*vx + vy*vy + vz*vz);
		vx *= len;
		vy *= len;
		vz *= len;
	} else {
		ux = 0;
		uy = scale;
		uz = 0;
		vx = 0;
		vy = 0;
		vz = scale;
	}
	vtx[0].x = (float)(ppos.x-ux-vx);
	vtx[0].y = (float)(ppos.y-uy-vy);
	vtx[0].z = (float)(ppos.z-uz-vz);
	vtx[1].x = (float)(ppos.x-ux+vx);
	vtx[1].y = (float)(ppos.y-uy+vy);
	vtx[1].z = (float)(ppos.z-uz+vz);
	vtx[2].x = (float)(ppos.x+ux+vx);
	vtx[2].y = (float)(ppos.y+uy+vy);
	vtx[2].z = (float)(ppos.z+uz+vz);
	vtx[3].x = (float)(ppos.x+ux-vx);
	vtx[3].y = (float)(ppos.y+uy-vy);
	vtx[3].z = (float)(ppos.z+uz-vz);
}

void OGLParticleStream::SetShadowCoords (const VECTOR3 &ppos, const VECTOR3 &cdir, double scale, TAVERTEX *vtx)
{
	double ux, uy, uz, vx, vy, vz, len;
	if (cdir.y || cdir.z) {
		ux =  0;
		uy =  cdir.z;
		uz = -cdir.y;
		len = scale / sqrt (uy*uy + uz*uz);
		uy *= len;
		uz *= len;
		vx = cdir.y*cdir.y + cdir.z*cdir.z;
		vy = -cdir.x*cdir.y;
		vz = -cdir.x*cdir.z;
		len = scale / sqrt(vx*vx + vy*vy + vz*vz);
		vx *= len;
		vy *= len;
		vz *= len;
	} else {
		ux = 0;
		uy = scale;
		uz = 0;
		vx = 0;
		vy = 0;
		vz = scale;
	}
	vtx[0].x = (float)(ppos.x-ux-vx);
	vtx[0].y = (float)(ppos.y-uy-vy);
	vtx[0].z = (float)(ppos.z-uz-vz);
	vtx[1].x = (float)(ppos.x-ux+vx);
	vtx[1].y = (float)(ppos.y-uy+vy);
	vtx[1].z = (float)(ppos.z-uz+vz);
	vtx[2].x = (float)(ppos.x+ux+vx);
	vtx[2].y = (float)(ppos.y+uy+vy);
	vtx[2].z = (float)(ppos.z+uz+vz);
	vtx[3].x = (float)(ppos.x+ux-vx);
	vtx[3].y = (float)(ppos.y+uy-vy);
	vtx[3].z = (float)(ppos.z+uz-vz);
}

void OGLParticleStream::CalcNormals (const VECTOR3 &ppos, NTAVERTEX *vtx)
{
	VECTOR3 cdir = unit (ppos);
	double ux, uy, uz, vx, vy, vz, len;
	if (cdir.y || cdir.z) {
		ux =  0;
		uy =  cdir.z;
		uz = -cdir.y;
		len = 3.0 / sqrt (uy*uy + uz*uz);
		uy *= len;
		uz *= len;
		vx = cdir.y*cdir.y + cdir.z*cdir.z;
		vy = -cdir.x*cdir.y;
		vz = -cdir.x*cdir.z;
		len = 3.0 / sqrt(vx*vx + vy*vy + vz*vz);
		vx *= len;
		vy *= len;
		vz *= len;
	} else {
		ux = 0;
		uy = 1.0;
		uz = 0;
		vx = 0;
		vy = 0;
		vz = 1.0;
	}
	static float scale = (float)(1.0/sqrt(19.0));
	vtx[0].nx = scale*(float)(-cdir.x-ux-vx);
	vtx[0].ny = scale*(float)(-cdir.y-uy-vy);
	vtx[0].nz = scale*(float)(-cdir.z-uz-vz);
	vtx[1].nx = scale*(float)(-cdir.x-ux+vx);
	vtx[1].ny = scale*(float)(-cdir.y-uy+vy);
	vtx[1].nz = scale*(float)(-cdir.z-uz+vz);
	vtx[2].nx = scale*(float)(-cdir.x+ux+vx);
	vtx[2].ny = scale*(float)(-cdir.y+uy+vy);
	vtx[2].nz = scale*(float)(-cdir.z+uz+vz);
	vtx[3].nx = scale*(float)(-cdir.x+ux-vx);
	vtx[3].ny = scale*(float)(-cdir.y+uy-vy);
	vtx[3].nz = scale*(float)(-cdir.z+uz-vz);
}

void OGLParticleStream::Render (OGLCamera *c)
{
	if (!pfirst) return;
//	dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &mWorld);

//	if (prevtex != tex) dev->SetTexture (0, prevtex = tex);

	if (diffuse) RenderDiffuse (c);
	else         RenderEmissive (c);
}

void OGLParticleStream::RenderDiffuse (OGLCamera *c)
{
	static OGLMaterial smokemat = { // emissive material for engine exhaust
		{1,1,1,1},
		{0,0,0,1},
		{0,0,0,1},
		{0.2f,0.2f,0.2f,1},
		0.0
	};

	ParticleSpec *p;
	int i0, j, n, stride = np/16+1;
	float *u, *v;
	NTAVERTEX *vtx;

	glm::dvec3 tmp = *g_client->GetScene()->GetCamera()->GetGPos();
	cam_ref.x = tmp.x;
	cam_ref.y = tmp.y;
	cam_ref.z = tmp.z;
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, FALSE);
*/
	static Shader s("Particle.vs","Particle.fs");
	s.Bind();
	auto vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection", *vp);
	s.SetMat4("u_Model", mModel);
	glm::vec4 color = {0.2f,0.2f,0.2f,1};
	SetMaterial (color);
	s.SetVec4("u_Color", color);

	CalcNormals (plast->pos - cam_ref, dvtx);
	int cnt=0;
	for (p = pfirst, vtx = dvtx, n = i0 = 0; p; p = p->next) {
		cnt++;
		SetDParticleCoords (p->pos - cam_ref, p->size, vtx);
		u = tu + p->texidx;
		v = tv + p->texidx;
		float alpha = (float)std::max (0.1, p->alpha0*(1.0-(oapiGetSimTime()-p->t0)*ipht2));
		for (j = 0; j < 4; j++, vtx++) {
			vtx->nx = dvtx[j].nx;
			vtx->ny = dvtx[j].ny;
			vtx->nz = dvtx[j].nz;
			vtx->tu = u[j];
			vtx->tv = v[j];
			vtx->alpha = alpha;
		}
	}

	dVBA->Bind();
	dVBO->Bind();
//	printf("d %p %d\n", dvtx, 4*cnt * sizeof(NTAVERTEX));
	dVBO->Update(dvtx, 4*cnt * sizeof(NTAVERTEX));
	glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);
	glDrawElements(GL_TRIANGLES, cnt*6, GL_UNSIGNED_SHORT, 0);

	dVBA->UnBind();
	s.UnBind();
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);

/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	*/
}

void OGLParticleStream::RenderEmissive (OGLCamera *c)
{
	ParticleSpec *p;
	float *u, *v;
	NTAVERTEX *vtx;

	glm::dvec3 tmp = *g_client->GetScene()->GetCamera()->GetGPos();
	cam_ref.x = tmp.x;
	cam_ref.y = tmp.y;
	cam_ref.z = tmp.z;
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	static Shader s("Particle.vs","Particle.fs");
	s.Bind();
	auto vp = c->GetViewProjectionMatrix();
	s.SetMat4("u_ViewProjection", *vp);
	s.SetMat4("u_Model", mModel);
	glm::vec4 color;
	SetMaterial (color);
	s.SetVec4("u_Color", color);
	int cnt=0;
	for (p = pfirst, vtx = evtx; p; p = p->next) {
		cnt++;
		SetEParticleCoords (p->pos - cam_ref, p->size, vtx);
		float alpha = (float)std::max (0.1, p->alpha0*(1.0-(oapiGetSimTime()-p->t0)*ipht2));
		u = tu + p->texidx;
		v = tv + p->texidx;
		for (int j = 0; j < 4; j++, vtx++) {
			vtx->tu = u[j];
			vtx->tv = v[j];
			vtx->alpha = alpha;
		}
	}

	eVBA->Bind();
	eVBO->Bind();
//	printf("e %p %d\n", evtx, 4*cnt * sizeof(NTAVERTEX));
	eVBO->Update(evtx, 4*cnt * sizeof(NTAVERTEX));
	glBindTexture(GL_TEXTURE_2D,  tex->m_TexId);
	glDrawElements(GL_TRIANGLES, cnt*6, GL_UNSIGNED_SHORT, 0);

	eVBA->UnBind();
	s.UnBind();
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);

	/*
	dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	dev->SetRenderState (D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	*/
}

// =======================================================================

ExhaustStream::ExhaustStream (oapi::GraphicsClient *_gc, OBJHANDLE hV,
	const double *srclevel, const VECTOR3 *thref, const VECTOR3 *thdir,
	PARTICLESTREAMSPEC *pss)
: OGLParticleStream (_gc, pss)
{
	Attach (hV, thref, thdir, srclevel);
	hPlanet = 0;
}

ExhaustStream::ExhaustStream (oapi::GraphicsClient *_gc, OBJHANDLE hV,
	const double *srclevel, const VECTOR3 &ref, const VECTOR3 &_dir,
	PARTICLESTREAMSPEC *pss)
: OGLParticleStream (_gc, pss)
{
	Attach (hV, ref, _dir, srclevel);
	hPlanet = 0;
}

void ExhaustStream::Update ()
{
	OGLParticleStream::Update ();

	double simt = oapiGetSimTime();
	double dt = oapiGetSimStep();
	double alpha0;

	VESSEL *vessel = (hRef ? oapiGetVesselInterface (hRef) : 0);

	if (np) {
		ParticleSpec *p;
		double lng, lat, r1, r2, rad, pref, slow;
		int i;
		if (vessel) hPlanet = vessel->GetSurfaceRef();
		if (hPlanet) {
			VECTOR3 pp;
			oapiGetGlobalPos (hPlanet, &pp);
			rad = oapiGetSize (hPlanet);
			VECTOR3 dv = pp-plast->pos; // gravitational dv
			double d = length (dv);
			dv *= GGRAV * oapiGetMass(hPlanet)/(d*d*d) * dt;

			ATMPARAM prm;
			oapiGetPlanetAtmParams (hPlanet, d, &prm);
			if (prm.rho) {
				pref = sqrt(prm.rho)/1.1371;
				slow = exp(-beta*pref*dt);
				dv *= exp(-prm.rho*2.0); // reduce gravitational effect in atmosphere (buoyancy)
			} else {
				pref = 0.0;
				slow = 1.0;
			}
			oapiGlobalToEqu (hPlanet, pfirst->pos, &lng, &lat, &r1);
			VECTOR3 av1 = oapiGetWindVector (hPlanet, lng, lat, r1-rad, 3);
			oapiGlobalToEqu (hPlanet, plast->pos, &lng, &lat, &r2);
			VECTOR3 av2 = oapiGetWindVector (hPlanet, lng, lat, r2-rad, 3);
			VECTOR3 dav = (av2-av1)/np;
			double r = oapiGetSize (hPlanet);

			for (p = pfirst, i = 0; p; p = p->next, i++) {
				p->vel += dv;
				VECTOR3 av = dav*i + av1; // atmosphere velocity
				VECTOR3 vv = p->vel-av;   // velocity difference
				p->vel = vv*slow + av;
				p->size += alpha * dt;

				VECTOR3 s (p->pos - pp);
				if (length(s) < r) {
					VECTOR3 dp = s * (r/length(s)-1.0);
					p->pos += dp;

					static double dv_scale = length(vv)*0.2;
					VECTOR3 dv = {((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
								  ((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
								  ((double)rand()/(double)RAND_MAX-0.5)*dv_scale};
					dv += vv;

					normalise(s);
					VECTOR3 vv2 = dv - s*dotp(s,dv);
					if (length(vv2)) vv2 *= 0.5*length(vv)/length(vv2);
					vv2 += s*(((double)rand()/(double)RAND_MAX)*dv_scale);
					p->vel = vv2*1.0/*2.0*/+av;
					double r = (double)rand()/(double)RAND_MAX;
					p->pos += (vv2-vv) * dt * r;
					//p->size *= (1.0+r);
				}
			}
		}
	}

	if (level && *level > 0 && (alpha0 = Level2Alpha(*level) * Atm2Alpha (vessel->GetAtmDensity())) > 0.01) {
		if (simt > t0+interval) {
			VECTOR3 vp, vv;
			MATRIX3 vR;
			vessel->GetRotationMatrix (vR);
			vessel->GetGlobalPos (vp);
			vessel->GetGlobalVel (vv);
			VECTOR3 vr = mul (vR, *dir) * (-speed);
			while (simt > t0+interval) {
				// create new particle
				double dt = simt-t0-interval;
				double dv_scale = speed*vrand; // exhaust velocity randomisation
				VECTOR3 dv = {((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
						      ((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
							  ((double)rand()/(double)RAND_MAX-0.5)*dv_scale};
				ParticleSpec *p = CreateParticle (mul (vR, *pos) + vp + (vr+dv)*dt,
					vv + vr+dv, size0, alpha0);
				p->size += alpha * dt;

				if (diffuse && hPlanet && bShadows) { // check for shadow render
					double lng, lat, alt;
					static const double eps = 1e-2;
					oapiGlobalToEqu (hPlanet, p->pos, &lng, &lat, &alt);
					//planet->GlobalToEquatorial (MakeVector(p->pos), lng, lat, alt);
					alt -= oapiGetSize(hPlanet) + oapiSurfaceElevation (hPlanet, lng, lat);
					if (alt*eps < vessel->GetSize()) p->flag |= 1; // render shadow
				}

				// determine next interval (pretty hacky)
				t0 += interval;
				if (speed > 10) {
					interval = std::max (0.015, size0 / (pdensity * (0.1*vessel->GetAirspeed() + size0)));
				} else {
					interval = 1.0/pdensity;
				}
				interval *= (double)rand()/(double)RAND_MAX + 0.5;
				interval*=0.2;
			}
		}
	} else t0 = simt;

}
/*
void ExhaustStream::RenderGroundShadow (LPDIRECT3DDEVICE7 dev, LPDIRECTDRAWSURFACE7 &prevtex)
{
	if (!diffuse || !hPlanet) return;

	bool needsetup = true;
	ParticleSpec *p;
	double rad, r, lng, lat;
	float *u, *v, alpha;
	int n, j, i0; 
	VECTOR3 sd, hn;

	NTVERTEX *vtx;
	VECTOR3 pp;
	oapiGetGlobalPos (hPlanet, &pp);

	for (p = pfirst, vtx = evtx, n = i0 = 0; p; p = p->next) {
		if (!(p->flag & 1)) continue;

		if (needsetup) {
			rad = oapiGetSize(hPlanet);
			MATRIX3 Rp;
			oapiGetRotationMatrix (hPlanet, &Rp);
			sd = unit(p->pos);  // shadow projection direction
			VECTOR3 pv0 = p->pos - pp;   // rel. particle position

			VECTOR3 pr0 = tmul (Rp, pv0);
			oapiLocalToEqu (hPlanet, pr0, &lng, &lat, &r);
			rad += oapiSurfaceElevation (hPlanet, lng, lat);

			// calculate the intersection of the vessel's shadow with the planet surface
			double fac1 = dotp (sd, pv0);
			if (fac1 > 0.0) return;       // shadow doesn't intersect planet surface
			double arg  = fac1*fac1 - (dotp (pv0, pv0) - rad*rad);
			if (arg <= 0.0) return;       // shadow doesn't intersect with planet surface
			double a = -fac1 - sqrt(arg);
			VECTOR3 shp = sd*a;           // projection point in global frame
			hn = unit (shp + pv0);        // horizon normal in global frame

			dev->SetTransform (D3DTRANSFORMSTATE_WORLD, &mWorld);
			if (prevtex != tex) dev->SetTexture (0, prevtex = tex);
			dev->SetRenderState (D3DRENDERSTATE_LIGHTING, FALSE);
			dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
			dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
			needsetup = false;
		}

		VECTOR3 pvr = p->pos - pp;   // rel. particle position

		// calculate the intersection of the vessel's shadow with the planet surface
		double fac1 = dotp (sd, pvr);
		if (fac1 > 0.0) break;       // shadow doesn't intersect planet surface
		double arg  = fac1*fac1 - (dotp (pvr, pvr) - rad*rad);
		if (arg <= 0.0) break;       // shadow doesn't intersect with planet surface
		double a = -fac1 - sqrt(arg);

		SetShadowCoords (p->pos - *cam_ref + sd*a, -hn, p->size, vtx);
		u = tu + p->texidx;
		v = tv + p->texidx;
		for (j = 0; j < 4; j++, vtx++) {
			vtx->tu = u[j];
			vtx->tv = v[j];
		}
		if (++n == stride || n+i0 == np) {
			alpha = (float)std::max (0.1, 0.60 * p->alpha0*(1.0-(oapiGetSimTime()-p->t0)*ipht2));
			dev->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(0,0,0,alpha));
			dev->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, FVF_XYZ_TEX,
				evtx+i0*4, n*4, idx, n*6, 0);
			i0 += n;
			n = 0;
		}
	}

	if (!needsetup) {
		dev->SetRenderState (D3DRENDERSTATE_LIGHTING, TRUE);
		dev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		dev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		dev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}
*/
// =======================================================================

ReentryStream::ReentryStream (oapi::GraphicsClient *_gc, OBJHANDLE hV, PARTICLESTREAMSPEC *pss)
: OGLParticleStream (_gc, pss)
{
	llevel = 1.0;
	Attach (hV, _V(0,0,0), _V(0,0,0), &llevel);
	hPlanet = 0;
}

void ReentryStream::SetMaterial (glm::vec4 &col)
{
	// should be heating-dependent
	col.r = 1.0f;
	col.g = 0.8f;
	col.b = 0.6f; 
}

void ReentryStream::Update ()
{
	OGLParticleStream::Update ();
	VESSEL *vessel = (hRef ? oapiGetVesselInterface (hRef) : 0);

	double simt = oapiGetSimTime();
	double simdt = oapiGetSimStep();
	double friction = 0.0;
	if (vessel) {
		double spd = vessel->GetAirspeed();
		friction = 0.5*pow(vessel->GetAtmDensity(),0.6) * pow(spd,3);
	}
	double alpha0;

	if (np) {
		ParticleSpec *p;
		double lng, lat, r1, r2, rad;
		int i;
		if (vessel) hPlanet = vessel->GetSurfaceRef();
		if (hPlanet) {
			rad = oapiGetSize (hPlanet);
			oapiGlobalToEqu (hPlanet, pfirst->pos, &lng, &lat, &r1);
			VECTOR3 av1 = oapiGetWindVector (hPlanet, lng, lat, r1-rad, 3);
			oapiGlobalToEqu (hPlanet, plast->pos, &lng, &lat, &r2);
			VECTOR3 av2 = oapiGetWindVector (hPlanet, lng, lat, r2-rad, 3);
			VECTOR3 dav = (av2-av1)/np;
			double r = oapiGetSize (hPlanet);

			for (p = pfirst, i = 0; p; p = p->next, i++) {
				VECTOR3 av = dav*i + av1;
				VECTOR3 vv = p->vel-av;
				double slow = exp(-beta*simdt);
				p->vel = vv*slow + av;
				p->size += alpha * simdt;
			}
		}
	}

	if (friction > 0 && (alpha0 = Atm2Alpha (friction)) > 0.01) {
		if (simt > t0+interval) {
			VECTOR3 vp, vv, av;
			vessel->GetGlobalPos (vp);
			vessel->GetGlobalVel (vv);

			if (hPlanet) {
				double lng, lat, r, rad;
				rad = oapiGetSize (hPlanet);
				oapiGlobalToEqu (hPlanet, vp, &lng, &lat, &r);
				av = oapiGetWindVector (hPlanet, lng, lat, r-rad, 3);
			} else
				av = vv;

			while (simt > t0+interval) {
				// create new particle
				double dt = simt-t0-interval;
				double ebt = exp(-beta*dt);
				double dv_scale = vessel->GetAirspeed()*vrand; // exhaust velocity randomisation
				VECTOR3 dv = {((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
						      ((double)rand()/(double)RAND_MAX-0.5)*dv_scale,
							  ((double)rand()/(double)RAND_MAX-0.5)*dv_scale};
				VECTOR3 dx = (vv-av) * (1.0-ebt)/beta + av*dt;
				CreateParticle (vp + dx - vv*dt, (vv+dv-av)*ebt + av, size0, alpha0);
				// determine next interval
				t0 += interval;
				interval = std::max (0.015, size0 / (pdensity * (0.1*vessel->GetAirspeed() + size0)));
				interval *= (double)rand()/(double)RAND_MAX + 0.5;
			}
		}
	} else t0 = simt;
}
