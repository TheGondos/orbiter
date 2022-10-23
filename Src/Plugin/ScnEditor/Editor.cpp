// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//              ORBITER MODULE: Scenario Editor
//                  Part of the ORBITER SDK
//
// Editor.cpp
//
// Implementation of ScnEditor class and editor tab subclasses
// derived from ScnEditorTab.
// ==============================================================

#include "Orbitersdk.h"
#include "Editor.h"
#include "font_awesome_5.h"
#include <stdio.h>
#include <imgui.h>
#include <imgui-knobs.h>
#include <filesystem>
namespace fs = std::filesystem;

extern ScnEditor *g_editor;

// ==============================================================
// Local prototypes
// ==============================================================

void OpenDialog (void *context);
void Crt2Pol (VECTOR3 &pos, VECTOR3 &vel);
void Pol2Crt (VECTOR3 &pos, VECTOR3 &vel);

static double lengthscale[4] = {1.0, 1e-3, 1.0/AU, 1.0};
static double anglescale[2] = {DEG, 1.0};

// ==============================================================
// ScnEditor class definition
// ==============================================================

ScnEditor::ScnEditor (): GUIElement("Scenario Editor", "ScnEditor")
{
	hEdLib = nullptr;
	dwCmd = oapiRegisterCustomCmd (
		"Scenario Editor",
		"Create, delete and configure spacecraft",
		::OpenDialog, this);

	m_preview = nullptr;
	m_currentVessel = nullptr;
	oe.frm = FRAME_ECL;
	vecState.frm = 0;
	vecState.crd = 0;
	memset(m_newVesselName, 0 , sizeof(m_newVesselName));
	aRot = {0,0,0};
	aVel = {0,0,0};
	m_customTabs = nullptr;
	OrbitalMode = PROP_ORBITAL_FIXEDSTATE;
	SOrbitalMode = PROP_SORBITAL_FIXEDSTATE;
	loc.last_desc = &loc.pad_desc;
}

ScnEditor::~ScnEditor ()
{
	CloseDialog();
	oapiUnregisterCustomCmd (dwCmd);
}

void ScnEditor::OpenDialog ()
{
	mjd = oapiGetSimMJD();
	date = *mjddate(mjd);
	oapiOpenDialog (this);
}

void ScnEditor::CloseDialog ()
{
	oapiCloseDialog (this);

	if (hEdLib) {
		oapiModuleUnload (hEdLib);
		hEdLib = nullptr;
		m_customTabs = nullptr;
	}
}

void ScnEditor::DrawConfigs(const char *path) {
    std::vector<fs::path> directories;
    for (auto &file : fs::directory_iterator(path)) {
        if(file.path() != "." && file.path() != "..") {
            if (file.is_directory()) {
                directories.push_back(file.path());
            }
        }
    }

    std::sort(directories.begin(), directories.end());//, alphanum_sort);
    for(auto &p: directories) {
        const bool is_selected = m_SelectedDirectory == p;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;//ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
        
        bool clicked = false;
        if(ImGui::TreeNodeEx(p.stem().c_str(), node_flags)) {
            clicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
            DrawConfigs(p.c_str());
            ImGui::TreePop();
        } else {
            clicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
        }
        if (clicked) {
            m_SelectedConfig.clear();
            m_SelectedDirectory = p;
        }
    }

    std::vector<fs::path> configs;
    for (auto &file : fs::directory_iterator(path)) {
        if(file.path().extension() == ".cfg") {
            if (file.is_regular_file()) {
				bool skip = true;
				FILEHANDLE hFile = oapiOpenFile (file.path().c_str(), FILE_IN);
				if (hFile) {
					bool b;
					skip = (oapiReadItem_bool (hFile, "EditorCreate", b) && !b);
					oapiCloseFile (hFile, FILE_IN);
				}
				if (skip) continue;

                configs.push_back(file.path());
            }
        }
    }

    std::sort(configs.begin(), configs.end());//, alphanum_sort);
    for(auto &p: configs) {
        const bool is_selected = m_SelectedConfig == p;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
        char node_text[256];
        sprintf(node_text, ICON_FA_PAPER_PLANE" %s", p.stem().c_str());
        ImGui::TreeNodeEx(node_text, node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedConfig = p;
            m_SelectedDirectory.clear();

			FILEHANDLE hFile = oapiOpenFile (p.c_str(), FILE_IN);
			if (hFile) {
				char imagename[256];
				if (oapiReadItem_string (hFile, "ImageBmp", imagename)) {
					if(m_preview) {
						oapiReleaseTexture(m_preview);
						m_preview = nullptr;
					}
					m_preview = oapiLoadTexture(imagename);
				}
				if (oapiReadItem_string (hFile, "ClassName", imagename)) {
					m_classname = imagename;
				} else {
					m_classname = p.stem();
				}
				oapiCloseFile (hFile, FILE_IN);
			}
        }
    }
}

void ScnEditor::CreateVessel() {
	if (oapiGetVesselByName (m_newVesselName)) {
		oapiAddNotification(OAPINOTIF_ERROR, "Cannot create new vessel", "Vessel name already in use");
		return;
	}

	// define an arbitrary status (the user will have to edit this after creation)
	VESSELSTATUS2 vs;
	memset (&vs, 0, sizeof(vs));
	vs.version = 2;
	vs.rbody = oapiGetGbodyByName ("Earth");
	if (!vs.rbody) vs.rbody = oapiGetGbodyByIndex (0);
	double rad = 1.1 * oapiGetSize (vs.rbody);
	double vel = sqrt (GGRAV * oapiGetMass (vs.rbody) / rad);
	vs.rpos = _V(rad,0,0);
	vs.rvel = _V(0,0,vel);
	m_currentVessel = oapiCreateVesselEx (m_newVesselName, m_classname.c_str(), &vs);
	oapiSetFocusObject (m_currentVessel);
//	if (SendDlgItemMessage (hTab, IDC_CAMERA, BM_GETSTATE, 0, 0) == BST_CHECKED)
	oapiCameraAttach (m_currentVessel, 1);
	m_newVesselName[0] = '\0';
	ReloadVessel();
}

