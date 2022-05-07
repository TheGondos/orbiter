#define ORBITER_MODULE
#include "glad.h"
#include "OGLClient.h"
#include "Scene.h"
#include "VObject.h"
#include "VStar.h"
#include "Texture.h"
#include "tilemgr2.h"
#include "TileMgr.h"
#include "HazeMgr.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
//#include "Orbiter.h"
#include "Particle.h"
#include "VVessel.h"

//extern Orbiter *g_pOrbiter;

const uint32_t SPEC_DEFAULT = (uint32_t)(-1); // "default" material/texture flag
const uint32_t SPEC_INHERIT = (uint32_t)(-2); // "inherit" material/texture flag

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

// ==============================================================
// API interface
// ==============================================================

using namespace oapi;

OGLClient *g_client = 0;

// ==============================================================
// Initialise module
DLLCLBK void InitModule (DynamicModule *hDLL)
{
	g_client = new OGLClient (hDLL);
	if (!oapiRegisterGraphicsClient (g_client)) {
		delete g_client;
		g_client = 0;
	}
}

// ==============================================================
// Clean up module

DLLCLBK void ExitModule (DynamicModule *hDLL)
{
	if (g_client) {
		oapiUnregisterGraphicsClient (g_client);
		delete g_client;
		g_client = 0;
	}
}

OGLClient::OGLClient (DynamicModule *hInstance):GraphicsClient(hInstance)
{
}

OGLClient::~OGLClient ()
{
}

void OGLClient::Style()
{
	ImGuiStyle & style = ImGui::GetStyle();
	ImVec4 * colors = style.Colors;
	
	/// 0 = FLAT APPEARENCE
	/// 1 = MORE "3D" LOOK
	int is3D = 1;

	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator]              = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

	style.PopupRounding = 3;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding  = ImVec2(6, 4);
	style.ItemSpacing   = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.WindowBorderSize = 1;
	style.ChildBorderSize  = 1;
	style.PopupBorderSize  = 1;
	style.FrameBorderSize  = is3D; 

	style.WindowRounding    = 3;
	style.ChildRounding     = 3;
	style.FrameRounding     = 3;
	style.ScrollbarRounding = 2;
	style.GrabRounding      = 3;

	#ifdef IMGUI_HAS_DOCK 
		style.TabBorderSize = is3D; 
		style.TabRounding   = 3;

		colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_Tab]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabActive]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_TabUnfocused]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 12);

	#endif
}

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
bool OGLClient::clbkInitialise()
{
    if (!GraphicsClient::clbkInitialise ()) return false;
    return true;
}

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
int OGLClient::clbkVisEvent (OBJHANDLE hObj, VISHANDLE vis, visevent msg, visevent_data context)
{
    VObject *vo = (VObject*)vis;
	vo->clbkEvent (msg, context);
	return 1;
}

ParticleStream *OGLClient::clbkCreateParticleStream (PARTICLESTREAMSPEC *pss)
{
    return NULL;
}

ParticleStream *OGLClient::clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir)
{
	ExhaustStream *es = new ExhaustStream (this, hVessel, lvl, ref, dir, pss);
	g_client->GetScene()->AddParticleStream (es);
	return es;
}
ParticleStream *OGLClient::clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir)
{
	ExhaustStream *es = new ExhaustStream (this, hVessel, lvl, ref, dir, pss);
	g_client->GetScene()->AddParticleStream (es);
	return es;
}

ParticleStream *OGLClient::clbkCreateReentryStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel)
{
	ReentryStream *rs = new ReentryStream (this, hVessel, pss);
	g_client->GetScene()->AddParticleStream (rs);
	return rs;
}

/**
 * \brief Fullscreen mode flag
 * \return true if the client is set up for running in fullscreen
 *   mode, false for windowed mode.
 */
bool OGLClient::clbkFullscreenMode () const
{
	return false;
}

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
void OGLClient::clbkGetViewportSize (int *w, int *h) const
{
	*w = m_width, *h = m_height;
}

/**
 * \brief Returns a specific render parameter
 * \param[in] prm parameter identifier (see \sa renderprm)
 * \param[out] value value of the queried parameter
 * \return true if the specified parameter is supported by the client,
 *    false if not.
 */
bool OGLClient::clbkGetRenderParam (int prm, int *value) const
{
	switch (prm) {
	case RP_COLOURDEPTH:
		*value = 32;
		return true;
	case RP_ZBUFFERDEPTH:
		*value = 24;
		return true;
	case RP_STENCILDEPTH:
		*value = 8;
		return true;
	case RP_MAXLIGHTS:
		*value = 8;
		return true;
	case RP_ISTLDEVICE:
		*value = 1;
		return true;
	case RP_REQUIRETEXPOW2:
		*value = 1;
		return true;
	}
	return false;
}

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
void OGLClient::clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, bool transparent)
{
	static int nHVTX = 1;
	static struct HVTX {
		float tu, tv;
		float x, y;
	} *hvtx = new HVTX[1];
glDisable(GL_DEPTH_TEST);
	static glm::mat4 ortho_proj = glm::ortho(0.0f, (float)g_client->GetScene()->GetCamera()->GetWidth(), (float)g_client->GetScene()->GetCamera()->GetHeight(), 0.0f);
	static Shader s("Overlay.vs","Overlay.fs");
//	static glm::vec3 vecTextColor = glm::vec3(1,1,1);

	static GLuint m_Buffer, m_VAO, IBO;
	static bool init = false;
	if(!init) {
		init=true;
		glGenVertexArrays(1, &m_VAO);
		CheckError("clbkRender2DPanel glGenVertexArrays");
		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glGenBuffers(1, &m_Buffer);
		CheckError("clbkRender2DPanel glGenBuffers");
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferData(GL_ARRAY_BUFFER, 2048 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
		CheckError("clbkRender2DPanel glBufferData");
		glGenBuffers(1, &IBO);
		CheckError("clbkRender2DPanel glGenBuffers");

		glBindVertexArray(0);
	}
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetFloat("color_keyed", 0.0);

	//s.SetVec3("font_color", vecTextColor);

/*
	int vtxFmt = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
	int dAlpha;
	pd3dDevice->GetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, &dAlpha);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	pd3dDevice->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	if (transparent)
		pd3dDevice->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	float rhw = 1;
	*/
	int i, j, nvtx, ngrp = oapiMeshGroupCount (hMesh);
	SURFHANDLE surf = 0, newsurf = 0;

	float scalex = (float)(T->m11),  dx = (float)(T->m13);
	float scaley = (float)(T->m22),  dy = (float)(T->m23);

	for (i = 0; i < ngrp; i++) {
		MESHGROUP *grp = oapiMeshGroup (hMesh, i);
		if (grp->UsrFlag & 2) continue; // skip this group

		if (grp->TexIdx == SPEC_DEFAULT) {//SPEC_DEFAULT) {
			newsurf = 0;
		} else if (grp->TexIdx == SPEC_INHERIT) {//SPEC_INHERIT) {
			// nothing to do
		} else if ((unsigned)grp->TexIdx >= (unsigned)TEXIDX_MFD0) {
			int mfdidx = grp->TexIdx-TEXIDX_MFD0;
			newsurf = GetMFDSurface (mfdidx);
			if (!newsurf) continue;
		} else if (hSurf) {
			newsurf = hSurf[grp->TexIdx];
		} else {
			newsurf = oapiGetTextureHandle (hMesh, grp->TexIdx+1);
		}
		if (newsurf != surf) {
//			pd3dDevice->SetTexture (0, (LPDIRECTDRAWSURFACE7)(surf = newsurf));
			surf = newsurf;
			glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)surf)->m_TexId);
		}

		nvtx = grp->nVtx;
		if (nvtx > nHVTX) { // increase buffer size
			delete []hvtx;
			hvtx = new HVTX[nHVTX = nvtx];
		}
		for (j = 0; j < nvtx; j++) {
			HVTX *tgtvtx = hvtx+j;
			NTVERTEX *srcvtx = grp->Vtx+j;
			tgtvtx->x = srcvtx->x*scalex + dx;
			tgtvtx->y = srcvtx->y*scaley + dy;
			tgtvtx->tu = srcvtx->tu;
			tgtvtx->tv = srcvtx->tv;
		}
//		pd3dDevice->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, vtxFmt, hvtx, nvtx, grp->Idx, grp->nIdx, 0);
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferSubData(GL_ARRAY_BUFFER, 0, nvtx * sizeof(HVTX), hvtx);
		CheckError("clbkRender2DPanel glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("clbkRender2DPanel glVertexAttribPointer");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CheckError("clbkRender2DPanel glBindBuffer");
		
		glEnableVertexAttribArray(0);
		CheckError("clbkRender2DPanel glEnableVertexAttribArray0");

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, grp->nIdx * 2, grp->Idx, GL_STATIC_DRAW);
		CheckError("clbkRender2DPanel glBufferData");

		glDrawElements(GL_TRIANGLES, grp->nIdx, GL_UNSIGNED_SHORT, 0);
	//		glDrawArrays(GL_TRIANGLES, 0, nvtx);
		CheckError("clbkRender2DPanel glDrawArrays");

		CheckError("clbkRender2DPanel glDrawArrays");
		glBindVertexArray(0);



	}
	s.UnBind();
	glBindTexture(GL_TEXTURE_2D, 0);
	/*
	pd3dDevice->SetTextureStageState (0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
	if (transparent)
		pd3dDevice->SetRenderState (D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	if (dAlpha != TRUE)
		pd3dDevice->SetRenderState (D3DRENDERSTATE_ALPHABLENDENABLE, dAlpha);
*/
}

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
void OGLClient::clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive)
{
	/*
	bool reset = false;
	int alphaop, alphaarg2, tfactor;
	if (alpha < 1.0f) {
		pd3dDevice->GetTextureStageState (0, D3DTSS_ALPHAOP, &alphaop);
		pd3dDevice->GetTextureStageState (0, D3DTSS_ALPHAARG2, &alphaarg2);
		pd3dDevice->GetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, &tfactor);
		pd3dDevice->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		pd3dDevice->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
		pd3dDevice->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, D3DRGBA(1,1,1,alpha));
		reset = true;
	}*/
	clbkRender2DPanel (hSurf, hMesh, T, additive);
/*
	if (reset) {
		pd3dDevice->SetTextureStageState (0, D3DTSS_ALPHAOP, alphaop);
		pd3dDevice->SetTextureStageState (0, D3DTSS_ALPHAARG2, alphaarg2);
		pd3dDevice->SetRenderState (D3DRENDERSTATE_TEXTUREFACTOR, tfactor);
	}
*/
}

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

