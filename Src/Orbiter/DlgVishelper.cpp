#include "DlgVishelper.h"
#include "Orbiter.h"
#include <imgui/imgui.h>

extern Orbiter *g_pOrbiter;
const std::string DlgVishelper::etype = "DlgVishelper";

DlgVishelper::DlgVishelper(const std::string &name) : GUIElement(name, "DlgVishelper") {
    show = false;
}

void DlgVishelper::Show() {
	if(show) {
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(140, 390), ImGuiCond_FirstUseEver);
		ImGui::Begin("Visual Helpers", &show);
        const char *tabs[] = {
            "Planetarium",// "Forces", "Axes"
        };

        void (DlgVishelper::* func[])() = {
            &DlgVishelper::DrawPlanetarium, &DlgVishelper::DrawForces, &DlgVishelper::DrawAxes
        };

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            for(size_t i = 0; i<sizeof(tabs)/sizeof(tabs[0]);i++) {
                if(ImGui::BeginTabItem(tabs[i])) {
                    (this->*func[i])();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}

void DlgVishelper::DrawPlanetarium() {
    static bool c;
    ImGui::Checkbox("Planetarium Mode (F9)", &c);

    if(!c)
        ImGui::BeginDisabled();

    ImGui::Separator();
    ImGui::Checkbox("Celestial Grid", &c);
    ImGui::Checkbox("Ecliptic Grid", &c);
    ImGui::Checkbox("Ecliptic", &c);
    ImGui::Checkbox("Target Equator", &c);
    ImGui::Separator();

    ImGui::TextUnformatted("Constellations");
    ImGui::Checkbox("Lines", &c);
    ImGui::Checkbox("Labels", &c);
    int labelsmode=0;
    ImGui::RadioButton("Full", &labelsmode, 0); ImGui::SameLine();
    ImGui::RadioButton("Short", &labelsmode, 1);
    ImGui::Separator();

    ImGui::TextUnformatted("Markers");
    ImGui::Checkbox("Vessels", &c);
    ImGui::Checkbox("Surface Bases", &c);
    ImGui::Checkbox("VOR Transmitters", &c);
    ImGui::Checkbox("Celestial", &c); ImGui::SameLine(); ImGui::Button("Config##cel");
    ImGui::Checkbox("Surface", &c); ImGui::SameLine(); ImGui::Button("Config##surf");
    ImGui::Checkbox("Bodies", &c);

    if(!c)
        ImGui::EndDisabled();

}

void DlgVishelper::DrawForces() {
    bool c;
    ImGui::Checkbox("Body Force Vectors", &c);
    ImGui::Separator();

    ImGui::TextUnformatted("Linear Forces");
    ImGui::Checkbox("Weight", &c);
    ImGui::Checkbox("Lift", &c);
    ImGui::Checkbox("Thrust", &c);
    ImGui::Checkbox("Drag", &c);
    ImGui::NewLine();
    ImGui::Checkbox("Total", &c);
    ImGui::Separator();

    ImGui::TextUnformatted("Angular Forces");
    ImGui::Checkbox("Torque", &c);
    ImGui::Separator();

    ImGui::TextUnformatted("Scale");
    int scalemode=0;
    ImGui::RadioButton("Linear", &scalemode, 0); ImGui::SameLine();
    ImGui::RadioButton("Logarithmic", &scalemode, 1);
    ImGui::SetNextItemWidth(-FLT_MIN);
    float scale=1.0;
    if(ImGui::SliderFloat("##slider Scale", &scale, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
    }
    ImGui::Separator();

    ImGui::TextUnformatted("Opacity");
    ImGui::SetNextItemWidth(-FLT_MIN);
    float opacity=0.5;
    if(ImGui::SliderFloat("##slider Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
    }
}

void DlgVishelper::DrawAxes() {
    static bool c;
    ImGui::Checkbox("Coordinate Axes", &c);
    ImGui::Separator();
    ImGui::TextUnformatted("Objects");
    ImGui::Checkbox("Vessels", &c);
    ImGui::Checkbox("Celestial Bodies", &c);
    ImGui::Checkbox("Surface Bases", &c);
    ImGui::Separator();
    ImGui::Checkbox("Show Negative Axes", &c);
    ImGui::Separator();

    ImGui::TextUnformatted("Scale");
    float scale=1.0;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if(ImGui::SliderFloat("##slider Scale", &scale, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
    }
    ImGui::Separator();

    ImGui::TextUnformatted("Opacity");
    ImGui::SetNextItemWidth(-FLT_MIN);
    float opacity=0.5;
    if(ImGui::SliderFloat("##slider Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
    }

}
