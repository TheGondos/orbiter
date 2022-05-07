#pragma once

#include "OrbiterAPI.h"

class DlgVishelper : public GUIElement {
public:
    DlgVishelper(const std::string &name);
    void Show() override;
    void DrawPlanetarium();
    void DrawForces();
    void DrawAxes();
    static const std::string etype;
};
