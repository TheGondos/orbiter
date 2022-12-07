#include "Select.h"
#include "Orbiter.h"
#include <imgui/imgui.h>

extern TimeData td;
extern Orbiter *g_pOrbiter;

const std::string Select::etype = "Select";

Select::Select(const std::string &name) : GUIElement(name, "Select") {
    show = false;
    m_Opened = false;
    m_Title = "Selection";
    m_CurrentEntry = nullptr;
}

void Select::Open(const char *_title, Callbk submenu_cbk, Callbk enter_cbk, void *_userdata, int cntx, int cnty) {
    m_Title = _title;

    m_cbSubmenu = submenu_cbk;
    m_cbEnter = enter_cbk;
    m_UserData = _userdata;

    m_RootMenu.clear();
    m_CurrentEntry = &m_RootMenu;

    submenu_cbk(this, 0, nullptr, _userdata);
    m_Opened = true;
    show = true;
}

void Select::Append(const char *str, int flags) {
    SelectEntry e;
    e.m_Flags = flags;
    e.m_Text = str;

    m_CurrentEntry->emplace_back(e);
}

void Select::AppendSeparator() {
    SelectEntry e;
    e.m_Flags = ITEM_SEPARATOR;

    m_CurrentEntry->emplace_back(e);
}

void Select::DrawMenu(std::list<SelectEntry> &entries) {
    int i=0;
    for(auto &e: entries) {
        if(e.m_Flags & ITEM_SEPARATOR) {
            ImGui::Separator();
        } else if(e.m_Flags & ITEM_SUBMENU) {
            if (ImGui::BeginMenu(e.m_Text.c_str())) {
                if(e.m_SubEntries.size()==0) {
                    m_CurrentEntry = &e.m_SubEntries;
                    m_cbSubmenu(this, i, e.m_Text.c_str(), m_UserData);
                }
                DrawMenu(e.m_SubEntries);
                ImGui::EndMenu();
                if(ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) && !(e.m_Flags & ITEM_NOHILIGHT)) {
                    if(ImGui::IsMouseReleased(0)) {
                        m_cbEnter(this, i, e.m_Text.c_str(), m_UserData);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            i++;
        } else {
            if(ImGui::MenuItem(e.m_Text.c_str())) {
                m_cbEnter(this, i, e.m_Text.c_str(), m_UserData);
            }
            i++;
        }
    }
}

void Select::Show() {
    if(!show) return;
    auto &io = ImGui::GetIO();

    if(io.MouseDown[0] && m_Opened) {
        return;
    }

    if(m_Opened) {
        ImGui::OpenPopup(m_Title.c_str());
        m_Opened = false;
    }
    
//    auto *viewport = ImGui::GetMainViewport();
//    ImGui::SetNextWindowViewport(viewport->ID);
    if (ImGui::BeginPopup(m_Title.c_str()))
    {
        DrawMenu(m_RootMenu);
        ImGui::EndPopup();
    }
}

const std::string InputBox::etype = "InputBox";

InputBox::InputBox(const std::string &name) : GUIElement(name, "InputBox") {
    show = false;
    m_Opened = false;
    m_Title = "InputBox";
}

void InputBox::Open (const char *_title, const char *_buf, int _vislen,
					 Callbk cbk, void *_userdata, int cntx, int cnty)
{
	OpenEx (_title, _buf, _vislen, cbk, 0, _userdata, 0, cntx, cnty);
}

bool InputBox::OpenEx (const char *_title, const char *_buf, int _vislen,
		Callbk enter_cbk, Callbk cancel_cbk, void *_userdata,
		int flags, int cntx, int cnty) {
    
    m_Title = _title;

    m_cbEnter = enter_cbk;
    m_cbCancel = cancel_cbk;
    m_UserData = _userdata;

    m_Opened = true;
    show = true;

    if(_buf)
        strcpy(m_Buf, _buf);
    else
        m_Buf[0]='\0';

    return true;
}

void InputBox::Show() {
    if(!show) return;
    char buf[256];
    sprintf(buf, "%s###InputBox", m_Title.c_str());

    bool firstTime = false;
    if(m_Opened) {
        ImGui::OpenPopup(buf);
        m_Opened = false;
        firstTime = true;
    }

    if (ImGui::BeginPopup(buf))
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        if(firstTime)
            ImGui::SetKeyboardFocusHere();
        bool entered = ImGui::InputText("##InputText", m_Buf, IM_ARRAYSIZE(m_Buf), ImGuiInputTextFlags_EnterReturnsTrue);

        if(ImGui::Button("OK") || entered) {
            m_cbEnter(this, m_Buf, m_UserData);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel")) {
            if(m_cbCancel)
                m_cbCancel(this, m_Buf, m_UserData);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
