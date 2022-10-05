#ifndef __OGLCLIENT_H
#define __OGLCLIENT_H

#include "GraphicsAPI.h"
#include <stdio.h>
#include "MeshManager.h"
#include "Texture.h"
#include "Shader.h"
//#include "OGLMesh.h"
#include <memory>

struct OGLMaterial {
    glm::vec4 diffuse;
    glm::vec4 ambient;
    glm::vec4 specular;
    glm::vec4 emissive;
    float specular_power;
}; 

using namespace oapi;

class Scene;
class OGLMeshManager;
class OAPIFUNC OGLClient: public oapi::GraphicsClient {
	friend class ::Orbiter; ///< Orbiter private class
	friend class ::Scene;

	std::unique_ptr<Scene> mScene;
	std::unique_ptr<OGLMeshManager> mMeshManager;
	std::unique_ptr<TextureManager> mTextureManager;
	std::unique_ptr<Shader> mBlitShader;
public:
	struct RenderContext {
		bool normalizeNormals;
		uint32_t ambient;
		bool bFog;
		VECTOR3	fogColor;
		float fogDensity;

		VECTOR3 backgroungColor;
		OGLMaterial material;
	} mRenderContext;

	OGLMeshManager *GetMeshManager() { return mMeshManager.get(); }
	TextureManager *GetTexMgr() { return mTextureManager.get(); }
	Scene *GetScene() { return mScene.get(); }

	void clbkSetViewportSize (int width, int height) override;

	/**
	 * \brief Create a graphics object.
	 *
	 * The graphics object is typically created during module initialisation
	 * (see \ref InitModule). Once the client is created, it must be registered
	 * with the Orbiter core via the oapiRegisterGraphicsClient function.
	 * \param hInstance module instance handle (as passed to InitModule)
	 */
	OGLClient (MODULEHANDLE hInstance);

	/**
	 * \brief Destroy the graphics object.
	 *
	 * Usually, the graphics object is destroyed when the module is unloaded
	 * (see opcDLLExit), after is has been detached from the Orbiter core
	 * via a call to oapiUnregisterGraphicsClient.
	 */
	virtual ~OGLClient ();

	/**
	 * \brief Perform any one-time setup tasks.
	 *
	 * This includes enumerating drivers, graphics modes, etc.
	 * Derived classes should also call the base class method to allow
	 * default setup.
	 * \default Initialises the VideoData structure from the Orbiter.cfg
	 *   file
	 * \par Calling sequence:
	 *   Called during processing of oapiRegisterGraphicsClient, after the
	 *   Launchpad Video tab has been inserted (if clbkUseLaunchpadVideoTab
	 *   returns true).
	 */
	virtual bool clbkInitialise ();

	/**
	 * \brief Request for video configuration data
	 *
	 * Called by Orbiter before the render window is opened or configuration
	 * parameters are written to file. Applications should here either update
	 * the provided VIDEODATA structure from any user selections made in the
	 * Launchpad Video tab and leave it to Orbiter to write these parameters
	 * to Orbiter.cfg, or write the current video settings to their own
	 * configuration file.
	 * \default None.
	 */
	virtual void clbkRefreshVideoData () {}

	/**
	 * \brief Texture request
	 *
	 * Load a texture from a file into a device-specific texture object, and
	 * return a generic SURFHANDLE for it. Derived classes should overload this
	 * method to add texture support.
	 * Usually, the client should read Orbiter's default texture files (in
	 * DXT? format). However, the client also has the option to load its own
	 * texture files stored in a different format, and pass them back via the
	 * SUFHANDLE interface.
	 * \param fname texture file name with path relative to orbiter
	 *   texture folders; can be used as input for OpenTextureFile.
	 * \param flags request for texture properties
	 * \return Texture handle, cast into generic SURFHANDLE, or NULL if texture
	 *   could not be loaded.
	 * \default Return NULL.
	 * \note If the client loads its own of texture files, they can either be
	 *   installed in the default locations, replacing Orbiter's set of
	 *   textures, or stored alongside the original textures, using different
	 *   names or directory locations. In the latter case, the fname parameter
	 *   passed to clbkLoadTexture must be adapted accordingly (for example,
	 *   by replacing the dds extension with jpg, or by adding an 'OGL/'
	 *   prefix to the path name, etc). Not overwriting the original texture
	 *   set has the advantage that other graphics clients relying on the
	 *   original textures can still be used.
	 * \note The following flags are supported:
	 *   - bit 0 set: force creation in system memory
	 *   - bit 1 set: decompress, even if format is supported by device
	 *   - bit 2 set: don't load mipmaps, even if supported by device
	 *   - bit 3 set: load as global resource (can be managed by graphics client)
	 * \note If bit 3 of flags is set, orbiter will not try to modify or release
	 *   the texture. The client should manage the texture (i.e. keep it in a
	 *   repository and release it at destruction). Any further call of
	 *   clbkLoadTexture should first scan the repository. If the texture is
	 *   already present, the function should just return a pointer to it.
	 */
	virtual SURFHANDLE clbkLoadTexture (const char *fname, int flags = 0);

