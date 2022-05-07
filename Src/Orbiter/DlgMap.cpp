#include "DlgMap.h"
#include "Orbiter.h"
#include "Psys.h"
#include "Celbody.h"
#include "Psys.h"
#include <imgui/imgui.h>

extern TimeData td;

extern PlanetarySystem *g_psys;
const std::string DlgMap::etype = "DlgMap";

DlgMap::DlgMap(const std::string &name) : GUIElement(name, "DlgMap") {
    show = false;
    m_SelectedBody = "Select...";
}


void DlgMap::AddCbodyNode(const CelestialBody *cbody) {
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool is_selected = m_SelectedBody == cbody->Name();
    if (is_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    if(cbody->nSecondary()) {
        if(!strcmp(cbody->Name(), "Sun"))
            node_flags|=ImGuiTreeNodeFlags_DefaultOpen;

        bool node_open = ImGui::TreeNodeEx(cbody->Name(), node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedBody = cbody->Name();
            SetBody(cbody->Name());
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
            m_SelectedBody = cbody->Name();
            SetBody(cbody->Name());
        }
    }
}

void DlgMap::DrawTree() {
    for (int i = 0; i < g_psys->nStar(); i++)
        AddCbodyNode (g_psys->GetStar(i));
/*
    if (ImGui::TreeNode("Spaceports")) {
        for (int i = 0; i < g_psys->nGrav(); i++) {
            Body *obj = g_psys->GetGravObj (i);
            if (obj->Type() != OBJTP_PLANET) continue;
            Planet *planet = (Planet*)obj;
            if (g_psys->nBase(planet) > 0 && ImGui::CollapsingHeader(planet->Name())) {
                for (int j = 0; j < g_psys->nBase(planet); j++) {
                    const char *name = g_psys->GetBase (planet,j)->Name();
                    const bool is_selected = m_SelectedBody == name;
                    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
                    ImGui::TreeNodeEx(name, node_flags);
                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                        m_SelectedBody = name;
                }
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Vessels")) {
        for (int i = 0; i < g_psys->nVessel(); i++) {
            const char *name = g_psys->GetVessel(i)->Name();
            const bool is_selected = m_SelectedBody == name;
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
            ImGui::TreeNodeEx(name, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                m_SelectedBody = name;
        }
        ImGui::TreePop();
    }*/
}


void DlgMap::SetBody(const char *body) {
    m_SelectedBody = body;
    if(m_Map) {
        Planet *b = g_psys->GetPlanet (body);
        if(b) {
            m_Map->SetCBody(b);
            m_Map->Update();
            m_Map->DrawMap ();

       		for (int i = 0; i < m_Map->GetCustomMarkerSet().nset; i++)
			    m_Map->GetCustomMarkerSet().set[i].active = true;

        }
    }
}
void DlgMap::DrawMap() {
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if(sz.x>=1.0f && sz.y >= 1.0f) {
        static ImVec2 oldsz = sz;//ImVec2(512,256);

        if(!m_Map) {
            Planet *earth = g_psys->GetPlanet ("Earth");
            if(earth) {
                m_SelectedBody = "Earth";
                m_Map=std::make_unique<VectorMap>(earth);
                m_Map->SetCBody(earth);
                m_Map->SetCanvas(nullptr, sz.x, sz.y);
                SetBody("Earth");
            }
        }

        if(sz.x != oldsz.x || sz.y != oldsz.y) {
            m_Map->SetCanvas(nullptr, sz.x, sz.y);

            m_Map->Update();
            m_Map->DrawMap ();

            oldsz = sz;
        }

        const double updDT = 1.0;
        if (td.SysT1 > updTmax || td.SysT1 < updTmin) {
            updTmax = td.SysT1 + updDT;
            updTmin = td.SysT1 - updDT;

            m_Map->Update();
            m_Map->DrawMap ();
        }

        ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
        ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
        ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
        ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white

        oapiIncrTextureRef(m_Map->GetMap());
        ImGui::ImageButton(m_Map->GetMap(), ImVec2(sz.x, sz.y), uv_min, uv_max, 0, tint_col, border_col);
        ImGuiIO& io = ImGui::GetIO();
        bool updateMap = false;
        if (ImGui::IsItemHovered() && io.MouseWheel != 0.0) {
            m_Zoom+=io.MouseWheel;
            if(m_Zoom<1.0) m_Zoom=1.0;

            m_Map->SetZoom(m_Zoom);
            updateMap = true;
        }
        if (ImGui::IsItemActive()) {
            double lngc = m_Map->CntLng();
            double latc = m_Map->CntLat();

            double scale = std::min (sz.x, 2.0f*sz.y);
            double scalefac = m_Map->ZoomFac()*scale/Pi2;

            double lng = lngc - io.MouseDelta.x/scalefac;
            double lat = std::max (-Pi05, std::min (Pi05, latc + io.MouseDelta.y/scalefac));
            if (lng != lngc || lat != latc) {
                m_Map->SetCenter (lng, lat);
                updateMap = true;
            }
        }

        if(updateMap) {
            m_Map->Update();
            m_Map->DrawMap ();
        }
    }
}

void DlgMap::DrawMenu() {
    if (ImGui::Button("Reset map")) {
        m_Zoom=1.0;
        m_Map->SetZoom(m_Zoom);
        m_Map->SetCenter(0.0,0.0);
        m_Map->Update();
        m_Map->DrawMap ();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Body :");
    ImGui::SameLine();
    if (ImGui::Button(m_SelectedBody.c_str()))
        ImGui::OpenPopup("body_select_popup");

    ImVec2 btn_pos = ImGui::GetItemRectMin();
    ImGui::SetNextWindowPos(btn_pos);

    if (ImGui::BeginPopup("body_select_popup"))
    {
        ImGui::TextUnformatted(m_SelectedBody.c_str());
        ImGui::Separator();
        DrawTree();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("body_options_popup");

    btn_pos = ImGui::GetItemRectMin();
    ImGui::SetNextWindowPos(btn_pos);

    if (ImGui::BeginPopup("body_options_popup"))
    {
        int df = m_Map->GetDisplayFlags();

        ImGui::Text("Options");
        ImGui::Separator();

        ImGui::Text("Vessels");
        int vesselmode = (df & DISP_VESSEL ? df & DISP_FOCUSONLY ? 1:0:2);
        ImGui::RadioButton("All", &vesselmode, 0); ImGui::SameLine();
        ImGui::RadioButton("Focus Only", &vesselmode, 1); ImGui::SameLine();
        ImGui::RadioButton("None", &vesselmode, 2);
        ImGui::Separator();
        int vmode = (vesselmode==1 ? DISP_VESSEL | DISP_FOCUSONLY : vesselmode==2 ? 0 : DISP_VESSEL);
        df &= ~(DISP_VESSEL | DISP_FOCUSONLY);
        df |= vmode;

        ImGui::Text("Orbit Display");
        if (ImGui::BeginTable("table orbit display", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Focus Vessel", &df, DISP_ORBITFOCUS);
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("Target", &df, DISP_ORBITSEL);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            int orbitmode=(df & DISP_ORBITPLANE) ? 0 : 1;
            ImGui::RadioButton("Orbit Plane", &orbitmode, 0);
            ImGui::TableSetColumnIndex(1);
            ImGui::RadioButton("Ground Track", &orbitmode, 1);
            df &= ~(DISP_GROUNDTRACK | DISP_ORBITPLANE);
            df |= (orbitmode == 0)? DISP_ORBITPLANE:DISP_GROUNDTRACK;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Horizon Line", &df, DISP_HORIZONLINE); 
            ImGui::EndTable();
        }
        ImGui::Separator();

        ImGui::Text("Terminator");
        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Line", &df, DISP_TERMINATOR_LINE);
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("Shaded", &df, DISP_TERMINATOR_SHADE);
            ImGui::EndTable();
        }
        ImGui::Separator();

        ImGui::Text("Surface Markers");
        if (ImGui::BeginTable("table2", 2, ImGuiTableFlags_SizingStretchSame)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Grid Line", &df, DISP_GRIDLINE);
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("Surface Bases", &df, DISP_BASE);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Coastlines", &df, DISP_COASTLINE);
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("VOR Transmitters", &df, DISP_NAVAID);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::CheckboxFlags("Contour Lines", &df,DISP_CONTOURS); 
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("Landmarks", &df, DISP_CUSTOM1);

            ImGui::EndTable();
        }
        ImGui::Separator();

        ImGui::Text("Other");
        ImGui::CheckboxFlags("Natural Satellites", &df, DISP_MOON);

        ImGui::EndPopup();

        m_Map->SetDisplayFlags(df);
    }
}

void DlgMap::Show() {
    if(!show) return; 

    if(ImGui::Begin("Map", &show)) {
        DrawMenu();
        DrawMap();
    }
    ImGui::End();
}
