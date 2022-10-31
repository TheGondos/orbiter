#include "OrbiterAPI.h"
#include "Orbiter.h"
#include <imgui/imgui.h>
#include "imgui_notify.h"
#include "imgui_impl_glfw.h"
#include "fa_solid_900.h"
#include "Camera.h"
#include "Pane.h"

extern Orbiter *g_pOrbiter;
extern Camera          *g_camera;         // observer camera
extern Pane            *g_pane;           // 2D output surface

DLLEXPORT ImGuiContext*   GImGui = NULL;

const ImWchar*  GetGlyphRangesOrbiter()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x00A0, 0x02D9, // Polish characters 
        0,
    };
    return &ranges[0];
}


static void ImGuiSetStyle()
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
#endif
}

GUIManager::GUIManager()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
//	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	hRenderWnd = g_pOrbiter->GetGraphicsClient()->clbkCreateRenderWindow();
	
	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();


    ImGuiSetStyle();

	ImFontConfig config;

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.FontDataOwnedByAtlas = false;
	
	fontDefault = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 14.0f, &config, GetGlyphRangesOrbiter());
	io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 14.0, &icons_config, icons_ranges);

	fontBold = io.Fonts->AddFontFromFileTTF("Roboto-Bold.ttf", 14.0f, &config, GetGlyphRangesOrbiter());

	fontH1 = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 18.0f, &config, GetGlyphRangesOrbiter());
//	io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 18.0, &icons_config, icons_ranges);

	fontH2 = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 24.0f, &config, GetGlyphRangesOrbiter());
//	io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 24.0, &icons_config, icons_ranges);

	fontH3 = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 30.0f, &config, GetGlyphRangesOrbiter());
//	io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 30.0, &icons_config, icons_ranges);
}

void GUIManager::Notify(enum NotifType type, const char *title, const char *content)
{
    ImGuiToastType toasttype = ImGuiToastType_Info;
    switch(type) {
        case Success:
            toasttype = ImGuiToastType_Success;
            break;
        case Warning:
            toasttype = ImGuiToastType_Warning;
            break;
        case Error:
            toasttype = ImGuiToastType_Error;
            break;
        case Info:
            toasttype = ImGuiToastType_Info;
            break;
    }
  	ImGui::InsertNotification({ toasttype, title, content });
}

void GUIManager::RenderGUI()
{
	static const auto toast2icon = [](ImGuiToastType type) {
		switch (type)
		{
		case ImGuiToastType_None:
			return (const char *)nullptr;
		case ImGuiToastType_Success:
			return ICON_FA_CHECK_CIRCLE;
		case ImGuiToastType_Warning:
			return ICON_FA_EXCLAMATION_TRIANGLE;
		case ImGuiToastType_Error:
			return ICON_FA_TIMES_CIRCLE;
		case ImGuiToastType_Info:
			return ICON_FA_INFO_CIRCLE;
		}
		assert(false);
	};

	g_pOrbiter->GetGraphicsClient()->clbkImGuiNewFrame ();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
	ImGui::RenderNotifications(toast2icon); // <-- Here we render all notifications
	ImGui::PopStyleVar(1); // Don't forget to Pop()
	ImGui::PopStyleColor(1);

	for(auto &ctrl: m_GUICtrls) {
		if(ctrl->show) {
			ctrl->Show();
		}
	}

	ImGui::Render();
	g_pOrbiter->GetGraphicsClient()->clbkImGuiRenderDrawData ();
}