	/**
	 * \brief Load a surface from file into a surface object, and return a SURFHANDLE for it.
	 * \param fname texture file name with path relative to orbiter texture folders
	 * \param attrib \ref surfacecaps (see notes)
	 * \return A SURFHANDLE for the loaded surface, for example a pointer to the surface object.
	 * \note If the request refers to a static surface that has already be loaded, or if the
	 *   client buffers the unmodified surfaces after loading, it can simply return a handle to
	 *   the existing surface object, instead of reading it again from file.
	 * \note The attrib bitflag can contain one of the following main attributes:
	 *  - OAPISURFACE_RO: Load the surface to be readable by the GPU pipeline
	 *  - OAPISURFACE_RW: Load the surface to be readable and writable by the GPU pipeline
	 *  - OAPISURFACE_GDI: Load the surface to be readable and writable by the CPU, and can be blitted into an uncompressed RO or RW surface without alpha channel
	 *  - OAPISURFACE_STATIC: Load the surface to be readable by the GPU pipeline
     *  In addition, the flag can contain any of the following auxiliary attributes:
	 *  - OAPISURFACE_MIPMAPS: Load the mipmaps for the surface from file, or create them if necessary
	 *  - OAPISURFACE_NOMIPMAPS: Don't load mipmaps, even if they are available in the file
	 *  - OAPISURFACE_NOALPHA: Load the surface without an alpha channel
	 *  - OAPISURFACE_UNCOMPRESS: Uncompress the surface on loading.
	 * \sa oapiCreateSurface(int,int,int)
	 */
	virtual SURFHANDLE clbkLoadSurface (const char *fname, int attrib);

	/**
	 * \brief Save the contents of a surface to a formatted image file or to the clipboard
	 * \param surf surface handle (0 for primary render surface)
	 * \param fname image file path relative to orbiter root directory (excluding file extension), or NULL to save to clipboard
	 * \param fmt output file format
	 * \param quality quality request if the format supports it (0-1)
	 * \return Should return true on success
	 * \default Nothing, returns false
	 * \note Implementations can make use of the \ref WriteImageDataToFile method to write to
	 *   a file in the desired format once a pointer to the image data in 24-bit uncompressed
	 *   format has been obtained.
	 */
	virtual bool clbkSaveSurfaceToImage (SURFHANDLE surf, const char *fname,
		ImageFileFormat fmt, float quality=0.7f);

	/**
	 * \brief Texture release request
	 *
	 * Called by Orbiter when a previously loaded texture can be released
	 * from memory. The client can use the appropriate device-specific method
	 * to release the texture.
	 * \param hTex texture handle
	 * \default None.
	 */
	virtual void clbkReleaseTexture (SURFHANDLE hTex);

	/**
	 * \brief Replace a texture in a device-specific mesh.
	 * \param hMesh device mesh handle
	 * \param texidx texture index (>= 0)
	 * \param tex texture handle
	 * \return Should return \e true if operation successful, \e false otherwise.
	 * \default None, returns \e false.
	 */
	virtual bool clbkSetMeshTexture (DEVMESHHANDLE hMesh, int texidx, SURFHANDLE tex);

	/**
	 * \brief Replace properties of an existing mesh material.
	 * \param hMesh device mesh handle
	 * \param matidx material index (>= 0)
	 * \param mat pointer to material structure
	 * \return Overloaded functions should return an integer error flag, with
	 *   the following codes: 0="success", 3="invalid mesh handle", 4="material index out of range"
	 * \default None, returns 2 ("client does not support operation").
	 */
	virtual int clbkSetMeshMaterial (DEVMESHHANDLE hMesh, int matidx, const MATERIAL *mat);

	/**
	 * \brief Retrieve the properties of one of the mesh materials.
	 * \param hMesh device mesh handle
	 * \param matidx material index (>= 0)
	 * \param mat [out] pointer to MATERIAL structure to be filled by the method.
	 * \return true if successful, false on error (index out of range)
	 * \default None, returns 2 ("client does not support operation").
	 */
	virtual int clbkMeshMaterial (DEVMESHHANDLE hMesh, int matidx, MATERIAL *mat);

	/**
     * \brief Set custom properties for a device-specific mesh.
	 * \param hMesh device mesh handle
	 * \param property property tag
	 * \param value new mesh property value
	 * \return The method should return \e true if the property tag was recognised
	 *   and the request could be executed, \e false otherwise.
	 * \note Currently only a single mesh property request type will be sent, but this may be
	 *  extended in future versions:
	 * - \c MESHPROPERTY_MODULATEMATALPHA \n \n
	 * if value==0 (default) disable material alpha information in textured mesh groups (only use texture alpha channel).\n
	 * if value<>0 modulate (mix) material alpha values with texture alpha maps.
	 * \default None, returns \e false.
	 */
	virtual bool clbkSetMeshProperty (DEVMESHHANDLE hMesh, int property, int value);

	/**
	 * \brief Message callback for a visual object.
	 * \param hObj handle of the object that created the message
	 * \param vis client-supplied identifier for the visual
	 * \param msg event identifier
	 * \param context message context
	 * \return Function should return 1 if it processes the message, 0 otherwise.
	 * \default None, returns 0.
	 * \note Messages are generated by Orbiter for objects that have been
	 *   registered with \ref RegisterVisObject by the client, until they are
	 *   un-registered with \ref UnregisterVisObject.
	 * \note Currently only vessel objects create visual messages.
	 * \note For currently supported event types, see \ref visevent.
	 * \note The \e vis pointer passed to this function is the same as that provided
	 *   by RegisterVisObject. It can be used by the client to identify the visual
	 *   object for which the message was created.
	 * \sa RegisterVisObject, UnregisterVisObject, visevent
	 */
	virtual int clbkVisEvent (OBJHANDLE hObj, VISHANDLE vis, visevent msg, visevent_data context) override;

	/**
	 * \brief Return a mesh handle for a visual, defined by its index
	 * \param vis visual identifier
	 * \param idx mesh index (>= 0)
	 * \return Mesh handle (client-specific)
	 * \note Derived clients should return a handle that identifies a
	 *   mesh for the visual (in client-specific format).
	 * \note Orbiter calls this method in response to a \ref VESSEL::GetMesh
	 *   call by an vessel module.
	 */
	virtual MESHHANDLE clbkGetMesh (VISHANDLE vis, unsigned int idx);