/*
static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
	g_client->SetSize(width, height);
}*/
//#include "Camera.h"
//extern Camera *g_camera;
void OGLClient::clbkSetViewportSize(int w, int h) {
	m_width = w;
	m_height = h;
	if(mScene) mScene->GetCamera()->SetSize(w, h);
	glViewport(0, 0, m_width, m_height);
//	if(g_camera) g_camera->ResizeViewport(w,h);
}

GLFWwindow *OGLClient::clbkCreateRenderWindow ()
{
	/* Initialize the library */
    if (!glfwInit()) {
		printf("glfwInit failed\n");
		exit(EXIT_FAILURE);
	}

    glfwWindowHint(GLFW_DOUBLEBUFFER , 1);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
        );
	
//	glfwWindowHint(GLFW_SAMPLES, 4);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

//	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    hRenderWnd = glfwCreateWindow(1280, 800, "xorbiter", NULL, NULL);
	m_width = 1280;
	m_height = 800;
    if (!hRenderWnd)
    {
		printf("!hRenderWnd\n");
        glfwTerminate();
        exit(-1);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(hRenderWnd);
    glfwSwapInterval(1); // VSync

    if(!gladLoadGL())
    {
        printf("OGLClient: gladLoadGL failed\n");
        exit(-1);
    }

	glViewport(0, 0, m_width, m_height);

//	glfwSetFramebufferSizeCallback(hRenderWnd, framebuffer_size_callback);

	//ValidateRect (hRenderWnd, NULL);
	// avoids white flash after splash screen
	int num_ext = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
	for(int i=0; i<num_ext; i++)
		if(!strcmp((const char *)glGetStringi(GL_EXTENSIONS,i), "GL_ARB_compatibility")) {
			printf("Compatiblity Profile");
			exit(-1);
		}

	mMeshManager = std::make_unique<OGLMeshManager>();
	mScene = std::make_unique<Scene>(m_width, m_height);
	mTextureManager = std::make_unique<TextureManager>();
	TileManager::GlobalInit ();
	HazeManager::GlobalInit();

	TileManager2Base::GlobalInit();
	VStar::GlobalInit ();
	OGLParticleStream::GlobalInit();
	VVessel::GlobalInit();
	mBlitShader = std::make_unique<Shader>("Blit.vs","Blit.fs");
	glEnable( GL_LINE_SMOOTH );
	glEnable( GL_POLYGON_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();
	Style();

    ImGui_ImplGlfw_InitForOpenGL(hRenderWnd, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	return hRenderWnd;
}

void OGLClient::clbkMakeContextCurrent(bool b) {
	glfwMakeContextCurrent(b ? hRenderWnd : nullptr);
}

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
void OGLClient::clbkDestroyRenderWindow (bool fastclose)
{
	GraphicsClient::clbkDestroyRenderWindow (fastclose);
	//Cleanup3DEnvironment();
	hRenderWnd = NULL;
}

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
void OGLClient::clbkRenderScene ()
{
	//scene->Render();
//	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//	CheckError("glClearColor");
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	CheckError("glClear");
	mScene->Render();
}

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
SURFHANDLE OGLClient::clbkLoadTexture (const char *fname, int flags)
{
	if (flags & 8) // load managed
		return (SURFHANDLE)mTextureManager->LoadTexture(fname, true, flags);
	else           // load individual
		return (SURFHANDLE)mTextureManager->LoadTexture(fname, false, flags);
}

/**
 * \brief Texture release request
 *
 * Called by Orbiter when a previously loaded texture can be released
 * from memory. The client can use the appropriate device-specific method
 * to release the texture.
 * \param hTex texture handle
 * \default None.
 */
void OGLClient::clbkReleaseTexture (SURFHANDLE hTex)
{
	mTextureManager->ReleaseTexture((OGLTexture *)hTex);
}

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
SURFHANDLE OGLClient::clbkLoadSurface (const char *fname, int flags)
{
    printf("OGLClient::clbkLoadSurface %s\n", fname);
	exit(-1);
    return NULL;
}

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
bool OGLClient::clbkSaveSurfaceToImage (SURFHANDLE surf, const char *fname,
		ImageFileFormat fmt, float quality)
{
    return false;
}

/**
 * \brief Replace a texture in a device-specific mesh.
 * \param hMesh device mesh handle
 * \param texidx texture index (>= 0)
 * \param tex texture handle
 * \return Should return \e true if operation successful, \e false otherwise.
 * \default None, returns \e false.
 */
bool OGLClient::clbkSetMeshTexture (DEVMESHHANDLE hMesh, int texidx, SURFHANDLE tex)
{
	return ((OGLMesh*)hMesh)->SetTexture (texidx, tex);
}

/**
 * \brief Replace properties of an existing mesh material.
 * \param hMesh device mesh handle
 * \param matidx material index (>= 0)
 * \param mat pointer to material structure
 * \return Overloaded functions should return an integer error flag, with
 *   the following codes: 0="success", 3="invalid mesh handle", 4="material index out of range"
 * \default None, returns 2 ("client does not support operation").
 */
int OGLClient::clbkSetMeshMaterial (DEVMESHHANDLE hMesh, int matidx, const MATERIAL *mat)
{
	return ((OGLMesh*)hMesh)->SetMaterial (matidx, mat);
}

/**
 * \brief Retrieve the properties of one of the mesh materials.
 * \param hMesh device mesh handle
 * \param matidx material index (>= 0)
 * \param mat [out] pointer to MATERIAL structure to be filled by the method.
 * \return true if successful, false on error (index out of range)
 * \default None, returns 2 ("client does not support operation").
 */
int OGLClient::clbkMeshMaterial (DEVMESHHANDLE hMesh, int matidx, MATERIAL *mat)
{
    printf("OGLClient::clbkMeshMaterial\n");
exit(-1);
	return 0;
}

/**
 * \brief Popup window open notification.
 * \note This method is called just before a popup window (e.g. dialog
 *   box) is opened. It allows the client to prepare for subsequent
 *   rendering of the window, if necessary.
 */
void OGLClient::clbkPreOpenPopup ()
{
	//if (clipper) pDD->FlipToGDISurface();
}

void OGLClient::clbkNewVessel (OBJHANDLE hVessel)
{
	mScene->NewVessel (hVessel);
}

// ==============================================================

void OGLClient::clbkDeleteVessel (OBJHANDLE hVessel)
{
	mScene->DeleteVessel (hVessel);
}

/**
 * \brief Return the width and height of a surface
 * \param[in] surf surface handle
 * \param[out] w surface width
 * \param[out] h surface height
 * \return true if surface dimensions could be obtained.
 * \default Sets w and h to 0 and returns false.
 * \sa clbkCreateSurface
 */
bool OGLClient::clbkGetSurfaceSize (SURFHANDLE surf, int *w, int *h)
{
	OGLTexture *tex = (OGLTexture *)surf;
	*w = tex->m_Width;
	*h = tex->m_Height;
	return true;
}

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
bool OGLClient::clbkSetMeshProperty (DEVMESHHANDLE hMesh, int property, int value)
{
//    printf("OGLClient::clbkSetMeshProperty\n");
	OGLMesh *mesh = (OGLMesh*)hMesh;
	switch (property) {
	case MESHPROPERTY_MODULATEMATALPHA:
		mesh->EnableMatAlpha (value != 0);
		return true;
	}
	return false;
}

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
MESHHANDLE OGLClient::clbkGetMesh (VISHANDLE vis, unsigned int idx)
{
	return (vis ? ((VObject*)vis)->GetMesh (idx) : NULL);
}

/**
 * \brief Mesh group data retrieval interface for device-specific meshes.
 * \param hMesh device mesh handle
 * \param grpidx mesh group index (>= 0)
 * \param grs data buffers and buffer size information. See \ref oapiGetMeshGroup
 *    for details.
 * \return Should return 0 on success, or error flags > 0.
 * \default None, returns -2.
 */
int OGLClient::clbkGetMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPREQUESTSPEC *grs)
{
	return ((OGLMesh*)hMesh)->GetGroup (grpidx, grs);
}

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
int OGLClient::clbkEditMeshGroup (DEVMESHHANDLE hMesh, int grpidx, GROUPEDITSPEC *ges)
{
	return ((OGLMesh*)hMesh)->EditGroup (grpidx, ges);
}

/**
 * \brief Increment the reference counter of a surface.
 * \param surf surface handle
 * \default None.
 * \note Derived classes should keep track on surface references, and
 *   overload this function to increment the reference counter.
 */
void OGLClient::clbkIncrSurfaceRef (SURFHANDLE surf)
{
	((OGLTexture *)surf)->m_RefCnt++;
	//((LPDIRECTDRAWSURFACE7)surf)->AddRef();
}

/**
 * \brief Create a pen resource for 2-D drawing.
 * \param style line style (0=invisible, 1=solid, 2=dashed)
 * \param width line width [pixel]
 * \param col line colour (format: 0xBBGGRR)
 * \return Pointer to pen resource
 * \default None, returns NULL.
 * \sa clbkReleasePen, oapi::Pen
 */
Pen *OGLClient::clbkCreatePen (int style, int width, uint32_t col) const
{
	return new OGLPen(style, width, col);
}

void OGLClient::clbkReleasePen (Pen *pen) const
{
	delete (OGLPen *)pen;
}

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
Font *OGLClient::clbkCreateFont (int height, bool prop, const char *face, Font::Style style, int orientation) const
{
	OGLFont *font = new OGLFont (height, prop, face, style, orientation);
	return font;
}

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
SURFHANDLE OGLClient::clbkCreateSurfaceEx (int w, int h, int attrib)
{
	return mTextureManager->GetTextureForRendering(w, h);
    /*
	HRESULT hr;
	LPDIRECTDRAWSURFACE7 surf;
	DDSURFACEDESC2 ddsd;
    ZeroMemory (&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwWidth  = w;
	ddsd.dwHeight = h;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH; 
	if (attrib & OAPISURFACE_TEXTURE)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;
	else
		ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
	if (attrib & OAPISURFACE_SYSMEM || (attrib & OAPISURFACE_GDI || attrib & OAPISURFACE_SKETCHPAD) && !(attrib & OAPISURFACE_TEXTURE)) 
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	else
		ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	if ((attrib & OAPISURFACE_ALPHA) && !(attrib & (OAPISURFACE_GDI | OAPISURFACE_SKETCHPAD)))
		ddsd.ddpfPixelFormat.dwFlags |=  DDPF_ALPHAPIXELS; // enable alpha channel
	if ((hr = pDD->CreateSurface (&ddsd, &surf, NULL)) != DD_OK) {
		LOGOUT_DDERR (hr);
		return NULL;
	}
	return (SURFHANDLE)surf;
    */
}

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
SURFHANDLE OGLClient::clbkCreateSurface (int w, int h, SURFHANDLE hTemplate)
{
	return mTextureManager->GetTextureForRendering(w, h);

/*
	HRESULT hr;
	LPDIRECTDRAWSURFACE7 surf;
	DDSURFACEDESC2 ddsd;
    ZeroMemory (&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH; 
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth  = w;
	ddsd.dwHeight = h;
	if (hTemplate) { // use pixel format information from template surface
		ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = 0;
		((LPDIRECTDRAWSURFACE7)hTemplate)->GetPixelFormat (&ddsd.ddpfPixelFormat);
		ddsd.dwFlags |= DDSD_PIXELFORMAT;
	}
	if ((hr = pDD->CreateSurface (&ddsd, &surf, NULL)) != DD_OK) {
		LOGOUT_DDERR (hr);
		return NULL;
	}
    
	return (SURFHANDLE)surf;
    */
}

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
SURFHANDLE OGLClient::clbkCreateTexture (int w, int h)
{
	return mTextureManager->GetTextureForRendering(w, h);
/*
	LPDIRECTDRAWSURFACE7 surf;
	DDSURFACEDESC2 ddsd;
    ZeroMemory (&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS ;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = 0;
	//ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0;
	GetFramework()->GetBackBuffer()->GetPixelFormat (&ddsd.ddpfPixelFormat);
	// take pixel format from render surface (should make it compatible)
	ddsd.ddpfPixelFormat.dwFlags &= ~DDPF_ALPHAPIXELS; // turn off alpha data
	pDD->CreateSurface (&ddsd, &surf, NULL);
	return surf;
    */
}

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
bool OGLClient::clbkReleaseSurface (SURFHANDLE surf)
{
	((OGLTexture *)surf)->Release();
    return true;
}

/**
 * \brief Release a drawing object.
 * \param sp pointer to drawing object
 * \default None.
 * \sa Sketchpad, clbkGetSketchpad
 */
void OGLClient::clbkReleaseSketchpad (oapi::Sketchpad *sp)
{
	delete (OGLPad *)sp;
}

/**
 * \brief De-allocate a font resource.
 * \param font pointer to font resource
 * \default None.
 * \sa clbkCreateFont, oapi::Font
 */
void OGLClient::clbkReleaseFont (Font *font) const
{
	delete (OGLFont *)font;
}

/**
 * \brief Simulation startup finalisation
 *
 * Called at the beginning of a simulation session after the scenarion has
 * been parsed and the logical object have been created.
 * \default None
 */
void OGLClient::clbkPostCreation ()
{
//	ExitOutputLoadStatus ();
	if (mScene) mScene->Initialise();

}

/**
 * \brief Set transparency colour key for a surface.
 * \param surf surface handle
 * \param ckey transparency colour key value
 * \default None, returns false.
 * \note Derived classes should overload this method if the renderer
 *   supports colour key transparency for surfaces.
 */
bool OGLClient::clbkSetSurfaceColourKey (SURFHANDLE surf, uint32_t ckey)
{
//	printf("clbkSetSurfaceColourKey %p %u\n", surf, ckey);
	((OGLTexture *)surf)->m_colorkey = ckey;
	return true;
}

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
oapi::Sketchpad *OGLClient::clbkGetSketchpad (SURFHANDLE surf)
{
	return new OGLPad(surf);
}

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
void OGLClient::clbkCloseSession (bool fastclose)
{
	GraphicsClient::clbkCloseSession (fastclose);
/*
	if (scene) {
		delete scene;
		scene = NULL;
	}
	GlobalExit();
    */
}

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
bool OGLClient::clbkDisplayFrame ()
{
	CheckError("before clbkDisplayFrame");
	glfwSwapBuffers(hRenderWnd);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	CheckError("glClearColor");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CheckError("glClear");

	return true;
}
void OGLClient::clbkRenderGUI ()
{
	CheckError("clbkRenderGUI");
	ImGui_ImplOpenGL3_NewFrame();
	CheckError("ImGui_ImplOpenGL3_NewFrame");
	ImGui_ImplGlfw_NewFrame();
	CheckError("ImGui_ImplGlfw_NewFrame");
	ImGui::NewFrame();
	CheckError("ImGui::NewFrame");

//	for(auto &ctrl: g_pOrbiter->m_pGUIManager->m_GUICtrls) {
//		if(ctrl->show) {
//			ctrl->Show();
//		}
//	}
//	CheckError("after ctrl->Show");

	oapiDrawDialogs();
	CheckError("after oapiDrawDialogs");

	ImGui::Render();
	CheckError("ImGui::Render");
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	CheckError("ImGui_ImplOpenGL3_RenderDrawData");

	ImGuiIO &io = ImGui::GetIO();
	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		CheckError("ImGui::UpdatePlatformWindows");
		ImGui::RenderPlatformWindowsDefault();
		CheckError("ImGui::RenderPlatformWindowsDefault");
		glfwMakeContextCurrent(backup_current_context);
	}
}

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
bool OGLClient::clbkSplashLoadMsg (const char *msg, int line)
{
    return false;
}

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
void OGLClient::clbkStoreMeshPersistent (MESHHANDLE hMesh, const char *fname)
{
	mMeshManager->StoreMesh (hMesh);
}

