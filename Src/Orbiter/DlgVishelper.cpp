#include "DlgVishelper.h"
#include "Orbiter.h"
#include "Psys.h"
#include "Camera.h"
#include "Planet.h"
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

std::string m_planetarium_selected_object = "Sun";
extern PlanetarySystem *g_psys;
extern Camera          *g_camera;

void AddCbodyNode(const CelestialBody *cbody) {
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool is_selected = m_planetarium_selected_object == cbody->Name();
    if (is_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    if(cbody->nSecondary()) {
        if(!strcmp(cbody->Name(), "Sun"))
            node_flags|=ImGuiTreeNodeFlags_DefaultOpen;

        bool node_open = ImGui::TreeNodeEx(cbody->Name(), node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_planetarium_selected_object = cbody->Name();
            ImGui::CloseCurrentPopup();
        }
        if(node_open) {
            for (int i = 0; i < cbody->nSecondary(); i++) {
                AddCbodyNode (cbody->Secondary(i));
            }
            ImGui::TreePop();
        }
    } else {
        ImGui::TreeNodeEx(cbody->Name(), node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_planetarium_selected_object = cbody->Name();
            ImGui::CloseCurrentPopup();
        }
    }
}
void DrawTree() {
    for (int i = 0; i < g_psys->nStar(); i++)
        AddCbodyNode (g_psys->GetStar(i));
}
void DrawMarkerList(const char *body) {
	Planet *planet = g_psys->GetPlanet (body, true);
	if (!planet)
        return;

	if (planet->LabelFormat() < 2) {
        int nlist;
		oapi::GraphicsClient::LABELLIST *list = planet->LabelList (&nlist);
		if (nlist) {

            bool selectAll = false;
            bool clearAll  = false;
            bool modified  = false;
            if(ImGui::Button("Select All")) {
                selectAll = true;
                modified  = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear All")) {
                clearAll = true;
                modified = true;
            }

            for (int i = 0; i < nlist; i++) {
                if(selectAll) list[i].active = true;
                if(clearAll)  list[i].active = false;
                if(ImGui::Checkbox(list[i].name, &list[i].active)) {
                    modified = true;
                }
            }

            if(modified) {
                std::ifstream cfg (g_pOrbiter->Cfg()->ConfigPath (planet->Name()));
                planet->ScanLabelLists (cfg);
            }
        }
	} else {
		int nlabel = planet->NumLabelLegend();
		if (nlabel) {
            bool selectAll = false;
            bool clearAll  = false;
            if(ImGui::Button("Select All")) {
                selectAll = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("Clear All")) {
                clearAll = true;
            }

			const oapi::GraphicsClient::LABELTYPE *lspec = planet->LabelLegend();
			for (int i = 0; i < nlabel; i++) {
                bool active = lspec[i].active;

                if(selectAll) active = true;
                if(clearAll)  active = false;

                if(ImGui::Checkbox(lspec[i].name, &active) || selectAll || clearAll) {
                    if(active != lspec[i].active)
                        planet->SetLabelActive(i, active);
                }
			}
		}
	}

}

void DlgVishelper::DrawPlanetarium() {
    auto g_client = g_pOrbiter->GetGraphicsClient();

    int *plnmode = (int*)g_client->GetConfigParam (CFGPRM_PLANETARIUMFLAG);

    ImGui::CheckboxFlags("Planetarium Mode (F9)", plnmode, PLN_ENABLE);

    if(!(*plnmode & PLN_ENABLE))
        ImGui::BeginDisabled();

    ImGui::CheckboxFlags("Celestial sphere labels", plnmode, PLN_CCMARK);
    ImGui::CheckboxFlags("Vessels", plnmode, PLN_VMARK);

    ImGui::Separator();
    ImGui::CheckboxFlags("Celestial Grid", plnmode, PLN_CGRID);
    ImGui::CheckboxFlags("Ecliptic Grid", plnmode, PLN_EGRID);
    ImGui::CheckboxFlags("Line of ecliptic", plnmode, PLN_ECL);
    ImGui::CheckboxFlags("Celestial equator", plnmode, PLN_EQU);
    ImGui::Separator();

    ImGui::TextUnformatted("Constellations");
    ImGui::CheckboxFlags("Constellation lines", plnmode, PLN_CONST);
    ImGui::CheckboxFlags("Constellation labels", plnmode, PLN_CNSTLABEL);
    int labelsmode=0;
    if(*plnmode & PLN_CNSTSHORT) labelsmode = 1;
    ImGui::RadioButton("Full", &labelsmode, 0); ImGui::SameLine();
    ImGui::RadioButton("Short", &labelsmode, 1);
    if(labelsmode) {
        *plnmode |= PLN_CNSTSHORT;
        *plnmode &= ~PLN_CNSTLONG;
    } else {
        *plnmode |= PLN_CNSTLONG;
        *plnmode &= ~PLN_CNSTSHORT;
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Markers");
    ImGui::CheckboxFlags("Celestial bodies", plnmode, PLN_CMARK);
    ImGui::CheckboxFlags("Planetary landmarks", plnmode, PLN_LMARK);
    ImGui::CheckboxFlags("Surface Bases", plnmode, PLN_BMARK);
    ImGui::CheckboxFlags("VOR Transmitters", plnmode, PLN_RMARK);
    if(ImGui::CollapsingHeader("Configure landmarks", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;

        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, ImGui::GetContentRegionAvail().y), true, window_flags);
        DrawTree();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true, window_flags);
        DrawMarkerList(m_planetarium_selected_object.c_str());
        ImGui::EndChild();

    }

    if(!(*plnmode & PLN_ENABLE))
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