void ScnEditor::VesselCreatePopup() {
	ImGui::Text("New vessel");
	ImGui::InputText("Name", m_newVesselName, IM_ARRAYSIZE(m_newVesselName));

    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y - 30), true);
	DrawConfigs("Config/Vessels");
    ImGui::EndChild();
	ImGui::SameLine();
    ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 30), true);
	if(m_preview) {
        const ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
        const ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
        const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
        const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white
		int w,h;
        oapiIncrTextureRef(m_preview);
		oapiGetTextureSize(m_preview, &w, &h);
		float ratio = (float)h/(float)w;
        ImGui::ImageButton(m_preview, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * ratio), uv_min, uv_max, 0, tint_col, border_col);
	}
    ImGui::EndChild();

	const bool ready = strlen(m_newVesselName) != 0 && !m_SelectedConfig.empty();

	if(!ready)
		ImGui::BeginDisabled();

	if(ImGui::Button("Create")) {
		CreateVessel();
		ImGui::CloseCurrentPopup();
	}
	if(!ready)
		ImGui::EndDisabled();

	ImGui::SameLine();
	if(ImGui::Button("Cancel")) {
		ImGui::CloseCurrentPopup();
	}
}

bool ScnEditor::DrawShipList(OBJHANDLE &selected)
{
	bool ret = false;
	if (ImGui::TreeNodeEx("Vessels", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < oapiGetVesselCount(); i++) {
			OBJHANDLE hV = oapiGetVesselByIndex (i);
			VESSEL *vessel = oapiGetVesselInterface (hV);
			const char *name = vessel->GetName();
			const bool is_selected = selected == hV;
			
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
			ImGui::TreeNodeEx(name, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				selected = hV;
				ReloadVessel();
				ret = true;
				//Don't do this for now because it can create lots of controller profiles :(
				//oapiSetFocusObject (m_currentVessel);
				//oapiCameraAttach (m_currentVessel, 1);
			}
		}
		ImGui::TreePop();
	}
	return ret;
}

void ScnEditor::DrawTabs ()
{
	const char *tabs[] = {
		ICON_FA_SATELLITE" Orbital elements", ICON_FA_LOCATION_ARROW" State vectors", ICON_FA_COMPASS" Rotation",
		ICON_FA_MAP_MARKER_ALT" Location", ICON_FA_LIFE_RING" Docking", ICON_FA_GAS_PUMP" Propellant"
	};

	void (ScnEditor::* func[])() = {
		&ScnEditor::DrawOrbitalElements, &ScnEditor::DrawStateVectors, &ScnEditor::DrawRotation,
		&ScnEditor::DrawLocation, &ScnEditor::DrawDocking, &ScnEditor::DrawPropellant
	};


	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		for(size_t i = 0; i<sizeof(tabs)/sizeof(tabs[0]);i++) {
			if(ImGui::BeginTabItem(tabs[i])) {
				(this->*func[i])();
				ImGui::EndTabItem();
			}
		}
		if(m_customTabs && m_currentVessel)
			m_customTabs(m_currentVessel);

		ImGui::EndTabBar();
	}
}

void ScnEditor::ReloadVessel()
{
	VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
	OBJHANDLE hRef = vessel->GetGravityRef();// oapiGetGbodyByName (m_selectedReference.c_str());

	char cbuf[64];
	oapiGetObjectName (hRef, cbuf, 64);
	oe.m_selectedReference = cbuf;
	vecState.m_selectedReference = cbuf;
	loc.planet = cbuf;
	vessel->GetElements(hRef, oe.el, &oe.prm, oe.elmjd, oe.frm);
	
	oapiGetRelativePos (m_currentVessel, hRef, &vecState.pos);
	oapiGetRelativeVel (m_currentVessel, hRef, &vecState.vel);

	oapiGetPlanetObliquityMatrix (hRef, &vecState.rotFixed);
	oapiGetRotationMatrix (hRef, &vecState.rotRotating);
	vessel->GetGlobalOrientation (aRot);
	aRot.x*=DEG;
	aRot.y*=DEG;
	aRot.z*=DEG;

	vessel->GetAngularVel (aVel);
	aVel.x*=DEG;
	aVel.y*=DEG;
	aVel.z*=DEG;

	LoadVesselLibrary(vessel);
}

