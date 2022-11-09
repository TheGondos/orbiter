#pragma once

class OGLTexture;
namespace Renderer
{
    enum BoolParam {
        DEPTH_TEST,
        BLEND,
        CULL_FACE,
        STENCIL_TEST,
        LAST_BOOL_PARAM
    };
    enum FrontFace {
        CW  = GL_CW,
        CCW = GL_CCW
    };

#ifdef DEBUG
    void CheckError(const char *s);
#else
    inline void CheckError(const char *s) {}
#endif
    void GlobalInit(int w, int h);
    void PushBool(BoolParam param, bool value);
    void PopBool(int num = 1);
    void PushDepthMask(bool);
    void PopDepthMask();
    void PushFrontFace(FrontFace);
    void PopFrontFace();
    void SetViewPort(int w, int h);
    void PushRenderTarget(OGLTexture *);
    void PopRenderTarget();
    void PushBlendFunc(GLenum a, GLenum b);
    void PopBlendFunc();
    void Sync();
};
