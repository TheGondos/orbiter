// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                  ORBITER MODULE: ExtMFD
//                  Part of the ORBITER SDK
//            Copyright (C) 2006 Martin Schweiger
//                   All rights reserved
//
// MFDWindow.cpp
//
// Class implementation for MFDWindow. Defines the properties and
// state of an MFD display in a dialog box
// ==============================================================

#include "MFDWindow.h"
#include <stdio.h>
#include <cstring>
#include <imgui.h>

// ==============================================================
// prototype definitions

//INT_PTR CALLBACK DlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ==============================================================
// class MFDWindow

const int button_yoffset = 30;

class DlgExtMFD: public GUIElement {
public:
    DlgExtMFD(const std::string &name, MFDWindow *mfd);
    void Show() override;
    static const std::string etype;
	MFDWindow *m_mfd;
	ImVec2 m_oldSize;
};

DlgExtMFD::DlgExtMFD(const std::string &name, MFDWindow *mfd) : GUIElement(name, "DlgExtMFD") {
    show = true;
	m_mfd = mfd;
	m_oldSize = ImVec2(0,0);
}

void DlgExtMFD::Show() {
    if(!show) return; 

	const int button_row_width = 50;
	const int button_bottom_height = 50;
	const ImVec2 button_sz = ImVec2(40,20);

	char cbuf[256] = "MFD [";
	oapiGetObjectName (m_mfd->GetVessel(), cbuf+5, 250);
	strcat (cbuf, "]##");
	strcat (cbuf, name.c_str());

	if(m_mfd->m_poweroff) {
		m_mfd->ProcessButton (12, PANEL_MOUSE_LBDOWN);
		m_mfd->m_poweroff = false;
	}

	ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiCond_FirstUseEver);
    if(ImGui::Begin(cbuf, &show)) {
		
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("ChildL", ImVec2(button_row_width, ImGui::GetContentRegionAvail().y - button_bottom_height), false, window_flags);

		ImVec2 sz = ImGui::GetContentRegionAvail();

		for(int i=0;i<6;i++) {
			const char *label = m_mfd->GetButtonLabel (i);
			if(label) {
				ImGui::PushID(i);
				ImGui::SetCursorPosY((i*sz.y-button_yoffset)/6 + button_yoffset);
				ImGui::Button(label, button_sz);
				if(ImGui::IsItemHovered()) {
					if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						m_mfd->ProcessButton (i, PANEL_MOUSE_LBDOWN);
					} else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						m_mfd->ProcessButton (i, PANEL_MOUSE_LBUP);
					}
				}
				ImGui::PopID();
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("ChildM", ImVec2(ImGui::GetContentRegionAvail().x-button_row_width, ImGui::GetContentRegionAvail().y - button_bottom_height), false, window_flags);
		sz = ImGui::GetContentRegionAvail();

		if(sz.x != m_oldSize.x || sz.y != m_oldSize.y) {
			if(sz.x > 80 && sz.y > 80) {
				MFDSPEC spec = {{0,0,(int)sz.x,(int)sz.y},6,6,button_yoffset,(int)(sz.y/6.0)};
				m_mfd->Resize (spec);
				m_oldSize = sz;
			}
		}
		SURFHANDLE surf = m_mfd->GetDisplaySurface();
		if(surf) {
			ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
			ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
			ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
			ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white
			oapiIncrTextureRef(surf);
			ImGui::ImageButton(surf, ImVec2(sz.x, sz.y), uv_min, uv_max, 0, tint_col, border_col);
		}
		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - button_bottom_height), false, window_flags);

		for(int i=6;i<12;i++) {
			const char *label = m_mfd->GetButtonLabel (i);
			if(label) {
				ImGui::PushID(i);
				ImGui::SetCursorPosY(((i-6)*sz.y-button_yoffset)/6 + button_yoffset);
				ImGui::Button(label, button_sz);
				if(ImGui::IsItemHovered()) {
					if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						m_mfd->ProcessButton (i, PANEL_MOUSE_LBDOWN);
					} else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						m_mfd->ProcessButton (i, PANEL_MOUSE_LBUP);
					}
				}
				ImGui::PopID();
			}
		}

		ImGui::EndChild();
		ImGui::BeginChild("ChildB", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, window_flags);

	    sz = ImGui::GetContentRegionAvail();

		//sz.x - button_sz.x * 4
		ImGui::SetCursorPosX(60);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9,0,0,1));
		if(ImGui::Button("PWR", button_sz)) {
			//We cannot call ProcessButton now because we've already submitted the texture to ImGui
			m_mfd->m_poweroff = true;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
		if(ImGui::Button("SEL", button_sz)) {
			m_mfd->ProcessButton (13, PANEL_MOUSE_LBDOWN);
		}
		ImGui::SameLine();
		if(ImGui::Button("MNU", button_sz)) {
			m_mfd->ProcessButton (14, PANEL_MOUSE_LBDOWN);
		}
		ImGui::EndChild();
    }
    ImGui::End();
}


MFDWindow::MFDWindow (const MFDSPEC &spec): ExternMFD (spec)
{
	fnth = 0;
	vstick = false;
	char cbuf[128];
	m_poweroff = false;
	sprintf(cbuf, "MFDWindow%ld", (uint64_t)Id().emfd);
	m_window = std::make_unique<DlgExtMFD>(cbuf, this);
	oapiOpenDialog (m_window.get());
}

MFDWindow::~MFDWindow ()
{
	oapiCloseDialog (m_window.get());
	oapiUnregisterExternMFD (Id().emfd);
}

void MFDWindow::SetVessel (OBJHANDLE hV)
{
	ExternMFD::SetVessel (hV);
}

void MFDWindow::ProcessButton (int bt, int event)
{
	switch (bt) {
	case 12:
		if (event == PANEL_MOUSE_LBDOWN)
			SendKey (OAPI_KEY_ESCAPE);
		break;
	case 13:
		if (event == PANEL_MOUSE_LBDOWN)
			SendKey (OAPI_KEY_F1);
		break;
	case 14:
		if (event == PANEL_MOUSE_LBDOWN)
			SendKey (OAPI_KEY_GRAVE);
		break;
	default:
		ExternMFD::ProcessButton (bt, event);
		break;
	}
}

void MFDWindow::clbkFocusChanged (OBJHANDLE hFocus)
{
	if (!vstick) {
		ExternMFD::clbkFocusChanged (hFocus);
	}
}

void MFDWindow::StickToVessel (bool stick)
{
	vstick = stick;
	if (!vstick) {
		SetVessel (oapiGetFocusObject());
	}
}
