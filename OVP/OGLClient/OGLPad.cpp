// Sketchpad implementation, heavily inspired from SkyboltClient
/*
Copyright 2021 Matthew Reid
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "glad.h"
#include "OGLPad.h"
#include "OGLClient.h"
#include "Scene.h"
#include "OGLCamera.h"
#include <cstring>
#include <fontconfig/fontconfig.h>
#include "Renderer.h"
#include <map>

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg/src/nanovg_gl.h"

static std::map<std::string,std::string> fontCache;

static NVGcolor nvgCol(uint32_t col)
{
   	unsigned char* c = reinterpret_cast<unsigned char*>(&col);

	NVGcolor color;    
    color.r = c[0]/255.0;
	color.g = c[1]/255.0;
	color.b = c[2]/255.0;
	color.a = 1;
	return color;
}

static NVGalign toNvgAlign(oapi::Sketchpad::TAlign_horizontal tah)
{
	switch (tah)
	{
		case oapi::Sketchpad::LEFT:
			return NVGalign::NVG_ALIGN_LEFT;
		case oapi::Sketchpad::CENTER:
			return NVGalign::NVG_ALIGN_CENTER;
		case oapi::Sketchpad::RIGHT:
			return NVGalign::NVG_ALIGN_RIGHT;
	}
    return NVGalign::NVG_ALIGN_LEFT;
}

static NVGalign toNvgAlign(oapi::Sketchpad::TAlign_vertical tav)
{
	switch (tav)
	{
		case oapi::Sketchpad::TOP:
			return NVGalign::NVG_ALIGN_TOP;
		case oapi::Sketchpad::BASELINE:
			return NVGalign::NVG_ALIGN_BASELINE;
		case oapi::Sketchpad::BOTTOM:
			return NVGalign::NVG_ALIGN_BOTTOM;
	}
    return NVGalign::NVG_ALIGN_BASELINE; 
}

NVGcontext *OGLPad::s_nvg_fb;
NVGcontext *OGLPad::s_nvg_mfd;
/////////////////// SKETCHPAD STUFF ////////////////////////////
void OGLPad::GlobalInit() {
    s_nvg_fb  = nvgCreateGL3(NVG_STENCIL_STROKES);
    s_nvg_mfd = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_YFLIP);
}

void OGLPad::GlobalExit() {
    nvgDeleteGL3(s_nvg_fb);
    nvgDeleteGL3(s_nvg_mfd);
}

OGLPad::OGLPad (SURFHANDLE s):oapi::Sketchpad(s)
{
	cfont = nullptr;
	cpen = nullptr;
	cbrush = nullptr;
	m_xOrigin = 0;
	m_yOrigin = 0;
    bgColorNative = 0;
    bgColor = nvgCol(0);
	textColorNative = 0;
    textColor = nvgCol(0);
	m_TextBackgroundMode = BK_TRANSPARENT;
	m_curX = 0;
	m_curY = 0;
	m_TextAlign = NVGalign::NVG_ALIGN_TOP|NVGalign::NVG_ALIGN_LEFT;
//	m_TextAlign = NVGalign::NVG_ALIGN_BOTTOM|NVGalign::NVG_ALIGN_LEFT;
//	m_TextAlign = NVGalign::NVG_ALIGN_BASELINE|NVGalign::NVG_ALIGN_LEFT;
	m_tex = (OGLTexture *)s;
	if(s)
	{
		Renderer::PushRenderTarget(m_tex);
		s_nvg = s_nvg_mfd;
		height = m_tex->m_Height;
		width = m_tex->m_Width;
	}
	else
	{
		s_nvg = s_nvg_fb;
		height = g_client->GetScene()->GetCamera()->GetHeight();
		width = g_client->GetScene()->GetCamera()->GetWidth();
		Renderer::PushBool(Renderer::CULL_FACE, false);
	}

    nvgBeginFrame(s_nvg, width, height, /* pxRatio */ 1.0);
}
OGLPad::~OGLPad ()
{
    nvgEndFrame(s_nvg);
	if(m_tex) {
		Renderer::PopRenderTarget();
		glBindTexture(GL_TEXTURE_2D, m_tex->m_TexId);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		Renderer::PopBool();
	}
	// Restore states destroyed by nanovg
	Renderer::Sync();
}

oapi::Font *OGLPad::SetFont (oapi::Font *font) const
{
	oapi::Font *ret = cfont;
	cfont = (OGLFont *)font;
	return ret;
}

oapi::Pen *OGLPad::SetPen (oapi::Pen *pen) const
{
	oapi::Pen *ret = cpen;
	cpen = (OGLPen *)pen;
	return ret;
}

