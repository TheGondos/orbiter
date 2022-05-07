#pragma once
#include "GraphicsAPI.h"
#include <unordered_map>
#include <memory>
//#include "glad.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include <glm/glm.hpp>

class OGLCamera;
class OGLTexture;

struct MeshGroup {
    std::unique_ptr<VertexBuffer> VBO;
    std::unique_ptr<IndexBuffer> IBO;
    std::unique_ptr<VertexArray> VBA;
    uint32_t mTexIdx;
    uint32_t mMatIdx;
    uint32_t flags;
    uint32_t UsrFlag;
    uint32_t nVtx;
    int16_t m_zBias;
};


struct OGLMaterial {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 emissive;
    float specular_power;
}; 


class OGLMesh {
public:
	OGLMesh (MESHHANDLE hMesh);
    OGLMesh (MESHGROUPEX *, OGLMaterial *, OGLTexture *);
	~OGLMesh ();

    std::vector<MeshGroup> mMeshGroups;
    std::vector<OGLTexture *> mTextures;
    std::vector<OGLMaterial> mMaterials;

    void Render(OGLCamera *c, glm::fmat4 &model);
	int EditGroup (int grp, GROUPEDITSPEC *ges);
    bool SetTexture (int texidx, SURFHANDLE tex);
    int SetMaterial (int matidx, const MATERIAL *mat);
    //bool GetMaterial (int matidx, MATERIAL *mat);
    int GetGroup (int grp, GROUPREQUESTSPEC *grs);
    MeshGroup *GetGroup (int idx) { return &mMeshGroups[idx]; }
	void TransformGroup (int n, const glm::fmat4 *m);
    void RenderGroup(OGLCamera *c, glm::fmat4 &model, MeshGroup *, OGLTexture *tex);

    static std::unique_ptr<Shader> mShader;
    void EnableMatAlpha(bool en) {m_enMatAlpha = en;}
    bool m_enMatAlpha;
};

class OGLMeshManager {
public:
	OGLMeshManager ();
	~OGLMeshManager();
	void Flush();
	void StoreMesh (MESHHANDLE hMesh);
	OGLMesh *GetMesh (MESHHANDLE hMesh);
private:
    std::unordered_map<MESHHANDLE, std::unique_ptr<OGLMesh>> mMeshMap;
};
