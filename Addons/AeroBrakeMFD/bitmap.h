#ifndef __CPWDIB_H
#define __CPWDIB_H

#if 0
#include <windows.h>

/*****************
* CPGdiObj Class *
*****************/

class CPGdiObj
{
public:
  CPGdiObj();
  virtual ~CPGdiObj();

  virtual void FreeObj();

  void Stock(int nType);
  HGDIOBJ Obj() const {return hObj;}

protected:
  friend class CPDC;

  HGDIOBJ hObj;    //l'oggetto GDI
};


/******************
* CPPalette Class *
******************/

class CPPalette: public CPGdiObj
{
  typedef CPGdiObj inherited;
public:
  CPPalette();
  CPPalette(HPALETTE hpal);
  CPPalette(const CPPalette* pal);
  CPPalette(const int n, const unsigned char* red,
            const unsigned char* green, const unsigned char* blue);
  virtual ~CPPalette();

  //Inherited section
  virtual void FreeObj();

  //Local section
  HPALETTE Palette() const {return reinterpret_cast<HPALETTE>(hObj);}

  bool Create(const int n, const unsigned char* red,
              const unsigned char* green, const unsigned char* blue);
  bool CreateDefault();

  int GetIndex(const unsigned char red, const unsigned char green,
               const unsigned char blue, bool exact=true) const;
  int GetIndex(COLORREF col, bool exact=true) const;
  bool GetRGB(const int index, unsigned char& red,
              unsigned char& green, unsigned char& blue) const;
  bool GetRGB(const int index, COLORREF& col) const;
};

 

// Inline functions

// WidthBytes performs DWORD-aligning of DIB scanlines.  The "bits"
// parameter is the bit count for the scanline (biWidth * biBitCount),
// and this function returns the number of DWORD-aligned bytes needed
// to hold those bits.
inline DWORD WidthBytes(DWORD bits)
{
  return ((bits + 31) / 32 * 4);
}



/**************
* CPDIB Class *
**************/

class CPDIB
{
public:
  CPDIB();
  ~CPDIB();

  operator BITMAPINFO*() {return info;}

  bool Create(int bits, int w, int h);
  bool Load(const char* file);

  LONG Width() const;
  LONG Height() const;
  WORD Depth() const;
  WORD NumColors() const;
  DWORD ImageSize() const;
  WORD PaletteSize() const;

  RGBQUAD* Colors() const;
  BYTE* Bits() const;

  CPPalette* CreatePalette() const;
  bool SetUsage(UINT usage, HPALETTE hpal=NULL);

  void Paint(HDC dc, int xd, int yd, int wd, int hd,int xs, int ys, int ws, int hs);

private:
  static const int HEADER_MARKER;
  BITMAPINFO* info;
  size_t alloc;

  void Copy(BITMAPINFO* source, size_t size);
  void Reset();
  bool Init(int bits, int w, int h);
  bool ConvertOldFormat(BYTE* bits);
};

#endif
#endif//ndef __CPWDIB_H
