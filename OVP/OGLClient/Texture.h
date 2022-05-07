#pragma once
//#include "glad.h"
#include <unordered_map>
#include <string>
#include "GraphicsAPI.h"

struct OGLTexture
{
    GLuint m_TexId;
    GLuint m_FBO;
    unsigned int m_RefCnt;
    bool m_InCache;
    int m_Width;
    int m_Height;
    bool Release();
    void AddRef() {m_RefCnt++;}
    void SetAsTarget(bool checkFB = true);
    void InitSize();
    uint32_t m_colorkey;
};

class TextureManager
{
public:
    TextureManager();
    ~TextureManager() {
        for(auto &kw:m_textures) {
            OGLTexture &tex = kw.second;
            glDeleteTextures(1, &tex.m_TexId);
        }
    }
    OGLTexture *CreateTextureFromTexId(GLuint texId, int width, int height);
    OGLTexture *LoadTexture(const char *filename, bool cache, int flags);
    OGLTexture *GetTextureForRendering(int w, int h);
    int LoadTextures (const char *fname, OGLTexture **pTex, uint32_t flags, int n);
    bool ReadTextureFromMemory (const char *buf, uint32_t ndata, OGLTexture **tex, uint32_t flags);
    void ReleaseTexture(OGLTexture *);

    std::unordered_map<std::string, OGLTexture> m_textures;
};
