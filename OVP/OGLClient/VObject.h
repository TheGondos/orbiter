#pragma once
#include "OrbiterAPI.h"
#include "GraphicsAPI.h"
#include <glm/glm.hpp>

// ==============================================================
// class VObject (interface)
// ==============================================================
/**
 * \brief Visual object base class.
 *
 * A vObject is a render object representing a 'logical'
 * Orbiter object (identified by its OBJHANDLE) in a scene.
 */
class OGLCamera;
class VObject {
public:
    VObject (OBJHANDLE handle);

	/**
	 * \brief Destroys the visual object
	 */
	virtual ~VObject () {}

    OBJHANDLE GetObject () const { return mHandle; }
    virtual void clbkEvent (visevent msg, visevent_data content) {};
    virtual bool Render (OGLCamera *) { return false; }
    virtual void CheckResolution() {}
    virtual bool Update ();

    static VObject *Create (OBJHANDLE handle);

    virtual MESHHANDLE GetMesh(unsigned int idx) { return nullptr; }
    
    const VECTOR3 &PosFromCamera() const { return cpos; }
	inline double CamDist() const { return cdist; }

    double mSize;
    MATRIX4 dmWorld;
protected:
    glm::fmat4 mModel;
    OBJHANDLE mHandle;
    double cdist;
    bool mVisible;
    VECTOR3 cpos;      // camera-relative object position
};