void OGLPad::nvgStrokeFromPen()
{
	if(cpen) {
		nvgStrokeWidth(s_nvg, std::max(1, cpen->m_Width));
		nvgStrokeColor(s_nvg, nvgCol(cpen->m_Color));
		nvgStrokeColor(s_nvg, nvgCol(cpen->m_Color));
		nvgStrokeDash(s_nvg, cpen->m_Style == 2?1:0);
		nvgStroke(s_nvg);
	}
}

void OGLPad::nvgStrokeFromBrush()
{
	if(cbrush) {
		nvgFillColor(s_nvg, nvgCol(cbrush->m_Color));
		nvgFill(s_nvg);
	}
}

oapi::Brush *OGLPad::SetBrush (oapi::Brush *brush) const
{
	oapi::Brush *ret = cbrush;
	cbrush = (OGLBrush *)brush;
	return ret;
}

void OGLPad::SetTextAlign (TAlign_horizontal tah, TAlign_vertical tav)
{
	m_TextAlign = (NVGalign)(toNvgAlign(tah) | toNvgAlign(tav));
}

uint32_t OGLPad::SetTextColor (uint32_t col)
{
	uint32_t oldColor = textColorNative;
    textColorNative = col;
	textColor = nvgCol(col);
	return oldColor;
}

uint32_t OGLPad::SetBackgroundColor (uint32_t col)
{
	uint32_t oldColor = bgColorNative;
    bgColorNative = col;
	bgColor = nvgCol(col);
	return oldColor;
}

void OGLPad::SetBackgroundMode (BkgMode mode)
{
	m_TextBackgroundMode = mode;
}

int OGLPad::GetCharSize ()
{
	int height;
	if (cfont) {
		if(nvgFindFont(s_nvg, cfont->m_facename.c_str()) == -1) nvgCreateFont(s_nvg, cfont->m_facename.c_str(), cfont->m_fontfile.c_str());
		nvgFontSize(s_nvg, cfont->m_Height);
		nvgFontFace(s_nvg, cfont->m_facename.c_str());
		nvgTextAlign(s_nvg, NVGalign::NVG_ALIGN_TOP|NVGalign::NVG_ALIGN_LEFT);
		float bounds[4];
		nvgTextBounds(s_nvg, 0, 0, "A", nullptr, bounds);
		height = bounds[3];
		return ((uint32_t)(bounds[3]-bounds[1]) + ((uint32_t)(bounds[2]-bounds[0])<<16));
	} else {
		return (10<<16) + 7;
	}
}

int OGLPad::GetTextWidth (const char *str, int len)
{
	if (len == 0) {
		len = strlen(str);
	}

	if (cfont) {
		if(nvgFindFont(s_nvg, cfont->m_facename.c_str()) == -1) nvgCreateFont(s_nvg, cfont->m_facename.c_str(), cfont->m_fontfile.c_str());
		nvgFontSize(s_nvg, cfont->m_Height);
		nvgFontFace(s_nvg, cfont->m_facename.c_str());
		nvgTextAlign(s_nvg, NVGalign::NVG_ALIGN_TOP|NVGalign::NVG_ALIGN_LEFT);
		float bounds[4];
		nvgTextBounds(s_nvg, 0, 0, str, str+len, bounds);
		return bounds[2]-bounds[0];
	}
	return 7 * len;
}

void OGLPad::SetOrigin (int x, int y)
{
	m_xOrigin = x;
	m_yOrigin = y;
}

void OGLPad::GetOrigin (int *x, int *y) const
{
	*x=m_xOrigin;
	*y=m_yOrigin;
}

