// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// Texture.cpp
// Texture loading and management routines for the D3D7 client.
//
// Methods for loading single (.dds) and multi-texture files (.tex)
// stored in DXT? format into DIRECTDRAWSURFACE7 instances.
// ==============================================================

#include "glad.h"
#include "OGLClient.h"
#include "Texture.h"
#include <cstring>
#include "SOIL2/incs/SOIL2.h"
#include <sys/stat.h>
#include "Renderer.h"
extern "C" {
#include "libnsbmp.h"
}

using namespace oapi;

static unsigned char *load_file(const char *path, size_t *data_size)
{
        FILE *fd;
        struct stat sb;
        unsigned char *buffer;
        size_t size;
        size_t n;

        fd = fopen(path, "rb");
        if (!fd) {
                perror(path);
                exit(EXIT_FAILURE);
        }

        if (stat(path, &sb)) {
                perror(path);
                exit(EXIT_FAILURE);
        }
        size = sb.st_size;

        buffer = (unsigned char *)malloc(size);
        if (!buffer) {
                fprintf(stderr, "Unable to allocate %lld bytes\n",
                                (long long) size);
                exit(EXIT_FAILURE);
        }

        n = fread(buffer, 1, size, fd);
        if (n != size) {
                perror(path);
                exit(EXIT_FAILURE);
        }

        fclose(fd);

        *data_size = size;
        return buffer;
}
static void *bitmap_create(int width, int height, unsigned int state)
{
        (void) state;  /* unused */
        return calloc(width * height, 4);
}


static unsigned char *bitmap_get_buffer(void *bitmap)
{
        assert(bitmap);
        return (unsigned char *)bitmap;
}


static void bitmap_destroy(void *bitmap)
{
        assert(bitmap);
        free(bitmap);
}

static void warning(const char *context, bmp_result code)
{
        fprintf(stderr, "%s failed: ", context);
        switch (code) {
                case BMP_INSUFFICIENT_MEMORY:
                        fprintf(stderr, "BMP_INSUFFICIENT_MEMORY");
                        break;
                case BMP_INSUFFICIENT_DATA:
                        fprintf(stderr, "BMP_INSUFFICIENT_DATA");
                        break;
                case BMP_DATA_ERROR:
                        fprintf(stderr, "BMP_DATA_ERROR");
                        break;
                default:
                        fprintf(stderr, "unknown code %i", code);
                        break;
        }
        fprintf(stderr, "\n");
}

static GLuint loadBMPTexture(const char *filename) {
    bmp_bitmap_callback_vt bitmap_callbacks = {
            bitmap_create,
            bitmap_destroy,
            bitmap_get_buffer,
    };
    bmp_result code;
    bmp_image bmp;
    size_t size;
    GLuint res;
    GLint alignment;

    /* create our bmp image */
    bmp_create(&bmp, &bitmap_callbacks);

    /* load file into memory */
    unsigned char *data = load_file(filename, &size);

    /* analyse the BMP */
    code = bmp_analyse(&bmp, size, data);
    if (code != BMP_OK) {
            warning("bmp_analyse", code);
            res = 0;
            goto cleanup;
    }

    /* decode the image */
    code = bmp_decode(&bmp);
    /* code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); */
    if (code != BMP_OK) {
            warning("bmp_decode", code);
            /* allow partially decoded images */
            if ((code != BMP_INSUFFICIENT_DATA) &&
                (code != BMP_DATA_ERROR)) {
                    res = 0;
                    goto cleanup;
            }
    }


      /* Generate texture */
      glGenTextures (1, &res);
      Renderer::CheckError("glGenTextures");
      glBindTexture (GL_TEXTURE_2D, res);
      Renderer::CheckError("glBindTexture");

      /* Setup some parameters for texture filters and mipmapping */
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
      Renderer::CheckError("glTexParameteri");
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      Renderer::CheckError("glTexParameteri2");
      glGetIntegerv (GL_UNPACK_ALIGNMENT, &alignment);
      Renderer::CheckError("glGetIntegerv");
      glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
      Renderer::CheckError("glPixelStorei");

      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB,
		    bmp.width, bmp.height, 0, GL_RGBA,
		    GL_UNSIGNED_BYTE, bmp.bitmap);
      Renderer::CheckError("glTexImage2D");

      glPixelStorei (GL_UNPACK_ALIGNMENT, alignment);
      Renderer::CheckError("glPixelStorei");

