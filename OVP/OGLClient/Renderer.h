#pragma once
#include <string>
#include <glm/glm.hpp>

class OGLTexture;
class Shader;

class OGLLight;
namespace Renderer
{
    void SetLight(OGLLight *);
    void GetLight(int idx, OGLLight *);
    void LightEnable(int, bool);
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
    void GlobalExit();
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
    Shader *GetShader(std::string name);
    void Bind(Shader *shader);
    void Unbind(Shader *shader);
    void PushLights();
    void EnableLighting(bool);
};