bool OGLPad::Text (int x, int y, const char *str, int len)
{
	if(nvgFindFont(s_nvg, cfont->m_facename.c_str()) == -1) {
		nvgCreateFont(s_nvg, cfont->m_facename.c_str(), cfont->m_fontfile.c_str());
	}
    nvgFontSize(s_nvg, cfont->m_Height);
    nvgFontFace(s_nvg, cfont->m_facename.c_str());
    nvgTextAlign(s_nvg, m_TextAlign);

    nvgResetTransform(s_nvg);
    nvgTranslate(s_nvg, x - m_xOrigin, y - m_yOrigin);
    nvgRotate(s_nvg, cfont->rotationRadians);

    if (m_TextBackgroundMode == BkgMode::BK_OPAQUE)
    {
        float bounds[4];
        nvgTextBounds(s_nvg, 0, 0, str, str+len, bounds);
        nvgBeginPath(s_nvg);
        nvgRect(s_nvg, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
        nvgFillColor(s_nvg, bgColor);
        nvgFill(s_nvg);
    }

    nvgFillColor(s_nvg, textColor);
    nvgText(s_nvg, 0, 0, str, str+len);

    nvgResetTransform(s_nvg); // undo rotation
    return true;
}

bool OGLPad::TextW (int x, int y, const wchar_t *str, int len)
{
	printf("OGLPad::TextW\n");
	abort();
	exit(EXIT_FAILURE);
	return true;
}


/**
 * \brief Draws a single pixel in a specified colour.
 * \param x x-coordinate of point [pixel]
 * \param y y-coordinate of point [pixel]
 * \param col pixel colour (format: 0xBBGGRR)
 */
void OGLPad::Pixel (int x, int y, uint32_t col)
{
	printf("OGLPad::Pixel unimpl\n");
	abort();
	exit(EXIT_FAILURE);
}

void OGLPad::MoveTo (int x, int y)
{
	m_curX = x;
	m_curY = y;
}

void OGLPad::Line (int x0, int y0, int x1, int y1)
{
	m_curX = x0;
	m_curY = y0;
	LineTo(x1, y1);
}

void OGLPad::LineTo (int x, int y)
{
	if(cpen->m_Style != 0) {
	    nvgBeginPath(s_nvg);
	    nvgMoveTo(s_nvg, m_curX - m_xOrigin, m_curY - m_yOrigin);
	    nvgLineTo(s_nvg, x - m_xOrigin, y - m_yOrigin);
	    nvgStrokeFromPen();
	}

	m_curX = x;
	m_curY = y;
}

void OGLPad::Rectangle (int x0, int y0, int x1, int y1)
{
    nvgBeginPath(s_nvg);
    nvgRect(s_nvg, x0 - m_xOrigin, y0 - m_yOrigin, std::abs(x1-x0), std::abs(y1-y0));
	nvgStrokeFromBrush();
	nvgStrokeFromPen();
}

void OGLPad::Ellipse (int x0, int y0, int x1, int y1)
{
    nvgBeginPath(s_nvg);
    float xcenter = (x0+x1)/2;
    float ycenter = (y0+y1)/2;
    nvgEllipse(s_nvg, xcenter - m_xOrigin, ycenter - m_yOrigin, std::abs(x1-x0)/2, std::abs(y1-y0)/2);
	nvgStrokeFromBrush();
	nvgStrokeFromPen();
}

void OGLPad::Polygon (const oapi::IVECTOR2 *pt, int npt)
{
    nvgBeginPath(s_nvg);
    nvgMoveTo(s_nvg, pt->x - m_xOrigin, pt->y - m_yOrigin);

    for (int i = 1; i < npt; ++i)
    {
        auto point = pt[i];
        nvgLineTo(s_nvg, point.x - m_xOrigin, point.y - m_yOrigin);
    }

    nvgClosePath(s_nvg);
	nvgStrokeFromBrush();
	nvgStrokeFromPen();
}

void OGLPad::Polyline (const oapi::IVECTOR2 *pt, int npt)
{
    nvgBeginPath(s_nvg);
    nvgMoveTo(s_nvg, pt->x - m_xOrigin, pt->y - m_yOrigin);

    for (int i = 1; i < npt; ++i)
    {
        auto point = pt[i];
        nvgLineTo(s_nvg, point.x - m_xOrigin, point.y - m_yOrigin);
    }

    nvgStrokeFromPen();
}

void OGLPad::PolyPolygon (const oapi::IVECTOR2 *pt, const int *npt, const int nline)
{
	int offset = 0;
	for(int i=0;i<nline;i++) {
		Polygon (&pt[offset], npt[i]);
		offset+=npt[i];
	}
}

void OGLPad::PolyPolyline (const oapi::IVECTOR2 *pt, const int *npt, const int nline)
{
	int offset = 0;
	for(int i=0;i<nline;i++) {
		Polyline (&pt[offset], npt[i]);
		offset+=npt[i];
	}
}

OGLFont::OGLFont (int height, bool prop, const char *face, Style style, int orientation): oapi::Font (height, prop, face, style, orientation)
{
	rotationRadians = -3.1415926535898/180.0 * (orientation * 0.1f);
    m_facename = face;
    m_Height = abs(height);

	//fontconfig fails to deliver a mono font if the family is not found -> force default font for monospace
	if(!prop) m_facename="monospace";
	if(style & Font::BOLD) m_facename+=":bold";
	if(style & Font::ITALIC) m_facename+=":italic";

	std::string facename = m_facename;
	facename+=height;

	if(auto n = fontCache.find(facename); n!=fontCache.end()) {
		m_fontfile = n->second;
		return;
	}

	FcConfig* config = FcInitLoadConfigAndFonts();
	FcPattern* pat = FcNameParse((const FcChar8*)m_facename.c_str());
	m_facename+=height;
	FcConfigSubstitute(config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);

	FcResult result;

	FcPattern* font = FcFontMatch(config, pat, &result);

	if (font) {
		FcChar8* file = NULL; 

		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
		{
			m_fontfile = (char*)file;
			fontCache[m_facename] = m_fontfile;
		}
	}
	FcPatternDestroy(font);
	FcPatternDestroy(pat);
	FcConfigDestroy(config);
}

OGLFont::~OGLFont ()
{

}