cleanup:
    /* clean up */
    bmp_finalise(&bmp);
    free(data);

    return res;
}

// ==============================================================
// Local prototypes

uint16_t GetNumberOfBits (uint32_t dwMask);

// ==============================================================
// ==============================================================
// Class TextureManager
// ==============================================================
// ==============================================================

TextureManager::TextureManager ()
{
	bMipmap    = true;//(g_client->GetFramework()->SupportsMipmaps() ? true:false);
//	npfp       = 0;
	firstTex   = NULL;
}

// ==============================================================

TextureManager::~TextureManager ()
{
	ClearRepository();
	//if (npfp) delete []pfp;
}

// ==============================================================
/*
bool TextureManager::ReadTexture (FILE *file, OGLTexture **ppdds, uint32_t flags)
{
    assert(false);
	if (!bMipmap) flags |= 4; // don't load mipmaps

	if (!ReadDDSSurface (file, ppdds, flags)) {
		return false;
	}

	return true;
}
*/
// =======================================================================

bool TextureManager::ReadTextureFromMemory (const uint8_t *buf, uint32_t nbuf, OGLTexture **ppdds, uint32_t flags)
{
    *ppdds = new OGLTexture;
    size_t xxx = nbuf;
    (*ppdds)->m_TexId = SOIL_direct_load_DDS_from_memory((const unsigned char *)buf, &xxx, 0, 0, 0);
    if((*ppdds)->m_TexId==0) {

        printf("%s\n",SOIL_last_result());
        abort();
        exit(-1);
    }

    (*ppdds)->m_RefCnt = 1;
    (*ppdds)->m_InCache = false;
    (*ppdds)->m_FBO = 0;
    (*ppdds)->InitSize();

    return (*ppdds)->m_TexId != 0;
}

// =======================================================================

bool TextureManager::LoadTexture (const char *fname, OGLTexture **ppdds, uint32_t flags)
{
    if(!strcasecmp(&fname[strlen(fname) - 4],".bmp")) {
        std::string bmpPath = oapiGetFilePath(fname);
        GLuint tid = loadBMPTexture(bmpPath.c_str());

        if(tid != 0) {
            *ppdds = new OGLTexture;
            (*ppdds)->m_TexId = tid;
            (*ppdds)->m_RefCnt = 1;
            (*ppdds)->m_InCache = false;
            (*ppdds)->m_FBO = 0;
            (*ppdds)->InitSize();
            return true;
        }
    }


    unsigned int SOILFlags = SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS;

    if(flags & 4) {
        SOILFlags = SOIL_FLAG_TEXTURE_REPEATS;
    }

    if(!(flags & 2)) {
        //SOILFlags |= SOIL_FLAG_DDS_LOAD_DIRECT;
        //SOIL_LOAD_RGBA SOIL_LOAD_AUTO
    }

	char cpath[256];
	*ppdds = 0;
    GLuint tid = SOIL_load_OGL_texture(fname, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags, 0);
    if(tid == 0) {
        if (g_client->TexturePath(fname, cpath)) {
            tid = SOIL_load_OGL_texture(cpath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags, 0);
        } else {
            g_client->PlanetTexturePath(fname, cpath);
            tid = SOIL_load_OGL_texture(cpath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags, 0);
        }
    }
    if(tid != 0) {
        *ppdds = new OGLTexture;
        (*ppdds)->m_TexId = tid;
        (*ppdds)->m_RefCnt = 1;
        (*ppdds)->m_InCache = false;
        (*ppdds)->m_FBO = 0;
        (*ppdds)->InitSize();
        return true;
    }
    return false;
}

