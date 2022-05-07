#include "bitmap.h"
#include <stdio.h>

//#include "cpwdc.h"
//#include "appresource.h"

/**************
* CPDIB Class *
**************/
#if 0
//Public section

CPDIB::CPDIB()
  :
  info(NULL),
  alloc(0)
{
  //nothing to do
}


CPDIB::~CPDIB()
{
  Reset();
}

bool CPDIB::Create(
  int bits,
  int w,
  int h)
{
  Reset();
  if (!Init(bits, w, h)) return false;

  DWORD colors = NumColors();
  DWORD* rgb = reinterpret_cast<DWORD*>(Colors());
  if (colors==2)
  {
    rgb[0] = 0x00000000;
    rgb[1] = 0x00FFFFFF;
  }
  register DWORD i = 0;
  for (colors/=16; i<colors; ++i)
  {
    *rgb++ = 0x00000000;    // 0000  black
    *rgb++ = 0x00800000;    // 0001  dark red
    *rgb++ = 0x00008000;    // 0010  dark green
    *rgb++ = 0x00808000;    // 0011  mustard
    *rgb++ = 0x00000080;    // 0100  dark blue
    *rgb++ = 0x00800080;    // 0101  purple
    *rgb++ = 0x00008080;    // 0110  dark turquoise
    *rgb++ = 0x00C0C0C0;    // 1000  gray
    *rgb++ = 0x00808080;    // 0111  dark gray
    *rgb++ = 0x00FF0000;    // 1001  red
    *rgb++ = 0x0000FF00;    // 1010  green
    *rgb++ = 0x00FFFF00;    // 1011  yellow
    *rgb++ = 0x000000FF;    // 1100  blue
    *rgb++ = 0x00FF00FF;    // 1101  pink (magenta)
    *rgb++ = 0x0000FFFF;    // 1110  cyan
    *rgb++ = 0x00FFFFFF;    // 1111  white
  }

  return true;
}


