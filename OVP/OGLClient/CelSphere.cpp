// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// CelSphere.cpp
// Class CelestialSphere (implementation)
//
// This class is responsible for rendering the celestial sphere
// background (stars, constellations, grids, labels, etc.)
// ==============================================================

#include "glad.h"
#include "CelSphere.h"
#include "OGLCamera.h"
#include "Renderer.h"

#define NSEG 64 // number of segments in celestial grid lines

using namespace oapi;

// ==============================================================
struct VERTEX_XYZ   { float x, y, z; };                   // transformed vertex

const float sphere_r = 1e4f; // the actual render distance for the celestial sphere
	                    	 // is irrelevant, since it is rendered without z-buffer,
	                 		 // but it must be within the fustrum limits - check this
	                		 // in case the near and far planes are dynamically changed!

CelestialSphere::CelestialSphere ()
{
	m_constellationsShader = Renderer::GetShader("Constellations");
	m_starsShader = Renderer::GetShader("Stars");
	m_eclShader = Renderer::GetShader("Ecliptic");

	LoadStars ();
	LoadConstellationLines ();
	CreateEcliptic ();
}

// ==============================================================

CelestialSphere::~CelestialSphere ()
{
}

// ==============================================================

void CelestialSphere::LoadStars ()
{
	StarRenderPrm *prm = (StarRenderPrm*)g_client->GetConfigParam ((int)CFGPRM_STARRENDERPRM);

	double a = 0.0, b = 0.0;
	int lvl, plvl = 256;
	int idx = 0;

	if (prm->mag_lo > prm->mag_hi) {
		if (prm->map_log) {
			// scaling factors for logarithmic brightness mapping
			a = -log(prm->brt_min)/(prm->mag_lo-prm->mag_hi);
		} else {
			// scaling factors for linear brightness mapping
			a = (1.0-prm->brt_min)/(prm->mag_hi-prm->mag_lo);
			b = prm->brt_min - prm->mag_lo*a;
		}
	} else {
		oapiWriteLog("WARNING: Inconsistent magnitude limits for background star brightness. Disabling background stars.");
	}

	if (prm->mag_lo <= prm->mag_hi) return;

	// Read binary data from file
	FILE *f = fopen ("Star.bin", "rb");
	if (!f) {
		oapiWriteLog("WARNING: Cannot open Star.bin");
		printf("WARNING: Cannot open Star.bin\n");
		exit(-1);
		return;
	}

	struct StarRec {
		float lng, lat, mag;
	};

	struct Vertex {
		float x,y,z,mag;
	};

	size_t len = fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	struct StarRec *buf = (struct StarRec *)new char[len];

	size_t r = fread(buf, 1, len, f);
	fclose(f);
	nstars = len / sizeof(StarRec);
	
	if(r == len) {
		Vertex *vbuf = new Vertex[nstars];

		for(size_t i=0;i<nstars;i++) {
			StarRec &rec = buf[i];
			double xz = (double)sphere_r * cos (rec.lat);
			vbuf[i].x = (float)(xz * cos (rec.lng));
			vbuf[i].z = (float)(xz * sin (rec.lng));
			vbuf[i].y = (float)(sphere_r * sin (rec.lat));

			float c;
			if (prm->map_log)
				c = (float)std::min (1.0, std::max (prm->brt_min, exp(-(rec.mag-prm->mag_hi)*a)));
			else
				c = (float)std::min (1.0, std::max (prm->brt_min, a*rec.mag+b));

			vbuf[i].mag = c;

			lvl = (int)(c*256.0*0.5);
			if (lvl > 255) lvl = 255;
			for (int k = lvl; k < plvl; k++) lvlid[k] = idx;
			plvl = lvl;
			idx++;
		}
		for (int i = 0; i < plvl; i++) lvlid[i] = idx;

		m_starsVAO = std::make_unique<VertexArray>();
		m_starsVAO->Bind();
		m_starsVBO = std::make_unique<VertexBuffer>(vbuf, nstars * sizeof(Vertex));
		glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		4,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
		Renderer::CheckError("glVertexAttribPointer");
		glEnableVertexAttribArray(0);
		Renderer::CheckError("glEnableVertexAttribArray");
		m_starsVAO->UnBind();

		delete[]vbuf;
	} else {
		fprintf(stderr, "Cannot read %zu bytes (%zu read)\n", len, r);
		exit(-1);
	}
	delete []buf;
}
void CelestialSphere::LoadConstellationLines ()
{
	VERTEX_XYZ *cnstvtx;
	const int maxline = 1000; // plenty for default data base, but check with custom data bases!
	GraphicsClient::ConstRec *cline = new GraphicsClient::ConstRec[maxline];
	ncline = g_client->LoadConstellationLines (maxline, cline);
	if (ncline) {
		cnstvtx = new VERTEX_XYZ[ncline*2]; // two end points per line
		double xz;
		for (size_t n = 0; n < ncline; n++) {
			GraphicsClient::ConstRec *rec = cline+n;
			xz = sphere_r * cos (rec->lat1);
			cnstvtx[n*2].x = (float)(xz * cos(rec->lng1));
			cnstvtx[n*2].z = (float)(xz * sin(rec->lng1));
			cnstvtx[n*2].y = (float)(sphere_r * sin(rec->lat1));
			xz = sphere_r * cos (rec->lat2);
			cnstvtx[n*2+1].x = (float)(xz * cos(rec->lng2));
			cnstvtx[n*2+1].z = (float)(xz * sin(rec->lng2));
			cnstvtx[n*2+1].y = (float)(sphere_r * sin(rec->lat2));
		}
	}
	delete []cline;

	m_constellationsVAO = std::make_unique<VertexArray>();
	m_constellationsVAO->Bind();
	m_constellationsVBO = std::make_unique<VertexBuffer>(cnstvtx, 2 * ncline * sizeof(VERTEX_XYZ));
	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	0,                  // stride
	(void*)0            // array buffer offset
	);
	Renderer::CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(0);
	Renderer::CheckError("glEnableVertexAttribArray");
	m_constellationsVAO->UnBind();
	delete[]cnstvtx;
}