// =======================================================================
bool TextureManager::LoadTexture (const char *fname, long ofs, OGLTexture **ppdds, uint32_t flags)
{
    unsigned int SOILFlags = SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS;

    if(flags & 4) {
        SOILFlags = SOIL_FLAG_TEXTURE_REPEATS;
    }

    if(!(flags & 2)) {
        //SOILFlags |= SOIL_FLAG_DDS_LOAD_DIRECT;
        //SOIL_LOAD_RGBA SOIL_LOAD_AUTO
    }

	char cpath[256];
	*ppdds = 0;
    GLuint tid = 0;
    if (g_client->TexturePath(fname, cpath)) {
        tid = SOIL_load_OGL_texture(cpath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags, ofs);
    } else {
    	g_client->PlanetTexturePath(fname, cpath);
        tid = SOIL_load_OGL_texture(cpath, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOILFlags, ofs);
    }

    if(tid != 0) {
        *ppdds = new OGLTexture;
        (*ppdds)->m_TexId = tid;
        (*ppdds)->m_RefCnt = 1;
        (*ppdds)->m_InCache = false;
        (*ppdds)->m_FBO = 0;
        (*ppdds)->InitSize();
        return true;
    }
    return false;
}

// =======================================================================

int TextureManager::LoadTextures (const char *fname, OGLTexture **ppdds, uint32_t flags, int n)
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
            SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_TEXTURE_REPEATS);
            //SOIL_FLAG_DDS_LOAD_DIRECT);

	} else if (g_client->TexturePath(fname, cpath)) {
		ftex = fopen(cpath, "rb");
		if (ftex) {
			fclose(ftex);

            ntex = SOIL_direct_load_DDS_blob(
                cpath,
                (int *)tmp,
                n,
                SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_TEXTURE_REPEATS);
                //SOIL_FLAG_DDS_LOAD_DIRECT);
		}
	} else {
//        printf("failed to open %s\n", fname);
        //exit(EXIT_FAILURE);
    }

    for(int i=0; i<ntex;i++) {
        ppdds[i] = new OGLTexture;
        ppdds[i]->m_InCache = false;
        ppdds[i]->m_RefCnt = 0;
        ppdds[i]->m_TexId = tmp[i];
        ppdds[i]->m_FBO = 0;
        ppdds[i]->InitSize();
    }

    delete []tmp;

	return ntex;
}

// =======================================================================
// Retrieve a texture. First scans the repository of loaded textures.
// If not found, loads the texture from file and adds it to the repository

bool TextureManager::GetTexture (const char *fname, OGLTexture **ppdds, uint32_t flags)
{
	TexRec *texrec = ScanRepository (fname);
	if (texrec) {
		// found in repository
		*ppdds = texrec->tex;
		return true;
	} else if (LoadTexture (fname, ppdds, flags)) {
		// loaded from file
		AddToRepository (fname, *ppdds);
		return true;
	} else {
		// not found
		return false;
	}
}

// =======================================================================
// Return a matching texture entry from the repository, if found.
// Otherwise, return NULL.

TextureManager::TexRec *TextureManager::ScanRepository (const char *fname)
{
	TexRec *texrec;
	uint32_t id = MakeTexId (fname);
	for (texrec = firstTex; texrec; texrec = texrec->next) {
		if (id == texrec->id && !strncmp (fname, texrec->fname, 64))
			return texrec;
	}
	return NULL;
}

// =======================================================================
// Add a new entry to the repository

void TextureManager::AddToRepository (const char *fname, OGLTexture *pdds)
{
	TexRec *texrec = new TexRec;
	texrec->tex = pdds;
	strncpy (texrec->fname, fname, 64);
	texrec->id = MakeTexId (fname);
	texrec->next = firstTex; // add to beginning of list
	firstTex = texrec;
}

