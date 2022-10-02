#include "DlgLaunchpad.h"
#include "Orbiter.h"
#include "Controller.h"
#include "font_awesome_5.h"
#include "imgui_md.h"

#include <dirent.h>
#include <filesystem>

namespace fs = std::filesystem;

extern Orbiter *g_pOrbiter;
const std::string DlgLaunchpad::etype = "DlgLaunchpad";

DlgLaunchpad::DlgLaunchpad(const std::string &name) : GUIElement(name, "DlgLaunchpad") {
    show = true;
    m_SelectedScenario = "./Scenarios//Demo/DG ISS Approach.scn";
}

void DlgLaunchpad::DrawVideo() {
    ImGui::Text("DrawVideo");
}
void DlgLaunchpad::DrawExtra() {
    ImGui::Text("DrawExtra");
}

void DlgLaunchpad::Show() {
	if(show) {
        const char *tabs[] = {
            ICON_FA_GLOBE" Scenarios", ICON_FA_TOOLS" Parameters", ICON_FA_TACHOMETER_ALT" Visual Effects", ICON_FA_PUZZLE_PIECE" Modules", ICON_FA_DESKTOP" Video", ICON_FA_GAMEPAD" Joystick", "Extra", ICON_FA_QUESTION_CIRCLE" About"
        };

        void (DlgLaunchpad::* func[])() = {
            &DlgLaunchpad::DrawScenarios, &DlgLaunchpad::DrawParameters, &DlgLaunchpad::DrawVisualEffects, &DlgLaunchpad::DrawModules,
            &DlgLaunchpad::DrawVideo, &DlgLaunchpad::DrawJoystick, &DlgLaunchpad::DrawExtra, &DlgLaunchpad::DrawAbout
        };

        ImGui::SetNextWindowPos(ImVec2(170, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(870, 540), ImGuiCond_FirstUseEver);
        ImGui::Begin("Launchpad", &show);

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

void DlgLaunchpad::DrawScenarios() {
    auto pCfg = g_pOrbiter->Cfg();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, ImGui::GetContentRegionAvail().y), true, window_flags);
    DrawDir(pCfg->CfgDirPrm.ScnDir);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true, window_flags);
    ImGui::Text("Scenario: %s", m_SelectedScenario.c_str());
    if(ImGui::Button("Start")) {
        char cbuf[256];
        sprintf(cbuf, "%s", m_SelectedScenario.c_str());
        cbuf[strlen(cbuf)-4]='\0';
        g_pOrbiter->Launch (cbuf+strlen("./Scenarios//"));
    }
    ImGui::EndChild();
}

bool alphanum_sort (std::pair<std::string, std::string> &i, std::pair<std::string, std::string> &j) {
    return i.second<j.second;
}

void DlgLaunchpad::DrawDir(const char *path) {
    char cbuf[280];
	struct dirent * dp = NULL;
	DIR *dir = opendir(path);
	if(dir == NULL) {
		return;
	}

    std::vector<std::pair<std::string, std::string>> directories;
	while ((dp = readdir(dir)) != NULL) {
		if(dp->d_type == DT_DIR) {
			if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
				char subdir[280];
				sprintf(subdir, "%s/%s", path, dp->d_name);
				strcpy (cbuf, subdir);
                directories.push_back(std::make_pair(cbuf, dp->d_name));
			}
		}
	}
	closedir(dir);

    std::sort(directories.begin(), directories.end(), alphanum_sort);
    for(auto &p: directories) {
        if(ImGui::TreeNodeEx(p.second.c_str())) {
            DrawDir(p.first.c_str());
            ImGui::TreePop();
        }
    }

    std::vector<std::pair<std::string, std::string>> scenarios;
	dir = opendir(path);
	while ((dp = readdir(dir)) != NULL) {
		auto len = strlen(dp->d_name);

		if(dp->d_type != DT_DIR) {
			if (!strcasecmp(dp->d_name + len - strlen(".scn"), ".scn")) {
				sprintf (cbuf, "%s/%s", path, dp->d_name);
                scenarios.push_back(std::make_pair(cbuf, dp->d_name));
			}
		}
	}
	closedir(dir);

    std::sort(scenarios.begin(), scenarios.end(), alphanum_sort);
    for(auto &p: scenarios) {
        const bool is_selected = m_SelectedScenario == p.first;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
        char node_text[256];
        sprintf(node_text, ICON_FA_PAPER_PLANE" %s", p.second.c_str());
        ImGui::TreeNodeEx(node_text, node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedScenario = p.first;
        }
    }
}
void DlgLaunchpad::DrawParameters() {
    ImGui::Text("DrawParameters");
}
void DlgLaunchpad::DrawVisualEffects() {
    ImGui::Text("DrawVisualEffects");
}

void DlgLaunchpad::DrawModules() {
    std::vector<std::string> moduleFiles;

    for (auto &file : fs::directory_iterator("Modules/Plugin")) {
        if(file.path().extension() == ".so") {
            moduleFiles.push_back(file.path().stem().c_str() + 3);
        }
    }

    std::sort(moduleFiles.begin(), moduleFiles.end());

    Config *pCfg = g_pOrbiter->Cfg();
    for (auto &filename : moduleFiles) {

        bool checked = pCfg->HasModule(filename.c_str());

        if(ImGui::Checkbox(filename.c_str(), &checked)) {
            if(checked)
                pCfg->AddModule(filename.c_str());
            else
                pCfg->DelModule(filename.c_str());
        }
    }
}

