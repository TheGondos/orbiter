#include "DlgFunction.h"
#include "Orbiter.h"
#include <imgui/imgui.h>

extern Orbiter *g_pOrbiter;
const std::string DlgFunction::etype = "DlgFunction";

DlgFunction::DlgFunction(const std::string &name) : GUIElement(name, "DlgFunction") {
    show = false;
}

void DlgFunction::Show() {
	if(show) {
        ImGui::SetNextWindowPos(ImVec2(880, 145), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 290), ImGuiCond_FirstUseEver);
		ImGui::Begin("Custom Functions", &show);

    	for (int i = 0; i < g_pOrbiter->ncustomcmd; i++) {
	    	if(ImGui::Button(g_pOrbiter->customcmd[i].label)) {
                g_pOrbiter->customcmd[i].func (g_pOrbiter->customcmd[i].context);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(g_pOrbiter->customcmd[i].desc);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        ImGui::End();
    }
}
