#include "DlgCamera.h"
#include "OrbiterAPI.h"
#include "Orbiter.h"
#include "Celbody.h"
#include "Psys.h"
#include "imgui.h"
#include "imgui_extras.h"
#include "IconsFontAwesome6.h"
#include "i18n.h"

extern PlanetarySystem *g_psys;
extern Camera *g_camera;
extern TimeData td;
extern Orbiter *g_pOrbiter;
extern Vessel *g_focusobj;

DlgCamera::DlgCamera() : ImGuiDialog(ICON_FA_VIDEO, _c("Camera dialog", "Orbiter: Camera"), {512,359}) {
    m_SelectedPreset = -1;
}

void DlgCamera::OnDraw() {
	struct CameraTab {
		const char *name;
		const char *helptopic;
		void (DlgCamera::* func)();
	};

	CameraTab tabs[] = {
		{_c("Camera dialog", "Control"), "/cam_control.htm", &DlgCamera::DrawControl},
		{_c("Camera dialog", "Target"),  "/cam_target.htm",  &DlgCamera::DrawTarget},
		{_c("Camera dialog", "Track"),   "/cam_track.htm",   &DlgCamera::DrawTrack},
		{_c("Camera dialog", "Ground"),  "/cam_ground.htm",  &DlgCamera::DrawGround},
		{_c("Camera dialog", "Preset"),  "/cam_preset.htm",  &DlgCamera::DrawPreset},
	};

	extmode = g_camera->GetExtMode ();
	intmode = g_camera->GetIntMode ();
	extcam = (g_camera->GetMode () != CAM_COCKPIT);
	ground_lock = g_camera->GroundObserver_TargetLock();
	rot_is_tilt = (!extcam || (extmode == CAMERA_GROUNDOBSERVER && !ground_lock));
	rot_is_pan = (extcam && extmode == CAMERA_GROUNDOBSERVER);
    
    if(followterrain[0]=='\0')
      	sprintf (followterrain, "%0.0lf", g_camera->GroundObserver_TerrainLimit());

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("CameraTabBar", tab_bar_flags))
    {
        for(size_t i = 0; i < sizeof(tabs)/sizeof(tabs[0]); i++) {
            if(ImGui::BeginTabItem(tabs[i].name)) {
				SetHelp("html/orbiter.chm", tabs[i].helptopic);
                (this->*tabs[i].func)();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

void DlgCamera::DrawControl() {
    ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;
    ImGui::BeginChild("ChildL", ImVec2(250, 0), window_flags);
        ImGui::SeparatorText(_c("Camera dialog", "Camera mode"));

		// TRANSLATORS: camera mode
		const char *cameraMode = _c("Camera dialog", "Internal");
		if (extcam) {
			switch (extmode) {
			case CAMERA_TARGETRELATIVE:
				cameraMode = _c("Camera dialog", "Target Relative"); break;
			case CAMERA_ABSDIRECTION:
				cameraMode = _c("Camera dialog", "Absolute Direction"); break;
			case CAMERA_GLOBALFRAME:
				cameraMode = _c("Camera dialog", "Global Frame"); break;
			case CAMERA_TARGETTOOBJECT:
				cameraMode = _c("Camera dialog", "Target Object"); break;
			case CAMERA_TARGETFROMOBJECT:
				cameraMode = _c("Camera dialog", "Target from Object"); break;
			case CAMERA_GROUNDOBSERVER:
				if (ground_lock) {
					cameraMode = _c("Camera dialog", "Ground Observer (locked)"); break;
				} else {
					cameraMode = _c("Camera dialog", "Ground Observer"); break;
				}
				break;
			}
		}
		ImGui::Text(_c("Camera dialog", "Camera mode : %s"), cameraMode);

		ImVec2 pos;
		double dt = td.SysDT;

		pos.x = 50;
		pos.y = 75;
		ImGui::SetCursorPos(pos); 
		ImGui::ArrowButton("##up", ImGuiDir_Up);
		if(ImGui::IsItemActive()){
			if (rot_is_tilt) g_camera->Rotate   ( 0,  dt);
			else             g_camera->ShiftTheta (-dt);
		}

		pos.x = 50;
		pos.y = 125;
		ImGui::SetCursorPos(pos); 
		ImGui::ArrowButton("##down", ImGuiDir_Down);
		if(ImGui::IsItemActive()){
			if (rot_is_tilt) g_camera->Rotate   ( 0, -dt);
			else             g_camera->ShiftTheta ( dt);
		}

		pos.x = 25;
		pos.y = 100;
		ImGui::SetCursorPos(pos); 
		ImGui::ArrowButton("##left", ImGuiDir_Left);
		if(ImGui::IsItemActive()){
			if (rot_is_tilt) g_camera->Rotate   ( dt,  0);
			else             g_camera->ShiftPhi   (-dt);
		}

		pos.x = 75;
		pos.y = 100;
		ImGui::SetCursorPos(pos); 
		ImGui::ArrowButton("##right", ImGuiDir_Right);
		if(ImGui::IsItemActive()){
			if (rot_is_tilt) g_camera->Rotate   (-dt,  0);
			else             g_camera->ShiftPhi   ( dt);
		}

		pos.x = 35;
		pos.y = 155;
		ImGui::SetCursorPos(pos); 
		if(ImGui::Button(_c("Camera dialog", "Forward"))) {
			g_camera->ResetCockpitDir();
		}
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR");
        ImGui::SeparatorText(_c("Camera dialog", "Field of view"));
		float fov=oapiCameraAperture()/RAD/0.5;
		if(ImGui::SliderFloat(_c("Camera dialog", "Field of view"), &fov, 10.0, 90.0, "%.1f")) {
			if(fov < 10.0f) fov = 10.0f;
			else if(fov>90.0f) fov = 90.0f;
			oapiCameraSetAperture(fov*RAD*0.5);
    }
    ImGui::EndChild();
}

void DlgCamera::AddCbodyNode(const CelestialBody *cbody) {
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool is_selected = m_SelectedTarget == cbody->Name();
    if (is_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;
    if(cbody->nSecondary()) {
        bool node_open = ImGui::TreeNodeEx(_c("Proper name", cbody->Name()), node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_SelectedTarget = cbody->Name();
        if(node_open) {
            for (int i = 0; i < cbody->nSecondary(); i++) {
                AddCbodyNode (cbody->Secondary(i));
            }
            ImGui::TreePop();
        }
    } else {
        ImGui::TreeNodeEx(_c("Proper name", cbody->Name()), node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_SelectedTarget = cbody->Name();
    }
}

void DlgCamera::DrawTarget() {
    const ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;

	ImGui::BeginChild("ChildL", ImVec2(250, 0), window_flags);
        ImGui::SeparatorText(_c("Camera dialog", "Available targets"));
		ImGui::BeginChild("ChildL_inner");
        for (int i = 0; i < g_psys->nStar(); i++)
            AddCbodyNode (g_psys->GetStar(i));

        if (ImGui::TreeNode(_c("Camera dialog", "Spaceports"))) {
                for (int i = 0; i < g_psys->nGrav(); i++) {
                    Body *obj = g_psys->GetGravObj (i);
                    if (obj->Type() != OBJTP_PLANET) continue;
                    Planet *planet = (Planet*)obj;
                    if (g_psys->nBase(planet) > 0) {
						if(ImGui::TreeNode(planet->Name())) {
							for (int j = 0; j < g_psys->nBase(planet); j++) {
								const char *name = g_psys->GetBase (planet,j)->Name();
								const bool is_selected = m_SelectedTarget == name;
								ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
								if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
								ImGui::TreeNodeEx(name, node_flags);
								if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
									m_SelectedTarget = name;
							}
							ImGui::TreePop();
						}
                    }
                }
                ImGui::TreePop();
        }
        if (ImGui::TreeNode(_c("Camera dialog", "Vessels"))) {
            for (int i = 0; i < g_psys->nVessel(); i++) {
                const char *name = g_psys->GetVessel(i)->Name();
                const bool is_selected = m_SelectedTarget == name;
                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
                ImGui::TreeNodeEx(name, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    m_SelectedTarget = name;
            }
            ImGui::TreePop();
        }
	    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR");
        ImGui::SeparatorText(_c("Camera dialog", "Selected target"));
        ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));
        ImGui::TextUnformatted(_c("Proper name", m_SelectedTarget.c_str()));
        if(ImGui::Button(_("Apply"), button_sz)) {
            Body *obj = g_psys->GetObj (m_SelectedTarget.c_str(), true);
            if (!obj) obj = g_psys->GetBase (m_SelectedTarget.c_str(), true);
            if ( obj) g_pOrbiter->SetView (obj, 1);
        }
        if(ImGui::Button(_c("Camera dialog", "Focus Cockpit"), button_sz)) {
            g_pOrbiter->SetView (g_focusobj, 0);
        }
        if(ImGui::Button(_c("Camera dialog", "Focus Extern"), button_sz)) {
            g_pOrbiter->SetView (g_focusobj, 1);
        }
    ImGui::EndChild();
}
void DlgCamera::DrawTrack() {
    ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;
    ImGui::BeginChild("ChildL", ImVec2(250, 0), window_flags);
    {
        ImGui::SeparatorText(_c("Camera dialog", "Moveable modes"));
        ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));

        if(ImGui::Button(_c("Camera dialog", "Target Relative"), button_sz)) {
            g_camera->SetTrackMode (CAMERA_TARGETRELATIVE);
        }
        if(ImGui::Button(_c("Camera dialog", "Absolute Direction"), button_sz)) {
            g_camera->SetTrackMode (CAMERA_ABSDIRECTION);
        }
        if(ImGui::Button(_c("Camera dialog", "Global Frame"), button_sz)) {
            g_camera->SetTrackMode (CAMERA_GLOBALFRAME);
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR");
        ImGui::SeparatorText(_c("Camera dialog", "Fixed modes"));
        ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));

        if(ImGui::Button(_c("Camera dialog", "Target From..."), button_sz)) {
			Body *obj = g_psys->GetObj (m_SelectedTarget.c_str(), true);
			if (!obj) obj = g_psys->GetBase (m_SelectedTarget.c_str(), true);
			if (obj && obj != g_camera->Target())
				g_camera->SetTrackMode (CAMERA_TARGETFROMOBJECT, obj);
        }
        if(ImGui::Button(_c("Camera dialog", "Target To..."), button_sz)) {
			Body *obj = g_psys->GetObj (m_SelectedTarget.c_str(), true);
			if (!obj) obj = g_psys->GetBase (m_SelectedTarget.c_str(), true);
			if (obj && obj != g_camera->Target())
				g_camera->SetTrackMode (CAMERA_TARGETTOOBJECT, obj);
        }
        for (int i = 0; i < g_psys->nStar(); i++)
            AddCbodyNode (g_psys->GetStar(i));

        if (ImGui::TreeNode(_c("Camera dialog", "Spaceports"))) {
            for (int i = 0; i < g_psys->nGrav(); i++) {
                Body *obj = g_psys->GetGravObj (i);
                if (obj->Type() != OBJTP_PLANET) continue;
                Planet *planet = (Planet*)obj;
                if (g_psys->nBase(planet) > 0) {
					if(ImGui::TreeNode(planet->Name())) {
						for (int j = 0; j < g_psys->nBase(planet); j++) {
							const char *name = g_psys->GetBase (planet,j)->Name();
							const bool is_selected = m_SelectedTarget == name;
							ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
							if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
							ImGui::TreeNodeEx(name, node_flags);
							if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
								m_SelectedTarget = name;
						}
			            ImGui::TreePop();
					}
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode(_c("Camera dialog", "Vessels"))) {
            for (int i = 0; i < g_psys->nVessel(); i++) {
                const char *name = g_psys->GetVessel(i)->Name();
                const bool is_selected = m_SelectedTarget == name;
                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
                ImGui::TreeNodeEx(name, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    m_SelectedTarget = name;
            }
            ImGui::TreePop();
        }
    ImGui::EndChild();
}
void DlgCamera::DrawGround() {
    const ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;;
    ImGui::BeginChild("ChildL", ImVec2(250, 0), window_flags);
    {
        ImGui::SeparatorText(_c("Camera dialog", "Ground Location"));
        for (int i = 0; i < g_psys->nGrav(); i++) {
            Body *obj = g_psys->GetGravObj (i);
            if (obj->Type() != OBJTP_PLANET) continue;
            Planet *planet = (Planet*)obj;


            if (planet->nGroundObserver() > 0) {
				if(ImGui::TreeNode(_c("Proper name", planet->Name()))) {
					for (int j = 0; j < planet->nGroundObserver(); j++) {
						const GROUNDOBSERVERSPEC *go = planet->GetGroundObserver (j);
						std::string name = std::string(go->site) + "-" + go->addr;
						const bool is_selected = m_SelectedSite == name;
						ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
						if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
						ImGui::TreeNodeEx(name.c_str(), node_flags);
						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
							m_SelectedSite = name;
							m_SitePlanet = planet->Name();
							sprintf(longitude, "%+0.6f", go->lng * 180.0/PI);
							sprintf(latitude, "%+0.6f", go->lat * 180.0/PI);
							sprintf(altitude, "%0.4g", go->alt);
						}
					}
					ImGui::TreePop();
				}
            }
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR");
        ImGui::SeparatorText(_c("Camera dialog", "Coordinates"));
        ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));
        ImGui::InputText(_("Longitude"), longitude, 64, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputText(_("Latitude"),  latitude,  64, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputText(_("Altitude"),  altitude,  64, ImGuiInputTextFlags_CharsDecimal);

        ImGui::InputText(_c("Camera dialog", "Follow Terrain"),  followterrain,  64, ImGuiInputTextFlags_CharsDecimal);
        
        if(ImGui::Button(_c("Camera dialog", "Current"), button_sz)) {
            const Planet *planet = g_psys->GetPlanet (m_SitePlanet.c_str());
            if(!planet) {
                planet = g_camera->ProxyPlanet();
                m_SitePlanet = planet->Name();
            }
            if (planet) {
                double lng, lat, alt;
                planet->GlobalToEquatorial (*g_camera->GPosPtr(), lng, lat, alt);
                alt -= planet->Size() + planet->Elevation (lng, lat);
                sprintf(longitude, "%+0.6f", lng * 180.0/PI);
                sprintf(latitude, "%+0.6f", lat * 180.0/PI);
                sprintf(altitude, "%0.4g", alt);
            }
            SetCurrentGroundpos();
        }
        if(ImGui::Button(_("Apply"), button_sz)) {
            ApplyObserver();
        }

        ImGui::Checkbox(_c("Camera dialog", "Target lock"), &m_TargetLock);
    ImGui::EndChild();
}

void DlgCamera::ApplyObserver() {
	Planet *planet = g_psys->GetPlanet (m_SitePlanet.c_str());
	if (!planet) return;

	double lng, lat, alt = 1.7;
	bool ok;
	ok = (sscanf (longitude, "%lf", &lng) == 1);
	ok = ok && (sscanf (latitude, "%lf", &lat) == 1);
	ok = ok && (sscanf (altitude, "%lf", &alt) == 1);

	double altlimit;
	if (sscanf (followterrain, "%lf", &altlimit))
		g_camera->SetGroundObserver_TerrainLimit (std::max(1.0,altlimit));

	if (ok)
		g_camera->SetGroundMode (CAMERA_GROUNDOBSERVER, planet, lng*RAD, lat*RAD, alt, NULL);

}

void DlgCamera::SetCurrentGroundpos() {
	if (g_camera->GetExtMode() == CAMERA_GROUNDOBSERVER) return; // nothing to do
	ApplyObserver ();

	if (!m_TargetLock) {
		// force lock to target to initialise camera direction
		g_camera->SetGroundObserver_TargetLock (true);
		g_camera->SetGroundObserver_TargetLock (false);
	}
	double alt;
	if (sscanf (followterrain, "%lf", &alt))
		g_camera->SetGroundObserver_TerrainLimit (std::max(1.0,alt));
}

void DlgCamera::DrawPreset() {
    ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;;;
    static float sz1 = 0.0;
    float sz2;
   // ImGui::Splitter(true, 0.5f, 8.0f, &sz1, &sz2, 8, 8, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("ChildL", ImVec2(250, 0), window_flags);
    {
        if (ImGui::BeginListBox("##listbox preset", ImVec2(ImGui::GetContentRegionAvail().x*0.99, ImGui::GetContentRegionAvail().y*0.99)))
        {
            for (int i = 0; i < g_camera->nPreset(); i++) {
                char buf[256];
                g_camera->GetPreset(i)->GetDescr (buf, 256);

                const bool is_selected = (m_SelectedPreset == i);
                ImGui::PushID(i);
                if (ImGui::Selectable(buf, is_selected))
                    m_SelectedPreset = i;
                ImGui::PopID();
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
            //ImGui::Text("Selected %d", m_SelectedPreset);
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR");
        ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));
        if(ImGui::Button(_("Add"), button_sz)) {
            g_camera->AddPreset();
        }
        if(ImGui::Button(_("Delete"), button_sz)) {
            g_camera->DelPreset(m_SelectedPreset);
        }
        if(ImGui::Button(_("Clear"), button_sz)) {
            g_camera->ClearPresets();
        }
        if(ImGui::Button(_c("Camera dialog", "Recall"), button_sz)) {
            g_camera->RecallPreset (m_SelectedPreset);
        }

    ImGui::EndChild();

}