void DlgLaunchpad::DrawJoystick() {
    InputController::DrawEditor();
}

struct my_markdown : public imgui_md 
{
	ImFont* get_font() const override
	{

		switch (m_hlevel)
		{
		case 1:
			return g_pOrbiter->m_pGUIManager->fontH1;
		case 2:
			return g_pOrbiter->m_pGUIManager->fontH2;
		case 3:
			return g_pOrbiter->m_pGUIManager->fontH3;
		default:
            if(m_is_em || m_is_strong || m_is_table_header)
                return g_pOrbiter->m_pGUIManager->fontBold;
            else
                return g_pOrbiter->m_pGUIManager->fontDefault;
		}
	};
    ImVec4 get_color() const
    {
        if (!m_href.empty()) {
            return ImVec4{86.0/255.0,180.0/255.0,233.0/255.0,1.0};
        }

        return  ImGui::GetStyle().Colors[ImGuiCol_Text];
    }

	void open_url() const override
	{
        char cmd[256];
        sprintf(cmd, "xdg-open %s", m_href.c_str());
        system(cmd);
	}

	bool get_image(image_info& nfo) const override
	{

        assert(0);
		//use m_href to identify images
		//nfo.texture_id = g_texture1;
		nfo.size = {40,20};
		nfo.uv0 = { 0,0 };
		nfo.uv1 = {1,1};
		nfo.col_tint = { 1,1,1,1 };
		nfo.col_border = { 0,0,0,0 };
		return true;
	}
	/*
	void html_div(const std::string& dclass, bool e) override
	{
		if (dclass == "red") {
			if (e) {
				m_table_border = false;
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			} else {
				ImGui::PopStyleColor();
				m_table_border = true;
			}
		}
	}*/
};

void DlgLaunchpad::DrawAbout() {
    const char aboutText[] = u8R"(
## xorbiter
[xorbiter](https://github.com/TheGondos/orbiter) is a linux port of the [Orbiter Space Flight Simulator](http://orbit.medphys.ucl.ac.uk/) forked from its [github repo](https://github.com/orbitersim/orbiter).
More information can be found on the [Orbiter forum](https://www.orbiter-forum.com/threads/linux-playground.40476/).

## Credits
* [Orbiter](https://github.com/orbitersim/orbiter) : *a spaceflight simulator based on Newtonian mechanics* by Dr Martin Schweiger
* [Dear ImGui](https://github.com/ocornut/imgui) : *a bloat-free graphical user interface library for C++* by Omar Cornut
* [md4c](https://github.com/mity/md4c) : Markdown for C by Martin Mitáš
* [imgui_md](https://github.com/mekhontsev/imgui_md) : Markdown renderer for Dear ImGui by Dmitry Mekhontsev
* [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) : *Node Editor in ImGui* by Michał Cichoń
* [libnsbmp](https://www.netsurf-browser.org/projects/libnsbmp/) : *BMP Decoding Library* from the [NetSurf](https://www.netsurf-browser.org/) project
* [GLFW](https://www.glfw.org/) : *An OpenGL Library*
* [imgui-notify](https://github.com/patrickcjk/imgui-notify) : *a (very) simple notification wrapper for Dear ImGui* by Deathstroke aka patrickcjk
* [Font Awesome](https://fontawesome.com/) : icon library
* [nanovg](https://github.com/memononen/nanovg) : *2D vector drawing library on top of OpenGL* by Mikko Mononen
* [SOIL2](https://github.com/alelievr/SOIL2) : *Simple OpenGL Image Library 2* originally from [Martín Lucas Golini](https://github.com/SpartanJ/soil2) and forked by Antoine Lelievre
* [lua-5.1.5](https://www.lua.org/) : *a powerful, efficient, lightweight, embeddable scripting language* by [PUC-Rio](https://www.puc-rio.br/index.html)
* [glad](https://github.com/Dav1dde/glad) : *GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs* by David Herberth

## Addons
In the interest of having some more variety, several add-ons are made compatible with this linux build.
* [AeroBrakeMFD](https://www.orbiter-forum.com/resources/aerobrakemfd.1171/) by Jarmo Nikkanen
* [Deepstar](https://www.orbiter-forum.com/threads/deepstar-development.26879/) by Alain Hosking and Abdullah Radwan
* [G42-200](https://www.orbiter-forum.com/showthread.php?t=41302) by Abdullah Radwan
* [NASSP](https://nassp.space/index.php/Main_Page) by the NASSP team
* [UCSO](https://www.orbiter-forum.com/threads/ucso-development-thread.37374/) by Abdullah Radwan
* [XRVessels](https://github.com/dbeachy1/XRVessels) by Doug Beachy

If you have any feedback or feel that you have been mispresented in the credits, please do leave a message in [the orbiter forum](https://www.orbiter-forum.com/threads/linux-playground.40476/) or file a PR in the github project to remedy the situation.

## License
In keeping with the spirit of Orbiter, every file residing inside the main git repo (i.e. not a submodule) is licensed under the MIT license unless stated otherwise into the file itself.

Addons present as submodules may be licensed under different terms so beware : if you want to package xorbiter along with these addons and redistribute the result, you will need to do so under a license complying with everyone of them (GPLv3 will most likely be the result).

)";

	static my_markdown s_printer;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("Child", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true, window_flags);

	s_printer.print(aboutText, aboutText + sizeof(aboutText));

    ImGui::EndChild();
}