/**
 * \brief Create a brush resource for 2-D drawing.
 * \param col line colour (format: 0xBBGGRR)
 * \return Pointer to brush resource
 * \default None, returns NULL.
 * \sa clbkReleaseBrush, oapi::Brush
 */
Brush *OGLClient::clbkCreateBrush (uint32_t col) const
{
	return new OGLBrush (col);
}

/**
 * \brief De-allocate a brush resource.
 * \param brush pointer to brush resource
 * \default None.
 * \sa clbkCreateBrush, oapi::Brush
 */
void OGLClient::clbkReleaseBrush (Brush *brush) const
{
	delete (OGLBrush *)brush;
}

/** \brief Launchpad video tab indicator
 *
 * Indicate if the the default video tab in the Orbiter launchpad dialog
 * is to be used for obtaining user video preferences. If a derived
 * class returns false here, the video tab is not shown.
 * \return true if the module wants to use the video tab in the launchpad
 *   dialog, false otherwise.
 * \default Return true.
 */
bool OGLClient::clbkUseLaunchpadVideoTab () const
{
    return false;
}

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
bool OGLClient::clbkFillSurface (SURFHANDLE surf, uint32_t col) const
{
	GLint result;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
	assert(result == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, ((OGLTexture *)surf)->m_FBO);
	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

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
bool OGLClient::clbkFillSurface (SURFHANDLE surf, int tgtx, int tgty, int w, int h, uint32_t col) const
{
	glm::mat4 ortho_proj = glm::ortho(0.0f, (float)g_client->GetScene()->GetCamera()->GetWidth(), (float)g_client->GetScene()->GetCamera()->GetHeight(), 0.0f);
	static Shader s("Untextured.vs","Untextured.fs");

	static GLuint m_Buffer, m_VAO;
	static bool init = false;
	if(!init) {
		init=true;
		glGenVertexArrays(1, &m_VAO);
		CheckError("clbkRender2DPanel glGenVertexArrays");
		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glGenBuffers(1, &m_Buffer);
		CheckError("clbkRender2DPanel glGenBuffers");
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
		CheckError("clbkRender2DPanel glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("clbkRender2DPanel glVertexAttribPointer");
		glEnableVertexAttribArray(0);
		CheckError("glEnableVertexAttribArray");

		glBindVertexArray(0);
	}

	const GLfloat vertex[] = {
		0, 0, (float)tgtx+w, (float)tgty+h,
		0, 0, (float)tgtx, (float)tgty,
		0, 0, (float)tgtx+w, (float)tgty,

		0, 0, (float)tgtx, (float)tgty+h,
		0, 0, (float)tgtx, (float)tgty,
		0, 0, (float)tgtx+w, (float)tgty+h,
	};

	GLint oldFB = 0;
	GLint oldViewport[4];
	OGLTexture *m_tex = (OGLTexture *)surf;
	if(m_tex) {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);
		glGetIntegerv(GL_VIEWPORT, oldViewport);
		ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, 0.0f, (float)m_tex->m_Height);
		m_tex->SetAsTarget(false);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Text glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Text glBufferSubData");
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Text glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	s.Bind();
	s.SetMat4("projection", ortho_proj);
	glm::vec3 color;
	color.r = (float)((col>>16)&0xff)/255.0;
	color.g = (float)((col>>8)&0xff)/255.0;
	color.b = (float)((col>>0)&0xff)/255.0;
	s.SetVec3("quad_color", color);

	glBindVertexArray(m_VAO);
	CheckError("clbkRender2DPanel glBindVertexArray");

	glDrawArrays(GL_TRIANGLES, 0, 6);
	CheckError("clbkRender2DPanel glDrawArrays");

	glBindVertexArray(0);

	s.UnBind();

	if(surf) {
		glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
		glViewport(oldViewport[0],oldViewport[1],oldViewport[2],oldViewport[3]);
	}

    return true;
}

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
bool OGLClient::clbkScaleBlt (SURFHANDLE tgt, int tgtx, int tgty, int tgtw, int tgth,
		                       SURFHANDLE src, int srcx, int srcy, int srcw, int srch, int flag) const
{
    printf("OGLClient::clbkScaleBlt tgtx=%d tgty=%d tgtw=%d tgth=%d\n", tgtx,tgty,tgtw,tgth);
    printf("OGLClient::clbkScaleBlt srcx=%d srcy=%d srcw=%d srch=%d\n", srcx,srcy,srcw,srch);
	glDisable(GL_BLEND);
	GLint oldFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);
	GLint oldViewport[4];
	glGetIntegerv(GL_VIEWPORT, oldViewport);

	OGLTexture *m_tex = (OGLTexture *)tgt;
	//ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, (float)m_tex->m_Height, 0.0f); //y-flipped
	glm::mat4 ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, 0.0f, (float)m_tex->m_Height);
	m_tex->SetAsTarget(false);
	static Shader s("Overlay.vs","Overlay.fs");

	static GLuint m_Buffer, m_VAO;
	static bool init = false;
	if(!init) {
		init=true;
		glGenVertexArrays(1, &m_VAO);
		CheckError("clbkRender2DPanel glGenVertexArrays");
		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glGenBuffers(1, &m_Buffer);
		CheckError("clbkRender2DPanel glGenBuffers");
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
		CheckError("clbkRender2DPanel glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("clbkRender2DPanel glVertexAttribPointer");
		glEnableVertexAttribArray(0);
		CheckError("glEnableVertexAttribArray");

		glBindVertexArray(0);
	}

	float tgt_w = tgtw;
	float tgt_h = tgth;

	float src_w = srcw;
	float src_h = srch;

	float s0 = 0;
	float s1 = 1;
	float t0 = 0;
	float t1 = 1;

	const GLfloat vertex[] = {
		s1, t1, (float)tgtx+tgt_w, (float)tgty+tgt_h,
		s0, t0, (float)tgtx, (float)tgty,
		s1, t0, (float)tgtx+tgt_w, (float)tgty,

		s0, t1, (float)tgtx, (float)tgty+tgt_h,
		s0, t0, (float)tgtx, (float)tgty,
		s1, t1, (float)tgtx+tgt_w, (float)tgty+tgt_h,
	};
	glDisable(GL_CULL_FACE);
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Text glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Text glBufferSubData");
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Text glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//	CheckError("OGLPad::Text glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Text glEnableVertexAttribArray0");
	glBindVertexArray(0);

	s.Bind();
	s.SetMat4("projection", ortho_proj);

	OGLTexture *key_tex = nullptr;
	if(flag & BLT_SRCCOLORKEY) {
		key_tex = (OGLTexture *)src;
	} else if(flag & BLT_TGTCOLORKEY) {
		key_tex = (OGLTexture *)tgt;
	} else {
		s.SetFloat("color_keyed", 0.0);
	}
	key_tex = (OGLTexture *)src;

	if(key_tex) {
		uint32_t ck = key_tex->m_colorkey;
		glm::vec4 ckv;
		ckv.r = ((ck>>16)&0xff)/255.0;
		ckv.g = ((ck>>8)&0xff)/255.0;
		ckv.b = ((ck>>0)&0xff)/255.0;
		ckv.a = ((ck>>24)&0xff)/255.0;
	//			printf("ck = %f %f %f %f %p\n", ckv.r, ckv.g, ckv.b, ckv.a, src);
		s.SetVec4("color_key", ckv);
		s.SetFloat("color_keyed", 1.0);
	}

	/*
	GLint whichID;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &whichID);
	assert(whichID==0);
	*/
	glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId);
	CheckError("glBindTexture");

	glBindVertexArray(m_VAO);
	CheckError("clbkRender2DPanel glBindVertexArray");

	glDrawArrays(GL_TRIANGLES, 0, 6);
	CheckError("clbkRender2DPanel glDrawArrays");

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	s.UnBind();

	glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
	glViewport(oldViewport[0],oldViewport[1],oldViewport[2],oldViewport[3]);

	/*
	static int done = 0;
	//static unsigned int VAO;
	static GLuint fbo;
	if(!done) {
		done = 1;

		glGenFramebuffers(1, &fbo);
	}
	GLint result;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
	assert(result == 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glBlitFramebuffer(tgtx, tgty, tgtx + ((OGLTexture *)src)->m_Width, tgty + ((OGLTexture *)src)->m_Height, 0, 0, ((OGLTexture *)src)->m_Width, ((OGLTexture *)src)->m_Height, 
					GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId);
	CheckError("OGLClient::clbkBlt glBindTexture");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CheckError("OGLClient::clbkBlt glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	CheckError("OGLClient::clbkBlt glTexParameteri2");
	glGenerateMipmap(GL_TEXTURE_2D);
	CheckError("OGLClient::clbkBlt glGenerateMipmap");
	glBindTexture(GL_TEXTURE_2D, 0);
*/
	glEnable(GL_BLEND);

	return true;}

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
bool OGLClient::clbkBlt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int flag) const
{
	glDisable(GL_BLEND);
	GLint oldFB;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);
	GLint oldViewport[4];
	glGetIntegerv(GL_VIEWPORT, oldViewport);

	OGLTexture *m_tex = (OGLTexture *)tgt;
	//ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, (float)m_tex->m_Height, 0.0f); //y-flipped
	glm::mat4 ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, 0.0f, (float)m_tex->m_Height);
	m_tex->SetAsTarget();
	/*
	glBindTexture(GL_TEXTURE_2D, m_tex->m_TexId);
	CheckError("OGLPad glBindTexture");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CheckError("OGLPad glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	CheckError("OGLPad glTexParameteri2");
	glGenerateMipmap(GL_TEXTURE_2D);
	CheckError("OGLPad glGenerateMipmap");*/
	static Shader s("Overlay.vs","Overlay.fs");

	static GLuint m_Buffer, m_VAO;
	static bool init = false;
	if(!init) {
		init=true;
		glGenVertexArrays(1, &m_VAO);
		CheckError("clbkRender2DPanel glGenVertexArrays");
		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glGenBuffers(1, &m_Buffer);
		CheckError("clbkRender2DPanel glGenBuffers");
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		CheckError("clbkRender2DPanel glBindBuffer");
		glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
		CheckError("clbkRender2DPanel glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("clbkRender2DPanel glVertexAttribPointer");
		glEnableVertexAttribArray(0);
		CheckError("glEnableVertexAttribArray");

		glBindVertexArray(0);
	}

	float tgt_w = m_tex->m_Width;
	float tgt_h = m_tex->m_Height;

	float src_w = ((OGLTexture *)src)->m_Width;
	float src_h = ((OGLTexture *)src)->m_Height;

	float s0 = 0;
	float s1 = 1;
	float t0 = 0;
	float t1 = 1;

	const GLfloat vertex[] = {
		s1, t1, (float)tgtx+src_w, (float)tgty+src_h,
		s0, t0, (float)tgtx, (float)tgty,
		s1, t0, (float)tgtx+src_w, (float)tgty,

		s0, t1, (float)tgtx, (float)tgty+src_h,
		s0, t0, (float)tgtx, (float)tgty,
		s1, t1, (float)tgtx+src_w, (float)tgty+src_h,
	};
	glDisable(GL_CULL_FACE);
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Text glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Text glBufferSubData");
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Text glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//	CheckError("OGLPad::Text glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Text glEnableVertexAttribArray0");
	glBindVertexArray(0);

	s.Bind();
	s.SetMat4("projection", ortho_proj);

	OGLTexture *key_tex = nullptr;
	if(flag & BLT_SRCCOLORKEY) {
		key_tex = (OGLTexture *)src;
	} else if(flag & BLT_TGTCOLORKEY) {
		key_tex = (OGLTexture *)tgt;
	} else {
		s.SetFloat("color_keyed", 0.0);
	}
	key_tex = (OGLTexture *)src;

	if(key_tex) {
		uint32_t ck = key_tex->m_colorkey;
		glm::vec4 ckv;
		ckv.r = ((ck>>16)&0xff)/255.0;
		ckv.g = ((ck>>8)&0xff)/255.0;
		ckv.b = ((ck>>0)&0xff)/255.0;
		ckv.a = ((ck>>24)&0xff)/255.0;
	//			printf("ck = %f %f %f %f %p\n", ckv.r, ckv.g, ckv.b, ckv.a, src);
		s.SetVec4("color_key", ckv);
		s.SetFloat("color_keyed", 1.0);
	}

	/*
	GLint whichID;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &whichID);
	assert(whichID==0);
	*/
	glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId);
	CheckError("glBindTexture");

	glBindVertexArray(m_VAO);
	CheckError("clbkRender2DPanel glBindVertexArray");

	glDrawArrays(GL_TRIANGLES, 0, 6);
	CheckError("clbkRender2DPanel glDrawArrays");

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	s.UnBind();

	glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
	glViewport(oldViewport[0],oldViewport[1],oldViewport[2],oldViewport[3]);


	/*
	static int done = 0;
	//static unsigned int VAO;
	static GLuint fbo;
	if(!done) {
		done = 1;

		glGenFramebuffers(1, &fbo);
	}
	GLint result;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
	assert(result == 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glBlitFramebuffer(tgtx, tgty, tgtx + ((OGLTexture *)src)->m_Width, tgty + ((OGLTexture *)src)->m_Height, 0, 0, ((OGLTexture *)src)->m_Width, ((OGLTexture *)src)->m_Height, 
					GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId);
	CheckError("OGLClient::clbkBlt glBindTexture");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CheckError("OGLClient::clbkBlt glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	CheckError("OGLClient::clbkBlt glTexParameteri2");
	glGenerateMipmap(GL_TEXTURE_2D);
	CheckError("OGLClient::clbkBlt glGenerateMipmap");
	glBindTexture(GL_TEXTURE_2D, 0);
*/
	glEnable(GL_BLEND);

	return true;
}

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
bool OGLClient::clbkBlt (SURFHANDLE tgt, int tgtx, int tgty, SURFHANDLE src, int srcx, int srcy, int w, int h, int flag) const
{
	glDisable(GL_BLEND);
	static int done = 0;
	//static unsigned int VAO;
	static GLuint fbo;
	if(!done) {
		done = 1;
		glGenFramebuffers(1, &fbo);
	}

	if(tgt) {
		GLint oldFB;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFB);
	GLint oldViewport[4];
	glGetIntegerv(GL_VIEWPORT, oldViewport);
OGLTexture *m_tex = (OGLTexture *)tgt;
		//ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, (float)m_tex->m_Height, 0.0f); //y-flipped
		glm::mat4 ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, 0.0f, (float)m_tex->m_Height);
		m_tex->SetAsTarget();
