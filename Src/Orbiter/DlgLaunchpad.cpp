#include "DlgLaunchpad.h"
#include "Orbiter.h"

#include <dirent.h>
#include <unistd.h>
#include <imgui/imgui.h>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

extern Orbiter *g_pOrbiter;
const std::string DlgLaunchpad::etype = "DlgLaunchpad";

DlgLaunchpad::DlgLaunchpad(const std::string &name) : GUIElement(name, "DlgLaunchpad") {
    show = true;
    m_SelectedScenario = "./Scenarios//Demo/DG ISS Approach.scn";
}

void DlgLaunchpad::Show() {
	if(show) {
        const char *tabs[] = {
            "Scenarios", "Parameters", "Visual Effects", "Modules", "Video", "Joystick", "Extra", "About"
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
        ImGui::TreeNodeEx(p.second.c_str(), node_flags);
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

void DlgLaunchpad::DrawVideo() {
    ImGui::Text("DrawVideo");
}
void DlgLaunchpad::DrawJoystick() {
    ImGui::Text("DrawJoystick");
}
void DlgLaunchpad::DrawExtra() {
    ImGui::Text("DrawExtra");
}
void DlgLaunchpad::DrawAbout() {
    ImGui::Text("DrawAbout");
}
