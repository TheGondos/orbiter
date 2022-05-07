#pragma once

#include "OrbiterAPI.h"

class DlgFunction : public GUIElement {
public:
    DlgFunction(const std::string &name);
    void Show() override;
    static const std::string etype;
};