void CelestialSphere::RenderStars (OGLCamera *c, VECTOR3 &bgcol)
{
	int bglvl = std::min (255, (int)((std::min(bgcol.x,1.0) + std::min(bgcol.y,1.0) + std::min(bgcol.z,1.0))*128.0));
	int ns = std::min ((int)nstars, lvlid[bglvl]);

	auto vp = c->GetViewProjectionMatrix();
	Renderer::Bind(m_starsShader);
	m_starsShader->SetMat4("u_ViewProjection", *vp);
	m_starsVAO->Bind();
	glDrawArrays(GL_POINTS, 0, ns);
	Renderer::CheckError("glDrawArrays");
	m_starsVAO->UnBind();
	Renderer::Unbind(m_starsShader);
}
void CelestialSphere::RenderConstellations (VECTOR3 &col, OGLCamera *c)
{
	auto vp = c->GetViewProjectionMatrix();
	Renderer::Bind(m_constellationsShader);
	m_constellationsShader->SetMat4("u_ViewProjection", *vp);
	m_constellationsVAO->Bind();
	glDrawArrays(GL_LINES, 0, ncline * 2); // Starting from vertex 0; 3 vertices total -> 1 triangle
	Renderer::CheckError("glDrawArrays");
	m_constellationsVAO->UnBind();
	Renderer::Unbind(m_constellationsShader);
}

void CelestialSphere::CreateEcliptic()
{
	necl = NSEG + 1;

	VERTEX_XYZ vbuf[necl];
	for (int i = 0; i <= NSEG; i++) {
		double lng = 2.0*PI * (double)i/(double)NSEG;
		vbuf[i].x = (float)(sphere_r * cos(lng));
		vbuf[i].z = (float)(sphere_r * sin(lng));
		vbuf[i].y = 0.0f;
	}

	m_eclVAO = std::make_unique<VertexArray>();
	m_eclVAO->Bind();
	m_eclVBO = std::make_unique<VertexBuffer>((void *)vbuf, necl * sizeof(VERTEX_XYZ));
	glVertexAttribPointer(
	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	3,                  // size
	GL_FLOAT,           // type
	GL_FALSE,           // normalized?
	0,                  // stride
	(void*)0            // array buffer offset
	);
	Renderer::CheckError("glVertexAttribPointer");
	glEnableVertexAttribArray(0);
	Renderer::CheckError("glEnableVertexAttribArray");
	m_eclVAO->UnBind();
}

void CelestialSphere::RenderEcliptic(OGLCamera *c)
{
	auto vp = c->GetViewProjectionMatrix();
	Renderer::Bind(m_eclShader);
	m_eclShader->SetMat4("u_ViewProjection", *vp);
	m_eclVAO->Bind();
	glDrawArrays(GL_LINE_STRIP, 0, necl);
	Renderer::CheckError("glDrawArrays");
	m_eclVAO->UnBind();
	Renderer::Unbind(m_eclShader);
}