// =======================================================================
// De-allocates the repository and release the DX7 textures

void TextureManager::ClearRepository ()
{
	while (firstTex) {
		TexRec *tmp = firstTex;
		firstTex = firstTex->next;
		if (tmp->tex) tmp->tex->Release();
		delete tmp;
	}
}

// =======================================================================

uint32_t TextureManager::MakeTexId (const char *fname)
{
	uint32_t id = 0;
	for (const char *c = fname; *c; c++)
		id += *c;
	return id;
}

// =======================================================================
// ReadDDSSurface()
// Read a compressed DDS surface from a stream
//    pddsdComp     contains the DDS surface description, and
//    pppddsCompTop contains the DDS surface
// =======================================================================
/*
 * \note The following flags are supported:
 *   - bit 0 set: force creation in system memory
 *   - bit 1 set: decompress, even if format is supported by device
 *   - bit 2 set: don't load mipmaps, even if supported by device
 *   - bit 3 set: load as global resource (can be managed by graphics client)
*/
/*
bool TextureManager::ReadDDSSurface (FILE *file,
	OGLTexture ** ppddsDXT, uint32_t flags)
{
    assert(false);
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *buffer = (uint8_t *)malloc(file_size);
	fread(buffer, file_size, 1, file);

	bool res = ReadDDSSurfaceFromMemory(buffer, file_size, ppddsDXT, flags);

	free(buffer);
	return res;
}
*/
// =======================================================================
// Read a compressed DDS surface from a memory buffer
GLuint texture_loadDDS(const uint8_t *buf, uint32_t nbuf) {
    assert(false);
  // lay out variables to be used
	const unsigned char* header;
	
	unsigned int width;
	unsigned int height;
	unsigned int mipMapCount;
	
	unsigned int blockSize;
	unsigned int format;
	
	unsigned int w;
	unsigned int h;
	
	const unsigned char* buffer = 0;
    unsigned int offset = 0;
    unsigned int size = 0;
    bool mipmaps = false;


	GLuint tid = 0;
	
	long file_size = nbuf;
	
  // allocate new unsigned char space with 4 (file code) + 124 (header size) bytes
  // read in 128 bytes from the file
	header = buf;
  
  // compare the `DDS ` signature
	if(memcmp(header, "DDS ", 4) != 0) {
        printf("texture_loadDDS: DDS header not found\n");
		goto exit;
    }
	
  // extract height, width, and amount of mipmaps - yes it is stored height then width
	height = (header[12]) | (header[13] << 8) | (header[14] << 16) | (header[15] << 24);
	width = (header[16]) | (header[17] << 8) | (header[18] << 16) | (header[19] << 24);
	mipMapCount = (header[28]) | (header[29] << 8) | (header[30] << 16) | (header[31] << 24);
    if(mipMapCount > 0)
        mipmaps = true;
	else
        mipMapCount = 1;        
    // figure out what format to use for what fourCC file type it is
    // block size is about physical chunk storage of compressed data in file (important)
	if(header[84] == 'D') {
		switch(header[87]) {
			case '1': // DXT1
				format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				blockSize = 8;
				break;
			case '3': // DXT3
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				blockSize = 16;
				break;
			case '5': // DXT5
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				blockSize = 16;
				break;
			case '0': // DX10
				// unsupported, else will error
				// as it adds sizeof(struct DDS_HEADER_DXT10) between pixels
				// so, buffer = malloc((file_size - 128) - sizeof(struct DDS_HEADER_DXT10));
			default: 
                printf("texture_loadDDS: unsupported compression\n");
                goto exit;
		}
        
        // allocate new unsigned char space with file_size - (file_code + header_size) magnitude
        // read rest of file
        buffer = buf + 128;

        // prepare new incomplete texture
        glGenTextures(1, &tid);
        Renderer::CheckError("glGenTextures");
        
        // bind the texture
        // make it complete by specifying all needed parameters and ensuring all mipmaps are filled
        glBindTexture(GL_TEXTURE_2D, tid);
        Renderer::CheckError("glBindTexture");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1); // opengl likes array length of mipmaps
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Renderer::CheckError("glTexParameteri");
        if(mipmaps)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to enable mipmaping
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        Renderer::CheckError("glTexParameteri");
        
        // prepare some variables
        w = width;
        h = height;
        
        // loop through sending block at a time with the magic formula
        // upload to opengl properly, note the offset transverses the pointer
        // assumes each mipmap is 1/2 the size of the previous mipmap
        for (unsigned int i=0; i<mipMapCount; i++) {
            if(w == 0 || h == 0) { // discard any odd mipmaps 0x1 0x2 resolutions
                mipMapCount--;
                continue;
            }
            size = ((w+3)/4) * ((h+3)/4) * blockSize;
            glCompressedTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, size, buffer + offset);
            Renderer::CheckError("glCompressedTexImage2D");
            offset += size;
            w /= 2;
            h /= 2;
        }
    } else {// BC4U/BC4S/ATI2/BC55/R8G8_B8G8/G8R8_G8B8/UYVY-packed/YUY2-packed unsupported
        // allocate new unsigned char space with file_size - (file_code + header_size) magnitude
        // read rest of file

        int dwRGBBitCount = (header[88]) | (header[89] << 8) | (header[90] << 16) | (header[91] << 24);

        buffer = buf + 128;

        // prepare new incomplete texture
        glGenTextures(1, &tid);
        Renderer::CheckError("glGenTextures");
        if(tid == 0)
            goto exit;
        
        // bind the texture
        // make it complete by specifying all needed parameters and ensuring all mipmaps are filled
        glBindTexture(GL_TEXTURE_2D, tid);
        Renderer::CheckError("glBindTexture");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1); // opengl likes array length of mipmaps
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Renderer::CheckError("glTexParameteri");
        if(mipmaps)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to enable mipmaping
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        Renderer::CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        Renderer::CheckError("glTexParameteri");
        
        // prepare some variables
        w = width;
        h = height;
        
        int pixelSize = 2;
        GLint internalformat = GL_RGBA4;
        GLenum format;
        GLenum type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if(dwRGBBitCount == 32) {//ARGB8888
            pixelSize = 4;
            internalformat = GL_RGBA8;
            type = GL_UNSIGNED_INT_8_8_8_8_REV;
        }
        // loop through sending block at a time with the magic formula
        // upload to opengl properly, note the offset transverses the pointer
        // assumes each mipmap is 1/2 the size of the previous mipmap
        for (unsigned int i=0; i<mipMapCount; i++) {
            if(w == 0 || h == 0) { // discard any odd mipmaps 0x1 0x2 resolutions
                mipMapCount--;
                continue;
            }
            glTexImage2D( GL_TEXTURE_2D, i, internalformat, w, h, 0, GL_BGRA_EXT, type, buffer + offset);
            Renderer::CheckError("glTexImage2D");
            offset += w * h * pixelSize;
            w /= 2;
            h /= 2;
        }

        

    }
    // discard any odd mipmaps, ensure a complete texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1);

    Renderer::CheckError("glTexParameteri");
    // unbind
    glBindTexture(GL_TEXTURE_2D, 0);
    Renderer::CheckError("glBindTexture");
	
  // easy macro to get out quick and uniform (minus like 15 lines of bulk)
