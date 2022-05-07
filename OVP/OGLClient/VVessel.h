#pragma once
#include "VObject.h"
#include "VesselAPI.h"
#include <unordered_map>
#include "MeshManager.h"
#include <glm/glm.hpp>
#include <memory>
#include "Texture.h"

struct TVERTEX {
	float x,y,z;
	float u,v;
};

class VVessel: public VObject {
public:
	VVessel (OBJHANDLE handle);

	~VVessel ();

	void clbkEvent (visevent msg, visevent_data content) override;

	bool Update () override;
    void InsertMesh (unsigned int idx);
    void DelMesh (unsigned int idx);
    MESHHANDLE GetMesh (unsigned int idx) override;

	bool Render (OGLCamera *, bool internalpass);
    bool Render (OGLCamera *c) override { return Render(c, false); }
	bool RenderExhaust (OGLCamera *c);
	void SetExhaustVertices (const VECTOR3 &edir, const VECTOR3 &cdir, const VECTOR3 &ref,
	double lscale, double wscale, TVERTEX *ev);

	/**
	 * \brief Update animations of the visual
	 *
	 * Synchronises the visual animation states with the logical
	 * animation states of the vessel object.
	 * \param mshidx mesh index
	 * \note If mshidx == (unsigned int)-1 (default), all meshes are updated.
	 */
	void UpdateAnimations (unsigned int mshidx = (unsigned int)-1);
	void InitAnimations();
	void ClearAnimations();
	void Animate (unsigned int an, double state, unsigned int mshidx);
	void AnimateComponent (ANIMATIONCOMP *comp, const glm::mat4 &T);
	ANIMATION *anim;      // list of animations (defined in the vessel object)
	double *animstate;    // list of visual animation states
	size_t nanim;           // number of animations
	static void GlobalInit();

private:
    void LoadMeshes();
	struct MESHREC {
		OGLMesh *mesh;
		std::unique_ptr<glm::fmat4> trans;
		uint16_t vismode;
	};

    std::unordered_map<uint32_t, MESHREC> mMeshes;

    VESSEL *mVessel;
	static OGLTexture *defexhausttex;
};
