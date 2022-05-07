#pragma once

#include "OrbiterAPI.h"

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

    std::string m_SelectedScenario;
    std::string m_SelectedScenarioPath;
    static const std::string etype;
};
