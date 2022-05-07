#pragma once

#include "OrbiterAPI.h"

class DlgRecorder : public GUIElement {
public:
    DlgRecorder(const std::string &name);
    void Show() override;
    static const std::string etype;
    void DrawNormalRecording(bool recording);
    void DrawPlaying();

    char m_ScenarioFile[128];
    void GetRecordName (char *str, int maxlen) const;
};