void GUIManager::UpdateCursor(float dx, float dy) {
	double x,y;
	glfwGetCursorPos(hRenderWnd, &x, &y);
	x+=dx;
	y+=dy;
	x = std::clamp(x ,0.0, (double)g_pOrbiter->ViewW() - 1.0);
	y = std::clamp(y ,0.0, (double)g_pOrbiter->ViewH() - 1.0);

	glfwSetCursorPos(hRenderWnd, x, y);

	ImGuiIO& io = ImGui::GetIO();
	if(prev_cursor_position_callback) prev_cursor_position_callback(hRenderWnd, x, y);
	if (io.WantCaptureMouse)
		return;

	g_pOrbiter->MouseEvent(oapi::MOUSE_MOVE, 0, x, y);
}
void GUIManager::LeftClick(bool clicked) {
	ImGuiIO& io = ImGui::GetIO();
	if(prev_mouse_button_callback && clicked) prev_mouse_button_callback(hRenderWnd, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
	if(prev_mouse_button_callback &&!clicked) prev_mouse_button_callback(hRenderWnd, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
	if (io.WantCaptureMouse)
		return;

	double x,y;
	glfwGetCursorPos(hRenderWnd, &x, &y);

	if(clicked)  g_pOrbiter->MouseEvent(oapi::MOUSE_LBUTTONDOWN, oapi::MouseModifier::MOUSE_NONE, x, y);
	if(!clicked) g_pOrbiter->MouseEvent(oapi::MOUSE_LBUTTONUP, oapi::MouseModifier::MOUSE_NONE, x, y);
}
void GUIManager::RightClick(bool clicked) {
	ImGuiIO& io = ImGui::GetIO();
	if(prev_mouse_button_callback && clicked) prev_mouse_button_callback(hRenderWnd, GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0);
	if(prev_mouse_button_callback &&!clicked) prev_mouse_button_callback(hRenderWnd, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, 0);
	if (io.WantCaptureMouse)
		return;

	double x,y;
	glfwGetCursorPos(hRenderWnd, &x, &y);

	if(clicked)  g_pOrbiter->MouseEvent(oapi::MOUSE_RBUTTONDOWN, oapi::MouseModifier::MOUSE_NONE, x, y);
	if(!clicked) g_pOrbiter->MouseEvent(oapi::MOUSE_RBUTTONUP, oapi::MouseModifier::MOUSE_NONE, x, y);
}


void GUIManager::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	ImGuiIO& io = ImGui::GetIO();
	if(prev_cursor_position_callback) prev_cursor_position_callback(window, xpos, ypos);
	if (io.WantCaptureMouse)
		return;

	g_pOrbiter->MouseEvent(oapi::MOUSE_MOVE, 0, xpos, ypos);
}
void GUIManager::GetCursorPos(double *x, double *y) {
	glfwGetCursorPos(hRenderWnd, x, y);
}

void GUIManager::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if(prev_mouse_button_callback) prev_mouse_button_callback(window, button, action, mods);
	if (io.WantCaptureMouse)
		return;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	int m = oapi::MouseModifier::MOUSE_NONE;

	if(mods & GLFW_MOD_SHIFT)   m |= oapi::MouseModifier::MOUSE_SHIFT;
	if(mods & GLFW_MOD_CONTROL) m |= oapi::MouseModifier::MOUSE_CTRL;
	if(mods & GLFW_MOD_ALT)     m |= oapi::MouseModifier::MOUSE_ALT;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        g_pOrbiter->MouseEvent(oapi::MOUSE_RBUTTONDOWN, m, xpos, ypos);
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        g_pOrbiter->MouseEvent(oapi::MOUSE_RBUTTONUP, m, xpos, ypos);
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        g_pOrbiter->MouseEvent(oapi::MOUSE_LBUTTONDOWN, m, xpos, ypos);
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_pOrbiter->MouseEvent(oapi::MOUSE_LBUTTONUP, m, xpos, ypos);
	}
}

void GUIManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += (float)xoffset;
	io.MouseWheel += (float)yoffset;

	if(prev_scroll_callback) prev_scroll_callback(window, xoffset, yoffset);
	if (io.WantCaptureMouse)
		return;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int mod = 0;
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
	|| glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
		mod = oapi::MouseModifier::MOUSE_CTRL;
		
	if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
	|| glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
		mod = oapi::MouseModifier::MOUSE_SHIFT;
		
	if(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
	|| glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
		mod = oapi::MouseModifier::MOUSE_ALT;
		
	// yoffset is  one of -1/0/1 -> too small, scale it 50 times
	g_pOrbiter->MouseEvent(oapi::MOUSE_WHEEL, mod, xoffset * 10.0, yoffset * 50.0);
}

