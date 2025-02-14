#pragma once

#include "imgui.h"

// ImGui extras

enum class ImGuiFont {
	DEFAULT,
	MONO
};


namespace ImGui {

	OAPIFUNC bool SliderFloatReset(const char* label, float* v, float v_min, float v_max, float v_default, const char* display_format = "%.3f");
	OAPIFUNC void HelpMarker(const char* desc, bool sameline = true);
	OAPIFUNC void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(-1.0f, -1.0f));
	OAPIFUNC void EndGroupPanel();
	OAPIFUNC bool MenuButton(const char *label, const char *tooltip = NULL, float xoffset = 0.0f);
	OAPIFUNC void PushFont(ImGuiFont);
	OAPIFUNC ImTextureID GetImTextureID (SURFHANDLE surf);
	OAPIFUNC void Image(SURFHANDLE surf, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	OAPIFUNC bool ImageButton(const char* str_id, SURFHANDLE surf, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
};
