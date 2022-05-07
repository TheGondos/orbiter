#pragma once
#include "GUIManager.h"
#include "Camera.h"

class CelestialBody;
class DlgCamera : public GUIElement {
public:
    DlgCamera(const std::string &name);
    void Show() override;
    static const std::string etype;
private:
    void DrawControl();
    void DrawTarget();
    void DrawTrack();
    void DrawGround();
    void DrawFoV();
    void DrawPreset();

	ExtCamMode extmode;
	IntCamMode intmode;
	bool extcam;
	bool ground_lock;
	bool rot_is_tilt, rot_is_pan;
    std::string m_SelectedTarget;
    std::string m_SelectedSite;
    std::string m_SitePlanet;
    int m_SelectedPreset;

    char longitude[64]="";
    char latitude[64]="";
    char altitude[64]="";
    char followterrain[64]="";
    bool m_TargetLock;

    void AddCbodyNode(const CelestialBody *body);
    void ApplyObserver();
    void SetCurrentGroundpos();
};
