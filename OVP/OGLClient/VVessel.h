// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//   ORBITER VISUALISATION PROJECT (OVP)
//   D3D7 Client module
// ==============================================================

// ==============================================================
// vVessel.h
// Vessel visualisation
// ==============================================================

#ifndef __vVessel_H
#define __vVessel_H

#include "VObject.h"
#include "OGLMesh.h"

// ==============================================================
// class vVessel (interface)
// ==============================================================
/**
 * \brief Visual representation of a vessel object.
 *
 * The vVessel instance is not persistent: It is created when the
 * object moves into visual range of the camera, and is destroyed
 * when it moves out of visual range.
 *
 * The vVessel contains a set of meshes and animations. At each
 * time step, it synchronises the visual with the logical animation
 * state, and renders the resulting meshes.
 */

struct VERTEX_XYZ_TEX {
	float x,y,z;
	float u,v;
};

class vVessel: public vObject {
public:
	/**
	 * \brief Creates a new vessel visual for a scene
	 * \param _hObj vessel object handle
	 * \param scene scene to which the visual is added
	 */
	vVessel (OBJHANDLE _hObj, const Scene *scene);

	~vVessel ();

	static void GlobalInit ();
	static void GlobalExit ();

	void clbkEvent (visevent msg, visevent_data content);

	virtual MESHHANDLE GetMesh (unsigned int idx);

	/**
	 * \brief Per-frame object parameter updates
	 * \return \e true if update was performed, \e false if skipped.
	 * \action
	 *   - Calls vObject::Update
	 *   - Calls \ref UpdateAnimations
	 */
	bool Update ();

	/**
	 * \brief Object render call
	 * \param dev render device
	 * \return \e true if render operation was performed (object active),
	 *   \e false if skipped (object inactive)
	 * \action Calls Render(dev,false), i.e. performs the external render pass.
	 * \sa Render(LPDIRECT3DDEVICE7,bool)
	 */
	bool Render () override;

	/**
	 * \brief Object render call
	 * \param dev render device
	 * \param internalpass flag for internal render pass
	 * \note This method renders either the external vessel meshes
	 *   (internalpass=false) or internal meshes (internalpass=true), e.g.
	 *   the virtual cockpit.
	 * \note The internal pass is only performed on the focus object, and only
	 *   in cockpit camera mode.
	 * \sa Render(LPDIRECT3DDEVICE7)
	 */
	bool Render (bool internalpass);

	bool RenderExhaust ();

	/**
	 * \brief Render the vessel's active light beacons
	 * \param dev render device
	 */
	void RenderBeacons ();

	void RenderGroundShadow (OBJHANDLE hPlanet, float depth);

protected:
	void LoadMeshes();
	void InsertMesh (unsigned int idx);
	void ClearMeshes();
	void DelMesh (unsigned int idx);
	void InitAnimations();
	void ClearAnimations();

	void SetExhaustVertices (const VECTOR3 &edir, const VECTOR3 &cdir, const VECTOR3 &ref,
		double lscale, double wscale, VERTEX_XYZ_TEX *ev);

	/**
	 * \brief Update animations of the visual
	 *
	 * Synchronises the visual animation states with the logical
	 * animation states of the vessel object.
	 * \param mshidx mesh index
	 * \note If mshidx == (UINT)-1 (default), all meshes are updated.
	 */
	void UpdateAnimations (unsigned int mshidx = (unsigned int)-1);

	/**
	 * \brief Modify local lighting due to planet shadow or
	 *   atmospheric dispersion.
	 * \param light pointer to D3DLIGHT7 structure receiving modified parameters
	 * \return \e true if lighting modifications should be applied, \e false
	 *   if global lighting conditions apply.
	 */
	bool ModLighting (OGLLight *light);

	void Animate (unsigned int an, double state, unsigned int mshidx);
	void AnimateComponent (ANIMATIONCOMP *comp, const glm::mat4 &T);

private:
	VESSEL *vessel;       // access instance for the vessel
	struct MESHREC {
		OGLMesh *mesh;   // DX7 mesh representation
		glm::fmat4 *trans; // mesh transformation matrix (rel. to vessel frame)
		uint16_t vismode;
	} *meshlist;          // list of associated meshes
	UINT nmesh;           // number of meshes

	ANIMATION *anim;      // list of animations (defined in the vessel object)
	double *animstate;    // list of visual animation states
	unsigned int nanim;           // number of animations
//	static LPDIRECTDRAWSURFACE7 mfdsurf; // texture for rendering MFDs and HUDs
	static OGLTexture *defexhausttex; // default exhaust texture
	double tCheckLight;    // time for next lighting check
	bool bLocalLight;      // modified local lighting parameters?
	OGLLight localLight;  // current local lighting parameters

	static inline Shader *exhaustShader;
	static inline Shader *meshUnlitShader;
	static inline Shader *shadowShader;
};

#endif // !__vVessel_H