exit:
	return tid;
}
/*
bool TextureManager::ReadDDSSurfaceFromMemory (const uint8_t *buf, uint32_t nbuf,
	OGLTexture ** ppddsDXT, uint32_t flags)
{
    assert(false);
		*ppddsDXT = new OGLTexture;
		size_t xxx = nbuf;
		unsigned int SOIL_flags = SOIL_FLAG_MIPMAPS;
        
        GLuint texid;

		if(flags & 2) texid = SOIL_load_OGL_texture_from_memory(buf, &xxx,	0, 0, 0);
        else texid = texture_loadDDS(buf, nbuf);

// flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS 
// | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT


//		GLuint texid = SOIL_load_OGL_texture_from_memory(buf, &xxx,	0, 0, SOIL_flags);
		//GLuint texid = texture_loadDDS(buf, nbuf);
        if(texid == 0) {
            assert(false);
            return false;
        }

		(*ppddsDXT)->m_TexId = texid;//SOIL_direct_load_DDS_from_memory(buffer, &xxx, 0, 0, 0);
		if((*ppddsDXT)->m_TexId==0) {
			printf("%s\n",SOIL_last_result());
			abort();
			exit(-1);
		}
		(*ppddsDXT)->m_RefCnt = 1;
		(*ppddsDXT)->m_InCache = false;
		(*ppddsDXT)->m_FBO = 0;
		(*ppddsDXT)->InitSize();
		
		return true;
	#if 0
  // lay out variables to be used
	const uint8_t *header = buf;
	const uint8_t *buffer = buf+128;

	unsigned int width;
	unsigned int height;
	unsigned int mipMapCount;
	
	unsigned int blockSize;
	unsigned int format;
	
	unsigned int w;
	unsigned int h;
	
    unsigned int offset = 0;
    unsigned int size = 0;
    bool mipmaps = false;
	bool res = false;

  // allocate new unsigned char space with 4 (file code) + 124 (header size) bytes
  // read in 128 bytes from the file
  
  // compare the `DDS ` signature
	if(memcmp(header, "DDS ", 4) != 0) {
        printf("DDS header not found\n");
		res = false;
		goto exit;
    }
	
  // extract height, width, and amount of mipmaps - yes it is stored height then width
	height = (header[12]) | (header[13] << 8) | (header[14] << 16) | (header[15] << 24);
	width = (header[16]) | (header[17] << 8) | (header[18] << 16) | (header[19] << 24);
	mipMapCount = (header[28]) | (header[29] << 8) | (header[30] << 16) | (header[31] << 24);
    if(mipMapCount > 0)
        mipmaps = true;
	else
        mipMapCount = 1;        
    // figure out what format to use for what fourCC file type it is
    // block size is about physical chunk storage of compressed data in file (important)
	if(header[84] == 'D') {
		switch(header[87]) {
			case '1': // DXT1
				format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				blockSize = 8;
				break;
			case '3': // DXT3
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				blockSize = 16;
				break;
			case '5': // DXT5
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				blockSize = 16;
				break;
			case '0': // DX10
				// unsupported, else will error
				// as it adds sizeof(struct DDS_HEADER_DXT10) between pixels
				// so, buffer = malloc((file_size - 128) - sizeof(struct DDS_HEADER_DXT10));
			default: 
                printf("unsupported DDS compression\n");
				res = false;
                goto exit;
		}
        
		*ppddsDXT = new OGLTexture;
		(*ppddsDXT)->m_RefCnt = 1;
		(*ppddsDXT)->m_InCache = false;
		(*ppddsDXT)->m_Width = width;
		(*ppddsDXT)->m_Height = height;
		(*ppddsDXT)->m_FBO = 0;

        // prepare new incomplete texture
        glGenTextures(1, &(*ppddsDXT)->m_TexId);
        CheckError("glGenTextures");
        
        // bind the texture
        // make it complete by specifying all needed parameters and ensuring all mipmaps are filled
        glBindTexture(GL_TEXTURE_2D, (*ppddsDXT)->m_TexId);
        CheckError("glBindTexture");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1); // opengl likes array length of mipmaps
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckError("glTexParameteri");
        if(mipmaps)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to enable mipmaping
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        CheckError("glTexParameteri");
        
        // prepare some variables
        w = width;
        h = height;
        
        // loop through sending block at a time with the magic formula
        // upload to opengl properly, note the offset transverses the pointer
        // assumes each mipmap is 1/2 the size of the previous mipmap
        for (unsigned int i=0; i<mipMapCount; i++) {
            if(w == 0 || h == 0) { // discard any odd mipmaps 0x1 0x2 resolutions
                mipMapCount--;
            continue;
            }
            size = ((w+3)/4) * ((h+3)/4) * blockSize;
            glCompressedTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, size, buffer + offset);
            CheckError("glCompressedTexImage2D");
            offset += size;
            w /= 2;
            h /= 2;
        }
		res = true;
    } else {// BC4U/BC4S/ATI2/BC55/R8G8_B8G8/G8R8_G8B8/UYVY-packed/YUY2-packed unsupported
        // allocate new unsigned char space with file_size - (file_code + header_size) magnitude
        // read rest of file

        int dwRGBBitCount = (header[88]) | (header[89] << 8) | (header[90] << 16) | (header[91] << 24);

		*ppddsDXT = new OGLTexture;
		(*ppddsDXT)->m_RefCnt = 1;
		(*ppddsDXT)->m_InCache = false;
		(*ppddsDXT)->m_Width = width;
		(*ppddsDXT)->m_Height = height;
		(*ppddsDXT)->m_FBO = 0;

        // prepare new incomplete texture
        glGenTextures(1, &(*ppddsDXT)->m_TexId);
        CheckError("glGenTextures");
        if((*ppddsDXT)->m_TexId == 0)
            goto exit;
        
        // bind the texture
        // make it complete by specifying all needed parameters and ensuring all mipmaps are filled
        glBindTexture(GL_TEXTURE_2D, (*ppddsDXT)->m_TexId);
        CheckError("glBindTexture");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1); // opengl likes array length of mipmaps
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckError("glTexParameteri");
        if(mipmaps)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to enable mipmaping
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        CheckError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        CheckError("glTexParameteri");
        
        // prepare some variables
        w = width;
        h = height;
        
        int pixelSize = 2;
        GLint internalformat = GL_RGBA4;
        GLenum format;
        GLenum type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if(dwRGBBitCount == 32) {//ARGB8888
            pixelSize = 4;
            internalformat = GL_RGBA8;
            type = GL_UNSIGNED_INT_8_8_8_8_REV;
        }
        // loop through sending block at a time with the magic formula
        // upload to opengl properly, note the offset transverses the pointer
        // assumes each mipmap is 1/2 the size of the previous mipmap
        for (unsigned int i=0; i<mipMapCount; i++) {
            if(w == 0 || h == 0) { // discard any odd mipmaps 0x1 0x2 resolutions
                mipMapCount--;
            continue;
            }
            glTexImage2D( GL_TEXTURE_2D, 0, internalformat, w, h, 0, GL_BGRA_EXT, type, buffer + offset);
            CheckError("glTexImage2D");
            offset += w * h * pixelSize;
            w /= 2;
            h /= 2;
        }

		res = true;
    }
    // discard any odd mipmaps, ensure a complete texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1);

    CheckError("glTexParameteri");
    // unbind
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckError("glBindTexture");
	
  // easy macro to get out quick and uniform (minus like 15 lines of bulk)
exit:
	return res;
	#endif
}
*/
OGLTexture *TextureManager::CreateTextureFromTexId(GLuint texId, int width, int height) {
    OGLTexture *tex = new OGLTexture;
    tex->m_RefCnt = 1;
    tex->m_InCache = true;
    tex->m_Width = width;
    tex->m_Height = height;
    tex->m_TexId = texId;
	tex->m_FBO = 0;
    return tex;
}

