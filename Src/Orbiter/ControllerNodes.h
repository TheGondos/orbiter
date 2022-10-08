#ifndef CONTROLLERNODE_H
#define CONTROLLERNODE_H

#include "Controller.h"
#include "GUIManager.h"

class CameraCtl : public Node
{
    public:
    enum {
        RotX = 0,
        RotY = 1,
        RotZ = 2,
        Reset = 3,
        ToggleView = 4
    };
    CameraCtl(ControllerGraph *);
    CameraCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};
class Thrusters : public Node
{
    public:
    Thrusters(ControllerGraph *);
    Thrusters(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    void GetOutputs(int thrusters[15]);
};

class AirCtl : public Node {
public:
    AirCtl(ControllerGraph *);
    AirCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    void GetOutputs(int af[6]);
};

class AFCtl : public Node {
public:
    enum {
        In_Toggle = 0,
        In_Elevator = 1,
        In_Rudder = 2,
        In_Aileron = 3,
        Out_Elevator = 0,
        Out_Rudder = 1,
        Out_Aileron = 2,
    };
    AFCtl(ControllerGraph *);
    AFCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;

    int ctrlsurfmode;
};

class RCSCtl : public Node {
public:
    enum {
        In_Disable = 0,
        In_Translation = 1,
        In_Rotation = 2,
        In_Toggle = 3,
        Out_Translation = 0,
        Out_Rotation = 1
    };
    RCSCtl(ControllerGraph *cg);
    RCSCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};

class Joystick : public Node
{
    public:
    Joystick(ControllerGraph *, int joy_id);
    Joystick(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override { UpdateOutputs(); };
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;
    void AddContextMenus();

    int joy_id;
    int nbAxis;
    int nbButtons;
    int nbHats;
    bool connected;
    std::string guid;
};

class Filter : public Node {
public:
    enum { Enable = 0 };
    struct PinPair {
        PinPair(ed::PinId i, ed::PinId o) {
            in = i;
            out = o;
        }
        ed::PinId in;
        ed::PinId out;
    };

    Filter(ControllerGraph *cg);
    Filter(ControllerGraph *cg, const crude_json::value &json);
    
    void AddEntry(ed::PinId i = ed::PinId::Invalid, ed::PinId o = ed::PinId::Invalid);
    virtual ed::PinId LinkWithAddPin(enum Pin::kind, enum Pin::type) override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;

    std::vector<PinPair> entries;
};
class Memory : public Node {
public:
    enum { Keep = 0 };
    struct PinPair {
        PinPair(ed::PinId i, ed::PinId o) {
            in = i;
            out = o;
        }
        ed::PinId in;
        ed::PinId out;
    };

    Memory(ControllerGraph *cg);
    Memory(ControllerGraph *cg, const crude_json::value &json);
    
    void AddEntry(ed::PinId i = ed::PinId::Invalid, ed::PinId o = ed::PinId::Invalid);
    virtual ed::PinId LinkWithAddPin(enum Pin::kind, enum Pin::type) override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;

    std::vector<PinPair> entries;
};

class Selector : public Node {
public:
    enum {
        Next = 0,
        Prev
    };

    Selector(ControllerGraph *cg);
    Selector(ControllerGraph *cg, const crude_json::value &json);
    void AddEntry(ed::PinId id = ed::PinId::Invalid);
    virtual ed::PinId LinkWithAddPin(enum Pin::kind, enum Pin::type) override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;

    ssize_t selected;
};
class Toggle : public Node {
    public:
    enum {
        In   = 0,
        OutA = 0,
        OutB = 1
    };