	/**
	 * \brief Mesh group data retrieval interface for device-specific meshes.
	 * \param hMesh device mesh handle
	 * \param grpidx mesh group index (>= 0)
	 * \param grs data buffers and buffer size information. See \ref oapiGetMeshGroup
	 *    for details.
	 * \return Should return 0 on success, or error flags > 0.
	 * \default None, returns -2.
	 */
	virtual int clbkGetMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPREQUESTSPEC *grs);

	/**
	 * \brief Mesh group editing interface for device-specific meshes.
	 * \param hMesh device mesh handle
	 * \param grpidx mesh group index (>= 0)
	 * \param ges mesh group modification specs
	 * \return Should return 0 on success, or error flags > 0.
	 * \default None, returns -2.
	 * \note Clients should implement this method to allow the modification
	 *   of individual groups in a device-specific mesh. Modifications may
	 *   include vertex values, index lists, texture and material indices,
	 *   and user flags.
	 */
	virtual int clbkEditMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPEDITSPEC *ges);
	//@}

	// ==================================================================
	/// \name Dialog interface
	//@{
	/**
	 * \brief Popup window open notification.
	 * \note This method is called just before a popup window (e.g. dialog
	 *   box) is opened. It allows the client to prepare for subsequent
	 *   rendering of the window, if necessary.
	 */
	virtual void clbkPreOpenPopup ();
	//@}

	/**
	 * \brief React to vessel creation
	 * \param hVessel object handle of new vessel
	 * \note Calls Scene::NewVessel() to check for visual
	 */
	void clbkNewVessel (OBJHANDLE hVessel) override;

	/**
	 * \brief React to vessel destruction
	 * \param hVessel object handle of vessel to be destroyed
	 * \note Calls Scene::DeleteVessel() to remove the visual
	 */
	void clbkDeleteVessel (OBJHANDLE hVessel) override;

	// ==================================================================
	/// \name Particle stream methods
	// @{

	/**
	 * \brief Create a generic particle stream.
	 * \param pss particle stream parameters
	 * \return Pointer to new particle stream.
	 * \default None, returns NULL. Derived classes should overload this method
	 *   to return a ParticleStream-derived class instance in order to support
	 *   particle streams.
	 * \sa ParticleStream
	 */
	virtual ParticleStream *clbkCreateParticleStream (PARTICLESTREAMSPEC *pss);

	/**
	 * \brief Create a particle stream associated with a vessel.
	 *
	 * Typically used for exhaust and plasma effects, but can also be used
	 * for other types of particles.
	 * \param pss particle stream parameters
	 * \param hVessel vessel handle
	 * \param lvl pointer to exhaust level control variable
	 * \param ref pointer to stream source position (vessel frame) [<b>m</b>]
	 * \param dir pointer to stream direction (vessel frame)
	 * \return Pointer to new particle stream
	 * \default None, returns NULL. Derived classes should overload this method
	 *   to return a ParticleStream-derived class instance in order to support
	 *   exhaust streams.
	 * \note The lvl, ref and dir parameters may be modified by orbiter after
	 *   the stream has been created, e.g. to reflect changes in engine thrust
	 *   level or gimballing.
	 */
	virtual ParticleStream *clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir);

	/**
	 * \brief Create a particle stream associated with a vessel.
	 *
	 * Typically used for exhaust and plasma effects, but can also be used
	 * for other types of particles.
	 * \param pss particle stream parameters
	 * \param hVessel vessel handle
	 * \param lvl pointer to exhaust level control variable
	 * \param ref pointer to stream source position (vessel frame) [<b>m</b>]
	 * \param dir pointer to stream direction (vessel frame)
	 * \return Pointer to new particle stream
	 * \default None, returns NULL. Derived classes should overload this method
	 *   to return a ParticleStream-derived class instance in order to support
	 *   exhaust streams.
	 * \note The lvl parameter may be modified by orbiter after
	 *   the stream has been created, e.g. to reflect changes in engine thrust
	 *   level.
	 * \note The ref and dir parameters are fixed in this version of the method.
	 */
	virtual ParticleStream *clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir);

	/**
	 * \brief Create a vessel particle stream for reentry heating effect
	 * \param pss particle stream parameters
	 * \param hVessel vessel handle
	 * \return Pointer to new particle stream
	 * \default None, returns NULL. Derived classes should overload this method
	 *   to return a ParticleStream-derived class instance in order to support
	 *   reentry streams.
	 */
	virtual ParticleStream *clbkCreateReentryStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel);
	// @}

	/**
	 * \brief Returns the handle of the main render window.
	 */
	GLFWwindow *GetRenderWindow () const { return hRenderWnd; }

	/**
	 * \brief Render window message handler
	 *
	 * Derived classes should also call the base class method to allow
	 * default message processing.
	 * \param hWnd render window handle
	 * \param uMsg Windows message identifier
	 * \param wParam WPARAM message parameter
	 * \param lParam LPARAM message parameter
	 * \return The return value depends on the message being processed.
	 * \note This is the standard Windows message handler for the render
	 *   window.
	 * \note This method currently intercepts only the WM_CLOSE and WM_DESTROY
	 *   messages, and passes everything else to the Orbiter core message
	 *   handler.
	 */
	//virtual LRESULT RenderWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/**
	 * \brief Fullscreen mode flag
	 * \return true if the client is set up for running in fullscreen
	 *   mode, false for windowed mode.
	 */
	virtual bool clbkFullscreenMode () const;

	/**
	 * \brief Returns the dimensions of the render viewport
	 * \param width render viewport width [pixel]
	 * \param height render viewport height [pixel]
	 * \note This function is called by orbiter after the render window or
	 *   fullscreen renderer has been created (see \ref clbkCreateRenderWindow).
	 * \note This should normally return the screen resolution in fullscreen
	 *   mode, and the size of the render window client area in windowed mode,
	 *   clients can also return smaller values if they only use part of the
	 *   screen area for scene rendering.
	 */
	virtual void clbkGetViewportSize (int *width, int *height) const;

	/**
	 * \brief Returns a specific render parameter
	 * \param[in] prm parameter identifier (see \sa renderprm)
	 * \param[out] value value of the queried parameter
	 * \return true if the specified parameter is supported by the client,
	 *    false if not.
	 */
	virtual bool clbkGetRenderParam (int prm, int *value) const;

	/**
	 * \brief Returns the surface containing the virtual cockpit HUD
	 * \param[out] hudspec pointer to structure containing mesh and group index,
	 *   and size parameters of VC HUD object
	 * \return HUD surface handle, or NULL if not available
	 */