void GUIManager::key_callback(GLFWwindow* window, int gkey, int scancode, int action, int mods)
{
	if(prev_key_callback) prev_key_callback(window, gkey, scancode, action, mods);

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) {
		return;
	}
	
	if(gkey == GLFW_KEY_UNKNOWN)
		return;

	static char glfw2oapi[GLFW_KEY_LAST];
	static int initdone=0;
	if(!initdone) {
		initdone=1;
		memset(glfw2oapi, 0, GLFW_KEY_LAST);
		glfw2oapi[GLFW_KEY_SPACE]=OAPI_KEY_SPACE;
		glfw2oapi[GLFW_KEY_APOSTROPHE]=OAPI_KEY_APOSTROPHE;
		glfw2oapi[GLFW_KEY_COMMA]=OAPI_KEY_COMMA;
		glfw2oapi[GLFW_KEY_MINUS]=OAPI_KEY_MINUS;
		glfw2oapi[GLFW_KEY_PERIOD]=OAPI_KEY_PERIOD;
		glfw2oapi[GLFW_KEY_SLASH]=OAPI_KEY_SLASH;
		glfw2oapi[GLFW_KEY_0]=OAPI_KEY_0;
		glfw2oapi[GLFW_KEY_1]=OAPI_KEY_1;
		glfw2oapi[GLFW_KEY_2]=OAPI_KEY_2;
		glfw2oapi[GLFW_KEY_3]=OAPI_KEY_3;
		glfw2oapi[GLFW_KEY_4]=OAPI_KEY_4;
		glfw2oapi[GLFW_KEY_5]=OAPI_KEY_5;
		glfw2oapi[GLFW_KEY_6]=OAPI_KEY_6;
		glfw2oapi[GLFW_KEY_7]=OAPI_KEY_7;
		glfw2oapi[GLFW_KEY_8]=OAPI_KEY_8;
		glfw2oapi[GLFW_KEY_9]=OAPI_KEY_9;
		glfw2oapi[GLFW_KEY_SEMICOLON]=OAPI_KEY_SEMICOLON;
		glfw2oapi[GLFW_KEY_EQUAL]=OAPI_KEY_EQUALS;
		glfw2oapi[GLFW_KEY_A]=OAPI_KEY_A;
		glfw2oapi[GLFW_KEY_B]=OAPI_KEY_B;
		glfw2oapi[GLFW_KEY_C]=OAPI_KEY_C;
		glfw2oapi[GLFW_KEY_D]=OAPI_KEY_D;
		glfw2oapi[GLFW_KEY_E]=OAPI_KEY_E;
		glfw2oapi[GLFW_KEY_F]=OAPI_KEY_F;
		glfw2oapi[GLFW_KEY_G]=OAPI_KEY_G;
		glfw2oapi[GLFW_KEY_H]=OAPI_KEY_H;
		glfw2oapi[GLFW_KEY_I]=OAPI_KEY_I;
		glfw2oapi[GLFW_KEY_J]=OAPI_KEY_J;
		glfw2oapi[GLFW_KEY_K]=OAPI_KEY_K;
		glfw2oapi[GLFW_KEY_L]=OAPI_KEY_L;
		glfw2oapi[GLFW_KEY_M]=OAPI_KEY_M;
		glfw2oapi[GLFW_KEY_N]=OAPI_KEY_N;
		glfw2oapi[GLFW_KEY_O]=OAPI_KEY_O;
		glfw2oapi[GLFW_KEY_P]=OAPI_KEY_P;
		glfw2oapi[GLFW_KEY_Q]=OAPI_KEY_Q;
		glfw2oapi[GLFW_KEY_R]=OAPI_KEY_R;
		glfw2oapi[GLFW_KEY_S]=OAPI_KEY_S;
		glfw2oapi[GLFW_KEY_T]=OAPI_KEY_T;
		glfw2oapi[GLFW_KEY_U]=OAPI_KEY_U;
		glfw2oapi[GLFW_KEY_V]=OAPI_KEY_V;
		glfw2oapi[GLFW_KEY_W]=OAPI_KEY_W;
		glfw2oapi[GLFW_KEY_X]=OAPI_KEY_X;
		glfw2oapi[GLFW_KEY_Y]=OAPI_KEY_Y;
		glfw2oapi[GLFW_KEY_Z]=OAPI_KEY_Z;
		glfw2oapi[GLFW_KEY_LEFT_BRACKET]=OAPI_KEY_LBRACKET;
		glfw2oapi[GLFW_KEY_BACKSLASH]=OAPI_KEY_BACKSLASH;
		glfw2oapi[GLFW_KEY_RIGHT_BRACKET]=OAPI_KEY_RBRACKET;
		glfw2oapi[GLFW_KEY_GRAVE_ACCENT]=OAPI_KEY_GRAVE;
		glfw2oapi[GLFW_KEY_ESCAPE]=OAPI_KEY_ESCAPE;
		glfw2oapi[GLFW_KEY_ENTER]=OAPI_KEY_RETURN;
		glfw2oapi[GLFW_KEY_TAB]=OAPI_KEY_TAB;
		glfw2oapi[GLFW_KEY_BACKSPACE]=OAPI_KEY_BACK;
		glfw2oapi[GLFW_KEY_INSERT]=OAPI_KEY_INSERT;
		glfw2oapi[GLFW_KEY_DELETE]=OAPI_KEY_DELETE;
		glfw2oapi[GLFW_KEY_RIGHT]=OAPI_KEY_RIGHT;
		glfw2oapi[GLFW_KEY_LEFT]=OAPI_KEY_LEFT;
		glfw2oapi[GLFW_KEY_DOWN]=OAPI_KEY_DOWN;
		glfw2oapi[GLFW_KEY_UP]=OAPI_KEY_UP;
		glfw2oapi[GLFW_KEY_PAGE_UP]=OAPI_KEY_PRIOR;
		glfw2oapi[GLFW_KEY_PAGE_DOWN]=OAPI_KEY_NEXT;
		glfw2oapi[GLFW_KEY_HOME]=OAPI_KEY_HOME;
		glfw2oapi[GLFW_KEY_END]=OAPI_KEY_END;
		glfw2oapi[GLFW_KEY_CAPS_LOCK]=OAPI_KEY_CAPITAL;
		glfw2oapi[GLFW_KEY_SCROLL_LOCK]=OAPI_KEY_SCROLL;
		glfw2oapi[GLFW_KEY_NUM_LOCK]=OAPI_KEY_NUMLOCK;
		glfw2oapi[GLFW_KEY_PRINT_SCREEN]=OAPI_KEY_SYSRQ;
		glfw2oapi[GLFW_KEY_PAUSE]=OAPI_KEY_PAUSE;
		glfw2oapi[GLFW_KEY_F1]=OAPI_KEY_F1;
		glfw2oapi[GLFW_KEY_F2]=OAPI_KEY_F2;
		glfw2oapi[GLFW_KEY_F3]=OAPI_KEY_F3;
		glfw2oapi[GLFW_KEY_F4]=OAPI_KEY_F4;
		glfw2oapi[GLFW_KEY_F5]=OAPI_KEY_F5;
		glfw2oapi[GLFW_KEY_F6]=OAPI_KEY_F6;
		glfw2oapi[GLFW_KEY_F7]=OAPI_KEY_F7;
		glfw2oapi[GLFW_KEY_F8]=OAPI_KEY_F8;
		glfw2oapi[GLFW_KEY_F9]=OAPI_KEY_F9;
		glfw2oapi[GLFW_KEY_F10]=OAPI_KEY_F10;
		glfw2oapi[GLFW_KEY_F11]=OAPI_KEY_F11;
		glfw2oapi[GLFW_KEY_F12]=OAPI_KEY_F12;
		glfw2oapi[GLFW_KEY_KP_0]=OAPI_KEY_NUMPAD0;
		glfw2oapi[GLFW_KEY_KP_1]=OAPI_KEY_NUMPAD1;
		glfw2oapi[GLFW_KEY_KP_2]=OAPI_KEY_NUMPAD2;
		glfw2oapi[GLFW_KEY_KP_3]=OAPI_KEY_NUMPAD3;
		glfw2oapi[GLFW_KEY_KP_4]=OAPI_KEY_NUMPAD4;
		glfw2oapi[GLFW_KEY_KP_5]=OAPI_KEY_NUMPAD5;
		glfw2oapi[GLFW_KEY_KP_6]=OAPI_KEY_NUMPAD6;
		glfw2oapi[GLFW_KEY_KP_7]=OAPI_KEY_NUMPAD7;
		glfw2oapi[GLFW_KEY_KP_8]=OAPI_KEY_NUMPAD8;
		glfw2oapi[GLFW_KEY_KP_9]=OAPI_KEY_NUMPAD9;
		glfw2oapi[GLFW_KEY_KP_DECIMAL]=OAPI_KEY_DECIMAL;
		glfw2oapi[GLFW_KEY_KP_DIVIDE]=OAPI_KEY_DIVIDE;
		glfw2oapi[GLFW_KEY_KP_MULTIPLY]=OAPI_KEY_MULTIPLY;
		glfw2oapi[GLFW_KEY_KP_SUBTRACT]=OAPI_KEY_SUBTRACT;
		glfw2oapi[GLFW_KEY_KP_ADD]=OAPI_KEY_ADD;
		glfw2oapi[GLFW_KEY_KP_ENTER]=OAPI_KEY_NUMPADENTER;
		glfw2oapi[GLFW_KEY_LEFT_SHIFT]=OAPI_KEY_LSHIFT;
		glfw2oapi[GLFW_KEY_LEFT_CONTROL]=OAPI_KEY_LCONTROL;
		glfw2oapi[GLFW_KEY_RIGHT_SHIFT]=OAPI_KEY_RSHIFT;
		glfw2oapi[GLFW_KEY_LEFT_ALT]=OAPI_KEY_LALT;
		glfw2oapi[GLFW_KEY_RIGHT_CONTROL]=OAPI_KEY_RCONTROL;
		glfw2oapi[GLFW_KEY_RIGHT_ALT]=OAPI_KEY_RALT;
	}

	if(action == GLFW_REPEAT) return; //ignore repeated keydown events when holding key
	char down = (action == GLFW_PRESS) ? 0x80:0x00;
	
	unsigned int key=(unsigned char)glfw2oapi[gkey];
	g_pOrbiter->KeyCallback(key, down);
}


