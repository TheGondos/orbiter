#pragma once

#include "OrbiterAPI.h"
#include "GraphicsAPI.h"
#include <string>
#include <map>

class DlgLaunchpad : public GUIElement {
public:
    DlgLaunchpad(const std::string &name);
    void Show() override;

    void DrawScenarios();
    void DrawParameters();
    void DrawVisualEffects();
    void DrawModules();
    void DrawVideo();
    void DrawJoystick();
    void DrawExtra();
    void DrawAbout();
    void DrawDir(const char *path);

    std::string GetScenarioDescription();

    bool RegisterExtraItem(LaunchpadItem *item, const char *category);
    bool UnregisterExtraItem(LaunchpadItem *item);

    std::string m_SelectedScenario;
    std::string m_SelectedDirectory;
    static const std::string etype;
    std::multimap<std::string, LaunchpadItem *> m_extraItems;
    std::string m_extraSelected;
};