//	SURFHANDLE GetVCHUDSurface (const VCHUDSPEC **hudspec) const;

	/**
	 * \brief Returns the surface containing an MFD display
	 * \param mfd MFD identifier (0 <= mfd < MAXMFD)
	 * \return MFD display surface handle, or NULL if not available
	 */
//	SURFHANDLE GetMFDSurface (int mfd) const;

	/**
	 * \brief Returns the surface containing a virtual cockpit MFD display
	 * \param[in] mfd MFD identifier (0 <= mfd < MAXMFD)
	 * \param[out] mfdspec pointer to structure containing mesh and group index
	 *   of the VC MFD display object
	 * \return MFD display surface handle, or NULL if not available
	 */
//	SURFHANDLE GetVCMFDSurface (int mfd, const VCMFDSPEC **mfdspec) const;

	/**
	 * \brief Render an instrument panel in cockpit view as a 2D billboard.
	 * \param hSurf array of texture handles for the panel surface
	 * \param hMesh billboard mesh handle
	 * \param T transformation matrix for panel mesh vertices (2D)
	 * \param additive If true, panel should be rendered additive (transparent)
	 * \default None.
	 * \note The texture index of each group in the mesh is interpreted as index into the
	 *   hSurf array. Special indices are TEXIDX_MFD0 and above, which specify the
	 *   surfaces representing the MFD displays. These are obtained separately and
	 *   don't need to be present in the hSurf list.
	 * \note The \e additive flag is used when rendering the default "glass
	 *   cockpit" if the user requested. "transparent MFDs". The renderer can
	 *   then use e.g. additive blending for rendering the panel.
	 */
	virtual void clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, bool additive = false);

	/**
	 * \brief Render an instrument panel in cockpit view as a 2D billboard.
	 * \param hSurf array of texture handles for the panel surface
	 * \param hMesh billboard mesh handle
	 * \param T transformation matrix for panel mesh vertices (2D)
	 * \param alpha opacity value, between 0 (transparent) and 1 (opaque)
	 * \param additive If true, panel should be rendered additive (transparent)
	 * \default None.
	 * \note The texture index of each group in the mesh is interpreted as index into the
	 *   hSurf array. Special indices are TEXIDX_MFD0 and above, which specify the
	 *   surfaces representing the MFD displays. These are obtained separately and
	 *   don't need to be present in the hSurf list.
	 * \note The \e additive flag is used when rendering the default "glass
	 *   cockpit" if the user requested. "transparent MFDs". The renderer can
	 *   then use e.g. additive blending for rendering the panel.
	 */
	virtual void clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive = false);

	// ==================================================================
	/// \name Surface-related methods
	// @{

	/**
	 * \brief Create a surface for texturing, as a blitting source, etc.
	 * 
	 * Surfaces are used for offscreen bitmap and texture manipulation,
	 * blitting and rendering.
	 * Derived classes should create a device-specific surface, and
	 * return a cast to a generic Orbiter SURFHANDLE.
	 * \param w surface width [pixels]
	 * \param h surface height [pixels]
	 * \param attrib \ref surfacecaps (bitflags). See notes.
	 * \return Surface handle (in the simplest case, just a pointer to the
	 *   surface, cast to a SURFHANDLE). On failure, this method should
	 *   return NULL.
	 * \default None, returns NULL.
	 * \note The attribute flag can contain one of the following main attributes:
	 *  - OAPISURFACE_RO: create a surface that can be read by the GPU pipeline, and that can be updated from system memory.
	 *  - OAPISURFACE_RW: create a surface that can be read and written by the GPU pipeline, and that can be updated from system memory.
	 *  - OAPISURFACE_GDI: create a surface that can be read and written from the CPU, and can be blitted into an uncompressed RO or RW surface without an alpha channel
	 *  In addition, the flag can contain any combination of the following auxiliary attributes:
	 *  - OAPISURFACE_MIPMAPS: create a full chain of mipmaps for the surface if possible
	 *  - OAPISURFACE_NOALPHA: create a surface without an alpha channel
	 */
	virtual SURFHANDLE clbkCreateSurfaceEx (int w, int h, int attrib);

	/**
	 * \brief Create an offscreen surface
	 *
	 * Surfaces are used for offscreen bitmap and texture manipulation,
	 * blitting and rendering.
	 * Derived classes should create a device-specific surface, and
	 * return a cast to a generic Orbiter SURFHANDLE.
	 * \param w surface width [pixels]
	 * \param h surface height [pixels]
	 * \param hTemplate surface format template
	 * \return pointer to surface, cast into a SURFHANDLE, or NULL to
	 *   indicate failure.
	 * \default None, returns NULL.
	 * \note If \e hTemplate is provided, this method should create the new
	 *   surface with the same pixel format.
	 * \sa clbkCreateTexture, clbkReleaseSurface
	 */
	virtual SURFHANDLE clbkCreateSurface (int w, int h, SURFHANDLE hTemplate = NULL);

	/**
	 * \brief Create a texture for rendering
	 * \param w texture width
	 * \param h texture height
	 * \return pointer to texture, returned as generic SURFHANDLE. NULL
	 *   indicates failure.
	 * \note This method is similar to \ref clbkCreateSurface, but the
	 *   returned surface handle must be usable as a texture when rendering
	 *   the scene. Clients which don't differentiate between offscreen
	 *   surfaces and textures may use identical code for both functions.
	 * \note Some clients may put restrictions on the texture format (e.g.
	 *   require square size (w=h), and/or powers of two (w=2^n). If the
	 *   texture cannot be created with the requested size, this method
	 *   should return NULL.
	 * \sa clbkCreateSurface, clbkReleaseSurface
	 */
	virtual SURFHANDLE clbkCreateTexture (int w, int h);

	/**
	 * \brief Increment the reference counter of a surface.
	 * \param surf surface handle
	 * \default None.
	 * \note Derived classes should keep track on surface references, and
	 *   overload this function to increment the reference counter.
	 */
	virtual void clbkIncrSurfaceRef (SURFHANDLE surf);

	/**
	 * \brief Decrement surface reference counter, release surface if counter
	 *   reaches 0.
	 * \param surf surface handle
	 * \return true on success
	 * \default None, returns false.
	 * \note Derived classes should overload this function to decrement a
	 *   surface reference counter and release the surface if required.
	 * \sa clbkCreateSurface, clbkIncrSurfaceRef
	 */
	virtual bool clbkReleaseSurface (SURFHANDLE surf);

	/**
	 * \brief Return the width and height of a surface
	 * \param[in] surf surface handle
	 * \param[out] w surface width
	 * \param[out] h surface height
	 * \return true if surface dimensions could be obtained.
	 * \default Sets w and h to 0 and returns false.
	 * \sa clbkCreateSurface
	 */
	virtual bool clbkGetSurfaceSize (SURFHANDLE surf, int *w, int *h);

	/**
	 * \brief Set transparency colour key for a surface.
	 * \param surf surface handle
	 * \param ckey transparency colour key value
	 * \default None, returns false.
	 * \note Derived classes should overload this method if the renderer
	 *   supports colour key transparency for surfaces.
	 */
	virtual bool clbkSetSurfaceColourKey (SURFHANDLE surf, uint32_t ckey);

	/**
	 * \brief Convert an RGB colour triplet into a device-specific colour value.
	 * \param r red component
	 * \param g green component
	 * \param b blue component
	 * \return colour value
	 * \note Derived classes should overload this method to convert RGB colour
	 *   definitions into device-compatible colour values, taking into account
	 *   the colour depth of the render device etc.
	 * \default Packs the RGB values into a uint32_t of the form 0x00RRGGBB, with
	 *   8 bits per colour component.
	 * \sa clbkFillSurface
	 */
	virtual uint32_t clbkGetDeviceColour (uint8_t r, uint8_t g, uint8_t b)
	{ return ((uint32_t)b << 16) + ((uint32_t)g << 8) + (uint32_t)r; }
	// @}

	// ==================================================================
	/// \name Surface blitting methods
	// @{

	/**
	 * \brief Copy one surface into an area of another one.
	 * \param tgt target surface handle
	 * \param tgtx left edge of target rectangle
	 * \param tgty top edge of target rectangle
	 * \param src source surface handle
	 * \param flag blitting parameters (see notes)
	 * \return true on success, false if the blit cannot be performed.
	 * \default None, returns false.
	 * \note By convention, tgt==NULL is valid and refers to the primary render
	 *   surface (e.g. for copying 2-D overlay surfaces).
	 * \note The following bit-flags are defined:
	 *   <table col=2>
	 *   <tr><td>BLT_SRCCOLORKEY</td><td>Use the colour key defined by the source surface for transparency</td></tr>
	 *   <tr><td>BLT_TGTCOLORKEY</td><td>Use the colour key defined by the target surface for transparency</td></tr>
	 *   </table>
	 *   If a client doesn't support some of the flags, it should quietly ignore it.
	 * \sa clbkBlt(SURFHANDLE,int,int,SURFHANDLE,int,int,int,int,int)
	 */
	virtual bool clbkBlt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int flag = 0) const;

	/**
	 * \brief Copy a rectangle from one surface to another.
	 * \param tgt target surfac handle
	 * \param tgtx left edge of target rectangle
	 * \param tgty top edge of target rectangle
	 * \param src source surface handle
	 * \param srcx left edge of source rectangle
	 * \param srcy top edge of source rectangle
	 * \param w width of rectangle
	 * \param h height of rectangle
	 * \param flag blitting parameters (see notes)
	 * \return true on success, false if the blit cannot be performed.
	 * \default None, returns false.
	 * \note By convention, tgt==NULL is valid and refers to the primary render
	 *   surface (e.g. for copying 2-D overlay surfaces).
	 * \note The following bit-flags are defined:
	 *   <table col=2>
	 *   <tr><td>BLT_SRCCOLORKEY</td><td>Use the colour key defined by the source surface for transparency</td></tr>
	 *   <tr><td>BLT_TGTCOLORKEY</td><td>Use the colour key defined by the target surface for transparency</td></tr>
	 *   </table>
	 *   If a client doesn't support some of the flags, it should quietly ignore it.
	 * \sa clbkBlt(SURFHANDLE,int,int,SURFHANDLE,int)
	 */
	virtual bool clbkBlt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int srcx, int srcy, int w, int h, int flag = 0) const;

	/**
	 * \brief Copy a rectangle from one surface to another, stretching or shrinking as required.
	 * \param tgt target surface handle
	 * \param tgtx left edge of target rectangle
	 * \param tgty top edge of target rectangle
	 * \param tgtw width of target rectangle
	 * \param tgth height of target rectangle
	 * \param src source surface handle
	 * \param srcx left edge of source rectangle
	 * \param srcy top edge of source rectangle
	 * \param srcw width of source rectangle
	 * \param srch height of source rectangle
	 * \param flag blitting parameters
	 * \return true on success, fals if the blit cannot be performed.
	 * \default None, returns false.
	 * \note By convention, tgt==NULL is valid and refers to the primary render
	 *   surface (e.g. for copying 2-D overlay surfaces).
	 * \sa clbkBlt(SURFHANDLE,int,int,SURFHANDLE,int),
	 *   clbkBlt(SURFHANDLE,int,int,SURFHANDLE,int,int,int,int,int)
	 */
	virtual bool clbkScaleBlt (SURFHANDLE tgt, int tgtx, int tgty, int tgtw, int tgth,
		                       SURFHANDLE src, int srcx, int srcy, int srcw, int srch, int flag = 0) const;

	/**
	 * \brief Fill a surface with a uniform colour
	 * \param surf surface handle
	 * \param col colour value
	 * \return true on success, false if the fill operation cannot be performed.
	 * \default None, returns false.
	 * \note Parameter col is a device-dependent colour value
	 *   (see \ref clbkGetDeviceColour).
	 * \sa clbkFillSurface(SURFHANDLE,int,int,int,int,uint32_t)
	 */
	virtual bool clbkFillSurface (SURFHANDLE surf, uint32_t col) const;

	/**
	 * \brief Fill an area in a surface with a uniform colour
	 * \param surf surface handle
	 * \param tgtx left edge of target rectangle
	 * \param tgty top edge of target rectangle
	 * \param w width of rectangle
	 * \param h height of rectangle
	 * \param col colour value
	 * \return true on success, false if the fill operation cannot be performed.
	 * \default None, returns false.
	 * \note Parameter col is a device-dependent colour value
	 *   (see \ref clbkGetDeviceColour).
	 * \sa clbkFillSurface(SURFHANDLE,uint32_t)
	 */
	virtual bool clbkFillSurface (SURFHANDLE surf, int tgtx, int tgty, int w, int h, uint32_t col) const;

	// ==================================================================
	/// \name 2-D drawing interface
	//@{
	/**
	 * \brief Create a 2-D drawing object ("sketchpad") associated with a surface.
	 * \param surf surface handle
	 * \return Pointer to drawing object.
	 * \default None, returns NULL.
	 * \note Clients should overload this function to provide 2-D drawing
	 *   support. This requires an implementation of a class derived from
	 *   \ref Sketchpad which provides the drawing context and drawing
	 *   primitives.
	 * \sa Sketchpad, clbkReleaseSketchpad
	 */
	virtual Sketchpad *clbkGetSketchpad (SURFHANDLE surf);

	/**
	 * \brief Release a drawing object.
	 * \param sp pointer to drawing object
	 * \default None.
	 * \sa Sketchpad, clbkGetSketchpad
	 */
	virtual void clbkReleaseSketchpad (Sketchpad *sp);

	/**
	 * \brief Create a font resource for 2-D drawing.
	 * \param height cell or character height [pixel]
	 * \param prop proportional/fixed width flag
	 * \param face font face name
	 * \param style font decoration style
	 * \param orientation text orientation [1/10 deg]
	 * \return Pointer to font resource
	 * \default None, returns NULL.
	 * \note For a description of the parameters, see Font constructor
	 *   \ref oapi::Font::Font
	 * \sa clbkReleaseFont, oapi::Font
	 */
	virtual Font *clbkCreateFont (int height, bool prop, const char *face, oapi::Font::Style style = oapi::Font::NORMAL, int orientation = 0) const;

	/**
	 * \brief De-allocate a font resource.
	 * \param font pointer to font resource
	 * \default None.
	 * \sa clbkCreateFont, oapi::Font
	 */
	virtual void clbkReleaseFont (Font *font) const;

	/**
	 * \brief Create a pen resource for 2-D drawing.
	 * \param style line style (0=invisible, 1=solid, 2=dashed)
	 * \param width line width [pixel]
	 * \param col line colour (format: 0xBBGGRR)
	 * \return Pointer to pen resource
	 * \default None, returns NULL.
	 * \sa clbkReleasePen, oapi::Pen
	 */
	virtual Pen *clbkCreatePen (int style, int width, uint32_t col) const;

	/**
	 * \brief De-allocate a pen resource.
	 * \param pen pointer to pen resource
	 * \default None.
	 * \sa clbkCreatePen, oapi::Pen
	 */
	virtual void clbkReleasePen (Pen *pen) const;

	/**
	 * \brief Create a brush resource for 2-D drawing.
	 * \param col line colour (format: 0xBBGGRR)
	 * \return Pointer to brush resource
	 * \default None, returns NULL.
	 * \sa clbkReleaseBrush, oapi::Brush
	 */
	virtual Brush *clbkCreateBrush (uint32_t col) const;

	/**
	 * \brief De-allocate a brush resource.
	 * \param brush pointer to brush resource
	 * \default None.
	 * \sa clbkCreateBrush, oapi::Brush
	 */
	virtual void clbkReleaseBrush (Brush *brush) const;
	//@}

