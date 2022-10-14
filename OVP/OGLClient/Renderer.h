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

    void CheckError(const char *s);
    void GlobalInit();
    void PushBool(BoolParam param, bool value);
    void PopBool(int num = 1);
    void PushDepthMask(bool);
    void PopDepthMask();
    void PushFrontFace(FrontFace);
    void PopFrontFace();
    void PushRenderTarget(OGLTexture *);
    void PopRenderTarget();
    void PushBlendFunc(GLenum a, GLenum b);
    void PopBlendFunc();
    void Sync();
};
