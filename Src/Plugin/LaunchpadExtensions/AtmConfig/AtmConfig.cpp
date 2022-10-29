// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#define STRICT 1
#define ORBITER_MODULE

#include "Orbitersdk.h"
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <regex>
#include <imgui.h>

namespace fs = std::filesystem;

class AtmConfig;

const char *CelbodyDir = "Modules/Celbody";
const char *ModuleItem = "MODULE_ATM";

AtmConfig *gItem;

static std::string GetAtmModule(const char *celbody) {
	char cfgname[256];
	std::string ret;
	strcpy (cfgname, celbody); strcat (cfgname, "/Atmosphere.cfg");
	FILEHANDLE hFile = oapiOpenFile (cfgname, FILE_IN, CONFIG);
	if (hFile) {
		char name[256];
		oapiReadItem_string (hFile, ModuleItem, name);
		ret = name;
		oapiCloseFile (hFile, FILE_IN);
	}
	return ret;
}

static void SetAtmModule(const char *celbody, const char *module) {
	char cfgname[256];
	strcpy (cfgname, celbody); strcat (cfgname, "/Atmosphere.cfg");
	FILEHANDLE hFile = oapiOpenFile (cfgname, FILE_OUT, CONFIG);
	if (hFile) {
		oapiWriteItem_string (hFile, ModuleItem, module);
		oapiCloseFile (hFile, FILE_OUT);
	}
}
/*
void AtmConfig::Write (const char *celbody)
{
}
*/
class AtmConfig: public LaunchpadItem {
public:
	AtmConfig();
	~AtmConfig();
	const char *Name() override { return "Atmosphere Configuration"; }
	const char *Description() override;
	void Read (const char *celbody);
	void Write(const char *celbody);
	int clbkWriteConfig () override;
	void Draw() override;

private:
	struct atmModules {
		std::string celbody;
		std::list<std::tuple<std::string,std::string,std::string>> modules;
		std::string currentModule;
		std::string currentModuleDesc;
	};

	std::vector<atmModules> modules;
};

AtmConfig::AtmConfig(): LaunchpadItem()
{
	for (auto &dir : fs::directory_iterator(CelbodyDir)) {
        if(dir.path() != "." && dir.path() != "..") {
            if (dir.is_directory()) {
				if(fs::exists(dir.path() / "Atmosphere")) {
					std::list<std::tuple<std::string,std::string,std::string>> mods;

				    for (auto &module : fs::directory_iterator(dir.path() / "Atmosphere")) {
						MODULEHANDLE hMod = oapiModuleLoad(module.path().c_str());

						const char *(*name_func)() = (const char*(*)())oapiModuleGetProcAddress (hMod, "ModelName");
						const char *(*desc_func)() = (const char*(*)())oapiModuleGetProcAddress (hMod, "ModelDesc");

						if(name_func && desc_func) {
							std::regex re(".*lib(\\w+)\\.so");
						    std::smatch match;
							const std::string s = module.path().string();

						    if (std::regex_search(s.begin(), s.end(), match, re)) {
								mods.push_back({match[1], std::string(name_func()), std::string(desc_func())});
							}
						}

						oapiModuleUnload(hMod);
					}

					if(!mods.empty()) {
						auto currentModule = GetAtmModule(dir.path().stem().c_str());
						modules.push_back({dir.path().stem(), mods, currentModule});
					}
				}
            }
        }
    }
}

AtmConfig::~AtmConfig ()
{
}

void AtmConfig::Draw ()
{
	for(auto &module: modules) {
		ImGui::BeginGroupPanel(module.celbody.c_str(), ImVec2(0,0));
		ImGui::PushItemWidth(150);
		ImGui::PushID(module.celbody.c_str());
		if(ImGui::BeginCombo("##combo", module.currentModule.c_str())) {
			for (auto &mod: module.modules) {
				if (ImGui::Selectable(std::get<0>(mod).c_str(), module.currentModule == std::get<0>(mod))) {
					module.currentModule = std::get<0>(mod);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(std::get<1>(mod).c_str());
					ImGui::Separator();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
					ImGui::TextWrapped("%s",std::get<2>(mod).c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		ImGui::PopItemWidth();
		ImGui::EndGroupPanel();
	}
}
int AtmConfig::clbkWriteConfig ()
{
	for(auto &module: modules) {
		SetAtmModule(module.celbody.c_str(), module.currentModule.c_str());
	}
	return 0;
}

const char *AtmConfig::Description()
{
	return "Configure atmospheric parameters for celestial bodies.";
}

// ==============================================================
// The DLL entry point
// ==============================================================

DLLCLBK void InitModule (MODULEHANDLE hDLL)
{
	gItem = new AtmConfig;
	// create the new config item
	oapiRegisterLaunchpadItem (gItem, "Celestial body configuration");
}

// ==============================================================
// The DLL exit point
// ==============================================================

DLLCLBK void ExitModule (MODULEHANDLE hDLL)
{
	// Unregister the launchpad items
	oapiUnregisterLaunchpadItem (gItem);
	delete gItem;
}