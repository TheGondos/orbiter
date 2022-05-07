#pragma once

#include "OrbiterAPI.h"

class CelestialBody;
class Vessel;
class Base;
class DlgInfo : public GUIElement {
public:
    DlgInfo(const std::string &name);
    void Show() override;
    void AddCbodyNode(const CelestialBody *cbody);
    void DrawTree();
    void DrawInfo();
    void DrawInfoVessel(Vessel *);
    void DrawInfoCelestialBody(CelestialBody *);
    void DrawInfoBase(Base *);
    std::string m_SelectedTarget;
    static const std::string etype;
};