/*
		glBindTexture(GL_TEXTURE_2D, m_tex->m_TexId);
		CheckError("OGLPad glBindTexture");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		CheckError("OGLPad glTexParameteri");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		CheckError("OGLPad glTexParameteri2");
		glGenerateMipmap(GL_TEXTURE_2D);
		CheckError("OGLPad glGenerateMipmap");*/
		static Shader s("Overlay.vs","Overlay.fs");

		static GLuint m_Buffer, m_VAO;
		static bool init = false;
		if(!init) {
			init=true;
			glGenVertexArrays(1, &m_VAO);
			CheckError("clbkRender2DPanel glGenVertexArrays");
			glBindVertexArray(m_VAO);
			CheckError("clbkRender2DPanel glBindVertexArray");

			glGenBuffers(1, &m_Buffer);
			CheckError("clbkRender2DPanel glGenBuffers");
			glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
			CheckError("clbkRender2DPanel glBindBuffer");
			glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
			CheckError("clbkRender2DPanel glBufferSubData");
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			CheckError("clbkRender2DPanel glVertexAttribPointer");
			glEnableVertexAttribArray(0);
			CheckError("glEnableVertexAttribArray");

			glBindVertexArray(0);
		}

		float tgt_w = m_tex->m_Width;
		float tgt_h = m_tex->m_Height;

		float src_w = ((OGLTexture *)src)->m_Width;
		float src_h = ((OGLTexture *)src)->m_Height;

 		float s0 = (float)srcx / src_w;
 		float s1 = (float)(srcx+(float)w) / src_w;
 		float t0 = (float)srcy / src_h;
 		float t1 = (float)(srcy+(float)h) / src_h;

		const GLfloat vertex[] = {
			s1, t1, (float)tgtx+w, (float)tgty+h,
			s0, t0, (float)tgtx, (float)tgty,
			s1, t0, (float)tgtx+w, (float)tgty,

			s0, t1, (float)tgtx, (float)tgty+h,
			s0, t0, (float)tgtx, (float)tgty,
			s1, t1, (float)tgtx+w, (float)tgty+h,
		};
