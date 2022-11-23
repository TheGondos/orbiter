// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "DlgLaunchpad.h"
#include "Orbiter.h"
#include "Controller.h"
#include "font_awesome_5.h"
#include "imgui_md.h"
#include "GUIManager.h"

#include <filesystem>

namespace fs = std::filesystem;

extern Orbiter *g_pOrbiter;
const std::string DlgLaunchpad::etype = "DlgLaunchpad";

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
		SURFHANDLE s;
		if(m_textureCache.count(m_href)) {
			s = m_textureCache[m_href];
		} else {
			s = oapiLoadTexture(m_href.c_str());
			if(!s) return false;
			m_textureCache[m_href] = s;
		}
		int w, h;
		oapiIncrTextureRef(s);
		oapiGetTextureSize(s, &w, &h);
		nfo.texture_id = s;
		nfo.size = { (float)w, (float)h };
		nfo.uv0 = { 0, 0 };
		nfo.uv1 = { 1, 1 };
		nfo.col_tint = { 1,1,1,1 };
		nfo.col_border = { 0,0,0,0 };
		return true;
	}

	mutable std::map<std::string, SURFHANDLE> m_textureCache;
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

DlgLaunchpad::DlgLaunchpad(const std::string &name) : GUIElement(name, "DlgLaunchpad") {
    show = true;
    //m_SelectedScenario = "./Scenarios//Demo/DG ISS Approach.scn";
}

void DlgLaunchpad::DrawVideo() {
	g_pOrbiter->m_pGUIManager->VideoTab();
}

void DlgLaunchpad::DrawExtra() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, ImGui::GetContentRegionAvail().y), true, window_flags);

	LaunchpadItem *lpi = nullptr;

	for(auto kv = m_extraItems.begin(); kv != m_extraItems.end(); kv = m_extraItems.upper_bound(kv->first)) {
		const char *category = kv->first.c_str();
		if(category[0] == '\0')
			category = "Misc";

		if(ImGui::CollapsingHeader(category)) {
			auto items = m_extraItems.equal_range(kv->first);
			for (auto &it = items.first; it != items.second; ++it) {
				const char *name = it->second->Name();
				const bool is_selected = m_extraSelected == name;
                if (ImGui::Selectable(name, is_selected))
                    m_extraSelected = name;
				const char *desc = it->second->Description();
				if (strlen(desc) > 0 && ImGui::IsItemHovered())
                	ImGui::SetTooltip("%s", desc);

				if(is_selected)
					lpi = it->second;
			}
		}
	}
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true, window_flags);
	if(lpi) {
		lpi->Draw();
		if(ImGui::Button("Save"))
			lpi->clbkWriteConfig();
	}
    ImGui::EndChild();

}

