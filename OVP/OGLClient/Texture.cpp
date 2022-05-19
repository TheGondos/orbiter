#include "glad.h"
#include "Texture.h"
//#include "load_dds.h"
#include "OGLClient.h"
#include "SOIL2/incs/SOIL2.h"
#include <cstring>
#include "bmp.h"

static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
        abort();
        exit(-1);
	}
}

bool OGLTexture::Release()
{
    if(m_InCache) {
       // printf("Releasing texture in cache\n");
        return false;
        exit(-1);
    }
    m_RefCnt--;
    if(!m_RefCnt) {
        if(m_TexId) {
            glDeleteTextures(1, &m_TexId);
            m_TexId = 0;
        }

        if(m_FBO) {
            //TODO delete FBO
        }
        delete this;
        return true;
    }
    return false;
}

void OGLTexture::SetAsTarget(bool checkFB)
{
    if(checkFB) {
        GLint result;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
        CheckError("glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result)");
        assert(result == 0);
    }
    if(!m_FBO) {
        glGenFramebuffers(1, &m_FBO);
        CheckError("glGenFramebuffers(1, &m_FBO)");
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        CheckError("glBindFramebuffer(GL_FRAMEBUFFER, m_FBO)");
        glBindTexture(GL_TEXTURE_2D, m_TexId);
        CheckError("glBindTexture(GL_TEXTURE_2D, m_TexId)");

        // Poor filtering. Needed !
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//glGenerateMipmap(GL_TEXTURE_2D);
        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_TexId, 0);
glBindTexture(GL_TEXTURE_2D, 0);
        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

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

    }
    else
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    glViewport(0,0,m_Width,m_Height);
}

void OGLTexture::InitSize()
{
    glBindTexture(GL_TEXTURE_2D, m_TexId);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_Width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_Height);
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextureManager::TextureManager() {
}

static GLuint LoadTextureFromFile(const char *filename, int flags) {
    //printf("LoadTextureFromFile %s\n", filename);
    char fullpath[256];

    sprintf(fullpath,"Textures2/%s", filename);
    std::string hitex = oapiGetFilePath(fullpath);

    sprintf(fullpath,"Textures/%s", filename);
    std::string lowtex = oapiGetFilePath(fullpath);

    unsigned int SOILFlags = SOIL_FLAG_MIPMAPS;// SOIL_FLAG_TEXTURE_REPEATS;

    //FIXME: bitmaps must not be power of 2 or XR2 panel is broken
    GLuint tid;
    if(!strcmp(&filename[strlen(filename) - 4],".bmp")) {
        /*
        std::string bmpPath = oapiGetFilePath(filename);
        tid = loadBMPTexture(bmpPath.c_str());

        if(tid != 0)
            return tid;
*/
        flags |=4;
    }

    if(flags & 4) {
        SOILFlags = 0;
    }

    if(!(flags & 2)) {
        //SOILFlags |= SOIL_FLAG_DDS_LOAD_DIRECT;
        //SOIL_LOAD_RGBA SOIL_LOAD_AUTO
    }
 
    tid = SOIL_load_OGL_texture(hitex.c_str(), 	SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags);
    if(tid==0) {
        tid = SOIL_load_OGL_texture(lowtex.c_str(), 	SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags);
    }
    if(tid==0) {
        std::string path = oapiGetFilePath(filename);
        tid = SOIL_load_OGL_texture(path.c_str(), 	SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags);
        if(tid==0) {
            
            tid = loadBMPTexture(path.c_str());
        }
    }
    if(tid==0) {
        printf("TextureManager::LoadTexture failed for %s\n", filename);
        //abort();
        //exit(EXIT_FAILURE);
        return 0;
    }

    return tid;
}

OGLTexture *TextureManager::LoadTexture(const char *filename, bool cache, int flags)
{
    //printf("TextureManager::LoadTexture : %s\n", filename);

    if(!cache) {
        OGLTexture *tex = new OGLTexture;
        tex->m_TexId = LoadTextureFromFile(filename, flags);
        tex->m_RefCnt = 1;
        tex->m_InCache = false;
        tex->m_FBO = 0;
        tex->InitSize();
        tex->m_colorkey = 0x00ffffff;
        return tex;
    }

    auto search = m_textures.find(std::string(filename));
    if (search == m_textures.end()) {
        OGLTexture tex;
        tex.m_TexId = LoadTextureFromFile(filename, flags);
        tex.m_RefCnt = 1;
        tex.m_InCache = true;
        tex.m_FBO = 0;
        tex.m_colorkey = 0x00ffffff;
        tex.InitSize();
        m_textures[std::string(filename)] = tex;
        return &m_textures[std::string(filename)];
    } else {
       // printf("Texture found in cache\n");
        OGLTexture *tex = &m_textures[std::string(filename)];
        tex->m_RefCnt++;
        return tex;
    }
}