glDisable(GL_CULL_FACE);
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		glBindVertexArray(m_VAO);
		CheckError("OGLPad::Text glBindBuffer");
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
		CheckError("OGLPad::Text glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("OGLPad::Text glVertexAttribPointer");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
//	CheckError("OGLPad::Text glBindBuffer0");
		glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Text glEnableVertexAttribArray0");
		glBindVertexArray(0);

		s.Bind();
		s.SetMat4("projection", ortho_proj);

		OGLTexture *key_tex = nullptr;
		if(flag & BLT_SRCCOLORKEY) {
			key_tex = (OGLTexture *)src;
		} else if(flag & BLT_TGTCOLORKEY) {
			key_tex = (OGLTexture *)tgt;
		} else {
			s.SetFloat("color_keyed", 0.0);
		}
key_tex = (OGLTexture *)src;

		if(key_tex) {
			uint32_t ck = key_tex->m_colorkey;
			glm::vec4 ckv;
			ckv.r = ((ck>>16)&0xff)/255.0;
			ckv.g = ((ck>>8)&0xff)/255.0;
			ckv.b = ((ck>>0)&0xff)/255.0;
			ckv.a = ((ck>>24)&0xff)/255.0;
//			printf("ck = %f %f %f %f %p\n", ckv.r, ckv.g, ckv.b, ckv.a, src);
			s.SetVec4("color_key", ckv);
			s.SetFloat("color_keyed", 1.0);
		}

/*
		GLint whichID;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &whichID);
		assert(whichID==0);
*/
		glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId);
		CheckError("glBindTexture");

		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glDrawArrays(GL_TRIANGLES, 0, 6);
		CheckError("clbkRender2DPanel glDrawArrays");

		glBindVertexArray(0);
glBindTexture(GL_TEXTURE_2D, 0);
		s.UnBind();

glBindFramebuffer(GL_FRAMEBUFFER, oldFB);
	glViewport(oldViewport[0],oldViewport[1],oldViewport[2],oldViewport[3]);
		/*
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		CheckError("OGLClient::clbkBlt glBindFramebuffer(GL_FRAMEBUFFER, fbo)");
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId, 0);
		CheckError("OGLClient::clbkBlt glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0");
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId, 0);
		CheckError("OGLClient::clbkBlt glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1");
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		CheckError("OGLClient::clbkBlt glDrawBuffer(GL_COLOR_ATTACHMENT1);");
		glBlitFramebuffer(srcx, srcy, srcx + w, srcy + h, tgtx, tgty, tgtx + w, tgty + h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		CheckError("OGLClient::clbkBlt glBlitFramebuffer(srcx, srcy, srcx + w, srcy + h, tgtx, tgty, tgtx + w, tgty + h, GL_COLOR_BUFFER_BIT, GL_NEAREST)");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CheckError("OGLClient::clbkBlt glBindFramebuffer(GL_FRAMEBUFFER, 0)");

		glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)tgt)->m_TexId);
		CheckError("OGLClient::clbkBlt glBindTexture");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		CheckError("OGLClient::clbkBlt glTexParameteri");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		CheckError("OGLClient::clbkBlt glTexParameteri2");
		glGenerateMipmap(GL_TEXTURE_2D);
		CheckError("OGLClient::clbkBlt glGenerateMipmap");
		glBindTexture(GL_TEXTURE_2D, 0);
		*/
	} else {
		glm::mat4 ortho_proj = glm::ortho(0.0f, (float)g_client->GetScene()->GetCamera()->GetWidth(), (float)g_client->GetScene()->GetCamera()->GetHeight(), 0.0f);
		static Shader s("Overlay.vs","Overlay.fs");

		static GLuint m_Buffer, m_VAO;
		static bool init = false;
		if(!init) {
			init=true;
			glGenVertexArrays(1, &m_VAO);
			CheckError("clbkRender2DPanel glGenVertexArrays");
			glBindVertexArray(m_VAO);
			CheckError("clbkRender2DPanel glBindVertexArray");

			glGenBuffers(1, &m_Buffer);
			CheckError("clbkRender2DPanel glGenBuffers");
			glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
			CheckError("clbkRender2DPanel glBindBuffer");
			glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
			CheckError("clbkRender2DPanel glBufferSubData");
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			CheckError("clbkRender2DPanel glVertexAttribPointer");
			glEnableVertexAttribArray(0);
			CheckError("glEnableVertexAttribArray");

			glBindVertexArray(0);
		}

		float tgt_w = (float)g_client->GetScene()->GetCamera()->GetWidth();
		float tgt_h = (float)g_client->GetScene()->GetCamera()->GetHeight();

		float src_w = ((OGLTexture *)src)->m_Width;
		float src_h = ((OGLTexture *)src)->m_Height;

 		float s0 = (float)srcx / src_w;
 		float s1 = (float)(srcx+(float)w) / src_w;
 		float t0 = (float)srcy / src_h;
 		float t1 = (float)(srcy+(float)h) / src_h;

		const GLfloat vertex[] = {
			s1, t1, (float)tgtx+w, (float)tgty+h,
			s0, t0, (float)tgtx, (float)tgty,
			s1, t0, (float)tgtx+w, (float)tgty,

			s0, t1, (float)tgtx, (float)tgty+h,
			s0, t0, (float)tgtx, (float)tgty,
			s1, t1, (float)tgtx+w, (float)tgty+h,
		};
glDisable(GL_CULL_FACE);
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
		glBindVertexArray(m_VAO);
		CheckError("OGLPad::Text glBindBuffer");
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
		CheckError("OGLPad::Text glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("OGLPad::Text glVertexAttribPointer");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
//	CheckError("OGLPad::Text glBindBuffer0");
		glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Text glEnableVertexAttribArray0");
		glBindVertexArray(0);

		s.Bind();
		s.SetMat4("projection", ortho_proj);

		OGLTexture *key_tex = nullptr;
		if(flag & BLT_SRCCOLORKEY) {
			key_tex = (OGLTexture *)src;
		} else if(flag & BLT_TGTCOLORKEY) {
			key_tex = (OGLTexture *)tgt;
		} else {
			s.SetFloat("color_keyed", 0.0);
		}
key_tex = (OGLTexture *)src;

		if(key_tex) {
			uint32_t ck = key_tex->m_colorkey;
			glm::vec4 ckv;
			ckv.r = ((ck>>16)&0xff)/255.0;
			ckv.g = ((ck>>8)&0xff)/255.0;
			ckv.b = ((ck>>0)&0xff)/255.0;
			ckv.a = ((ck>>24)&0xff)/255.0;
//			printf("ck = %f %f %f %f %p\n", ckv.r, ckv.g, ckv.b, ckv.a, src);
			s.SetVec4("color_key", ckv);
			s.SetFloat("color_keyed", 1.0);
		}


		GLint whichID;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &whichID);
		assert(whichID==0);

		glBindTexture(GL_TEXTURE_2D, ((OGLTexture *)src)->m_TexId);
		CheckError("glBindTexture");

		glBindVertexArray(m_VAO);
		CheckError("clbkRender2DPanel glBindVertexArray");

		glDrawArrays(GL_TRIANGLES, 0, 6);
		CheckError("clbkRender2DPanel glDrawArrays");

		glBindVertexArray(0);
glBindTexture(GL_TEXTURE_2D, 0);
		s.UnBind();


	}
	glEnable(GL_BLEND);

	return true;
}
#define MAX_POLYS 4096
/////////////////// SKETCHPAD STUFF ////////////////////////////
OGLPad::OGLPad (SURFHANDLE s):oapi::Sketchpad(s)
{
	cfont = nullptr;
	cpen = nullptr;
	cbrush = nullptr;
	m_xOrigin = 0;
	m_yOrigin = 0;
	/**
	 * \brief Vertical text alignment modes.
	 * \sa SetTextAlign
	 */
	enum TAlign_vertical {
		TOP,         ///< align top of text line
		BASELINE,    ///< align base line of text line
		BOTTOM       ///< align bottom of text line
	};

	m_TextAlignH = oapi::Sketchpad::TAlign_horizontal::LEFT;
	m_TextAlignV = oapi::Sketchpad::TAlign_vertical::BOTTOM;

	m_tex = (OGLTexture *)s;

	if(s)
	{
		GLint result;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &result);
		assert(result == 0);
		//ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, (float)m_tex->m_Height, 0.0f); //y-flipped
		ortho_proj = glm::ortho(0.0f, (float)m_tex->m_Width, 0.0f, (float)m_tex->m_Height);
		m_tex->SetAsTarget();
		height = m_tex->m_Height;

		glBindTexture(GL_TEXTURE_2D, m_tex->m_TexId);
		CheckError("OGLPad glBindTexture");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		CheckError("OGLPad glTexParameteri");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		CheckError("OGLPad glTexParameteri2");
		glGenerateMipmap(GL_TEXTURE_2D);
		CheckError("OGLPad glGenerateMipmap");
	}
	else
	{
		ortho_proj = glm::ortho(0.0f, (float)g_client->GetScene()->GetCamera()->GetWidth(), (float)g_client->GetScene()->GetCamera()->GetHeight(), 0.0f);
		height = g_client->GetScene()->GetCamera()->GetHeight();
		glDisable(GL_CULL_FACE);
	}
	glGenVertexArrays(1, &m_VAO);
	CheckError("OGLPad glGenVertexArrays");
	glBindVertexArray(m_VAO);
	CheckError("OGLPad glBindVertexArray");

	glGenBuffers(1, &m_Buffer);
	CheckError("OGLPad glGenBuffers");
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	CheckError("OGLPad glBindBuffer");
	glBufferData(GL_ARRAY_BUFFER, MAX_POLYS * 3 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	CheckError("OGLPad glBufferData");

	glBindVertexArray(0);
	CheckError("OGLPad glBindVertexArray0");
}
OGLPad::~OGLPad ()
{
	if(m_tex) {
		glBindTexture(GL_TEXTURE_2D, m_tex->m_TexId);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		glEnable(GL_CULL_FACE);
	}

	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_Buffer);
	CheckError("OGLPad glDeleteBuffer");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * \brief Selects a new font to use.
 * \param font pointer to font resource
 * \return Previously selected font.
 * \sa OGLFont, OGLClient::clbkCreateFont
 */