bool ScnEditor::DrawCBodies(std::string &ref, const char *name) {
	bool ret = false;
	std::vector<std::string> bodies;
	for (int n = 0; n < oapiGetGbodyCount(); n++) {
		char cbuf[256];
		oapiGetObjectName (oapiGetGbodyByIndex (n), cbuf, 256);
		bodies.push_back(cbuf);
	}
	if(bodies.empty()) return false;
	std::sort(bodies.begin(), bodies.end());
	if(ref.empty()) ref = bodies[0];

	ImGui::PushItemWidth(100);
	if(ImGui::BeginCombo(name, ref.c_str())) {
		for (auto &body: bodies) {
			if (ImGui::Selectable(body.c_str(), body == ref)) {
				ref = body;
				ret = true;
			}
		}
        ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	return ret;
}

void ScnEditor::DrawBases(OBJHANDLE hPlanet, std::string &ref) {
	std::vector<std::string> bases;
	for (int n = 0; n < oapiGetBaseCount(hPlanet); n++) {
		char cbuf[256];
		oapiGetObjectName (oapiGetBaseByIndex (hPlanet, n), cbuf, 256);
		bases.push_back(cbuf);
	}
	if(bases.empty()) return;
	std::sort(bases.begin(), bases.end());
	if(ref.empty()) ref = bases[0];

	ImGui::BeginGroupPanel("Surface bases");
	ImGui::PushItemWidth(150);
	if(ImGui::BeginCombo("##surfacebase", ref.c_str())) {
		for (auto &body: bases) {
			if (ImGui::Selectable(body.c_str(), body == ref)) {
				ref = body;
				loc.pad_desc.clear();
				loc.runway_desc.clear();
			}
		}
        ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	if(!loc.base.empty()) {
		OBJHANDLE hBase = oapiGetBaseByName(hPlanet, loc.base.c_str());
		if(hBase) {
//			DrawPads(hBase, loc.pad);
//			DrawRunways(hBase, loc.runway, loc.runway_endpoint);
			DrawBaseLocations(hBase);
		}
	}

	if(!loc.base.empty()) {
		ImGui::SameLine();
		if(ImGui::Button("Preset position")) {
			OBJHANDLE hPlanet = oapiGetObjectByName(loc.planet.c_str());
			OBJHANDLE hBase = oapiGetBaseByName(hPlanet, loc.base.c_str());
			if(loc.pad != -1) {
				oapiGetBasePadEquPos (hBase, loc.pad, &loc.longitude, &loc.latitude);
			} else if(loc.runway != -1) {
				oapiGetBaseRwyEquPos(hBase, loc.runway, loc.runway_endpoint, &loc.heading, &loc.longitude, &loc.latitude);
				loc.heading *= DEG;
			} else {
				oapiGetBaseEquPos (hBase, &loc.longitude, &loc.latitude);
			}
			loc.longitude *= DEG;
			loc.latitude *= DEG;
		}
	}

	ImGui::EndGroupPanel();
}

void ScnEditor::DrawBaseLocations(OBJHANDLE hBase) {
	int nrwy = oapiGetBaseRwyCount (hBase);
	int npad = oapiGetBasePadCount (hBase);
	if(nrwy + npad == 0) return;

	if(ImGui::BeginCombo("##Locations", loc.last_desc->c_str())) {
		ImGui::PushItemWidth(100);
		if(nrwy) {
			char cbuf[16];
			for (int n = 0; n < nrwy; n++) {
				sprintf (cbuf, "%d", n + 1);
				double dummy;
				double dir1;
				double dir2;
				oapiGetBaseRwyEquPos (hBase, n, 1, &dir1, &dummy, &dummy);
				oapiGetBaseRwyEquPos (hBase, n, 2, &dir2, &dummy, &dummy);
				int appr1 = (int)(dir1 * DEG * 0.1) % 36;
				int appr2 = (int)(dir2 * DEG * 0.1) % 36;
				char cbuf1[16];
				char cbuf2[16];
				sprintf(cbuf1, "Runway %d", appr1);
				sprintf(cbuf2, "Runway %d", appr2);

				if (ImGui::Selectable(cbuf1, *loc.last_desc == cbuf1)) {
					loc.runway = n;
					loc.runway_endpoint = 1;
					loc.runway_desc = cbuf1;
					loc.last_desc = &loc.runway_desc;
					loc.pad = -1;
				}
				if (ImGui::Selectable(cbuf2, *loc.last_desc == cbuf2)) {
					loc.runway = n;
					loc.runway_endpoint = 2;
					loc.runway_desc = cbuf2;
					loc.last_desc = &loc.runway_desc;
					loc.pad = -1;
				}
			}
		}
		
		if(npad) {
			char cbuf[16];
			for (int n = 0; n < npad; n++) {
				sprintf (cbuf, "Pad %d", n + 1);
				if (ImGui::Selectable(cbuf, *loc.last_desc == cbuf)) {
					loc.pad_desc = cbuf;
					loc.pad = n;
					loc.last_desc = &loc.pad_desc;
					loc.runway = -1;
				}
			}
		}
		ImGui::PopItemWidth();
		ImGui::EndCombo();
	}
}
/*
bool ScnEditor::DrawRunways(OBJHANDLE hBase, std::string &ref, int &endpoint) {
	bool ret = false;
	int n, nrwy = oapiGetBaseRwyCount (hBase);
	if(nrwy) {
		if(ref.empty()) ref = "1";
		ImGui::Text("Runway : ");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		if(ImGui::BeginCombo("##Runways", ref.c_str())) {
			char cbuf[16];
			for (n = 0; n < nrwy; n++) {
				sprintf (cbuf, "%d", n + 1);
				double dummy;
				double dir1;
				double dir2;
				oapiGetBaseRwyEquPos (hBase, n, 1, &dir1, &dummy, &dummy);
				oapiGetBaseRwyEquPos (hBase, n, 2, &dir2, &dummy, &dummy);
				dir1*=(DEG * 0.1);
				dir2*=(DEG * 0.1);
				int appr1 = dir1;
				int appr2 = dir2;
				char cbuf1[8];
				char cbuf2[8];
				sprintf(cbuf1, "Runway %d", appr1);
				sprintf(cbuf2, "Runway %d", appr2);

				if (ImGui::Selectable(cbuf1, cbuf == ref && endpoint == 1)) {
					endpoint = 1;
					ref = cbuf;
					ret = true;
				}
				if (ImGui::Selectable(cbuf2, cbuf == ref && endpoint == 2)) {
					endpoint = 2;
					ref = cbuf;
					ret = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}
	return ret;
}
bool ScnEditor::DrawPads(OBJHANDLE hBase, std::string &ref) {
	bool ret = false;
	int n, npad = oapiGetBasePadCount (hBase);
	if(ref.empty()) ref = "1";
	if(npad) {
		ImGui::SameLine();
		ImGui::Text("Pad : ");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		if(ImGui::BeginCombo("##Pads", ref.c_str())) {
			char cbuf[16];
			for (n = 1; n <= npad; n++) {
				sprintf (cbuf, "%d", n);
				if (ImGui::Selectable(cbuf, cbuf == ref)) {
					ref = cbuf;
					ret = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}
	return ret;
}
*/

void ScnEditor::DrawOrbitalElements()
{
	if(!m_currentVessel) return;
	ImGui::BeginGroupPanel("Reference");
		ImGui::BeginGroupPanel("Orbit", ImVec2(ImGui::GetContentRegionAvail().x * 0.33f, 0));
		DrawCBodies(oe.m_selectedReference, "##Orbit");
		ImGui::EndGroupPanel();
		ImGui::SameLine();
		ImGui::BeginGroupPanel("Frame", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
		bool frmChanged = ImGui::RadioButton("Ecliptic", &oe.frm, 0); ImGui::SameLine();
		frmChanged     |= ImGui::RadioButton("Equatorial", &oe.frm, 1);
		if(frmChanged) ReloadVessel();
		ImGui::EndGroupPanel();
		ImGui::SameLine();
		ImGui::BeginGroupPanel("Epoch");
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("MJD", &oe.elmjd, 0.0, 0.0, "%g");
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();

	ImGui::EndGroupPanel();

	static int smaUnit;
	VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
	OBJHANDLE hRef = vessel->GetGravityRef();
	lengthscale[3] = 1.0/oapiGetSize (hRef);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginGroupPanel("Osculating elements", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
		ImGui::BeginGroupPanel("Unit");
		ImGui::RadioButton("m", &smaUnit,0); ImGui::SameLine();
		ImGui::RadioButton("km", &smaUnit,1); ImGui::SameLine();
		ImGui::RadioButton("AU", &smaUnit,2); ImGui::SameLine();
		ImGui::RadioButton("Planet radius", &smaUnit,3);
		ImGui::EndGroupPanel();

		ImGui::PushItemWidth(100);
		oe.el.a*=lengthscale[smaUnit];
		ImGui::InputDouble("Semi-major Axis [SMa]", &oe.el.a, 0.0, 0.0, "%g");
		oe.el.a/=lengthscale[smaUnit];
		ImGui::InputDouble("Eccentricity [Ecc]", &oe.el.e, 0.0, 0.0, "%g");
		oe.el.i*=DEG;
		ImGui::InputDouble("Inclination [Inc]°", &oe.el.i, 0.0, 0.0, "%g");
		oe.el.i/=DEG;
		oe.el.theta*=DEG;
		ImGui::InputDouble("Longitude of ascending node [LAN]°", &oe.el.theta, 0.0, 0.0, "%g");
		oe.el.theta/=DEG;
		oe.el.omegab*=DEG;
		ImGui::InputDouble("Longitude of periapsis [LPe]°", &oe.el.omegab, 0.0, 0.0, "%g");
		oe.el.omegab/=DEG;
		oe.el.L*=DEG;
		ImGui::InputDouble("Mean longitude at epoch [eps]°", &oe.el.L, 0.0, 0.0, "%g");
		oe.el.L/=DEG;
		ImGui::PopItemWidth();
	ImGui::EndGroupPanel();
	ImGui::SameLine();
	ImGui::BeginGroupPanel("Secondary parameters", ImVec2(ImGui::GetContentRegionAvail().x, 0));
		bool closed = (oe.el.e < 1.0); // closed orbit?

		ImGui::Text ("Periapsis : %g m", oe.prm.PeD);
		ImGui::Text ("PeT : %g s", oe.prm.PeT);
		ImGui::Text ("MnA : %0.3f °", oe.prm.MnA*DEG);
		ImGui::Text ("TrA : %0.3f °", oe.prm.TrA*DEG);
		ImGui::Text ("MnL : %0.3f °", oe.prm.MnL*DEG);
		ImGui::Text ("TrL : %0.3f °", oe.prm.TrL*DEG);
		if (closed) {
			ImGui::Text ("Period : %g s", oe.prm.T);
			ImGui::Text ("Apoapsis : %g m", oe.prm.ApD);
			ImGui::Text ("ApT : %g s", oe.prm.ApT);
		}
	ImGui::EndGroupPanel();

	if(ImGui::Button("Apply")) {
		OBJHANDLE hRef = oapiGetGbodyByName (oe.m_selectedReference.c_str());
		if (hRef) {
			oe.el.a = fabs (oe.el.a);
			if (oe.el.e > 1.0) oe.el.a = -oe.el.a;
			VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);

			if (!vessel->SetElements (hRef, oe.el, &oe.prm, oe.elmjd, oe.frm)) {
				oapiAddNotification(OAPINOTIF_ERROR, "Failed to set orbital elements", "Trajectory is inside the body");
			}
		}
	}
}
void ScnEditor::DrawStateVectors()
{
	if(!m_currentVessel) return;
	ImGui::BeginGroupPanel("Reference");
		ImGui::BeginGroupPanel("Orbit", ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, 0));
		DrawCBodies(vecState.m_selectedReference, "##Orbit");
		ImGui::EndGroupPanel();

		ImGui::SameLine();
		ImGui::BeginGroupPanel("Frame", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
		ImGui::RadioButton("Ecliptic", &vecState.frm ,0);
		ImGui::SameLine();
		ImGui::RadioButton("Equator (fixed)", &vecState.frm ,1);
		ImGui::SameLine();
		ImGui::RadioButton("Equator (rotating)", &vecState.frm ,2);
		ImGui::EndGroupPanel();

		ImGui::SameLine();
		ImGui::BeginGroupPanel("Coordinates");	
		ImGui::RadioButton("Cartesian", &vecState.crd, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Polar", &vecState.crd, 1);
		ImGui::EndGroupPanel();
	ImGui::EndGroupPanel();

	VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
	OBJHANDLE hRef = oapiGetObjectByName(vecState.m_selectedReference.c_str());

	VECTOR3 pos = vecState.pos, vel = vecState.vel;
	if (vecState.frm) {
		MATRIX3 rot;
		if (vecState.frm == 1) rot = vecState.rotFixed;
		else          rot = vecState.rotRotating;
		pos = tmul (rot, pos);
		vel = tmul (rot, vel);
	}
	// map cartesian -> polar coordinates
	if (vecState.crd) {
		Crt2Pol (pos, vel);
		pos.data[1] *= DEG; pos.data[2] *= DEG;
		vel.data[1] *= DEG; vel.data[2] *= DEG;
	}
	// in the rotating reference frame we need to subtract the angular
	// velocity of the planet
	if (vecState.frm == 2) {
		double T = oapiGetPlanetPeriod (hRef);
		if (vecState.crd) {
			vel.data[1] -= 360.0/T;
		} else { // map back to cartesian
			double r   = std::hypot (pos.x, pos.z);
			double phi = atan2 (pos.z, pos.x);
			double v   = 2.0*PI*r/T;
			vel.x     += v*sin(phi);
			vel.z     -= v*cos(phi);
		}
	}
	if(vecState.crd) {
		//Polar
		ImGui::BeginGroupPanel("Position", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("Radius (m)", &pos.x, 0.0, 0.0, "%g");
		ImGui::InputDouble("Longitude (°)", &pos.y, 0.0, 0.0, "%g");
		ImGui::InputDouble("Latitude (°)", &pos.z, 0.0, 0.0, "%g");
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();
		ImGui::SameLine();
		ImGui::BeginGroupPanel("Velocity");
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("dRadius/dt (m/s)", &vel.x, 0.0, 0.0, "%g");
		ImGui::InputDouble("dLongitude/dt (°/s)", &vel.y, 0.0, 0.0, "%g");
		ImGui::InputDouble("dLatitude/dt (°/s)", &vel.z, 0.0, 0.0, "%g");
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();
	} else {
		//Cartesian
		ImGui::BeginGroupPanel("Position", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("x (m)", &pos.x, 0.0, 0.0, "%g");
		ImGui::InputDouble("y (m)", &pos.y, 0.0, 0.0, "%g");
		ImGui::InputDouble("z (m)", &pos.z, 0.0, 0.0, "%g");
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();
		ImGui::SameLine();
		ImGui::BeginGroupPanel("Velocity");
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("dx/dt (m/s)", &vel.x, 0.0, 0.0, "%g");
		ImGui::InputDouble("dy/dt (m/s)", &vel.y, 0.0, 0.0, "%g");
		ImGui::InputDouble("dz/dt (m/s)", &vel.z, 0.0, 0.0, "%g");
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();
	}

	// in the rotating reference frame we need to add the angular
	// velocity of the planet
	MATRIX3 rot;
	VECTOR3 refpos, refvel;
	VESSELSTATUS vs;
	if (vecState.frm == 2) {
		double T = oapiGetPlanetPeriod (hRef);
		if (vecState.crd) {
			vel.data[1] += 360.0/T;
		} else { // map back to cartesian
			double r   = std::hypot (pos.x, pos.z);
			double phi = atan2 (pos.z, pos.x);
			double v   = 2.0*PI*r/T;
			vel.x     -= v*sin(phi);
			vel.z     += v*cos(phi);
		}
	}
	// map polar -> cartesian coordinates
	if (vecState.crd) {
		pos.data[1] *= RAD, pos.data[2] *= RAD;
		vel.data[1] *= RAD, vel.data[2] *= RAD;
		Pol2Crt (pos, vel);
	}
	// map from celestial/equatorial frame of reference
	if (vecState.frm) {
		if (vecState.frm == 1) rot = vecState.rotFixed;
		else          rot = vecState.rotRotating;
		pos = mul (rot, pos);
		vel = mul (rot, vel);
	}
	vecState.pos = pos;
	vecState.vel = vel;
	if(ImGui::Button("Apply")) {
		// change reference in case the selected reference object is
		// not the same as the VESSELSTATUS reference
		vessel->GetStatus (vs);
		oapiGetGlobalPos (hRef, &refpos);     pos += refpos;
		oapiGetGlobalVel (hRef, &refvel);     vel += refvel;
		oapiGetGlobalPos (vs.rbody, &refpos); pos -= refpos;
		oapiGetGlobalVel (vs.rbody, &refvel); vel -= refvel;
		if (vs.status != 0) { // enforce freeflight mode
			vs.status = 0;
			vessel->GetRotationMatrix (rot);
			vs.arot.x = atan2(rot.m23, rot.m33);
			vs.arot.y = -asin(rot.m13);
			vs.arot.z = atan2(rot.m12, rot.m11);
			vessel->GetAngularVel(vs.vrot);
		}
		veccpy (vs.rpos, pos);
		veccpy (vs.rvel, vel);
		vessel->DefSetState (&vs);
	}
	ImGui::SameLine();
	if(ImGui::Button("Set from...")) {
		ImGui::OpenPopup("CopyVesselSV");
	}
	if (ImGui::BeginPopup("CopyVesselSV"))
	{
		OBJHANDLE h;
		if(DrawShipList(h)) {
			VESSELSTATUS2 vsSrc;
			VESSELSTATUS2 vsDst;
			memset(&vsDst, 0, sizeof(vsDst));
			memset(&vsSrc, 0, sizeof(vsSrc));
			vsSrc.version = 2;
			vsDst.version = 2;
			VESSEL *vesselSrc = oapiGetVesselInterface (h);
			vesselSrc->GetStatusEx(&vsSrc);
			vessel->GetStatusEx(&vsDst);

			vsDst.arot = vsSrc.arot;
			vsDst.rbody = vsSrc.rbody;
			vsDst.rpos = vsSrc.rpos;
			vsDst.rvel = vsSrc.rvel;
			vsDst.status = vsSrc.status;
			vsDst.surf_hdg = vsSrc.surf_hdg;
			vsDst.surf_lat = vsSrc.surf_lat;
			vsDst.surf_lng = vsSrc.surf_lng;
			vsDst.vrot = vsSrc.vrot;

			vessel->DefSetStateEx (&vsDst);
			ReloadVessel();

			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ScnEditor::DrawRotation()
{
	if(!m_currentVessel) return;
	ImGui::PushItemWidth(100);
	ImGui::BeginGroupPanel("Euler angles", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0));
	ImGui::InputDouble("alpha", &aRot.x, 0.0, 0.0, "%g");
	ImGui::InputDouble("beta", &aRot.y, 0.0, 0.0, "%g");
	ImGui::InputDouble("gamma", &aRot.z, 0.0, 0.0, "%g");
	ImGui::EndGroupPanel();
	ImGui::SameLine();
	ImGui::BeginGroupPanel("Angular velocity");
	ImGui::InputDouble("Pitch (°/s)", &aVel.x, 0.0, 0.0, "%g");
	ImGui::InputDouble("Yaw (°/s)", &aVel.y, 0.0, 0.0, "%g");
	ImGui::InputDouble("Bank (°/s)", &aVel.z, 0.0, 0.0, "%g");
	ImGui::EndGroupPanel();
	ImGui::PopItemWidth();

	if(ImGui::Button("Apply")) {
		VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
		VECTOR3 avel = aVel;
		VECTOR3 arot = aRot;
		avel.x*=RAD;
		avel.y*=RAD;
		avel.z*=RAD;
		vessel->SetAngularVel (avel);

		arot.x*=RAD;
		arot.y*=RAD;
		arot.z*=RAD;
		vessel->SetGlobalOrientation (arot);
	}
	ImGui::SameLine();
	if(ImGui::Button("Kill rotation")) {
		VECTOR3 avel{0,0,0};
		VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
		vessel->SetAngularVel (avel);
	}
}
void ScnEditor::DrawLocation()
{
	ImGui::BeginGroupPanel("Celestial body");
	if(DrawCBodies(loc.planet, "##Celestial body")) {
		loc.base.clear();
	}
	ImGui::EndGroupPanel();
	if(!loc.planet.empty()) {
		OBJHANDLE hPlanet = oapiGetObjectByName(loc.planet.c_str());
		DrawBases(hPlanet, loc.base);
	}

	ImGui::BeginGroupPanel("Position");
	ImGui::InputDouble("Longitude", &loc.longitude, 0.0, 0.0, "%f");
	ImGui::InputDouble("Latitude", &loc.latitude, 0.0, 0.0, "%f");
	ImGui::InputDouble("Heading (°)", &loc.heading, 0.0, 0.0, "%f");
	ImGui::EndGroupPanel();
	if(ImGui::Button("Apply")) {
		char cbuf[256];
		VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
		OBJHANDLE hRef = oapiGetGbodyByName (loc.planet.c_str());
		if (!hRef) return;
		VESSELSTATUS2 vs;
		memset (&vs, 0, sizeof(vs));
		vs.version = 2;
		vs.rbody = hRef;
		vs.status = 1; // landed
		vs.arot.x = 10; // use default touchdown orientation
		vs.surf_lng = loc.longitude * RAD;
		vs.surf_lat = loc.latitude * RAD;
		vs.surf_hdg = loc.heading * RAD;
		vessel->DefSetStateEx (&vs);
	}
}
void ScnEditor::DrawDocking()
{
	if(!m_currentVessel) return;
	VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
	int ndock = vessel->DockCount();
	if(ndock == 0) {
		ImGui::TextUnformatted("Vessel has no docking ports");
		return;
	}

	static int mode = 1;
	ImGui::RadioButton("Teleport the target vessel", &mode, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Teleport the current vessel", &mode, 2);

	if (ImGui::BeginTable("Docking Ports", 5, ImGuiTableFlags_Borders))
	{
		ImGui::TableSetupColumn("Port");
		ImGui::TableSetupColumn("IDS enabled");
		ImGui::TableSetupColumn("Frequency");
		ImGui::TableSetupColumn("Status");
		ImGui::TableSetupColumn("Action");
		ImGui::TableHeadersRow();
		for(int i = 0; i < ndock; i++) {
			ImGui::PushID(i);
			DOCKHANDLE hDock = vessel->GetDockHandle (i);
			NAVHANDLE hIDS = vessel->GetIDS (hDock);
			float freq = -1;
			if (hIDS) {
				freq = oapiGetNavFreq (hIDS);
			}
			bool b = freq != -1;
			ImGui::TableNextColumn(); ImGui::Text("%d", i + 1);
			ImGui::TableNextColumn(); 
			if(ImGui::Checkbox("##IDS", &b)) {
				vessel->EnableIDS(hDock, b);
			}
			if (hIDS) {
				ImGui::TableNextColumn();
				if(ImGui::InputFloat("##Frequency", &freq, 0, 0, "%.2f")) {
					int ch = (int)((freq-108.0)*20.0+0.5);
					int dch  = 0;
					ch = std::max(0, std::min (639, ch+dch));

					vessel->SetIDSChannel(hDock, ch);
				}
			} else {
				ImGui::TableNextColumn(); ImGui::Text("<none>");
			}

			OBJHANDLE hMate = vessel->GetDockStatus (hDock);
			if (hMate) { // dock is engaged
				char cbuf[128];
				oapiGetObjectName (hMate, cbuf, sizeof(cbuf));
				ImGui::TableNextColumn(); ImGui::Text("Docked with %s", cbuf);
				ImGui::TableNextColumn();
				if(ImGui::Button(ICON_FA_UNLINK" Undock")) {
					vessel->Undock (i);
				}
			} else {
				ImGui::TableNextColumn(); ImGui::Text("Free");
				ImGui::TableNextColumn();
				if(ImGui::Button(ICON_FA_LINK" Dock with...")) {
					ImGui::OpenPopup("DockWith");
				}
				if (ImGui::BeginPopup("DockWith"))
				{
					if (ImGui::TreeNodeEx("Vessels", ImGuiTreeNodeFlags_DefaultOpen)) {
						for (int v = 0; v < oapiGetVesselCount(); v++) {
							OBJHANDLE hV = oapiGetVesselByIndex (v);
							if(hV == m_currentVessel) continue;
							VESSEL *vesselTgt = oapiGetVesselInterface (hV);
							int ndock = vesselTgt->DockCount();
							if(ndock == 0) continue;
							const char *name = vesselTgt->GetName();

							if(ImGui::TreeNodeEx(name)) {
								for(int d = 0; d < ndock ; d++) {
									char cbuf[16];
									sprintf(cbuf,"Port %d",d+1);
									ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
									DOCKHANDLE hDock = vesselTgt->GetDockHandle (d);
									OBJHANDLE hMate = vesselTgt->GetDockStatus (hDock);
									if(hMate) ImGui::BeginDisabled();
									ImGui::TreeNodeEx(cbuf, node_flags);
									if(hMate) ImGui::EndDisabled();
									if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
										int res = vessel->Dock (hV, i, d, mode);
										switch(res) {
											case 1:
												oapiAddNotification(OAPINOTIF_ERROR, "Docking failed","Docking port on the vessel already in use");
												break;
											case 2:
												oapiAddNotification(OAPINOTIF_ERROR, "Docking failed","Docking port on the target vessel already in use");
												break;
											case 3:
												oapiAddNotification(OAPINOTIF_ERROR, "Docking failed","Target vessel already part of the vessel's superstructure");
												break;
											default:
												break;
										}
										ImGui::CloseCurrentPopup();
									}
								}
								ImGui::TreePop();
							}
						}
						ImGui::TreePop();
					}
					ImGui::EndPopup();
				}
			}
			ImGui::TableNextRow();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}
void ScnEditor::DrawPropellant()
{
	if(!m_currentVessel) return;
	VESSEL *vessel = oapiGetVesselInterface (m_currentVessel);
	int ntank = ntank = vessel->GetPropellantCount();
	if(ntank == 0) {
		ImGui::TextUnformatted("Vessel has no fuel tanks");
		return;
	}
	if (ImGui::BeginTable("Fuel tanks", ntank, ImGuiTableFlags_Borders, ImVec2(0.0f, 0.0f), 60.0f))
	{
		for(int tank = 0; tank < ntank; tank++) {
			PROPELLANT_HANDLE hP = vessel->GetPropellantHandleByIndex (tank);
			double m0 = vessel->GetPropellantMaxMass (hP);
			double m  = vessel->GetPropellantMass (hP);

			ImGui::TableNextColumn();
			if(m0 == 0.0) continue;

			float ratio = m / m0 * 100.0f;

			char cbuf[32];
			sprintf(cbuf, "Tank %d", tank + 1);

			if (ImGuiKnobs::Knob(cbuf, &ratio, 0.0f, 100.0f, 1.0f, "%.2f%%", ImGuiKnobVariant_WiperOnly, 50.0, ImGuiKnobFlags_DragHorizontal)) {
				// value was changed
				m = m0 * ratio / 100.0;
				vessel->SetPropellantMass(hP, m);
			}
		}
		ImGui::TableNextRow();
		for(int tank = 0; tank < ntank; tank++) {
			ImGui::TableNextColumn();
			PROPELLANT_HANDLE hP = vessel->GetPropellantHandleByIndex (tank);
			double m0 = vessel->GetPropellantMaxMass (hP);
			if(m0 == 0.0) continue;
			double m  = vessel->GetPropellantMass (hP);

			ImGui::SetNextItemWidth(60);
			ImGui::PushID(tank);
			if(ImGui::InputDouble("##fuelmass", &m, 0.0, 0.0, "%.1f")) {
				m = std::clamp(m, 0.0, m0);
				vessel->SetPropellantMass(hP, m);
			}
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::Text("/%.1fkg", m0);
		}
		ImGui::EndTable();
	}
	/*
void EditorTab_Propellant::SetLevel (double level, bool setall)
{
	char cbuf[256];
	int i, j;
	DWORD n, k, k0, k1;
	double m0;
	VESSEL *vessel = oapiGetVesselInterface (ed->hVessel);
	ntank = vessel->GetPropellantCount();
	if (!ntank) return;
	GetWindowText (GetDlgItem (hTab, IDC_EDIT1), cbuf, 256);
	i = sscanf (cbuf, "%d", &n);
	if (!i || --n >= ntank) return;
	if (setall) k0 = 0, k1 = ntank;
	else        k0 = n, k1 = n+1;
	for (k = k0; k < k1; k++) {
		PROPELLANT_HANDLE hP = vessel->GetPropellantHandleByIndex (k);
		m0 = vessel->GetPropellantMaxMass (hP);
		vessel->SetPropellantMass (hP, level*m0);
		if (k == n) {
			sprintf (cbuf, "%f", level);
			SetWindowText (GetDlgItem (hTab, IDC_EDIT2), cbuf);
			sprintf (cbuf, "%0.2f", level*m0);
			SetWindowText (GetDlgItem (hTab, IDC_EDIT3), cbuf);
			i = oapiGetGaugePos (GetDlgItem (hTab, IDC_PROPLEVEL));
			j = (int)(level*100.0+0.5);
			if (i != j) oapiSetGaugePos (GetDlgItem (hTab, IDC_PROPLEVEL), j);
		}
	}
	RefreshTotals();
}
	*/
}

void ScnEditor::Show ()
{
    if(!show) return;
	if(ImGui::Begin("Scenario Editor", &show)) {
	    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y), true);
		if(ImGui::CollapsingHeader("Date")) {
			int year = date.tm_year + 1900;
			int month = date.tm_mon;
			int day = date.tm_mday;
			int hour = date.tm_hour;
			int minute = date.tm_min;
			int second = date.tm_sec;
			bool changed = false;

			ImGui::BeginGroupPanel("Universal time DD/MM/YYYY hh:mm:ss");
			ImGui::PushItemWidth(30);
			changed |= ImGui::InputInt("##DD", &day, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			changed |= ImGui::InputInt("##MM", &month, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::PushItemWidth(40);
			changed |= ImGui::InputInt("##YYYY", &year, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			
			changed |= ImGui::InputInt("##HH", &hour, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			changed |= ImGui::InputInt("##Min", &minute, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			changed |= ImGui::InputInt("##Sec", &second, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::PopItemWidth();
			
			ImGui::EndGroupPanel();

			if(changed) {
				date.tm_year = year - 1900;
				date.tm_mon  = std::clamp(month, 1, 12);
				date.tm_mday = std::clamp(day, 1, 31);
				date.tm_hour = std::clamp(hour, 0, 23);
				date.tm_min  = std::clamp(minute, 0, 59);
				date.tm_sec  = std::clamp(second, 0, 60);

				mjd = date2mjd (&date);
			}
			changed = false;

			ImGui::BeginGroupPanel("Modified Julian Date (MJD)");
			ImGui::SetNextItemWidth(100);
			changed |= ImGui::InputDouble("##MJD", &mjd, 0, 0, "%f");
			ImGui::EndGroupPanel();

			ImGui::BeginGroupPanel("Julian Date (JD)");
			ImGui::SetNextItemWidth(100);
			double jd = mjd + 2400000.5;
			if(ImGui::InputDouble("##JD", &jd, 0, 0, "%f")) {
				changed = true;
				mjd = jd - 2400000.5;
			}
			ImGui::EndGroupPanel();

			ImGui::BeginGroupPanel("Julian Century (JC)");
			ImGui::SetNextItemWidth(100);
			double jc = MJD2JC(mjd);
			if(ImGui::InputDouble("##JC", &jc, 0, 0, "%f")) {
				changed = true;
				mjd = JC2MJD(jc);
			}
			ImGui::EndGroupPanel();

			ImGui::BeginGroupPanel("Epoch");
			ImGui::SetNextItemWidth(100);
			double epoch = MJD2Jepoch(mjd);
			if(ImGui::InputDouble("##Epoch", &epoch, 0, 0, "%f")) {
				changed = true;
				mjd = Jepoch2MJD(epoch);
			}
			ImGui::EndGroupPanel();


			ImGui::BeginGroupPanel("Orbital propagation");
			ImGui::RadioButton("Fixed", &OrbitalMode, PROP_ORBITAL_FIXEDSTATE);ImGui::SameLine();
			ImGui::RadioButton("Rotate", &OrbitalMode, PROP_ORBITAL_FIXEDSURF);ImGui::SameLine();
			ImGui::RadioButton("Propagate", &OrbitalMode, PROP_ORBITAL_ELEMENTS);
			ImGui::EndGroupPanel();

			ImGui::BeginGroupPanel("Suborbital propagation");
			ImGui::RadioButton("Fixed##sub", &SOrbitalMode, PROP_SORBITAL_FIXEDSTATE);ImGui::SameLine();
			ImGui::RadioButton("Rotate##sub", &SOrbitalMode, PROP_SORBITAL_FIXEDSURF);ImGui::SameLine();
			ImGui::RadioButton("Propagate##sub", &SOrbitalMode, PROP_SORBITAL_ELEMENTS);
			ImGui::RadioButton("Destroy##sub", &SOrbitalMode, PROP_SORBITAL_DESTROY);
			ImGui::EndGroupPanel();


			if(ImGui::Button("Now")) {
				mjd = oapiGetSysMJD();
				changed = true;
			}
			if(changed)
				date = *mjddate (mjd);

			ImGui::SameLine();
			if(ImGui::Button("Apply")) {
				oapiSetSimMJD (mjd, OrbitalMode | SOrbitalMode);
			}
			ImGui::SameLine();
			if(ImGui::Button("Refresh")) {
				mjd = oapiGetSimMJD();
				date = *mjddate(mjd);
			}
		}

		if(ImGui::CollapsingHeader("Vessels")) {
			if(ImGui::Button("New")) {
				ImGui::OpenPopup("NewVessel");
			}
			ImGui::SameLine();
			const bool disabled = !m_currentVessel;
			if(disabled) ImGui::BeginDisabled();
			if(ImGui::Button("Delete selected")) {
				if(oapiGetVesselCount()>1) {
					oapiDeleteVessel (m_currentVessel);
					m_currentVessel = nullptr;
				} else {
					oapiAddNotification(OAPINOTIF_ERROR, "Cannot delete vessel", "One vessel at least must exists");
				}
			}
			ImGui::SameLine();
			if(ImGui::Button("Focus")) {
				oapiSetFocusObject (m_currentVessel);
				oapiCameraAttach (m_currentVessel, 1);
			}
			ImGui::SameLine();
			if(ImGui::Button("Refresh")) {
				ReloadVessel();
			}
			if(disabled) ImGui::EndDisabled();

			ImGui::SetNextWindowSize({500,500});
			if (ImGui::BeginPopup("NewVessel"))
			{
				VesselCreatePopup();
				ImGui::EndPopup();
			}
			ImGui::Separator();

			DrawShipList(m_currentVessel);

		}
		if(ImGui::Button("Save scenario...")) {
			ImGui::OpenPopup("SaveScenario");
		}
		if (ImGui::BeginPopup("SaveScenario"))
		{
			static char name[256] = {0};
			static char desc[1024] = {0};
			ImGui::Text("New scenario");
			ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
			ImGui::InputTextMultiline("Descrption", desc, sizeof(desc));
			if(ImGui::Button("Save") && strlen(name) > 1) {
				oapiSaveScenario(name, desc);
				memset(name, 0, sizeof(name));
				memset(desc, 0, sizeof(desc));
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if(ImGui::Button("Cancel")) {
				memset(name, 0, sizeof(name));
				memset(desc, 0, sizeof(desc));
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	    ImGui::EndChild();
		ImGui::SameLine();
	    ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);
		DrawTabs();
	    ImGui::EndChild();
	}
	ImGui::End();
}

void ScnEditor::VesselDeleted (OBJHANDLE hV)
{
	// update editor after vessel has been deleted
	if (m_currentVessel == hV) { // vessel is currently being edited
		m_currentVessel = nullptr;
	}
}

void ScnEditor::Pause (bool pause)
{
//	if (hDlg) oapiSetTitleButtonState (hDlg, IDPAUSE, pause ? 1:0);
}

MODULEHANDLE ScnEditor::LoadVesselLibrary (const VESSEL *vessel)
{
	// load vessel-specific editor extensions
	char cbuf[256];
	if (hEdLib) {
		oapiModuleUnload (hEdLib); // remove previous library
		m_customTabs = nullptr;
	}
	if (vessel->GetEditorModule (cbuf)) {
		char path[280];
		sprintf (path, "lib%s.so", cbuf);

		hEdLib = oapiModuleLoad (path);
		// now load vessel-specific interface
		if (hEdLib) {
			typedef ScnDrawCustomTabs (*SEC_Init)(void);
			SEC_Init secInit = (SEC_Init)oapiModuleGetProcAddress(hEdLib, "secInit");
			if (secInit) {
				m_customTabs = secInit ();
			}
		}
	}
	else hEdLib = nullptr;
	return hEdLib;
}
/*
// ==============================================================
// nonmember functions
*/
void OpenDialog (void *context)
{
	((ScnEditor*)context)->OpenDialog();
}

void Crt2Pol (VECTOR3 &pos, VECTOR3 &vel)
{
	// position in polar coordinates
	double r      = sqrt  (pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
	double lng    = atan2 (pos.z, pos.x);
	double lat    = asin  (pos.y/r);
	// derivatives in polar coordinates
	double drdt   = (vel.x*pos.x + vel.y*pos.y + vel.z*pos.z) / r;
	double dlngdt = (vel.z*pos.x - pos.z*vel.x) / (pos.x*pos.x + pos.z*pos.z);
	double dlatdt = (vel.y*r - pos.y*drdt) / (r*sqrt(r*r - pos.y*pos.y));
	pos.data[0] = r;
	pos.data[1] = lng;
	pos.data[2] = lat;
	vel.data[0] = drdt;
	vel.data[1] = dlngdt;
	vel.data[2] = dlatdt;
}

void Pol2Crt (VECTOR3 &pos, VECTOR3 &vel)
{
	double r   = pos.data[0];
	double lng = pos.data[1], clng = cos(lng), slng = sin(lng);
	double lat = pos.data[2], clat = cos(lat), slat = sin(lat);
	// position in cartesian coordinates
	double x   = r * cos(lat) * cos(lng);
	double z   = r * cos(lat) * sin(lng);
	double y   = r * sin(lat);
	// derivatives in cartesian coordinates
	double dxdt = vel.data[0]*clat*clng - vel.data[1]*r*clat*slng - vel.data[2]*r*slat*clng;
	double dzdt = vel.data[0]*clat*slng + vel.data[1]*r*clat*clng - vel.data[2]*r*slat*slng;
	double dydt = vel.data[0]*slat + vel.data[2]*r*clat;
	pos.data[0] = x;
	pos.data[1] = y;
	pos.data[2] = z;
	vel.data[0] = dxdt;
	vel.data[1] = dydt;
	vel.data[2] = dzdt;
}
