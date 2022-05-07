#pragma once

#include "GUIManager.h"
#include "VectorMap.h"

class CelestialBody;
class DlgMap : public GUIElement {
public:
    DlgMap(const std::string &name);
    void Show() override;
    void DrawMap();
    void DrawMenu();
    void SetBody(const char *body);
    void AddCbodyNode(const CelestialBody *cbody);
    void DrawTree();

    std::unique_ptr<VectorMap> m_Map;
    double updTmin = 0.0, updTmax = 0.0;
    std::string m_SelectedBody;
    double m_Zoom = 1.0;
    static const std::string etype;
};