void DlgLaunchpad::Show() {
	if(show) {
        const char *tabs[] = {
            ICON_FA_GLOBE" Scenarios", ICON_FA_TOOLS" Parameters", ICON_FA_TACHOMETER_ALT" Visual Effects",
			ICON_FA_PUZZLE_PIECE" Modules", ICON_FA_DESKTOP" Video", ICON_FA_GAMEPAD" Joystick", ICON_FA_TASKS" Extra",
			ICON_FA_QUESTION_CIRCLE" About"
        };

        void (DlgLaunchpad::* func[])() = {
            &DlgLaunchpad::DrawScenarios, &DlgLaunchpad::DrawParameters, &DlgLaunchpad::DrawVisualEffects, &DlgLaunchpad::DrawModules,
            &DlgLaunchpad::DrawVideo, &DlgLaunchpad::DrawJoystick, &DlgLaunchpad::DrawExtra, &DlgLaunchpad::DrawAbout
        };

        ImGui::SetNextWindowPos(ImVec2(170, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(870, 540), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        ImGui::Begin("Launchpad", &show, ImGuiWindowFlags_NoCollapse);
		if(!show)
			g_pOrbiter->m_pGUIManager->CloseWindow();

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
    if(!m_SelectedScenario.empty()) {
        if(ImGui::Button(ICON_FA_PLAY_CIRCLE" Launch scenario")) {
            char cbuf[256];
            sprintf(cbuf, "%s", m_SelectedScenario.c_str());
            cbuf[strlen(cbuf)-4]='\0';
            g_pOrbiter->Launch (cbuf+strlen("./Scenarios/"));
        }
        ImGui::Separator();
    }
    std::string desc = GetScenarioDescription();
    const char *dsc = desc.c_str();

	static my_markdown s_printer;

	s_printer.print(dsc, dsc + strlen(dsc));

    ImGui::EndChild();
}

char *ScanFileDesc (std::istream &is, const char *blockname)
{
	char *buf = 0;
	char blockbegin[256] = "BEGIN_";
	char blockend[256] = "END_";
	strncpy (blockbegin+6, blockname, 240);
	strncpy (blockend+4, blockname, 240);

	if (FindLine (is, blockbegin)) {
		int i, len, buflen = 0;
		const int linelen = 256;
		char line[linelen];
		for(i = 0;; i++) {
			if (!is.getline(line, linelen-2)) {
				if (is.eof()) break;
				else is.clear();
			}
			if (_strnicmp (line, blockend, strlen(blockend))) {
				len = strlen(line);
				strcat (line, "\n");
                len++;
				char *tmp = new char[buflen+len+1];
				if (buflen) {
					memcpy (tmp, buf, buflen*sizeof(char));
					delete []buf;
				}
				memcpy (tmp+buflen, line, len*sizeof(char));
				buflen += len;
				tmp[buflen] = '\0';
				buf = tmp;
			} else {
				break;
			}
		}
	}
	return buf;
}


void AppendChar (char *&line, int &linelen, char c, int pos)
{
	if (pos == linelen) {
		char *tmp = new char[linelen+256];
		memcpy (tmp, line, linelen);
		delete []line;
		line = tmp;
		linelen += 256;
	}
	line[pos] = c;
}

struct ReplacementPair {
	const char *src, *tgt;
};

void Html2Text (char **pbuf)
{
	if (!pbuf) return;
	char *buf = *pbuf;
	int i, p;

	// 1. perform subsitutions of known html tags
	const int ntagpair = 4;
	ReplacementPair tagpair[ntagpair] = {
		{"<h1>", "## "},
		{"</h1>", "\r\n\r\n"},
		{"<p>", "\r\n"},
		{"</p>", "\r\n"}
	};
	std::string str(buf);
	delete []buf;
	for (i = 0; i < ntagpair; i++) {
		while ((p = str.find (tagpair[i].src)) != std::string::npos)
			str.replace (p, strlen(tagpair[i].src), tagpair[i].tgt, strlen(tagpair[i].tgt));
	}
	const char *cbuf = str.c_str();

	// 2. remove all remaining tags
	int nbuf = 0, nbuflen = 256;
	char *outbuf = new char[nbuflen];
	bool skip = false;
	for (i = 0; cbuf[i]; i++) {
		if (skip) {
			if (cbuf[i] == '>') skip = false;
		} else if (cbuf[i] == '<') {
			skip = true;
		} else if (nbuf > 0 && outbuf[nbuf-1] == '\n' && cbuf[i] == ' ') {
		} else {
			AppendChar (outbuf, nbuflen, cbuf[i], nbuf++);
		}
	}
	AppendChar (outbuf, nbuflen, '\0', nbuf++);

	// 3. perform subsitutions of known html symbols
	const int nsympair = 5;
	ReplacementPair sympair[nsympair] = {
		{"&gt;", ">"},
		{"&lt;", "<"},
		{"&ge;", ">="},
		{"&le;", "<="},
		{"&amp;", "\001"}
	};
	str = outbuf;
	delete []outbuf;
	for (i = 0; i < nsympair; i++) {
		while ((p = str.find (sympair[i].src) ) != std::string::npos)
			str.replace (p, strlen(sympair[i].src), sympair[i].tgt, strlen(sympair[i].tgt));
	}
	cbuf = str.c_str();

	// 4. remove all remaining sybols
	nbuf = 0, nbuflen = 256;
	outbuf = new char[nbuflen];
	skip = false;
	for (i = 0; cbuf[i]; i++) {
		if (skip) {
			if (cbuf[i] == ';') skip = false;
		} else if (cbuf[i] == '&') {
			skip = true;
		} else if (cbuf[i] == '\001') { // replace the '&' placeholder
			AppendChar (outbuf, nbuflen, '&', nbuf++);
		} else {
			AppendChar (outbuf, nbuflen, cbuf[i], nbuf++);
		}
	}
	AppendChar (outbuf, nbuflen, '\0', nbuf++);

	*pbuf = outbuf;
}

std::string DlgLaunchpad::GetScenarioDescription() {
    std::ifstream ifs;

    if(!m_SelectedDirectory.empty()) {
   		ifs.open (m_SelectedDirectory + "/Description.txt");
    } else if(!m_SelectedScenario.empty()) {
		ifs.open (m_SelectedScenario, std::ios::in);
    } else {
        return {};
    }

    std::string ret;

    char *buf = ScanFileDesc (ifs, "HYPERDESC");
    if (!buf) {
        buf = ScanFileDesc (ifs, "DESC");
    }
    if (buf) { // prepend style preamble
        Html2Text(&buf);
        ret=buf;
        delete []buf;
    }
    return ret;

}

bool alphanum_sort (fs::path &i, fs::path &j) {
    return i < j;
}

void DlgLaunchpad::DrawDir(const char *path) {
    std::vector<fs::path> directories;
    for (auto &file : fs::directory_iterator(path)) {
        if(file.path() != "." && file.path() != "..") {
            if (file.is_directory()) {
                directories.push_back(file.path());
            }
        }
    }

    std::sort(directories.begin(), directories.end(), alphanum_sort);
    for(auto &p: directories) {
        const bool is_selected = m_SelectedDirectory == p;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;//ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
        
        bool clicked = false;
        if(ImGui::TreeNodeEx(p.stem().c_str(), node_flags)) {
            clicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
            DrawDir(p.c_str());
            ImGui::TreePop();
        } else {
            clicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
        }
        if (clicked) {
            m_SelectedScenario.clear();
            m_SelectedDirectory = p;
        }
    }

    std::vector<fs::path> scenarios;
    for (auto &file : fs::directory_iterator(path)) {
        if(file.path().extension() == ".scn") {
            if (file.is_regular_file()) {
                scenarios.push_back(file.path());
            }
        }
    }

    std::sort(scenarios.begin(), scenarios.end(), alphanum_sort);
    for(auto &p: scenarios) {
        const bool is_selected = m_SelectedScenario == p;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
        char node_text[256];
        sprintf(node_text, ICON_FA_PAPER_PLANE" %s", p.stem().c_str());
        ImGui::TreeNodeEx(node_text, node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_SelectedScenario = p;
            m_SelectedDirectory.clear();
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
            if(checked) {
                pCfg->AddModule(filename.c_str());
				if(filename.c_str() == pCfg->m_videoPlugin) {
					oapiAddNotification(OAPINOTIF_INFO, "Changing video plugin requires a restart",filename.c_str());
				} else {
					g_pOrbiter->LoadModule ("Modules/Plugin", filename.c_str(), false);
				}
			} else {
                pCfg->DelModule(filename.c_str());
				if(filename.c_str() == pCfg->m_videoPlugin) {
					oapiAddNotification(OAPINOTIF_INFO, "Changing video plugin requires a restart",filename.c_str());
				} else {
					g_pOrbiter->UnloadModule (filename.c_str());
				}
			}
        }
    }
}

void DlgLaunchpad::DrawJoystick() {
    InputController::DrawEditor(false);
}

void DlgLaunchpad::DrawAbout() {
    const char aboutText[] = u8R"(
## xorbiter
[xorbiter](https://github.com/TheGondos/orbiter) is a linux port of the [Orbiter Space Flight Simulator](http://orbit.medphys.ucl.ac.uk/) forked from its [github repo](https://github.com/orbitersim/orbiter).
More information can be found on the [Orbiter forum](https://www.orbiter-forum.com/threads/linux-playground.40476/).

## Credits
* [Orbiter](https://github.com/orbitersim/orbiter) : *a spaceflight simulator based on Newtonian mechanics* by Dr Martin Schweiger
* [Dear ImGui](https://github.com/ocornut/imgui) : *a bloat-free graphical user interface library for C++* by Omar Cornut
* [md4c](https://github.com/mity/md4c) : *Markdown for C* by Martin Mitáš
* [imgui_md](https://github.com/mekhontsev/imgui_md) : *Markdown renderer for Dear ImGui* by Dmitry Mekhontsev
* [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) : *Node Editor in ImGui* by Michał Cichoń
* [imgui-knobs](https://github.com/altschuler/imgui-knobs) : knobs widget by Simon Altschuler
* [libnsbmp](https://www.netsurf-browser.org/projects/libnsbmp/) : *BMP Decoding Library* from the [NetSurf](https://www.netsurf-browser.org/) project
* [GLFW](https://www.glfw.org/) : *An OpenGL Library*
* [imgui-notify](https://github.com/patrickcjk/imgui-notify) : *a (very) simple notification wrapper for Dear ImGui* by Deathstroke aka patrickcjk
* [Font Awesome](https://fontawesome.com/) : icon library
* [nanovg](https://github.com/memononen/nanovg) : *2D vector drawing library on top of OpenGL* by Mikko Mononen
* [SOIL2](https://github.com/alelievr/SOIL2) : *Simple OpenGL Image Library 2* originally from [Martín Lucas Golini](https://github.com/SpartanJ/soil2) and forked by Antoine Lelievre
* [lua-5.1.5](https://www.lua.org/) : *a powerful, efficient, lightweight, embeddable scripting language* by [PUC-Rio](https://www.puc-rio.br/index.html)
* [glad](https://github.com/Dav1dde/glad) : *GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs* by David Herberth
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) : database of game controller mappings by Gabriel Jacobo

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

bool DlgLaunchpad::RegisterExtraItem(LaunchpadItem *item, const char *category) {
	m_extraItems.emplace(std::make_pair(std::string(category), item));
	return true;
}
bool DlgLaunchpad::UnregisterExtraItem(LaunchpadItem *item) {
	for(auto &kv: m_extraItems) {
		auto items = m_extraItems.equal_range(kv.first);
		for (auto it = items.first; it != items.second; ) {
			if (it->second == item) {
				it = m_extraItems.erase(it);
				return true;
			} else {
				++it;
			}
		}
	}
	return false;
}