protected:
	/** \brief Launchpad video tab indicator
	 *
	 * Indicate if the the default video tab in the Orbiter launchpad dialog
	 * is to be used for obtaining user video preferences. If a derived
	 * class returns false here, the video tab is not shown.
	 * \return true if the module wants to use the video tab in the launchpad
	 *   dialog, false otherwise.
	 * \default Return true.
	 */
	virtual bool clbkUseLaunchpadVideoTab () const;

	/**
	 * \brief Simulation session start notification
	 *
	 * Called at the beginning of a simulation session to allow the client
	 * to create the 3-D rendering window (or to switch into fullscreen
	 * mode).
	 * \return Should return window handle of the rendering window.
	 * \default For windowed mode, opens a window of the size specified by the
	 *   VideoData structure (for fullscreen mode, opens a small dummy window)
	 *   and returns the window handle.
	 * \note For windowed modes, the viewW and viewH parameters should return
	 *   the window client area size. For fullscreen mode, they should contain
	 *   the screen resolution.
	 * \note Derived classes should perform any required per-session
	 *   initialisation of the 3D render environment here.
	 */
	virtual GLFWwindow *clbkCreateRenderWindow () override;
 	virtual void clbkMakeContextCurrent(bool) override;

	/**
	 * \brief Simulation startup finalisation
	 *
	 * Called at the beginning of a simulation session after the scenarion has
	 * been parsed and the logical object have been created.
	 * \default None
	 */
	virtual void clbkPostCreation ();

	/**
	 * \brief End of simulation session notification
	 *
	 * Called before the end of a simulation session. At the point of call,
	 * logical objects still exist (OBJHANDLEs valid), and external modules
	 * are still loaded.
	 * \param fastclose Indicates a "fast shutdown" request (see notes)
	 * \default None.
	 * \note Derived clients can use this function to perform cleanup operations
	 *   for which the simulation objects are still required.
	 * \note If fastclose == true, the user has selected one of the fast
	 *   shutdown options (terminate Orbiter, or respawn Orbiter process). In
	 *   this case, the current process will terminate, and the graphics client
	 *   can skip object cleanup and deallocation in order to speed up the
	 *   closedown process.
	 * \sa clbkDestroyRenderWindow
	 */
	virtual void clbkCloseSession (bool fastclose);

	/**
	 * \brief Render window closure notification
	 *
	 * Called at the end of a simulation session to allow the client to close
	 * the 3-D rendering window (or to switch out of fullscreen mode) and
	 * clean up the session environment. At the point of call, all logical
	 * simulation objects have been destroyed, and object modules have been
	 * unloaded. This method should not access any OBJHANDLE or VESSEL
	 * objects any more. For closedown operations that require access to the
	 * simulation objects, use clbkCloseSession instead.
	 * \param fastclose Indicates a "fast shutdown" request (see notes)
	 * \default None.
	 * \note Derived classes should perform any required cleanup of the 3D
	 *   render environment here.
	 * \note The user may change the video parameters before starting a new
	 *   simulation session. Therefore, device-specific options should be
	 *   destroyed and re-created at the start of the next session.
	 * \note If fastclose == true, the user has selected one of the fast
	 *   shutdown options (terminate Orbiter, or respawn Orbiter process). In
	 *   this case, the current process will terminate, and the graphics client
	 *   can skip object cleanup and deallocation in order to speed up the
	 *   closedown process.
	 * \sa clbkCloseSession
	 */
	virtual void clbkDestroyRenderWindow (bool fastclose);

	/**
	 * \brief Per-frame update notification
	 *
	 * Called once per frame, after the logical world state has been updated,
	 * but before clbkRenderScene(), to allow the client to perform any
	 * logical state updates.
	 * \param running true if simulation is running, false if paused.
	 * \default None.
	 * \note Unlike clbkPreStep and clbkPostStep, this method is also called
	 *   while the simulation is paused.
	 */
	//virtual void clbkUpdate (bool running);

	/**
	 * \brief Per-frame render notification
	 *
	 * Called once per frame, after the logical world state has been updated,
	 * to allow the client to render the current scene.
	 * \note This method is also called continuously while the simulation is
	 *   paused, to allow camera panning (although in that case the logical
	 *   world state won't change between frames).
	 * \note After the 3D scene has been rendered, this function should call
	 *   \ref Render2DOverlay to initiate rendering of 2D elements (2D instrument
	 *   panel, HUD, etc.)
	 */
	virtual void clbkRenderScene ();
	virtual void clbkImGuiNewFrame ();
	virtual void clbkImGuiRenderDrawData ();

	/**
	 * \brief Display a scene on screen after rendering it.
	 *
	 * Called after clbkRenderScene to allow the client to display the rendered
	 * scene (e.g. by page-flipping, or blitting from background to primary
	 * frame buffer. This method can also be used by the client to display any
	 * top-level 2-D overlays (e.g. dialogs) on the primary frame buffer.
	 * \return Should return true on successful operation, false on failure or
	 *   if no operation was performed.
	 * \default None, returns false.
	 */
	virtual bool clbkDisplayFrame () override;
	virtual bool clbkClearFrame () override;

	/**
	 * \brief Display a load status message on the splash screen
	 *
	 * Called repeatedly while a simulation session is loading, to allow the
	 * client to echo load status messages on its splash screen if desired.
	 * \param msg Pointer to load status message string
	 * \param line message line to be displayed (0 or 1), where 0 indicates
	 *   a group or category heading, and 1 indicates an individual action
	 *   relating to the most recent group.
	 * \return Should return true if it displays the message, false if not.
	 * \default None, returns false.
	 */
	virtual bool clbkSplashLoadMsg (const char *msg, int line);

	/**
	 * \brief Store a persistent mesh template
	 *
	 * Called when a plugin loads a mesh with oapiLoadMeshGlobal, to allow the
	 * client to store a copy of the mesh in client-specific format. Whenever
	 * the mesh is required later, the client can create an instance as a copy
	 * of the template, rather than creating it by converting from Orbiter's
	 * mesh format.
	 * \param hMesh mesh handle
	 * \param fname mesh file name
	 * \default None.
	 * \note Use \ref oapiMeshGroup to to obtain mesh data and convert them to
	 *   a suitable format.
	 * \note the mesh templates loaded with \ref oapiLoadMeshGlobal are shared between
	 *   all vessel instances and should never be edited. Vessels should make
	 *   individual copies of the mesh before modifying them (e.g. for animations)
	 * \note The file name is provide to allow the client to parse the mesh directly
	 *   from file, rather than copying it from the hMesh object, or to use an
	 *   alternative mesh file.
	 * \note The file name contains a path relative to Orbiter's main mesh
	 *   directory.
	 */
	virtual void clbkStoreMeshPersistent (MESHHANDLE hMesh, const char *fname);

	/**
	 * \brief Displays the default Orbiter splash screen on top of
	 *   the render window.
	 */
	void ShowDefaultSplash ();

	/**
	 * \brief Write a block of raw image data to a formatted image file.
	 * \param data image specification structure
	 * \param fname output file name (relative to orbiter root directory)
	 * \param fmt output format
	 * \param quality requested image quality, if supported by the format
	 * \return \e true on success, \e false if inconsistencies in the image
	 *   specifications were detected (see notes)
	 * \note The following limitations to the provided image data currently
	 *  apply:
	 *  - data.bpp must be 24
	 *  - data.stride must be aligned to 4 bytes, i.e. (data.width * data.bpp + 31) & ~31) >> 3
	 *  - data.bufsize must be >= data.stride * data.height
	 * \sa ImageData, ImageFileFormat
	 */