oapi::Font *OGLPad::SetFont (oapi::Font *font) const
{
	oapi::Font *ret = cfont;
	cfont = font;
	return ret;
}

/**
 * \brief Selects a new pen to use.
 * \param pen pointer to pen resource, or NULL to disable outlines
 * \return Previously selected pen.
 * \sa OGLPen, OGLClient::clbkCreatePen
 */
oapi::Pen *OGLPad::SetPen (oapi::Pen *pen) const
{
	oapi::Pen *ret = cpen;
	cpen = pen;
	return ret;
}

/**
 * \brief Selects a new brush to use.
 * \param brush pointer to brush resource, or NULL to disable fill mode
 * \return Previously selected brush.
 * \sa OGLBrush, OGLClient::clbkCreateBrush
 */
oapi::Brush *OGLPad::SetBrush (oapi::Brush *brush) const
{
	oapi::Brush *ret = cbrush;
	cbrush = brush;
	return ret;
}

/**
 * \brief Set horizontal and vertical text alignment.
 * \param tah horizontal alignment
 * \param tav vertical alignment
 */
void OGLPad::SetTextAlign (TAlign_horizontal tah, TAlign_vertical tav)
{
	m_TextAlignH = tah;
	m_TextAlignV = tav;
}

/**
 * \brief Set the foreground colour for text output.
 * \param col colour description (format: 0xBBGGRR)
 * \return Previous colour setting.
 */
uint32_t OGLPad::SetTextColor (uint32_t col)
{
	uint32_t oldColor = textColor;
	textColor = col;
	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	vecTextColor = glm::vec3(r, g, b);
	return oldColor;
}

/**
 * \brief Set the background colour for text output.
 * \param col background colour description (format: 0xBBGGRR)
 * \return Previous colour setting
 * \note The background colour is only used if the background mode
 *   is set to BK_OPAQUE.
 * \sa SetBackgroundMode
 */
uint32_t OGLPad::SetBackgroundColor (uint32_t col)
{
	return 0;
}

/**
 * \brief Set the background mode for text and drawing operations.
 * \param mode background mode (see \ref BkgMode)
 * \note This function affects text output and dashed line drawing.
 * \note In opaque background mode, text background and the gaps
 *   between dashed lines are drawn in the current background colour
 *   (see SetBackgroundColor). In transparent mode, text background
 *   and line gaps are not modified.
 * \note The default background mode (before the first call of
 *   SetBackgroundMode) is transparent.
 * \sa SetBackgroundColor, Text, OGLClient::clbkCreatePen
 */
void OGLPad::SetBackgroundMode (BkgMode mode)
{
}

/** brief Returns height and (average) width of a character in the currently
 *   selected font.
 * \return Height of character cell [pixel] in the lower 16 bit of the return value,
 *   and (average) width of character cell [pixel] in the upper 16 bit.
 * \note The height value is given by tmHeight-tmInternalLeading from the
 *   TEXTMETRIC structure returned by the OGL GetTextMetrics function.
 * \note The width value is given by tmAveCharWidth from the
 *   TEXTMETRIC structure returned by the OGL GetTextMetrics function.
 */
int OGLPad::GetCharSize ()
{
	OGLFont *f = (OGLFont *)cfont;
	return (f->m_Height-1 + (f->m_Height*11/16<<16));// FIXME: empirical values, should be computed from ttf file...
//	return 11 + (7<<16); 
}

/**
 * \brief Returns the width of a text string in the currently selected font.
 * \param str text string
 * \param len string length, or 0 for auto (0-terminated string)
 * \return width of the string, drawn in the currently selected font [pixel]
 * \sa SetFont
 */
int OGLPad::GetTextWidth (const char *str, int len)
{
	OGLFont *font = (OGLFont *)cfont;

	float xpos = 0;
	float ypos = 0;
	for(int i=0;i<len;i++) {
		unsigned int c = (unsigned char)str[i];
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(font->m_CharData, 512, 512, c, &xpos, &ypos, &q, 1);
	}

	return xpos;
}

bool OGLPad::GetTextWidthAndHeight (const char *str, int len, int *width, int *top, int *bottom)
{
	OGLFont *font = (OGLFont *)cfont;

	float xpos = 0.0f;
	float ypos = 0.0f;
	float ymax = 0.0f;
	float ymin = 0.0f;
	/*
   float x0,y0,s0,t0; // top-left
   float x1,y1,s1,t1; // bottom-right
   */
	for(int i=0;i<len;i++) {
		unsigned int c = (unsigned char)str[i];
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(font->m_CharData, 512, 512, c, &xpos, &ypos, &q, 1);
		if(q.y1 > ymax)
			ymax = q.y1;
		if(q.y0 < ymin)
			ymin = q.y0;
	}

	*width  = (int)xpos;
	*top    = (int)ymax;
	*bottom = (int)ymin;
	return true;
}


/**
 * \brief Set the position in the surface bitmap which is mapped to the
 *   origin of the coordinate system for all drawing functions.
 * \param x horizontal position of the origin [pixel]
 * \param y vertical position of the origin [pixel]
 * \note By default, the reference point for drawing function coordinates is
 *   the top left corner of the bitmap, with positive x-axis to the right,
 *   and positive y-axis down.
 * \note SetOrigin can be used to shift the logical reference point to a
 *   different position in the surface bitmap (but not to change the
 *   orientation of the axes).
 */
void OGLPad::SetOrigin (int x, int y)
{
	m_xOrigin = x;
	m_yOrigin = y;
}

/**
 * \brief Returns the position in the surface bitmap which is mapped to
 *   the origin of the coordinate system for all drawing functions.
 * \param [out] x pointer to integer receiving horizontal position of the origin [pixel]
 * \param [out] y pointer to integer receiving vertical position of the origin [pixel]
 * \default Returns (0,0)
 * \sa SetOrigin
 */
void OGLPad::GetOrigin (int *x, int *y) const
{
	*x=m_xOrigin;
	*y=m_yOrigin;
}

/**
 * \brief Draws a text string.
 * \param x reference x position [pixel]
 * \param y reference y position [pixel]
 * \param str text string
 * \param len string length for output
 * \return \e true on success, \e false on failure.
 */



/*
	enum TAlign_horizontal {
		LEFT,        ///< align left
		CENTER,      ///< align center
		RIGHT        ///< align right
	};

	enum TAlign_vertical {
		TOP,         ///< align top of text line
		BASELINE,    ///< align base line of text line
		BOTTOM       ///< align bottom of text line
	};

	m_TextAlignH = oapi::Sketchpad::TAlign_horizontal::LEFT;
	m_TextAlignV = oapi::Sketchpad::TAlign_vertical::BOTTOM;
*/


