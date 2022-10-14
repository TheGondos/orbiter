#include "glad.h"
#include "Renderer.h"
#include "Texture.h"
#include <vector>
#include <cassert>
#include <cstring>
#include <stdio.h>

namespace Renderer
{

void CheckError(const char *s) {
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
    // Process/log the error.
        printf("GLError: %s - 0x%04X\n", s, err);
        assert(0);
        exit(-1);
    }
}

struct FBParamSave {
    GLint fb;
    GLint viewport[4];
};

struct BoolParamSave {
    BoolParam param;
    GLenum glparam;
    GLboolean value;
};

std::vector<BoolParamSave> lastBoolParams;
std::vector<FBParamSave> lastFBParams;
bool boolParams[LAST_BOOL_PARAM];
GLint current_fb;
GLint current_viewport[4];

bool current_depthmask;
std::vector<bool> lastDepthmasks;

void PushDepthMask(bool value) {
    lastDepthmasks.push_back(current_depthmask);
    current_depthmask = value;
    glDepthMask(value ? GL_TRUE : GL_FALSE);
}
void PopDepthMask() {
    bool v = lastDepthmasks.back();
    lastDepthmasks.pop_back();
    current_depthmask = v;
    glDepthMask(current_depthmask ? GL_TRUE : GL_FALSE);
}

FrontFace current_frontface;
std::vector<FrontFace> lastFrontFaces;
void PushFrontFace(FrontFace ff) {
    lastFrontFaces.push_back(current_frontface);
    current_frontface = ff;
    glFrontFace(ff);

}
void PopFrontFace() {
    FrontFace v = lastFrontFaces.back();
    lastFrontFaces.pop_back();
    current_frontface = v;
    glFrontFace(current_frontface);
}

struct BlendParamSave {
    GLenum a;
    GLenum b;
};
BlendParamSave current_blendfunc;
std::vector<BlendParamSave> lastblendfunc;

void PushBlendFunc(GLenum a, GLenum b) {
    BlendParamSave save{a, b};
    lastblendfunc.push_back(current_blendfunc);
    current_blendfunc = save;
    glBlendFunc(a, b);
}
void PopBlendFunc() {
    BlendParamSave v = lastblendfunc.back();
    lastblendfunc.pop_back();
    glBlendFunc(current_blendfunc.a, current_blendfunc.b);
}

void GlobalInit() {
    PushBool(DEPTH_TEST, true);
    PushBool(STENCIL_TEST, false);
    PushBool(BLEND, true);
    PushBool(CULL_FACE, true);
    PushDepthMask(true);
    PushFrontFace(CW);
    PushBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    current_fb = 0;
    current_viewport[0] = 0;
    current_viewport[1] = 0;
    current_viewport[2] = 100;
    current_viewport[3] = 100;
}

static GLenum bpToGL(enum BoolParam param) {
    switch(param) {
        case DEPTH_TEST:   return GL_DEPTH_TEST;
        case BLEND:        return GL_BLEND;
        case CULL_FACE:    return GL_CULL_FACE;
        case STENCIL_TEST: return GL_STENCIL_TEST;
        default:
            assert(0);
            break;
    }
}

void Sync() {
    for(int i=0; i<LAST_BOOL_PARAM;i++) {
        GLenum e = bpToGL((BoolParam)i);
        if(boolParams[i]) {
            glEnable(e);
        } else {
            glDisable(e);
        }
    }
    glDepthMask(current_depthmask ? GL_TRUE : GL_FALSE);
    glFrontFace(current_frontface);
    glBlendFunc(current_blendfunc.a, current_blendfunc.b);
}

void PushBool(enum BoolParam param, bool value) {
    BoolParamSave save{param, bpToGL(param), boolParams[param]};
    lastBoolParams.push_back(save);
    boolParams[param] = value;
    if(value)
        glEnable(save.glparam);
    else
        glDisable(save.glparam);
}
void PopBool(int num) {
    for(int i = 0; i < num; i++) {
        BoolParamSave save = lastBoolParams.back();
        lastBoolParams.pop_back();
        boolParams[save.param] = save.value;
        if(save.value) {
            glEnable(save.glparam);
        } else {
            glDisable(save.glparam);
        }
        CheckError("PopBool");
    }
}

void PopRenderTarget() {
    FBParamSave save = lastFBParams.back();
    lastFBParams.pop_back();
    //printf("PopRenderTarget -> %d - %d %d %d %d\n", save.fb, save.viewport[0],save.viewport[1],save.viewport[2],save.viewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, save.fb);
    glViewport(save.viewport[0],save.viewport[1],save.viewport[2],save.viewport[3]);
    current_fb = save.fb;
    memcpy(current_viewport, save.viewport, sizeof(current_viewport));
}

void PushRenderTarget(OGLTexture *tex) {
    //printf("PushRenderTarget %d - %d %d %d %d\n", current_fb, current_viewport[0],current_viewport[1],current_viewport[2],current_viewport[3]);
    assert(tex);

    FBParamSave save;
    save.fb = current_fb;
    memcpy(save.viewport, current_viewport, sizeof(current_viewport));
    lastFBParams.push_back(save);

    if(!tex->m_FBO) {
        glGenFramebuffers(1, &tex->m_FBO);
        CheckError("glGenFramebuffers(1, &m_FBO)");
        glBindFramebuffer(GL_FRAMEBUFFER, tex->m_FBO);
        CheckError("glBindFramebuffer(GL_FRAMEBUFFER, m_FBO)");
        glBindTexture(GL_TEXTURE_2D, tex->m_TexId);
        CheckError("glBindTexture(GL_TEXTURE_2D, m_TexId)");

        // Poor filtering. Needed !
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//glGenerateMipmap(GL_TEXTURE_2D);
        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->m_TexId, 0);
        CheckError("SetAsTarget");
        glBindTexture(GL_TEXTURE_2D, 0);
        CheckError("SetAsTarget");
        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
        CheckError("SetAsTarget");

        switch(glCheckFramebufferStatus(GL_FRAMEBUFFER))
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                printf("SetAsTarget: glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
                abort();
                exit(-1);
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                printf("SetAsTarget: glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
                abort();
                exit(-1);
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                printf("SetAsTarget: glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
                abort();
                exit(-1);
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                printf("SetAsTarget: glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
                abort();
                exit(-1);
            case GL_FRAMEBUFFER_UNSUPPORTED:
                printf("SetAsTarget: glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_UNSUPPORTED\n");
                abort();
                exit(-1);
            case GL_FRAMEBUFFER_COMPLETE:
                break;
        }

    } else {
        CheckError("SetAsTarget");
        glBindFramebuffer(GL_FRAMEBUFFER, tex->m_FBO);
        /*
        CheckError("SetAsTarget");
        glBindTexture(GL_TEXTURE_2D, tex->m_TexId);
        CheckError("glBindTexture(GL_TEXTURE_2D, m_TexId)");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		CheckError("OGLPad glTexParameteri");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		CheckError("OGLPad glTexParameteri2");*/
    }
    current_fb = tex->m_FBO;
    current_viewport[0] = 0;
    current_viewport[1] = 0;
    current_viewport[2] = tex->m_Width;
    current_viewport[3] = tex->m_Height;
    glViewport(0,0,tex->m_Width,tex->m_Height);
}


};