//	bool WriteImageDataToFile (const ImageData &data,
//		const char *fname, ImageFileFormat fmt=IMAGE_JPG, float quality=0.7f);

	/**
	 * \brief Read an image from a memory buffer
	 * \param pBuf pointer to memory buffer
	 * \param nBuf size of memory buffer
	 * \param w width of image after scaling (0 to keep original width)
	 * \param h height of image after scaling (0 to keep original height)
	 * \note This function automatically recognises different image formats
	 *   in the memory buffer (bmp, jpg, png, tif)
	 * \note This function can be used to read in an image from a resource
	 *   stored in the executable file (see Windows API functions
	 *   LoadResource, LockResource, SizeofResource)
	 * \sa ReadImageFromFile, WriteImageDataToFile
	 */
//	HBITMAP ReadImageFromMemory (uint8_t *pBuf, int nBuf, UINT w, UINT h);

	/**
	 * \brief Read an image from a file into a bitmap
	 * \param fname file name
	 * \param fmt image format
	 * \param w width of image after scaling (0 to keep original width)
	 * \param h height of image after scaling (0 to keep original height)
	 * \note This function can read different image formats (bmp, jpg, png, tif)
	 * \sa ReadImageFromMemory, WriteImageDataToFile
	 */
//	HBITMAP ReadImageFromFile (const char *fname, UINT w=0, UINT h=0);

	// ==================================================================
	/// \name Marker and label-related methods
	// @{
	struct LABELSPEC {
		VECTOR3 pos;
		char *label[2];
	};
	/**
	 * \brief Label list description for celestial and surface markers
	 */
	struct LABELLIST {
		char name[64];   ///< list name
		LABELSPEC *list; ///< marker array
		int length;      ///< length of the marker array
		int colour;      ///< marker colour index (0-5)
		int shape;       ///< marker shape index (0-4)
		float size;      ///< marker size factor
		float distfac;   ///< marker distance cutout factor
		int flag;      ///< reserved
		bool active;     ///< active list flag
	};