void GUIManager::framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	if(g_camera) g_camera->ResizeViewport(width, height);
	g_pOrbiter->GetGraphicsClient()->clbkSetViewportSize(width, height);
	g_pOrbiter->GetRenderParameters();
	if(g_pane) g_pane->Resize(width, height);
}

static GUIManager *s_GUIManager;
static void g_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	s_GUIManager->scroll_callback(window, xoffset, yoffset);
}
static void g_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	s_GUIManager->mouse_button_callback(window, button, action, mods);
}
static void g_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	s_GUIManager->cursor_position_callback(window, xpos, ypos);
}
static void g_key_callback(GLFWwindow* window, int gkey, int scancode, int action, int mods) {
	s_GUIManager->key_callback(window, gkey, scancode, action, mods);
}
static void g_framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	s_GUIManager->framebuffer_size_callback(window, width, height);
}

void GUIManager::SetupCallbacks()
{
	s_GUIManager = this;
	prev_cursor_position_callback = glfwSetCursorPosCallback(hRenderWnd, g_cursor_position_callback);
	prev_key_callback = glfwSetKeyCallback(hRenderWnd, g_key_callback);
	prev_mouse_button_callback = glfwSetMouseButtonCallback(hRenderWnd, g_mouse_button_callback);
	prev_scroll_callback = glfwSetScrollCallback(hRenderWnd, g_scroll_callback);
	glfwSetFramebufferSizeCallback(hRenderWnd, g_framebuffer_size_callback);
	glfwSetJoystickCallback(InputController::JoystickCallback);
}
