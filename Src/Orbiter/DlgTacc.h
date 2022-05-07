#pragma once

#include "OrbiterAPI.h"

class DlgTacc : public GUIElement {
public:
    DlgTacc(const std::string &name);
    void Show() override;
    static const std::string etype;
};