bool OGLPad::Text (int x, int y, const char *str, int len)
{
	if(len<0)
		abort();
	//y +=16; //FIXME

	int xoffset = 0;
	int yoffset = 0;

	int width;
	int top;
	int bottom;
	GetTextWidthAndHeight (str, len, &width, &top, &bottom);
//if(strcmp(str,"DG-01"))
//exit(-1);
//y-=(bottom-top);
	y-=bottom;
	switch(m_TextAlignH) {
		case oapi::Sketchpad::TAlign_horizontal::LEFT:
			break;
		case oapi::Sketchpad::TAlign_horizontal::CENTER:
			xoffset = -(width / 2);
			break;
		case oapi::Sketchpad::TAlign_horizontal::RIGHT:
			xoffset = -width;
			break;
	}

	switch(m_TextAlignV) {
		case oapi::Sketchpad::TAlign_vertical::TOP:
			//yoffset = -bottom;
			//yoffset = top;
			break;
		case oapi::Sketchpad::TAlign_vertical::BASELINE:
			break;
		case oapi::Sketchpad::TAlign_vertical::BOTTOM:
			//yoffset = bottom;
			//yoffset = top;
			break;
	}

	x+=xoffset;
	y+=yoffset;

	x+=m_xOrigin;
	y+=m_yOrigin;

	char tmp[len + 1];
	memcpy(tmp, str, len);
	tmp[len]=0;
	OGLFont *font = (OGLFont *)cfont;
//	y +=font->m_Height+1;

	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	//CheckError("glClearColor");
	//glClear(GL_COLOR_BUFFER_BIT);
	//CheckError("glClearColor");

	static Shader s("OGLFont.vs","OGLFont.fs");
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetVec3("font_color", vecTextColor);
	glBindTexture(GL_TEXTURE_2D, font->m_Atlas);
	CheckError("OGLPad::Text glBindTexture");
	
	float xpos = x;
	float ypos = y;
	for(int i=0;i<len;i++) {
		unsigned int c = (unsigned char)str[i];
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(font->m_CharData, 512, 512, c, &xpos, &ypos, &q, 1);

		const GLfloat vertex[] = {
			q.s0, q.t0, q.x0, q.y0,
			q.s1, q.t1, q.x1, q.y1,
			q.s1, q.t0, q.x1, q.y0,

			q.s0, q.t0, q.x0, q.y0,
			q.s0, q.t1, q.x0, q.y1,
			q.s1, q.t1, q.x1, q.y1,
		};

		glBindBuffer(GL_ARRAY_BUFFER, font->m_Buffer);
/*
		void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		memcpy(map, vertex, sizeof(vertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(font->m_VAO);
*/
		glBindVertexArray(font->m_VAO);
		CheckError("OGLPad::Text glBindBuffer");
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
		CheckError("OGLPad::Text glBufferSubData");
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		CheckError("OGLPad::Text glVertexAttribPointer");
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
//	CheckError("OGLPad::Text glBindBuffer0");
		glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Text glEnableVertexAttribArray0");

		glDrawArrays(GL_TRIANGLES, 0, 6);
	
		CheckError("OGLPad::Text glDrawArrays");
		glBindVertexArray(0);
	}

//	glDisable(GL_BLEND);
	
	CheckError("OGLPad::Text glBindVertexArray0");

	glBindTexture(GL_TEXTURE_2D, 0);
	s.UnBind();
	return true;
}

bool OGLPad::TextW (int x, int y, const wchar_t *str, int len)
{
	printf("OGLPad::TextW\n");
	exit(-1);
	return true;
}


/**
 * \brief Draws a single pixel in a specified colour.
 * \param x x-coordinate of point [pixel]
 * \param y y-coordinate of point [pixel]
 * \param col pixel colour (format: 0xBBGGRR)
 */
void OGLPad::Pixel (int x, int y, uint32_t col)
{
	x+=m_xOrigin;
	y+=m_yOrigin;
	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	glm::vec4 pixelColor = glm::vec4(r, g, b, 1.0f);

	static Shader s("OGLLine.vs", "OGLLine.fs");
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetVec4("line_color", pixelColor);

	const GLfloat vertex[] = {
		(GLfloat)x, (GLfloat)y, 0.0f,
	};

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
/*
		void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		memcpy(map, vertex, sizeof(vertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(font->m_VAO);
*/
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Line glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Line glBufferSubData");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Line glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

//	CheckError("OGLPad::Line glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Line glEnableVertexAttribArray0");

	glDrawArrays(GL_POINTS, 0, 1);
	
	CheckError("OGLPad::Line glDrawArrays");
	glBindVertexArray(0);
	

//	glDisable(GL_BLEND);
	
	CheckError("OGLPad::Line glBindVertexArray0");

	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	s.UnBind();
}

/**
 * \brief Moves the drawing reference to a new point.
 * \param x x-coordinate of new reference point [pixel]
 * \param y y-coordinate of new reference point [pixel]
 * \note Some methods use the drawing reference point for
 *   drawing operations, e.g. \ref LineTo.
 * \sa LineTo
 */
void OGLPad::MoveTo (int x, int y)
{
	m_curX = x;
	m_curY = y;
}

/**
 * \brief Draws a line to a specified point.
 * \param x x-coordinate of line end point [pixel]
 * \param y y-coordinate of line end point [pixel]
 * \note The line starts at the current drawing reference
 *   point.
 * \sa MoveTo
 */
void OGLPad::LineTo (int x, int y)
{
	Line(m_curX, m_curY, x, y);
	m_curX = x;
	m_curY = y;
}

/**
 * \brief Draws a line between two points.
 * \param x0 x-coordinate of first point [pixel]
 * \param y0 y-coordinate of first point [pixel]
 * \param x1 x-coordinate of second point [pixel]
 * \param y1 y-coordinate of second point [pixel]
 * \note The line is drawn with the currently selected pen.
 * \sa SetPen
 */
void OGLPad::Line (int x0, int y0, int x1, int y1)
{
	x0+=m_xOrigin;
	y0+=m_yOrigin;
	x1+=m_xOrigin;
	y1+=m_yOrigin;

	m_curX = x1;
	m_curY = y1;

	OGLPen *p = (OGLPen *)cpen;
	uint32_t col = 0xff0000;
	if(p)
		col = p->m_Color;

	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	glm::vec4 lineColor = glm::vec4(r, g, b, 1.0f);

	float dist = 0.0f;
	if(p && p->m_Style == 2) //dashed line
		dist = sqrt((x0-x1)*(x0-x1) + (y0-y1)*(y0-y1))/10.0f;

	static Shader s("OGLLine.vs", "OGLLine.fs");
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetVec4("line_color", lineColor);

	const GLfloat vertex[] = {
		(GLfloat)x0, (GLfloat)y0, 0.0f,
		(GLfloat)x1, (GLfloat)y1, dist,
	};

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
/*
		void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		memcpy(map, vertex, sizeof(vertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(font->m_VAO);
*/
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Line glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Line glBufferSubData");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Line glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

//	CheckError("OGLPad::Line glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Line glEnableVertexAttribArray0");

	glDrawArrays(GL_LINES, 0, 2);
	
	CheckError("OGLPad::Line glDrawArrays");
	glBindVertexArray(0);
	

//	glDisable(GL_BLEND);
	
	CheckError("OGLPad::Line glBindVertexArray0");

	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	s.UnBind();
}

/**
 * \brief Draw a rectangle (filled or outline).
 * \param x0 left edge of rectangle [pixel]
 * \param y0 top edge of rectangle [pixel]
 * \param x1 right edge of rectangle [pixel]
 * \param y1 bottom edge of rectangle [pixel]
 * \note The rectangle is filled with the currently selected
 *   brush resource.
 * \sa Ellipse
 */
void OGLPad::Rectangle (int x0, int y0, int x1, int y1)
{
	MoveTo (x0, y0);
	LineTo (x1, y0);
	LineTo (x1, y1);
	LineTo (x0, y1);
	LineTo (x0, y0);
}

/**
 * \brief Draw an ellipse from its bounding box.
 * \param x0 left edge of bounding box [pixel]
 * \param y0 top edge of bounding box [pixel]
 * \param x1 right edge of bounding box [pixel]
 * \param y1 bottom edge of bounding box [pixel]
 * \note The ellipse is filled with the currently selected
 *   brush resource.
 * \sa Rectangle
 */
void OGLPad::Ellipse (int x0, int y0, int x1, int y1)
{
	x0+=m_xOrigin;
	y0+=m_yOrigin;
	x1+=m_xOrigin;
	y1+=m_yOrigin;

	OGLPen *p = (OGLPen *)cpen;
	uint32_t col = p->m_Color;
	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	float a = 1.0f;
	glm::vec4 lineColor = glm::vec4(r, g, b, a);
	OGLBrush *br = (OGLBrush *)cbrush;
	if(br) {
		col = br->m_Color;
		r = (col&0xff)/255.0;
		g = ((col>>8)&0xff)/255.0;
		b = ((col>>16)&0xff)/255.0;
		a = 1.0f;
	} else {
		r=g=b=a=0.0f;
	}
	glm::vec4 brushColor = glm::vec4(r, g, b, a);
#define NSIDE 64

	GLfloat vertex[3 + NSIDE * 3 + 3]; //x, y, dist

	float xcenter = (x1 + x0) / 2.0f;
	float ycenter = (y1 + y0) / 2.0f;
	float xsize = abs(x1 - x0) / 2.0f;
	float ysize = abs(y1 - y0) / 2.0f;

	float circ = 2.0f*3.14159265f*sqrt(xsize*xsize+ysize*ysize); //2π√a2+b22

	vertex[0] = xcenter;
	vertex[1] = ycenter;
	vertex[2] = 0;

	for(int i=1;i<NSIDE + 2;i++) {
		vertex[i * 3 + 0] = xcenter + xsize * cos(2 * 3.14159265 * (float)i / (float)NSIDE);
		vertex[i * 3 + 1] = ycenter + ysize * sin(2 * 3.14159265 * (float)i / (float)NSIDE);
		if(p->m_Style == 2) //dashed line
			vertex[i * 3 + 2] = circ * (float)i / (float)NSIDE / 10.0f;
		else
			vertex[i * 3 + 2] = 0.0f;
	}

	static Shader s("OGLLine.vs", "OGLLine.fs");
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetVec4("line_color", lineColor);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Ellipse glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), vertex);
	CheckError("OGLPad::Ellipse glBufferSubData");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Ellipse glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

