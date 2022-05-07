#pragma once

#include "OrbiterAPI.h"

class DlgFocus : public GUIElement {
public:
    DlgFocus(const std::string &name);
    void Show() override;
    void DrawAll();
    void DrawNearby();
    void DrawLocation();
    void DrawClass();
    std::string m_SelectedShip;
    float m_Range = 1.0;
    static const std::string etype;
};
