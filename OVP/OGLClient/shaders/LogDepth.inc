// See Logarithmic Depth Buffer from https://outerra.blogspot.com/2012/11/maximizing-depth-buffer-range-and.html

#define VS_LOGZ() max(1e-6, gl_Position.w + 1.0)
#define FS_LOGZ(x) (log2(x) / log2(5e9f + 1.0)) // must match farplane in OGLCamera.cpp
