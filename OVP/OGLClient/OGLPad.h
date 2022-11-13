// Sketchpad implementation, heavily inspired from SkyboltClient
/*
Copyright 2021 Matthew Reid
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "GraphicsAPI.h"
//#include "stb_truetype.h"
#include <string>
#include "Texture.h"
#include "Shader.h"
#include "nanovg/src/nanovg.h"

using namespace oapi;

#ifdef OGLCLIENT_EXPORTS
#define OGLCLIENTEXPORT DLLEXPORT
#else
#define OGLCLIENTEXPORT
#endif

// ======================================================================
// class OGLPad
// ======================================================================
/**
 * \brief The OGLPad class defines the context for 2-D drawing using
 *   OpenGL calls.
 */
class OGLFont;
class OGLPen;
class OGLBrush;
class OGLCLIENTEXPORT OGLPad: public oapi::Sketchpad {
public:
	OGLPad (SURFHANDLE s);
	~OGLPad ();
	oapi::Font *SetFont (oapi::Font *font) const;
	oapi::Pen *SetPen (oapi::Pen *pen) const;
	oapi::Brush *SetBrush (oapi::Brush *brush) const;
	void SetTextAlign (TAlign_horizontal tah=LEFT, TAlign_vertical tav=TOP);
	uint32_t SetTextColor (uint32_t col);
	uint32_t SetBackgroundColor (uint32_t col);
	void SetBackgroundMode (BkgMode mode);
	int GetCharSize ();
	int GetTextWidth (const char *str, int len = 0);
	void SetOrigin (int x, int y);
	void GetOrigin (int *x, int *y) const;
	bool Text (int x, int y, const char *str, int len);
	bool TextW (int x, int y, const wchar_t *str, int len);

	//bool TextBox (int x1, int y1, int x2, int y2, const char *str, int len);

	void Pixel (int x, int y, uint32_t col);
	void MoveTo (int x, int y);
	void LineTo (int x, int y);
	void Line (int x0, int y0, int x1, int y1);
	void Rectangle (int x0, int y0, int x1, int y1);
	void Ellipse (int x0, int y0, int x1, int y1);
	void Polygon (const oapi::IVECTOR2 *pt, int npt);
	void Polyline (const oapi::IVECTOR2 *pt, int npt);
	void PolyPolygon (const oapi::IVECTOR2 *pt, const int *npt, const int nline);
	void PolyPolyline (const oapi::IVECTOR2 *pt, const int *npt, const int nline);

    static void GlobalInit();
    static void GlobalExit();

private:
    void nvgStrokeFromPen();
    void nvgStrokeFromBrush();
	mutable OGLFont *cfont; // currently selected font (NULL if none)
	mutable OGLPen *cpen;   // currently selected pen (NULL if none)
	mutable OGLBrush *cbrush; // currently selected brush (NULL if none)
    static NVGcontext *s_nvg_fb;
    static NVGcontext *s_nvg_mfd;
	NVGcontext *s_nvg;

	uint32_t height;
	uint32_t width;
	NVGcolor textColor;
    uint32_t textColorNative;
	NVGcolor bgColor;
    uint32_t bgColorNative;
	int32_t m_curX;
	int32_t m_curY;
	OGLTexture *m_tex;

	int m_TextAlign;
    BkgMode m_TextBackgroundMode;

	int m_xOrigin;
	int m_yOrigin;
};


// ======================================================================
// class OGLFont
// ======================================================================
class OGLCLIENTEXPORT OGLFont: public oapi::Font {
public:
	OGLFont (int height, bool prop, const char *face, Style style=NORMAL, int orientation=0);
	~OGLFont ();

	int m_Height;
    float rotationRadians = 0;
    std::string m_facename;
    std::string m_fontfile;
};


// ======================================================================
// class OGLPen
// ======================================================================
class OGLCLIENTEXPORT OGLPen: public oapi::Pen {
public:
	OGLPen (int style, int width, uint32_t col):oapi::Pen(style, width, col) { m_Style = style; m_Width = width; m_Color = col;}
	~OGLPen () {}

	int m_Style;
	int m_Width;
	uint32_t m_Color;
};

// ======================================================================
// class OGLBrush
// ======================================================================
class OGLCLIENTEXPORT OGLBrush: public oapi::Brush {
public:
	OGLBrush (uint32_t col):oapi::Brush(col) { m_Color = col; }
	~OGLBrush () {}

	uint32_t m_Color;
};