protected:
	SURFHANDLE surfBltTgt;  ///< target surface for a blitting group (-1=none, NULL=main window render surface)
	oapi::Font *splashFont; // font for splash screen displays

private:
	/**
	 * \brief Render window initialisation
	 *
	 * - Sets the viewW and viewH values
	 * - Sets the GWLP_USERDATA data of the render window to *this
	 * \param hWnd Render window handle. If this is NULL, InitRenderWnd
	 *   will create a dummy window and return its handle.
	 * \return Render window handle
	 * \note This is called after clbkCreateRenderWindow returns.
	 */
	//HWND InitRenderWnd (HWND hWnd);

	GLFWwindow *hRenderWnd;        // render window handle

	uint32_t m_width, m_height;

	//FIXME : replace with STB image
//	IWICImagingFactory *m_pIWICFactory; // Windows Image Component factory instance
};

// ======================================================================
// class VisObject
// ======================================================================
/**
 * \brief Visual object representation.
 *
 * A VisObject is the visual representation of an Orbiter object (vessel,
 * planet, etc.). The 'logical' object representation resides in the Orbiter
 * core, while its 'visual' representation is located in the graphics client.
 *
 * Visual representations should be non-permanent: they should be created
 * when the object enters the visual range of the camera, and deleted when
 * they leave it.
 *
 * Only a single VisObject instance should be created per object, even if
 * the visual is present in multiple views. If the graphics client supports
 * multiple views, the view-specific parameters (e.g. visibility flags) should
 * be implemented by the client, e.g. by deriving a class from VisObject that
 * holds an array of the view-specific data. In that case, the VisObject should
 * be created when the object becomes visible in any one view, and destroyed
 * when the object disappears from the last view.
 */
class VisObject {
public:
	/**
	 * \brief Creates a visual for object hObj.
	 * \param hObj object handle
	 * \sa oapi::GraphicsClient::RegisterVisObject
	 */
	VisObject (OBJHANDLE hObj);

	/**
	 * \brief Destroys the visual.
	 * \sa oapi::GraphicsClient::UnregisterVisObject
	 */
	virtual ~VisObject ();

	/**
	 * \brief Returns the object handle associated with the visual.
	 * \return Object handle
	 */
	OBJHANDLE GetObject () const { return hObject; }

	/**
	 * \brief Message callback.
	 * \param event message identifier
	 * \param context message content (message-specific)
	 * \default None.
	 * \note This method is called by the Orbiter core to notify the visual
	 *   of certain events (e.g. adding and deleting meshes)
	 * \note For currently supported event types, see \ref visevent.
	 */
	virtual void clbkEvent (visevent msg, visevent_data content) {}

protected:
	OBJHANDLE hObject;
};

extern OGLClient *g_client;

#endif