    Toggle(ControllerGraph *cg);
    Toggle(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;

    bool oldValue;
};

class Splitter : public Node {
    public:
    enum {
        In  = 0,
        Plus = 0,
        Minus = 1,
    };
    Splitter(ControllerGraph *cg);
    Splitter(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};

class Negate : public Node {
    public:
    enum {
        In  = 0,
        Out = 0,
    };
    Negate(ControllerGraph *cg);
    Negate(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};

class Decoder : public Node {
    public:
    enum {
        A = 0,
        B = 1,
    };
    Decoder(ControllerGraph *cg);
    Decoder(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};
class AndGate : public Node {
    public:
    enum {
        A = 0,
        B = 1,
    };
    AndGate(ControllerGraph *cg);
    AndGate(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};

class Inverter : public Node {
    public:
    enum {
        In  = 0,
        Out = 0,
    };
    Inverter(ControllerGraph *cg);
    Inverter(ControllerGraph *cg, const crude_json::value &json);

    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};

class Scaler : public Node {
    public:
    enum {
        In  = 0,
        Out = 0,
    };
    Scaler(ControllerGraph *cg);
    Scaler(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};
class Hat2Axis : public Node {
    public:
    enum {
        In  = 0,
        OutX = 0,
        OutY = 1
    };
    Hat2Axis(ControllerGraph *cg);
    Hat2Axis(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};
class Hat2Btn : public Node {
    public:
    enum {
        In  = 0,
        U = 0,
        D = 1,
        R = 2,
        L = 3
    };
    Hat2Btn(ControllerGraph *cg);
    Hat2Btn(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
};
class Btn2Axis : public Node {
    public:
    enum {
        In_P       = 0,
        In_N       = 1,
        Reset      = 2,
        Out        = 0
    };
    Btn2Axis(ControllerGraph *cg);
    Btn2Axis(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;

    float decay;
    float rampup;
};
class Btn2HalfAxis : public Node {
    public:
    enum {
        In_P = 0,
        Reset = 1,
        Out = 0
    };
    Btn2HalfAxis(ControllerGraph *cg);
    Btn2HalfAxis(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual void Draw() override;
    virtual crude_json::value ToJSON() override;

    float decay;
    float rampup;
};

class NavMode : public Node {
    enum {
        KillRot = 0,
        HLevel,
        Prograde,
        Retrograde,
        Normal,
        Antinormal,
        HAltitude,
        Clear
    };

    public:
    NavMode(ControllerGraph *cg);
    NavMode(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};

class PanelCtl : public Node {
    enum {
        SwitchLeft,
        SwitchRight,
        SwitchUp,
        SwitchDown,
        ShiftX,
        ShiftY,
        NoPanel,
        MFDOnly,
        Panel2D,
        VCockpit,
        ToggleMode
    };
    public:
    PanelCtl(ControllerGraph *cg);
    PanelCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};

class HUDCtl : public Node {
    enum {
        Show,
        Toggle,
        Switch,
        Orbit,
        Surface,
        Docking,
        ToggleColour,
        Intensity
    };
    public:
    HUDCtl(ControllerGraph *cg);
    HUDCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};

class TimeCtl : public Node {
    enum {
        TogglePause,
        Warp_01,
        Warp_1,
        Warp_10,
        Warp_100,
        Warp_1000,
        Warp_10000,
    };
    public:
    TimeCtl(ControllerGraph *cg);
    TimeCtl(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
};

class GraphNote : public Node {
    public:
    static const int MAX_NOTE_SIZE = 128;
    GraphNote(ControllerGraph *cg);
    GraphNote(ControllerGraph *cg, const crude_json::value &json);
    virtual crude_json::value ToJSON() override;
    virtual void Draw() override;
    char note[MAX_NOTE_SIZE];
};

class GraphNotification : public Node {
    public:
    static const int MAX_NOTE_SIZE = 128;
    GraphNotification(ControllerGraph *cg);
    GraphNotification(ControllerGraph *cg, const crude_json::value &json);
    virtual void UpdateOutputs() override;
    virtual crude_json::value ToJSON() override;
    virtual void Draw() override;
    enum GUIManager::NotifType type;
    char title[MAX_NOTE_SIZE];
    char content[MAX_NOTE_SIZE];
};

class KeyBinds: public Node {
    public:
    struct KeyBind {
        KeyBind(ed::PinId i, int k, bool c = false, bool a = false, bool s = false) {
            id = i;
            key = k;
            ctrl = c;
            alt = a;
            shift =s;
        }
        ed::PinId id;
        int key;
        bool ctrl;
        bool alt;
        bool shift;
    };

    KeyBinds(ControllerGraph *cg);
    KeyBinds(ControllerGraph *cg, const crude_json::value &json);
    virtual ed::PinId LinkWithAddPin(enum Pin::kind, enum Pin::type) override;
    virtual crude_json::value ToJSON() override;
    virtual void Draw() override;
    void AddEntry(ed::PinId e = ed::PinId::Invalid, int key = 0, bool ctrl = false, bool alt = false, bool shift = false);
    virtual void UpdateOutputs() override;

    std::vector<KeyBind> bindings;
};

class CursorCtl : public Node
{
    public:
    enum {
        Enable = 0,
        dX = 1,
        dY = 2,
        LClick = 3,
        RClick = 4
    };
    CursorCtl(ControllerGraph *);
    CursorCtl(ControllerGraph *cg, const crude_json::value &json);
    void Draw() override;
    virtual crude_json::value ToJSON() override;
    virtual void UpdateOutputs() override;
    virtual void SimulateOutputs() override;
    bool oldLClick;
    bool oldRClick;
    float maxspeed;
    float acceleration;
    float timepressed;
};

#endif