// from http://www.opengl-tutorial.org/fr/intermediate-tutorials/tutorial-14-render-to-texture/
OGLTexture *TextureManager::GetTextureForRendering(int w, int h, int attrib)
{
    OGLTexture *tex = new OGLTexture;
    tex->m_RefCnt = 1;
    tex->m_InCache = true;
    tex->m_Width = w;
    tex->m_Height = h;
    GLint oldFB;

    GLenum format = GL_RGBA;
    if(attrib & OAPISURFACE_NOALPHA)
        format = GL_RGB;

	Renderer::CheckError("GetTextureForRendering");

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);

    glGenFramebuffers(1, &tex->m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tex->m_FBO);

    glGenTextures(1, &tex->m_TexId);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, tex->m_TexId);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0,format, w, h, 0,format, GL_UNSIGNED_BYTE, 0);

    // Poor filtering. Needed !
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glGenerateMipmap(GL_TEXTURE_2D);

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
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
	Renderer::CheckError("!GetTextureForRendering");
    return tex;
}

// =======================================================================
// BltToUncompressedSurface()
// Creates an uncompressed surface and blits the compressed surface to 
// it using the specified pixel format.
// =======================================================================
/*
bool TextureManager::BltToUncompressedSurface (DDSURFACEDESC2 ddsd, 
    DDPIXELFORMAT ddpf, LPDIRECTDRAWSURFACE7 pddsDXT, 
    LPDIRECTDRAWSURFACE7* ppddsNewSurface)
{
	HRESULT hr;
    LPDIRECTDRAWSURFACE7 pddsDXTAttached;
    LPDIRECTDRAWSURFACE7 pddsNew;
    LPDIRECTDRAWSURFACE7 pddsNewAttached;

    // Set surface caps for the new surface
    ddsd.ddpfPixelFormat = ddpf;

    // Create an un-compressed surface based on the enumerated texture formats
    if (FAILED (hr = pDD->CreateSurface (&ddsd, &pddsNew, NULL))) {
		//LOGOUT_DDERR(hr);
        return hr;
	}
    *ppddsNewSurface = pddsNew;

    // Copy compress image to un-compressed surface, including mips (if any)
    while (TRUE) {
        if (FAILED (hr = pddsNew->Blt (NULL, pddsDXT, NULL, DDBLT_WAIT, NULL))) {
			//LOGOUT_DDERR(hr);
            return hr;
		}

        // Get next surface in DXT's mipmap chain
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
		ddsd.ddsCaps.dwCaps2 = 0;
		ddsd.ddsCaps.dwCaps3 = 0;
		ddsd.ddsCaps.dwCaps4 = 0;

        if (FAILED (pddsDXT->GetAttachedSurface (&ddsd.ddsCaps, &pddsDXTAttached)))
            return S_OK; // Failure here means were done with the mipmap chain
        pddsDXT = pddsDXTAttached;  

        // Get next surface in the new surface's mipmap chain
        if (FAILED (pddsNew->GetAttachedSurface (&ddsd.ddsCaps, &pddsNewAttached)))
            return E_FAIL;
        pddsNew = pddsNewAttached;
    }
}
*/
// =======================================================================
// =======================================================================
// Non-member utility functions
// =======================================================================
// =======================================================================

// =======================================================================
// GetNumberOfBits()
// xReturns the number of bits set in a DWORD mask
// =======================================================================

uint16_t GetNumberOfBits (uint32_t dwMask)
{
    uint16_t wBits = 0;
    while (dwMask) {
        dwMask = dwMask & (dwMask - 1);
        wBits++;
    }
    return wBits;
}

OGLTexture::OGLTexture() {
    m_colorkey = SURF_NO_CK;
}

bool OGLTexture::Release()
{
    if(m_InCache) {
//        printf("Releasing texture in cache\n");
        return false;
        exit(-1);
    }
    m_RefCnt--;
    if(!m_RefCnt) {
        if(m_TexId) {
            glDeleteTextures(1, &m_TexId);
			Renderer::CheckError("glDeleteTextures");
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

void OGLTexture::InitSize()
{
	Renderer::CheckError("InitSize");
    glBindTexture(GL_TEXTURE_2D, m_TexId);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_Width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_Height);
    glBindTexture(GL_TEXTURE_2D, 0);
	Renderer::CheckError("!InitSize");
}