// =======================================================================

int TextureManager::LoadTextures (const char *fname, OGLTexture **pTex, uint32_t flags, int n)
{
	char cpath[256];
	int ntex = 0;
	FILE* ftex = 0;
	g_client->PlanetTexturePath(fname, cpath);

    GLuint *tmp = new GLuint[n];

	if ((ftex = fopen(cpath, "rb"))) {
		fclose(ftex);

        ntex = SOIL_direct_load_DDS_blob(
            cpath,
            (int *)tmp,
            n,
//            SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_TEXTURE_REPEATS);
            SOIL_FLAG_DDS_LOAD_DIRECT);

	} else if (g_client->TexturePath(fname, cpath)) {
		ftex = fopen(cpath, "rb");
		if (ftex) {
			fclose(ftex);

            ntex = SOIL_direct_load_DDS_blob(
                cpath,
                (int *)tmp,
                n,
//                SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_TEXTURE_REPEATS);
                SOIL_FLAG_DDS_LOAD_DIRECT);
		}
	} else {
//        printf("failed to open %s\n", fname);
        //exit(EXIT_FAILURE);
    }

    for(int i=0; i<ntex;i++) {
        pTex[i] = new OGLTexture;
        pTex[i]->m_InCache = false;
        pTex[i]->m_RefCnt = 0;
        pTex[i]->m_TexId = tmp[i];
        pTex[i]->m_FBO = 0;
        pTex[i]->m_colorkey = 0x00ffffff;
        pTex[i]->InitSize();
    }

    delete []tmp;

	return ntex;
}

/*
unsigned int SOIL_direct_load_DDS_from_memory(
		const unsigned char *const buffer,
		int *buffer_length,
		unsigned int reuse_texture_ID,
		int flags,
		int loading_as_cubemap );
*/
bool TextureManager::ReadTextureFromMemory (const char *buf, uint32_t ndata, OGLTexture **tex, uint32_t flags)
{
    *tex = new OGLTexture;
    size_t xxx = ndata;
    (*tex)->m_TexId = SOIL_direct_load_DDS_from_memory((const unsigned char *)buf, &xxx, 0, 0, 0);
    if((*tex)->m_TexId==0) {

        printf("%s\n",SOIL_last_result());
        abort();
        exit(-1);
    }

    (*tex)->m_RefCnt = 1;
    (*tex)->m_InCache = false;
    (*tex)->m_FBO = 0;
    (*tex)->m_colorkey = 0x00ffffff;
    (*tex)->InitSize();

    return (*tex)->m_TexId != 0;
}


void TextureManager::ReleaseTexture(OGLTexture *tex)
{
    if(tex->Release()) {
       // printf("No more references, texture deleted");
    }
}

// from http://www.opengl-tutorial.org/fr/intermediate-tutorials/tutorial-14-render-to-texture/
OGLTexture *TextureManager::GetTextureForRendering(int w, int h)
{
    OGLTexture *tex = new OGLTexture;
    tex->m_RefCnt = 1;
    tex->m_InCache = false;
    tex->m_Width = w;
    tex->m_Height = h;
    tex->m_colorkey = 0x00ffffff;
    GLint oldFB;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);

    glGenFramebuffers(1, &tex->m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tex->m_FBO);

    glGenTextures(1, &tex->m_TexId);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, tex->m_TexId);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, w, h, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // Poor filtering. Needed !
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->m_TexId, 0);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE\n");
        abort();
        exit(-1);
    }
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
    return tex;
}

OGLTexture *TextureManager::CreateTextureFromTexId(GLuint texId, int width, int height) {
    OGLTexture *tex = new OGLTexture;
    tex->m_RefCnt = 1;
    tex->m_InCache = true;
    tex->m_Width = width;
    tex->m_Height = height;
    tex->m_TexId = texId;
    tex->m_colorkey = 0x00ffffff;
    return tex;
}