//	CheckError("OGLPad::Ellipse glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Ellipse glEnableVertexAttribArray0");

	s.SetVec4("line_color", brushColor);
	glFrontFace(GL_CCW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, NSIDE+2);
	glFrontFace(GL_CW);
	CheckError("OGLPad::Ellipse GL_TRIANGLE_FAN");
	s.SetVec4("line_color", lineColor);
	glDrawArrays(GL_LINE_LOOP, 1, NSIDE);
	CheckError("OGLPad::Ellipse GL_LINE_LOOP");
	
	CheckError("OGLPad::Ellipse glDrawArrays");
	glBindVertexArray(0);

//	glDisable(GL_BLEND);
	
	CheckError("OGLPad::Ellipse glBindVertexArray0");

	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	s.UnBind();
	//exit(-1);
}

/**
 * \brief Draw a closed polygon given by vertex points.
 * \param pt list of vertex points
 * \param npt number of points in the list
 * \note The polygon is outlined with the current pen, and
 *   filled with the current brush.
 * \note The polygon is closed, i.e. the last point is
 *   joined with the first one.
 * \sa Polyline, PolyPolygon, Rectangle, Ellipse
 */
void OGLPad::Polygon (const oapi::IVECTOR2 *pt, int npt)
{
	//FIXME : fill polygon
	//if(cbrush==nullptr) {
		MoveTo(pt->x, pt->y);
		for(int i=1;i<npt;i++) {
			LineTo(pt[i].x,pt[i].y);
		}
		LineTo(pt->x, pt->y);
	//} else {
	//	printf("Filled polygon not implemented col=0x%08x npt=%d\n", ((OGLBrush *)cbrush)->m_Color, npt);
	//}
}

/**
 * \brief Draw a line of piecewise straight segments.
 * \param pt list of vertex points
 * \param npt number of points in the list
 * \note The line is drawn with the currently selected pen.
 * \note Polylines are open figures: the end points are
 *   not connected, and no fill operation is performed.
 * \sa Polygon, PolyPolyline Rectangle, Ellipse
 */
void OGLPad::Polyline (const oapi::IVECTOR2 *pt, int npt)
{
/*	
	MoveTo(pt->x, pt->y);
	for(int i=1;i<npt;i++) {
		LineTo(pt[i].x,pt[i].y);
	}
return;*/
/*
	x0+=m_xOrigin;
	y0+=m_yOrigin;
	x1+=m_xOrigin;
	y1+=m_yOrigin;
*/

//	m_curX = x1;
//	m_curY = y1;

	OGLPen *p = (OGLPen *)cpen;
	uint32_t col = 0xff0000;
	if(p)
		col = p->m_Color;

	float r = (col&0xff)/255.0;
	float g = ((col>>8)&0xff)/255.0;
	float b = ((col>>16)&0xff)/255.0;
	glm::vec4 lineColor = glm::vec4(r, g, b, 1.0f);

	//float dist = 0.0f;
	//if(p && p->m_Style == 2) //dashed line
	//	dist = sqrt((x0-x1)*(x0-x1) + (y0-y1)*(y0-y1))/10.0f;

	static Shader s("OGLLine.vs", "OGLLine.fs");
	s.Bind();
	s.SetMat4("projection", ortho_proj);
	s.SetVec4("line_color", lineColor);

	static GLfloat vertex[3*102400];
	for(int i=0;i<npt;i++) {
		vertex[3*i+0] = (GLfloat)pt[i].x + m_xOrigin;
		vertex[3*i+1] = (GLfloat)pt[i].y + m_yOrigin;
		vertex[3*i+2] = 0.0f;

		m_curX = pt[i].x;
		m_curY = pt[i].y;
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBindVertexArray(m_VAO);
	CheckError("OGLPad::Polyline glBindBuffer");
	glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(GLfloat) * npt, vertex);
	CheckError("OGLPad::Polyline glBufferSubData");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	CheckError("OGLPad::Polyline glVertexAttribPointer");
	glBindBuffer(GL_ARRAY_BUFFER, 0);

//	CheckError("OGLPad::Polyline glBindBuffer0");
	glEnableVertexAttribArray(0);
	//CheckError("OGLPad::Polyline glEnableVertexAttribArray0");

	glDrawArrays(GL_LINE_STRIP, 0, npt);

	
	CheckError("OGLPad::Polyline glDrawArrays");
	glBindVertexArray(0);
	

//	glDisable(GL_BLEND);
	
	CheckError("OGLPad::Polyline glBindVertexArray0");

	//glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	s.UnBind();



}

/**
 * \brief Draw a set of polygons.
 * \param pt list of vertex points for all polygons
 * \param npt list of number of points for each polygon
 * \param nline number of polygons
 * \note The number of entries in npt must be >= nline, and
 *   the number of points in pt must be at least the sum of
 *   the values in npt.
 * \sa Polygon, Polyline, PolyPolyline
 */
void OGLPad::PolyPolygon (const oapi::IVECTOR2 *pt, const int *npt, const int nline)
{
	int offset = 0;
	for(int i=0;i<nline;i++) {
		Polygon (&pt[offset], npt[i]);
		offset+=npt[i];
	}
}

/**
 * \brief Draw a set of polylines.
 * \param pt list of vertex points for all lines
 * \param npt list of number of points for each line
 * \param nline number of lines
 * \note The number of entries in npt must be >= nline, and
 *   the number of points in pt must be at least the sum of
 *   the values in npt.
 * \sa Polyline, Polygon, PolyPolygon
 */
void OGLPad::PolyPolyline (const oapi::IVECTOR2 *pt, const int *npt, const int nline)
{
	int offset = 0;
	for(int i=0;i<nline;i++) {
		Polyline (&pt[offset], npt[i]);
		offset+=npt[i];
	}
}


#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"
#define BITMAP_W 512
#define BITMAP_H 512
/*
	enum Style {
		NORMAL=0,    ///< no decoration
		BOLD=1,      ///< boldface
		ITALIC=2,    ///< italic
		UNDERLINE=4  ///< underlined
	};
*/
/*
LiberationMono-BoldItalic.ttf  LiberationMono-Regular.ttf     LiberationSans-Italic.ttf            LiberationSansNarrow-Italic.ttf   LiberationSerif-BoldItalic.ttf  LiberationSerif-Regular.ttf
LiberationMono-Bold.ttf        LiberationSans-BoldItalic.ttf  LiberationSansNarrow-BoldItalic.ttf  LiberationSansNarrow-Regular.ttf  LiberationSerif-Bold.ttf
LiberationMono-Italic.ttf      LiberationSans-Bold.ttf        LiberationSansNarrow-Bold.ttf        LiberationSans-Regular.ttf        LiberationSerif-Italic.ttf
*/

static const char *GetFont(Font::Style style, bool prop)
{
	if(prop) {
		if((style & Font::BOLD) && (style & Font::ITALIC)) {
			return "LiberationSans-BoldItalic.ttf";
		}
		if(style & Font::BOLD) {
			return "LiberationSans-Bold.ttf";
		}
		if(style & Font::ITALIC) {
			return "LiberationSans-Italic.ttf";
		}
		return "LiberationSans-Regular.ttf";
 	} else {
		if((style & Font::BOLD) && (style & Font::ITALIC)) {
			return "LiberationMono-BoldItalic.ttf";
		}
		if(style & Font::BOLD) {
			return "LiberationMono-Bold.ttf";
		}
		if(style & Font::ITALIC) {
			return "LiberationMono-Italic.ttf";
		}
		return "LiberationMono-Regular.ttf";
	}
}


OGLFont::OGLFont (int height, bool prop, const char *face, Style style, int orientation): oapi::Font (height, prop, face, style, orientation)
{
	stbtt_pack_context pc;

	m_Height = abs(height);

	const char *f = GetFont(style, prop);

	char tmp[255];
	sprintf(tmp,"/usr/share/fonts/truetype/liberation/%s", f);

	FILE *file;
	if((file = fopen(tmp, "rb")) == NULL){
		printf("Cannot open font file '%s'\n", tmp);
		exit(-1);
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	unsigned char* ttf_buffer = new unsigned char[size];
	unsigned char* temp_bitmap = new unsigned char[BITMAP_W * BITMAP_H];

	size_t ret = fread(ttf_buffer, 1, size, file);
	if(ret != (size_t)size) {
		printf("Error reading font\n");
		exit(EXIT_FAILURE);
	}
	fclose(file);

	stbtt_PackBegin(&pc, temp_bitmap, BITMAP_W, BITMAP_H, 0, 1, NULL);
	stbtt_PackSetOversampling(&pc, 2, 2);
	stbtt_PackFontRange(&pc, ttf_buffer, 0, height, 0, 256, m_CharData);
	stbtt_PackEnd(&pc);

	glGenTextures(1, &m_Atlas);
	CheckError("OGLFont glGenTextures");
	glBindTexture(GL_TEXTURE_2D, m_Atlas);
	CheckError("OGLFont glBindTexture");
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BITMAP_W, BITMAP_H, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, BITMAP_W, BITMAP_H, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
	CheckError("OGLFont glTexImage2D");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CheckError("OGLFont glTexParameteri");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CheckError("OGLFont glTexParameteri2");
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[]ttf_buffer;
	delete[]temp_bitmap;


	glGenVertexArrays(1, &m_VAO);
	CheckError("OGLFont glGenVertexArrays");
	glBindVertexArray(m_VAO);
	CheckError("OGLFont glBindVertexArray");

	glGenBuffers(1, &m_Buffer);
	CheckError("OGLFont glGenBuffers");
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	CheckError("OGLFont glBindBuffer");
	glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	CheckError("OGLFont glBufferData");

	glBindVertexArray(0);
}

OGLFont::~OGLFont ()
{
	glDeleteTextures(1, &m_Atlas);
	glDeleteBuffers(1, &m_Buffer);
	glDeleteVertexArrays(1, &m_VAO);
}