bool CPDIB::Load(const char* file){
  Reset();

  // Go read the DIB file header and check if it's valid.
  BITMAPFILEHEADER header;
  FILE* f;
  f=fopen(file,"rb");
  if (!f){
    return false;
  }
  if (fread(&header, sizeof(header),1,f)!=1 || header.bfType != HEADER_MARKER){
    fclose(f);
    return false;
  }

  // Allocate memory for DIB
  alloc = header.bfSize - sizeof(BITMAPFILEHEADER);
  BYTE* b = new BYTE[alloc];
  info = reinterpret_cast<BITMAPINFO*>(b);
  //Get a pointer to bits
  b += header.bfOffBits - sizeof(BITMAPFILEHEADER);
  
  // Go read the bits.
  if (!info || fread(info, alloc,1,f)!=1){
    Reset();
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
  //return ConvertOldFormat(b);
}


LONG CPDIB::Width() const
{
  return info? info->bmiHeader.biWidth: 0;
}



LONG CPDIB::Height() const
{
  return info? info->bmiHeader.biHeight: 0;
}



WORD CPDIB::Depth() const
{
  return info? info->bmiHeader.biBitCount: 0;
}



WORD CPDIB::NumColors() const
{
  WORD bits = 0;
  if (info)
  {
    DWORD used = info->bmiHeader.biClrUsed;
    if (used) return (WORD)used;
    bits = info->bmiHeader.biBitCount;
  }
  switch (bits)
  {
  case 1:  return 2;
  case 4:  return 16;
  case 8:  return 256;
  default: return 0;
  }
}



DWORD CPDIB::ImageSize() const
{
  return info? info->bmiHeader.biSizeImage: 0;
}



WORD CPDIB::PaletteSize() const
{
  return info? NumColors()*sizeof(RGBQUAD): 0;
}



RGBQUAD* CPDIB::Colors() const
{
  if (!info) return NULL;

  return (RGBQUAD*)((BYTE*)info + info->bmiHeader.biSize);
}



BYTE* CPDIB::Bits() const
{
  if (!info) return NULL;

  DWORD offset = info->bmiHeader.biCompression==BI_BITFIELDS?
                 3: NumColors(); //info->bmiHeader.biClrUsed;
  return (BYTE*)(Colors() + offset);
}



CPPalette* CPDIB::CreatePalette() const
{
  CPPalette* res = NULL;
  const WORD colors = NumColors();
  if (colors)
  {
    BYTE* r = new(BYTE[colors]);
    BYTE* g = new(BYTE[colors]);
    BYTE* b = new(BYTE[colors]);
    register int i = 0;
    for (; i<colors; ++i)
    {
      r[i] = info->bmiColors[i].rgbRed;
      g[i] = info->bmiColors[i].rgbGreen;
      b[i] = info->bmiColors[i].rgbBlue;
    }
    res = new(CPPalette);
    if (!res->Create(colors, r, g, b))
    {
      delete res;
      res = NULL;
    }
    delete [] r;
    delete [] g;
    delete [] b;
  }
  return res;
}



bool CPDIB::SetUsage(
  UINT usage,
  HPALETTE hpal /* =NULL */)
{
  if (!info) return false;

  WORD colors = NumColors();
  if (colors>0)
  {
    switch (usage)
    {
    case DIB_PAL_COLORS:
      {
        // Set the DIB color table to palette indexes
        WORD FAR* pw = reinterpret_cast<WORD*>(Colors());
        register WORD n = 0;
        for (; n<colors; ++n,++pw) *pw = n;
        return true;
      }
    default:
    case DIB_RGB_COLORS:
      {
        // Set the DIB color table to RGBQUADS
        PALETTEENTRY ape[256];
        if (colors>256) colors = 256;
        if (!hpal) hpal = (HPALETTE)::GetStockObject(DEFAULT_PALETTE);
        if (::GetPaletteEntries(hpal, 0, colors, ape))
        {
          RGBQUAD* rgb = Colors();
          register int n = 0;
          for (; n<colors; ++n)
          {
            rgb[n].rgbBlue = ape[n].peBlue;
            rgb[n].rgbGreen = ape[n].peGreen;
            rgb[n].rgbRed = ape[n].peRed;
            rgb[n].rgbReserved = ape[n].peFlags;
          }
          return true;
        }
        break;
      }
    }
  }
  return false;
}

const unsigned long ROP_DSna = 0x00220326;

void CPDIB::Paint(
  HDC dc,
  int xd,
  int yd,
  int wd,
  int hd,
  int xs,
  int ys,
  int ws,
  int hs)
{
  // Get target DC (for simplicity) and create the bitamp to draw.
  const HDC target = dc;
  HGDIOBJ bitmap = ::CreateDIBitmap(target, &info->bmiHeader,
                      CBM_INIT, Bits(), info, DIB_RGB_COLORS);

  // Create three temporary DCs and two temporary bitmaps.
  const HDC source = ::CreateCompatibleDC(target);
  const HDC mask = ::CreateCompatibleDC(target);
  const HDC copy = ::CreateCompatibleDC(target);
  HGDIOBJ mask_bmp = ::CreateCompatibleBitmap(mask, wd, hd);
  HGDIOBJ copy_bmp = ::CreateCompatibleBitmap(target, wd, hd);

  // Get effective source dimensions.
  RECT or = {0, 0, Width(), Height()};
  RECT sr = {xs, ys, xs+ws, ys+hs};
  ::IntersectRect(&or, &or, &sr);
  ws = or.right - (xs=or.left);
  hs = or.bottom - (ys=or.top);

  if (ws!=wd || hs!=hd)
  {
    // Resize bitmap to destination dimensions.
    HGDIOBJ bmp = ::SelectObject(source, bitmap);
    const HDC temp = ::CreateCompatibleDC(target);
    bitmap = ::CreateCompatibleBitmap(target, wd, hd);
    ::SelectObject(temp, bitmap);
    if (ws==0 || hs==0)
    {
      ::BitBlt(temp, 0, 0, wd, hd, target, xd, yd, SRCCOPY);
    }
    else
    {
      ::SetStretchBltMode(temp, COLORONCOLOR);
      ::StretchBlt(temp, 0, 0, wd, hd, source, xs, ys, ws, hs, SRCCOPY);
    }
    ::DeleteObject(::SelectObject(source, bmp));
    ::DeleteDC(temp);
    xs = ys = 0; // ws = wd; hs = hd;
  }

  // Select a bitmap for each DC (and store old object for final clean-up).
  bitmap = ::SelectObject(source, bitmap);
  mask_bmp = ::SelectObject(mask, mask_bmp);
  copy_bmp = ::SelectObject(copy, copy_bmp);

  // Set proper mapping mode.
  ::SetMapMode(source, ::GetMapMode(target));

  // Create a monochrome mask where we have 0's in the image, 1's elsewhere.
  COLORREF back = ::SetBkColor(source, 0);
  ::BitBlt(mask, 0, 0, wd, hd, source, xs, ys, SRCCOPY);
  ::SetBkColor(source, back);

  if (1)
  {
    // Copy the background of the target DC to the destination.
    ::BitBlt(copy, 0, 0, wd, hd, target, xd, yd, SRCCOPY);
  }
  else
  {
    // Create and select a brush of the background color.
    HGDIOBJ brush = ::CreateSolidBrush(back);
    brush = ::SelectObject(copy, brush);
    ::PatBlt(copy, 0, 0, wd, hd, PATCOPY);
    ::DeleteObject(::SelectObject(copy, brush));
  }

  // Mask out the places where the bitmap will be placed.
  ::BitBlt(copy, 0, 0, wd, hd, mask, 0, 0, SRCAND);

  // Mask out the transparent colored pixels on the bitmap.
  ::BitBlt(source, xs, ys, wd, hd, mask, 0, 0, ROP_DSna);

  // XOR the bitmap with the background on the destination DC.
  ::BitBlt(copy, 0, 0, wd, hd, source, xs, ys, SRCPAINT);

  // Copy the destination to the screen.
  ::BitBlt(target, xd, yd, wd, hd, copy, 0, 0, SRCCOPY);

  // Delete temporary bitmaps and DCs.
  ::DeleteObject(::SelectObject(copy, copy_bmp));
  ::DeleteObject(::SelectObject(mask, mask_bmp));
  ::DeleteObject(::SelectObject(source, bitmap));
  ::DeleteDC(copy);
  ::DeleteDC(mask);
  ::DeleteDC(source);
}

//Private section

const int CPDIB::HEADER_MARKER = 0x4d42; // 'BM'


void CPDIB::Copy(
  BITMAPINFO* source,
  size_t size)
{
  if (source)
  {
    info = reinterpret_cast<BITMAPINFO*>(new (BYTE[size]));
    if (info)
    {
      memcpy(info, source, size);
      alloc = size;
    }
  }
}



void CPDIB::Reset()
{
  if (info)
  {
    delete [] info;
    info = NULL;
  }
  alloc = 0;
}



bool CPDIB::Init(
  int bits,
  int w,
  int h)
{
  if (info) return false;

  DWORD size_image = WidthBytes(bits*w)*h;
  DWORD colors = bits==1? 2: bits==4? 16: bits==8? 256: 0;
  alloc = sizeof(BITMAPINFOHEADER) + size_image +
          sizeof(RGBQUAD) * colors;
  info = reinterpret_cast<BITMAPINFO*>(new(BYTE[alloc]));
  if (!info)
  {
    alloc = 0;
    return false;
  }

  info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info->bmiHeader.biWidth = w;
  info->bmiHeader.biHeight = h;
  info->bmiHeader.biPlanes = 1;
  info->bmiHeader.biBitCount = bits;
  info->bmiHeader.biCompression = BI_RGB;
  info->bmiHeader.biSizeImage = size_image;
  info->bmiHeader.biXPelsPerMeter = 0;
  info->bmiHeader.biYPelsPerMeter = 0;
  info->bmiHeader.biClrUsed = colors;
  info->bmiHeader.biClrImportant = 0;
  return true;
}



bool CPDIB::ConvertOldFormat(
  BYTE* bits)
{
  //Precondizioni

  bool res = true;
  if (info->bmiHeader.biSize==sizeof(BITMAPCOREHEADER))
  {
    /* NC 19-02-2001
     * Codice non testato direttamente, in mancanza di file .BMP
     * in formato vecchio. Il test � stato eseguito duplicando un
     * file in formato nuovo.
     */
    BITMAPINFO* old_info = info;
    info = NULL;
    BITMAPCOREHEADER* h = reinterpret_cast<BITMAPCOREHEADER*>(old_info);
    res = Init(h->bcBitCount, h->bcWidth, h->bcHeight);
    if (res)
    {
      DWORD colors = NumColors();
      if (colors)
      {
        RGBQUAD* q = Colors();
        RGBTRIPLE* t = (RGBTRIPLE*)((BYTE*)h + sizeof(BITMAPCOREHEADER));
        register DWORD i = 0;
        for (; i<colors; ++i)
        {
          q[i].rgbBlue = t[i].rgbtBlue;
          q[i].rgbGreen = t[i].rgbtGreen;
          q[i].rgbRed = t[i].rgbtRed;
          q[i].rgbReserved = 0;
        }
      }
      memcpy(Bits(), bits, ImageSize());
    }
    delete [] old_info;
  }
  return res;
}

/*****************
* CPGdiObj Class *
*****************/

CPGdiObj::CPGdiObj()
  :
  hObj(NULL)
{
  //nothing to do
}



CPGdiObj::~CPGdiObj()
{
  FreeObj();
}


void CPGdiObj::FreeObj()
{
  if (hObj)
  {
    ::DeleteObject(hObj);
    hObj = NULL;
  }
}



void CPGdiObj::Stock(
  int nType)
{
  FreeObj();
  hObj = ::GetStockObject(nType);
}


 
/******************
* CPPalette Class *
******************/

CPPalette::CPPalette()
{
  //nothing to do
}



CPPalette::CPPalette(
  HPALETTE hpal)
{
  hObj = hpal;
}


CPPalette::CPPalette(
  const CPPalette* pal)
{
  const int size = sizeof(LOGPALETTE) + 255*sizeof(PALETTEENTRY);
  LOGPALETTE* p = (LOGPALETTE*)new(BYTE[size]);
  int count = ::GetPaletteEntries(pal->Palette(), 0, 256,
                                  p->palPalEntry);
  p->palVersion = 0x300;
  p->palNumEntries = count;
  hObj = ::CreatePalette(p);
  delete [] p;
}



CPPalette::CPPalette(
  const int n,
  const unsigned char* red,
  const unsigned char* green,
  const unsigned char* blue)
{
  Create(n, red, green, blue);
}



CPPalette::~CPPalette()
{
}


void CPPalette::FreeObj()
{
  inherited::FreeObj();
}

const int PALVERSION = 0x300;

bool CPPalette::Create(
  const int n,
  const unsigned char* red,
  const unsigned char* green,
  const unsigned char* blue)
{
  FreeObj();
  
  const int size = sizeof(LOGPALETTE) + (n-1)*sizeof(PALETTEENTRY);
  LOGPALETTE* p = (LOGPALETTE*)new(BYTE[size]);
  if (!p) return false;

  p->palVersion = PALVERSION;
  p->palNumEntries = n;
  register int i = 0;
  for (; i<n; ++i)
  {
    p->palPalEntry[i].peRed = red[i];
    p->palPalEntry[i].peGreen = green[i];
    p->palPalEntry[i].peBlue = blue[i];
    p->palPalEntry[i].peFlags = 0;
  }
  hObj = ::CreatePalette(p);
  delete [] p;
  return (hObj != NULL);
}



bool CPPalette::CreateDefault()
{
  FreeObj();

  PALETTEENTRY pe[20];
  UINT len = ::GetSystemPaletteEntries(::GetDC(NULL), 0, 20, pe);
  if (len<=20)
  {
    const int size = sizeof(LOGPALETTE) + 19*sizeof(PALETTEENTRY);
    LOGPALETTE* p = (LOGPALETTE*)new(BYTE[size]);
    if (!p) return false;

    p->palVersion = PALVERSION;
    p->palNumEntries = len;
    register UINT i = 0;
    for (; i<len; ++i) p->palPalEntry[i] = pe[i];
    hObj = ::CreatePalette(p);
    delete [] p;
  }
  return (hObj != NULL);
}



int CPPalette::GetIndex(
  const unsigned char red,
  const unsigned char green,
  const unsigned char blue,
  bool exact /* =true */) const
{
  return GetIndex(RGB(red, green, blue), exact);
}



int CPPalette::GetIndex(
  COLORREF col,
  bool exact /* =true */) const
{
  int res = ::GetNearestPaletteIndex(Palette(), col);
  if (res!=CLR_INVALID && exact)
  {
    COLORREF comp = col;
    GetRGB(res, comp);
    if (comp!=col) res = -1;
  }
  return res;
}



bool CPPalette::GetRGB(
  const int index,
  unsigned char& red,
  unsigned char& green,
  unsigned char& blue) const
{
  if (!hObj || index < 0 || index > 255) return false;

  PALETTEENTRY entry;
  if (::GetPaletteEntries(Palette(), index, 1, &entry))
  {
    red = entry.peRed;
    green = entry.peGreen;
    blue = entry.peBlue;
    return true;
  }
  return false;
}



bool CPPalette::GetRGB(
  const int index,
  COLORREF& col) const
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  bool res = GetRGB(index, red, green, blue);
  if (res) col = RGB(red, green, blue);
  return res;
}


#endif
