#pragma once
#include "VObject.h"

class RingManager;
class SurfaceManager;
template<class T> class TileManager2;
class HazeManager;
class CloudManager;
class SurfTile;
class CloudTile;
class VBase;
class VPlanet: public VObject {
public:
	VPlanet (OBJHANDLE handle);
	~VPlanet ();

    bool Render (OGLCamera *) override;
    void RenderSphere ();
    void CheckResolution() override;
    bool Update() override;
    SurfaceManager *surfmgr;
	TileManager2<SurfTile> *surfmgr2;   // planet surface tile manager (v2)
	TileManager2<CloudTile> *cloudmgr2; // planet cloud layer tile manager (v2)
    RingManager *ringmgr;
    HazeManager *hazemgr;
	VBase **vbase;            // list of base visuals
	uint32_t nbase;
	int max_patchres;         // max surface LOD level
	int patchres;             // surface LOD level
	float render_rad;         // distance to be rendered past planet centre
	float dist_scale;         // planet rescaling factor
	double maxdist, max_centre_dist;
	FogParam fog;             // distance fog render parameters

	struct RenderPrm { // misc. parameters for rendering the planet
		// persistent options
		bool bAtm;              // planet has atmosphere
		bool bCloud;            // planet has a cloud layer
		bool bCloudShadow;      // planet renders cloud shadows on surface
		bool bCloudBrighten;    // oversaturate cloud brightness?
		bool bFogEnabled;	    // does this planet support fog rendering?
		double atm_href;	    // reference altitude for atmospheric effects [m]
		double atm_amb0;        // scale parameter for ambient level modification
		uint32_t amb0col;          // baseline ambient colour [rgba]
		double cloudalt;        // altitude of cloud layer, if present [m]
		double shadowalpha;     // opacity of shadows (0-1)
		double horizon_excess;  // extend horizon visibility radius
		double tilebb_excess;   // extend tile visibility bounding box
		double horizon_minrad;  // scale factor for lower edge of rendered horizon (account for negative elevations)

		// frame-by-frame options
		bool bAddBkg;		    // render additive to sky background (i.e. planet seen through atm.layer of another planet)
		int cloudvis;           // cloud visibility: bit0=from below, bit1=from above
		double cloudrot;	    // cloud layer rotation state
		bool bFog;			    // render distance fog in this frame?
		bool bTint;			    // render atmospheric tint?
		bool bCloudFlatShadows; // render cloud shadows onto a sphere?
		VECTOR3 rgbTint;        // tint colour
		glm::vec4 mFogColor;
		float mFogDensity;
	} prm;

	struct CloudData {        // cloud render parameters (for legacy interface)
		CloudManager *cloudmgr; // cloud tile manager
		double viewap;          // visible radius
		glm::mat4 mWorldC;      // cloud world matrix
		glm::mat4 mWorldC0;     // cloud shadow world matrix
		bool cloudshadow;       // render cloud shadows on the surface
		float shadowalpha;      // alpha value for cloud shadows
		double microalt0, microalt1; // altitude limits for micro-textures
	} *clouddata;

    void RenderCloudLayer (int cullmode, const RenderPrm &prm);
	void RenderCloudShadows (OGLCamera *c, const RenderPrm &prm);
	void RenderBaseSurfaces (OGLCamera *c);
	void RenderBaseStructures (OGLCamera *c);
	bool ModLighting (uint32_t &ambient);
};
