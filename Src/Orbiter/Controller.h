#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "OrbiterAPI.h"
#include <imgui_node_editor.h>
#include "utilities/builders.h"
#include <set>
#include <list>
#include <functional>
#include <memory>
#include <optional>

#include "crude_json.h"

namespace ed = ax::NodeEditor;

class Node;
class ControllerGraph;
class Thrusters;
class AirCtl;
struct Link
{
    ed::LinkId Id;
    ed::PinId  InputId;
    ed::PinId  OutputId;
};

class Pin final
{
public:
    enum type {
        Button,
        HalfAxis,
        Axis,
        Hat,
        Trigger,
        Add,
        Add_Button,
        Add_Trigger,
    };
    enum kind {
        Input,
        Output
    };
    Pin() = delete;
    Pin(Pin &) = delete;
    Pin(Pin &&from);
    Pin& operator=(Pin&& from);
    ~Pin();
    Pin(std::string n, Pin::type t, Pin::kind k, Node *, ed::PinId id = ed::PinId::Invalid);
    bool operator==(const Pin &other);
    void UpdateInput();
    bool hasInputs() { return !inputs.empty(); }
    ed::PinId id;
    char name[32];
    Pin::type type;
    Pin::kind kind;
    float fVal;
    float deadzone;
    bool bVal;
    bool bOldValue;
    int hVal;
    Node *node;

    std::function<void(Pin &)> cbContextMenu;

    std::list<Pin *> inputs;
};

using json_ctr = std::function<Node *(ControllerGraph *, const crude_json::value&)>;
#define MAX_COMMENT_SIZE 32

class Node
{
public:
    Node(ControllerGraph *cg, const char *n);
    Node(ControllerGraph *cg, const crude_json::value &json);
    virtual ~Node() {}
    virtual void Draw();
    virtual void UpdateOutputs() {};
    virtual void SimulateOutputs() {};

    void RefreshInputs();

    Pin &AddInput(const char *str, enum Pin::type t, ed::PinId id = ed::PinId::Invalid);
    Pin &AddOutput(const char *str, enum Pin::type t, ed::PinId id = ed::PinId::Invalid);

    virtual crude_json::value ToJSON();

    ed::NodeId id;
    std::string name;
    bool visible;
    bool deletable;
    bool is_controller;
    bool is_joystick;
    bool minimized;
    bool has_comment;
    char comment[MAX_COMMENT_SIZE];
    ControllerGraph *graph;

    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
    std::set<Node *> children;
    std::set<Node *> parents;

    void EnableAddPins(bool in, bool out, enum Pin::type = Pin::Add);
    std::optional<Pin> inAdd;
    std::optional<Pin> outAdd;
    bool bAddPin;
    virtual ed::PinId LinkWithAddPin(enum Pin::kind, enum Pin::type) { assert(0); };
};

class ControllerGraph final {
    public:
    std::vector<Link> links;
    std::vector<Node *> nodes;
    std::list<Node *> sorted;
    ed::EditorContext* m_Context;
    std::unordered_map<void *, Pin *> pins;
    std::string filename;
    std::string classname;
    Pin *draggedPin;
    int _lastaddedlink = -1;
    bool dirty;
    bool unsaved;
    bool disabled;
    Thrusters *thrusters;
    AirCtl *airfoils;

    Pin &PinFromId(ed::PinId id) {
        return *pins.at(id.AsPointer());
    }
    int _lastid = 1;
    int GetId(void) { return _lastid++; }

    ControllerGraph();
    ~ControllerGraph();
    void Execute(int thrusters[15], int af[6]);
    void Simulate();
    void Refresh();
    void Editor();
    void Save();
    void Load(const char *path);
    void Reload();
    void Clear();
    void Disable();
    void DrawKnownJoysticks();
    void SynchronizeJoysticks();
    void DrawControllers();

    using controller_ctr = std::function<Node *(ControllerGraph *)>;

    template <typename T, bool S> static void Register(std::string name) {
        classes[name]=[](ControllerGraph *cg, const crude_json::value&json)->Node * { return new T(cg, json); };
        if constexpr(S)
            controllers.push_back([](ControllerGraph *cg)->Node * { return new T(cg); });
    }
    static inline std::map<std::string, json_ctr> classes;
    static inline std::vector<controller_ctr> controllers;
    Node *NodeFromJSON(const crude_json::value &json) {
        const std::string &name = json["class"].get<std::string>();
        return classes.at(name)(this, json);
    }
};

class InputController {
public:
    static void GlobalInit();
    static void SwitchProfile(const char *);
    static void DrawEditor(bool ingame);
    static void ProcessInput(int ctrl[15], int af[6]);
    static void JoystickCallback(int jid, int event);

    static inline std::map<std::string, std::unique_ptr<ControllerGraph>> controllers;
    static inline ControllerGraph *currentController;
    static inline std::string defaultTab;
};

void DrawIcon(enum Pin::type type, bool connected, int alpha);
void DrawInput(const Pin &pin, bool minimized = false);
void DrawOutput(const Pin &pin, bool minimized = false);
extern ax::NodeEditor::Utilities::BlueprintNodeBuilder builder;

class DlgJoystick : public GUIElement {
public:
    DlgJoystick(const std::string &name): GUIElement(name, "DlgJoystick") {
        show = false;
    }
    void Show() override {
        if(!show) return;
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Joystick Configuration", &show);

        InputController::DrawEditor(true);

        ImGui::End();
    }
    static inline const std::string etype = "DlgJoystick";
};

#endif
