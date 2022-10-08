#include "Controller.h"
#include "ControllerNodes.h"
#include "OrbiterAPI.h"
#include "Orbiter.h"

#include <imgui/imgui.h>
#include <imgui_node_editor.h>

#include <map>
#include <cassert>
#include <numeric>
#include <filesystem>
#include "utilities/widgets.h"
#include "font_awesome_5.h"

namespace fs = std::filesystem;

ax::NodeEditor::Utilities::BlueprintNodeBuilder builder;

Pin::Pin(Pin &&from) {
    id = from.id;
    strcpy(name, from.name);
    type = from.type;
    kind = from.kind;
    fVal = from.fVal;
    bVal = from.bVal;
    hVal = from.hVal;
    node = from.node;
    deadzone = from.deadzone;
    bOldValue = from.bOldValue;
    inputs = std::move(from.inputs);
    cbContextMenu = from.cbContextMenu;
    node->graph->pins[id.AsPointer()] = this;
    from.id = ed::PinId::Invalid;
}

Pin& Pin::operator=(Pin&&from) {
    node->graph->pins.erase(id.AsPointer());
    id = from.id;
    strcpy(name, from.name);
    type = from.type;
    kind = from.kind;
    fVal = from.fVal;
    bVal = from.bVal;
    hVal = from.hVal;
    node = from.node;
    deadzone = from.deadzone;
    bOldValue = from.bOldValue;
    inputs = std::move(from.inputs);
    cbContextMenu = from.cbContextMenu;
    node->graph->pins[id.AsPointer()] = this;
    from.id = ed::PinId::Invalid;
    return *this;
}

Pin::Pin(std::string n, enum Pin::type t, enum Pin::kind k, Node *nd, ed::PinId i) {
    node = nd;
    if(i == ed::PinId::Invalid)
        id = node->graph->GetId();
    else
        id = i;
    strncpy(name, n.c_str(), 32);
    type = t;
    kind = k;
    fVal = 0.0;
    bVal = false;
    hVal = 0;
    deadzone = 0;
    bOldValue = false;
    node->graph->pins[id.AsPointer()] = this;
    cbContextMenu = nullptr;
}

bool Pin::operator==(const Pin &other) {
    return other.id == id;
}

Pin::~Pin() {
    if(id != ed::PinId::Invalid) {
        node->graph->pins.erase(id.AsPointer());
        node->graph->links.erase(std::remove_if(node->graph->links.begin(), 
                            node->graph->links.end(),
                            [=](Link &x){return x.InputId == id || x.OutputId == id;}),
                        node->graph->links.end());
        node->graph->dirty = true;
    }
}

void Pin::UpdateInput() {
    switch(type) {
        case Button:
            bVal = false;
            for(auto &p: inputs) {
                bVal|=p->bVal;
            }
            break;
        case Trigger:
        {
            bool bIn = false;
            for(auto &p: inputs) {
                bIn|=p->bVal;
            }
            if(!bOldValue && bIn) {
                bVal = true;
            } else {
                bVal = false;
            }
            bOldValue = bIn;
            break;
        }
        case HalfAxis:
            fVal = 0.0f;
            for(auto &p: inputs) {
                fVal+=p->fVal;
            }
            fVal = std::clamp(fVal, 0.0f, 1.0f);
            break;
        case Axis:
            fVal = 0.0f;
            for(auto &p: inputs) {
                fVal+=p->fVal;
            }
            fVal = std::clamp(fVal, -1.0f, 1.0f);
            break;
        case Hat:
            hVal = false;
            for(auto &p: inputs) {
                hVal|=p->hVal;
            }
            break;
    }
}


void Hat2Str(int h, char *buf)
{
    char *p = buf;

    if(h == 0) {
        *p++ = '0';
    } else {
        if(h & GLFW_HAT_UP) {
            *p++ = 'U';
        }
        if(h & GLFW_HAT_DOWN) {
            *p++ = 'D';
        }
        if(h & GLFW_HAT_RIGHT) {
            *p++ = 'R';
        }
        if(h & GLFW_HAT_LEFT) {
            *p++ = 'L';
        }
    }
    *p++ = 0;
}

//Okabe_Ito <- c("#E69F00", "#56B4E9", "#009E73", "#F0E442", "#0072B2", "#D55E00", "#CC79A7", "#000000")
static const ImU32 colBtn   = IM_COL32(230, 159,   0, 255);
static const ImU32 colAxis  = IM_COL32( 86, 180, 233, 255);
static const ImU32 colHAxis = IM_COL32(  0, 158, 115, 255);
static const ImU32 colHat   = IM_COL32(240, 228,  66, 255);
static const ImU32 colTrg   = IM_COL32(204, 121, 167, 255);
static const ImU32 colWhite = IM_COL32(255, 255, 255, 255);

ImU32 GetIconColor(enum Pin::type type)
{
    switch (type)
    {
        default:
        case Pin::Button:   return colBtn;
        case Pin::Trigger:  return colBtn;
        case Pin::HalfAxis: return colHAxis;
        case Pin::Axis:     return colAxis;
        case Pin::Hat:      return colHat;
        case Pin::Add:      return colWhite;
        case Pin::Add_Button: return colBtn;
        case Pin::Add_Trigger: return colBtn;
    }
};

void DrawIcon(enum Pin::type type, bool connected, int alpha)
{
    ax::Drawing::IconType iconType;
    ImColor color = GetIconColor(type);
    color.Value.w = alpha / 255.0f;
    switch (type)
    {
        case Pin::Button:   iconType = ax::Drawing::IconType::Circle;  break;
        case Pin::Trigger:  iconType = ax::Drawing::IconType::Pulse;   break;
        case Pin::HalfAxis: iconType = ax::Drawing::IconType::Flow;    break;
        case Pin::Axis:     iconType = ax::Drawing::IconType::Diamond; break;
        case Pin::Hat:      iconType = ax::Drawing::IconType::Grid;    break;
        case Pin::Add:      iconType = ax::Drawing::IconType::Plus;    break;
        case Pin::Add_Button:      iconType = ax::Drawing::IconType::Plus;    break;
        case Pin::Add_Trigger:      iconType = ax::Drawing::IconType::Plus;    break;
        default: return;
    }

    ax::Widgets::Icon(ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

bool canLink(const Pin *a, const Pin *b) {
    const Pin *in = a;
    const Pin *out = b;
    if(a->kind == Pin::Output) {
        in = b;
        out = a;
    }
    if(in->kind == out->kind) return false;
    if(in->type == Pin::Add && out->type == Pin::Add) return false;
    if(in->type == Pin::Trigger && out->type == Pin::Button) return true;
    if(in->type == Pin::Button && out->type == Pin::Add_Button) return true;
    if(in->type == Pin::Trigger && out->type == Pin::Add_Button) return true;
    if(in->type == Pin::Add_Button && out->type == Pin::Button) return true;
    if(in->type == Pin::Add_Trigger && out->type == Pin::Button) return true;
    if(in->type == Pin::Add || out->type == Pin::Add) return true;

    if(in->type != out->type) return false;

    return true;
}

void DrawInput(const Pin &pin, bool minimized) {
    builder.Input(pin.id);
    auto alpha = ImGui::GetStyle().Alpha;

    if(pin.node->graph->draggedPin && !canLink(pin.node->graph->draggedPin, &pin)) {
        alpha = alpha * (48.0f / 255.0f);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    DrawIcon(pin.type, true, (int)(alpha * 255));
    if(!minimized)
        ImGui::TextUnformatted(pin.name);
    
    ImGui::PopStyleVar();
    builder.EndInput();
}

void DrawOutput(const Pin &pin, bool minimized)
{
    builder.Output(pin.id);
    auto alpha = ImGui::GetStyle().Alpha;
    if(pin.node->graph->draggedPin && !canLink(pin.node->graph->draggedPin, &pin)) {
        alpha = alpha * (48.0f / 255.0f);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if(!minimized) {
        char buf[8];
        switch(pin.type) {
            case Pin::Button:   ImGui::Text("%s : %d", pin.name, pin.bVal); break;
            case Pin::Trigger:  ImGui::Text("%s : %d", pin.name, pin.bVal); break;
            case Pin::Axis:     ImGui::Text("%s : %0.02f", pin.name, pin.fVal); break;
            case Pin::HalfAxis: ImGui::Text("%s : %0.02f", pin.name, pin.fVal); break;
            case Pin::Hat:      Hat2Str(pin.hVal, buf); ImGui::Text("%s : %s", pin.name, buf); break;
            case Pin::Add:      ImGui::Text("%s", pin.name); break;
            case Pin::Add_Button:      ImGui::Text("%s", pin.name); break;
        }
    }

    DrawIcon(pin.type, true, (int)(alpha * 255));
    ImGui::PopStyleVar();
    builder.EndOutput();
}

Node::Node(ControllerGraph *cg, const char *n) {
    graph = cg;
    name = n;
    id = graph->GetId();
    visible = true;
    deletable = true;
    is_controller = false;
    is_joystick = false;
    minimized = false;
}

void Node::EnableAddPins(bool in, bool out, enum Pin::type t) {
    if(in) {
        inAdd.emplace("New", t, Pin::Input, this, ed::PinId::Invalid);
    }
    if(out) {
        outAdd.emplace("New", t, Pin::Output, this, ed::PinId::Invalid);
    }
}

Node::Node(ControllerGraph *cg, const crude_json::value &json) {
    graph = cg;
    name  = json["name"].get<std::string>();
    id    = std::stoi(json["id"].get<std::string>());
    visible   = json["visible"].get<bool>();
    deletable = json["deletable"].get<bool>();
    is_controller = json["is_controller"].get<bool>();
    minimized = json["minimized"].get<bool>();
    is_joystick = json["is_joystick"].get<bool>();

    if(json.contains("inputs")) {
        for(auto &i: json["inputs"].get<crude_json::array>()) {
            std::string name = i["name"].get<std::string>();
            enum Pin::type type = (enum Pin::type)stoi(i["type"].get<std::string>());
            ed::PinId id = std::stoi(i["id"].get<std::string>());
            AddInput(name.c_str(), type, id);
        }
    }
    if(json.contains("outputs")) {
        for(auto &o: json["outputs"].get<crude_json::array>()) {
            std::string name = o["name"].get<std::string>();
            enum Pin::type type = (enum Pin::type)stoi(o["type"].get<std::string>());
            ed::PinId id = std::stoi(o["id"].get<std::string>());
            Pin &p = AddOutput(name.c_str(), type, id);
            if(o.contains("deadzone")) {
                float deadzone = o["deadzone"].get<crude_json::number>();
                p.deadzone = deadzone;
            }
        }
    }
}

crude_json::value Node::ToJSON() {
    crude_json::value ret;
    ret["id"] = std::to_string((ptrdiff_t)id.AsPointer());
    ret["name"] = name;
    ret["visible"] = visible;
    ret["deletable"] = deletable;
    ret["is_controller"] = is_controller;
    ret["is_joystick"] = is_joystick;
    ret["minimized"] = minimized;

    ImVec2 pos = ed::GetNodePosition(id);

    if(pos.x == FLT_MAX) pos.x = 0;
    if(pos.y == FLT_MAX) pos.y = 0;

    ret["xpos"] = pos.x;
    ret["ypos"] = pos.y;

    for(auto &n: inputs) {
        crude_json::value pin;
        pin["id"] = std::to_string((ptrdiff_t)n.id.AsPointer());
        pin["name"] = n.name;
        pin["type"] = std::to_string(n.type);
        ret["inputs"].push_back(pin);
    }
    for(auto &n: outputs) {
        crude_json::value pin;
        pin["id"] = std::to_string((ptrdiff_t)n.id.AsPointer());
        pin["name"] = n.name;
        pin["type"] = std::to_string(n.type);
        if(n.deadzone != 0.0f)
            pin["deadzone"] = n.deadzone;
        ret["outputs"].push_back(pin);
    }
    return ret;
}

void Node::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    builder.EndHeader();

    for(auto &i : inputs)
        DrawInput(i, minimized);
    if(inAdd) DrawInput(*inAdd, minimized);

    for(auto &o : outputs)
        DrawOutput(o, minimized);
    if(outAdd) DrawOutput(*outAdd, minimized);
}

void Node::RefreshInputs() {
    for(auto &i: inputs) {
        i.UpdateInput();
    }
}

Pin &Node::AddInput(const char *str, enum Pin::type t, ed::PinId id) {
    return inputs.emplace_back(str, t, Pin::Input, this, id);
}
Pin &Node::AddOutput(const char *str, enum Pin::type t, ed::PinId id) {
    return outputs.emplace_back(str, t, Pin::Output, this, id);
}

ControllerGraph::~ControllerGraph() {
    for(auto &n: nodes) {
        delete n;
    }
}
void ControllerGraph::Clear() {
    unsaved = true;
    links.clear();
    for(auto &n: nodes) {
        delete n;
    }
    nodes.clear();
    for(int i = 0; i < GLFW_JOYSTICK_LAST + 1; i++) {
        if(glfwJoystickPresent(i))
            nodes.push_back(new Joystick(this, i));
    }
    for(auto &c: controllers) {
        nodes.push_back(c(this));
        nodes.back()->visible = false;
    }
    dirty = true;
    SynchronizeJoysticks();
}

void ControllerGraph::Reload() {
    Load(filename.c_str());
}
void ControllerGraph::Load(const char *path) {
    filename = path;

    for(auto &n: nodes) {
        delete n;
    }
    nodes.clear();
    links.clear();
    std::pair<crude_json::value, bool> ret = crude_json::value::load(path);
    if(ret.second) {
        auto &json = ret.first;
        classname = json["classname"].get<std::string>();
        disabled = json["disabled"].get<bool>();
        if(disabled) {
            return;
        }
        ed::SetCurrentEditor(m_Context);
        if(json.contains("nodes")) {
            for(auto &n: json["nodes"].get<crude_json::array>()) {
                Node *node = NodeFromJSON(n);
                ImVec2 pos;
                pos.x = n["xpos"].get<crude_json::number>();
                pos.y = n["ypos"].get<crude_json::number>();
                ed::SetNodePosition(node->id, pos);
                nodes.push_back(node);
                dirty = true;
            }
        }
        if(json.contains("links")) {
            for(auto &l: json["links"].get<crude_json::array>()) {
                ed::LinkId id = std::stoi(l["id"].get<std::string>());
                ed::PinId inputPinId = std::stoi(l["in"].get<std::string>());
                ed::PinId outputPinId = std::stoi(l["out"].get<std::string>());
                links.push_back({ id, inputPinId, outputPinId });
                dirty = true;
            }
        }
        _lastid = std::stoi(json["_lastid"].get<std::string>());

        dirty = true;
        Refresh();
        unsaved = false;
        ed::SetCurrentEditor(nullptr);
        SynchronizeJoysticks();
    } else {
        oapiAddNotification(OAPINOTIF_ERROR, "Error loading file", path);
        disabled = true;
    }
}

void ControllerGraph::Save() {
    unsaved = false;
    crude_json::value out;
    for(auto &n: nodes) {
        out["nodes"].push_back(n->ToJSON());
    }
    for(auto &l: links) {
        crude_json::value link;
        link["id"] = std::to_string((ptrdiff_t)l.Id.AsPointer());
        link["in"] = std::to_string((ptrdiff_t)l.InputId.AsPointer());
        link["out"] = std::to_string((ptrdiff_t)l.OutputId.AsPointer());
        out["links"].push_back(link);
    }
    out["_lastid"] = std::to_string(_lastid);
    out["classname"] = classname;
    out["disabled"] = false;
    out.save(filename);
}

ControllerGraph::ControllerGraph() {
    unsaved = false;
    dirty = true;
    draggedPin = nullptr;
    disabled = false;
    ed::Config config;
    config.SettingsFile = nullptr;
    m_Context = ed::CreateEditor(&config);

    Refresh();
}
void ControllerGraph::Execute(int ctrl[15], int af[6]) {
    for(auto &n: sorted) {
        n->RefreshInputs();
        n->UpdateOutputs();
    }
    if(thrusters)
        thrusters->GetOutputs(ctrl);
    else
        memset(ctrl, 0, sizeof(int)*15);

    if(airfoils)
        airfoils->GetOutputs(af);
    else
        memset(af, 0, sizeof(int)*6);
}
void ControllerGraph::Simulate() {
    for(auto &n: sorted) {
        n->RefreshInputs();
        if(n->is_controller)
            n->SimulateOutputs();
        else {
            n->UpdateOutputs();
        }
    }
}

void ControllerGraph::Disable() {
    crude_json::value out;
    out["classname"] = classname;
    out["disabled"] = true;
    disabled = true;
    links.clear();
    for(auto &n: nodes) {
        delete n;
    }
    nodes.clear();
    sorted.clear();
    out.save(filename);
}

void ControllerGraph::Refresh() {
    if(!dirty) return;
    unsaved = true;
    dirty = false;
    sorted.clear();
    std::set<Node *> noincoming;
    //printf("------------Graph-----------\n");
    thrusters = nullptr;
    airfoils = nullptr;
    for(auto &n: nodes) {
        n->children.clear();
        n->parents.clear();
        for(auto &i : n->inputs) {
            i.inputs.clear();
        }
        if(dynamic_cast<Thrusters *>(n))
            thrusters = dynamic_cast<Thrusters *>(n);
        else if(dynamic_cast<AirCtl *>(n))
            airfoils = dynamic_cast<AirCtl *>(n);
    }
    for(auto &l: links) {
        Pin &from = PinFromId(l.InputId);
        Pin &to = PinFromId(l.OutputId);
        Node *parent = from.node;
        Node *child = to.node;
        parent->children.insert(child);
        child->parents.insert(parent);
        to.inputs.emplace_back(&from);
    }
    for(auto &n: nodes) {
        /*
        printf("Node %s, children :\n", n->name.c_str());
        for(auto &p: n->children) {
            printf("-%s\n",p->name.c_str());
        }
        for(auto &i : n->inputs) {
            if(i.inputs.size()) {
                printf("  Pin %s:\n",i.name);
                for(auto &p: i.inputs) {
                    printf("  ->%s\n",p->name);
                }
            }
        }*/
        if(n->parents.empty())
            noincoming.insert(n);
    }

    while(!noincoming.empty()) {
        const auto &n = noincoming.begin();
        Node *node = *n;
        noincoming.erase(n);
        sorted.push_back(node);
        for(auto &child: node->children) {
            child->parents.erase(node);
            if(child->parents.empty())
                noincoming.insert(child);
        }
    }
    //printf("sorted size=%d nodes=%d\n", sorted.size(), nodes.size());
    if(sorted.size() != nodes.size()) {
        oapiAddNotification(OAPINOTIF_ERROR, "Can link back to an ancestor node", "Removing last added link!\n");
        auto id = std::find_if(links.begin(), links.end(), [this](Link& link) { return link.Id == (ed::LinkId)_lastaddedlink; });
        if (id != links.end()) {
            links.erase(id);
            dirty = true;
        }
        _lastaddedlink = -1;
    }
/*
    printf("Sorted list:\n");
    for(auto &n: sorted) {
        printf("+%s\n", n->name.c_str());
    }
    */
}



ImVec4 GetStateColor(const Pin &pin) {
    ImVec4 color = {0.5,0.5,0.5,1.0};
    switch(pin.type) {
        case Pin::Axis:
            if(pin.fVal > 0.0f) {
                color.x = 0.5f * (1.0f-pin.fVal) + 0.902 * pin.fVal;
                color.y = 0.5f * (1.0f-pin.fVal) + 0.624 * pin.fVal;
                color.z = 0.5f * (1.0f-pin.fVal) + 0.000 * pin.fVal;
            } else {
                color.x = 0.5f * (1.0f+pin.fVal) - 0.337 * pin.fVal;
                color.y = 0.5f * (1.0f+pin.fVal) - 0.706 * pin.fVal;
                color.z = 0.5f * (1.0f+pin.fVal) - 0.914 * pin.fVal;
            }
            break;
        case Pin::HalfAxis:
        //colBtn 0.902, 0.624, 0    colAxis 0.337, 0.706, 0.914
            color.x = 0.5f * (1.0f-pin.fVal) + 0.902 * pin.fVal;
            color.y = 0.5f * (1.0f-pin.fVal) + 0.624 * pin.fVal;
            color.z = 0.5f * (1.0f-pin.fVal) + 0.000 * pin.fVal;
            break;
        case Pin::Trigger:
        case Pin::Button:
            if(pin.bVal) {
                color = {0.902,0.624,0.000,1.0};
            }
            break;
        case Pin::Hat:
            if(pin.hVal) {
                color = {0.902,0.624,0.000,1.0};
            }
            break;
    }
    return color;
}

//Enumerate glfw joysticks and joysticks present in the graph
//then [tries to] reconcile both
void ControllerGraph::SynchronizeJoysticks() {
    struct JoyBind {
        int joy_id;
        Joystick *joystick;
    };
    std::vector<JoyBind> jbindings;

    for(auto &n: nodes) {
        if(n->is_joystick) {
            Joystick *joy = (Joystick *)n;
            if(joy->connected && !glfwJoystickPresent(joy->joy_id)) {
                joy->connected = false;
                joy->joy_id = -1;
            }
            jbindings.push_back(JoyBind{joy->joy_id, joy});
        }
    }

    for(int j = 0; j < GLFW_JOYSTICK_LAST; j++) {
        int present = glfwJoystickPresent(j);
        if(present) {
            auto id = std::find_if(jbindings.begin(), jbindings.end(), [j](auto& jbind) { return jbind.joy_id == j; });
            if (id == jbindings.end()) {
                const char *guid = glfwGetJoystickGUID(j);
                const char *name = glfwGetJoystickName(j);
                auto id2 = std::find_if(jbindings.begin(), jbindings.end(), [guid](auto& jbind) { return jbind.joystick->guid == guid; });
                if (id2 == jbindings.end()) {
                    nodes.push_back(new Joystick(this, j));
                    dirty = true;
                } else {
                    id2->joystick->connected = true;
                    id2->joystick->joy_id = j;
                }
            }
        }
    }
}

void ControllerGraph::DrawKnownJoysticks() {
    for(auto &n: nodes) {
        if(n->is_joystick) {
            ImGui::Checkbox(n->name.c_str(), &n->visible);
        }
    }
}
void ControllerGraph::DrawControllers() {
    for(auto &n: nodes) {
        if(n->is_controller) {
            ImGui::Checkbox(n->name.c_str(), &n->visible);
        }
    }
}

static std::string g_copiedGraph;

void ControllerGraph::Editor() {
    ImGui::Text("Links: %d Nodes: %d Pins: %d", (int)links.size(), (int)nodes.size(),(int)pins.size());

    ed::SetCurrentEditor(m_Context);

    if(ImGui::Button(ICON_FA_SAVE" Save")) {
        Save();
    }
    ImGui::SameLine();
    if(ImGui::Button(ICON_FA_ERASER" Clear")) {
        Clear();
    }
    ImGui::SameLine();
    if(ImGui::Button(ICON_FA_TRASH" Discard")) {
        Reload();
        ed::SetCurrentEditor(m_Context);
    }
    ImGui::SameLine();
    if(ImGui::Button(ICON_FA_COPY" Copy")) {
        g_copiedGraph = filename;
    }
    ImGui::SameLine();
    if(g_copiedGraph.empty()) {
        ImGui::BeginDisabled();
        ImGui::Button(ICON_FA_PASTE" Paste");
        ImGui::EndDisabled();
    } else if(ImGui::Button(ICON_FA_PASTE" Paste")) {
        std::string fname = filename;
        std::string cname = classname;
        Load(g_copiedGraph.c_str());
        filename = fname;
        classname = cname;
        g_copiedGraph.clear();
        ed::SetCurrentEditor(m_Context);
        dirty = true;
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_GAMEPAD" Joysticks"))
        ImGui::OpenPopup("joystick_list");

    ImVec2 btn_pos = ImGui::GetItemRectMin();
    ImGui::SetNextWindowPos(btn_pos);

    if (ImGui::BeginPopup("joystick_list"))
    {
        ImGui::TextUnformatted("Joysticks");
        ImGui::Separator();
        DrawKnownJoysticks();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SLIDERS_H" Actions"))
        ImGui::OpenPopup("actions_list");

    btn_pos = ImGui::GetItemRectMin();
    ImGui::SetNextWindowPos(btn_pos);

    if (ImGui::BeginPopup("actions_list"))
    {
        ImGui::TextUnformatted("Actions");
        ImGui::Separator();
        //DrawTree();
        DrawControllers();
        ImGui::EndPopup();
    }

    ed::Begin("My Editor", ImVec2(0.0, 0.0f));

    for(auto &n : nodes) {
        if(n->visible) {
            builder.Begin(n->id);
            n->Draw();
            builder.End();
        }
    }

    for (const Link& link : links)
    {
        ImVec4 color = GetStateColor(PinFromId(link.InputId));
        ed::Link(link.Id, link.InputId, link.OutputId, color);
    }
    if (ed::BeginCreate())
    {
        ed::PinId inputPinId, outputPinId;
        if (ed::QueryNewLink(&inputPinId, &outputPinId))
        {
            if(inputPinId != ed::PinId::Invalid) {
                draggedPin = &PinFromId(inputPinId);
            } else {
                draggedPin = &PinFromId(outputPinId);
            }

            if(inputPinId != ed::PinId::Invalid && outputPinId != ed::PinId::Invalid) {
                auto ip = &PinFromId(inputPinId);
                auto op = &PinFromId(outputPinId);

                if (ip->kind == Pin::Input) {
                    std::swap(ip, op);
                    std::swap(inputPinId, outputPinId);
                }
                if(!canLink(ip, op)) {
                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if(ed::AcceptNewItem(ImColor(128, 255, 128), 2.0f)) {
                    if(op->type == Pin::Add_Trigger && ip->type == Pin::Add) {
                        // Add -> Add_Trigger
                        ed::PinId newpinin = ip->node->LinkWithAddPin(Pin::Output, Pin::Button);
                        ed::PinId newpinout = op->node->LinkWithAddPin(Pin::Input, Pin::Trigger);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), newpinin, newpinout });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(ip->type == Pin::Add_Trigger) {
                        // Add_Trigger should not be used as source
                        assert(0);
                    } else if(op->type == Pin::Add_Trigger) {
                        enum Pin::type type = Pin::Trigger;
                        ed::PinId newpin = op->node->LinkWithAddPin(Pin::Input, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), inputPinId, newpin });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(op->type == Pin::Add_Button && ip->type == Pin::Add) {
                        // Add -> Add_Button not needed for now...
                        assert(0);
                    } else if(ip->type == Pin::Add_Button && op->type == Pin::Add) {
                        enum Pin::type type = Pin::Button;
                        ed::PinId newpinin = ip->node->LinkWithAddPin(Pin::Output, type);
                        ed::PinId newpinout = op->node->LinkWithAddPin(Pin::Input, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), newpinin, newpinout });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(ip->type == Pin::Add_Button) {
                        enum Pin::type type = Pin::Button;
                        ed::PinId newpin = ip->node->LinkWithAddPin(Pin::Output, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), newpin, outputPinId });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(op->type == Pin::Add_Button) {
                        enum Pin::type type = Pin::Button;
                        ed::PinId newpin = op->node->LinkWithAddPin(Pin::Input, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), inputPinId, newpin });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(ip->type == Pin::Add) {
                        enum Pin::type type = op->type;
                        if(type == Pin::Trigger) type = Pin::Button;
                        ed::PinId newpin = ip->node->LinkWithAddPin(Pin::Output, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), newpin, outputPinId });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else if(op->type == Pin::Add) {
                        enum Pin::type type = ip->type;
                        if(type == Pin::Trigger) type = Pin::Button;
                        ed::PinId newpin = op->node->LinkWithAddPin(Pin::Input, type);
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), inputPinId, newpin });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    } else {
                        _lastaddedlink = _lastid;
                        links.push_back({ ed::LinkId(_lastid++), inputPinId, outputPinId });
                        dirty = true;
                        ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
                    }
                }
            }
        }
    } else
        draggedPin = nullptr;
    
    ed::EndCreate(); // Wraps up object creation action handling.

    if (ed::BeginDelete())
    {
        ed::LinkId linkId = 0;
        while (ed::QueryDeletedLink(&linkId))
        {
            if (ed::AcceptDeletedItem())
            {
                auto id = std::find_if(links.begin(), links.end(), [linkId](auto& link) { return link.Id == linkId; });
                if (id != links.end()) {
                    dirty = true;
                    links.erase(id);
                }
            }
        }

        ed::NodeId nodeId = 0;
        while (ed::QueryDeletedNode(&nodeId))
        {
            if (ed::AcceptDeletedItem())
            {
                auto id = std::find_if(nodes.begin(), nodes.end(), [nodeId](auto& node) { return node->id == nodeId; });
                if (id != nodes.end()) {
                    delete *id;
                    nodes.erase(id);
                    dirty = true;
                }
            }
        }
    }
    ed::EndDelete();


    static ed::PinId contextPinId;
    static ed::LinkId contextLinkId;
    static ed::NodeId contextNodeId;

    auto openPopupPosition = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowPinContextMenu(&contextPinId)) {
        auto &pin = PinFromId(contextPinId);
        if (pin.cbContextMenu) {
            ImGui::OpenPopup("Pin Context Menu");
        }
    } else if (ed::ShowLinkContextMenu(&contextLinkId)) {
        ImGui::OpenPopup("Link Context Menu");
    } else if (ed::ShowNodeContextMenu(&contextNodeId)) {
        ImGui::OpenPopup("Node Context Menu");
    }
    else if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("Create New Node");
    }

    if (ImGui::BeginPopup("Pin Context Menu"))
    {
        auto &pin = PinFromId(contextPinId);

        if (pin.cbContextMenu) pin.cbContextMenu(pin);

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Link Context Menu"))
    {
        if (ImGui::MenuItem("Delete link")) {
            ed::DeleteLink(contextLinkId);
            dirty = true;
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Node Context Menu"))
    {
        auto id = std::find_if(nodes.begin(), nodes.end(), [](auto& node) { return node->id == contextNodeId; });
        if (id != nodes.end()) {
            auto &node = *id;
            if(node->minimized) {
                if (ImGui::MenuItem("Maximize")) {
                    node->minimized = false;
                }
            } else {
                if (ImGui::MenuItem("Minimize")) {
                    node->minimized = true;
                }
            }
            if(node->deletable) {
                if (ImGui::MenuItem("Delete node")) {
                    ed::DeleteNode(contextNodeId);
                    dirty = true;
                }
            } else {
                if (ImGui::MenuItem("Hide node")) {
                    node->visible = false;
                }
            }
        }

        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePostion = openPopupPosition;
        Node *node = nullptr;
        ImGui::TextUnformatted("New Node");
        ImGui::Separator();
        if (ImGui::BeginMenu("Logic")) {
            if (ImGui::MenuItem("Negate"))
                node = new Negate(this);
            if (ImGui::MenuItem("Inverter"))
                node = new Inverter(this);
            if (ImGui::MenuItem("Toggle"))
                node = new Toggle(this);
            if (ImGui::MenuItem("Filter"))
                node = new Filter(this);
            if (ImGui::MenuItem("Decoder"))
                node = new Decoder(this);
            if (ImGui::MenuItem("Selector"))
                node = new Selector(this);
            if (ImGui::MenuItem("Memory"))
                node = new Memory(this);
            if (ImGui::MenuItem("And Gate"))
                node = new AndGate(this);
                
                
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Conversion")) {
            if (ImGui::MenuItem("Splitter"))
                node = new Splitter(this);
            if (ImGui::MenuItem("Scaler"))
                node = new Scaler(this);
            if (ImGui::MenuItem("Hat2Axis"))
                node = new Hat2Axis(this);
            if (ImGui::MenuItem("Hat2Btn"))
                node = new Hat2Btn(this);
            if (ImGui::MenuItem("Btn2Axis"))
                node = new Btn2Axis(this);
            if (ImGui::MenuItem("Btn2HalfAxis"))
                node = new Btn2HalfAxis(this);
               
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("KeyBinds"))
            node = new KeyBinds(this);
        if (ImGui::MenuItem("Note"))
            node = new GraphNote(this);
        if (ImGui::MenuItem("Notification"))
            node = new GraphNotification(this);


        if(node) {
            ed::SetNodePosition(node->id, newNodePostion);
            nodes.push_back(node);
            dirty = true;
        }

        if(ImGui::BeginMenu("Joysticks")) {
            for(auto &n : nodes) {
                if(n->is_joystick) {
                    ImGui::Checkbox(n->name.c_str(), &n->visible);
                }
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Actions")) {
            for(auto &n : nodes) {
                if(n->is_controller) {
                    ImGui::Checkbox(n->name.c_str(), &n->visible);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    ed::Resume();

    ed::End();

    ed::SetCurrentEditor(nullptr);
}

void InputController::GlobalInit() {
    printf("InputController::GlobalInit\n");

    ControllerGraph::Register<Joystick, false>("Joystick");
    ControllerGraph::Register<CameraCtl, true>("CameraCtl");
    ControllerGraph::Register<CursorCtl, true>("CursorCtl");
    ControllerGraph::Register<Thrusters, true>("Thrusters");
    ControllerGraph::Register<AirCtl, true>("AirCtl");
    ControllerGraph::Register<RCSCtl, true>("RCSCtl");
    ControllerGraph::Register<Splitter, false>("Splitter");
    ControllerGraph::Register<Negate, false>("Negate");
    ControllerGraph::Register<Inverter, false>("Inverter");
    ControllerGraph::Register<Scaler, false>("Scaler");
    ControllerGraph::Register<Hat2Axis, false>("Hat2Axis");
    ControllerGraph::Register<Hat2Btn, false>("Hat2Btn");
    ControllerGraph::Register<Btn2Axis, false>("Btn2Axis");
    ControllerGraph::Register<Btn2HalfAxis, false>("Btn2HalfAxis");
    ControllerGraph::Register<Toggle, false>("Toggle");
    ControllerGraph::Register<Memory, false>("Memory");
    ControllerGraph::Register<Filter, false>("Filter");
    ControllerGraph::Register<Decoder, false>("Decoder");
    ControllerGraph::Register<AndGate, false>("AndGate");
    ControllerGraph::Register<Selector, false>("Selector");
    ControllerGraph::Register<NavMode, true>("NavMode");
    ControllerGraph::Register<PanelCtl, true>("PanelCtl");
    ControllerGraph::Register<HUDCtl, true>("HUDCtl");
    ControllerGraph::Register<GraphNote, false>("GraphNote");
    ControllerGraph::Register<GraphNotification, false>("GraphNotification");
    ControllerGraph::Register<KeyBinds, false>("KeyBinds");
    ControllerGraph::Register<AFCtl, true>("AFCtl");
    ControllerGraph::Register<TimeCtl, true>("TimeCtl");
    
    if(!std::filesystem::exists("Controllers")) {
        std::filesystem::create_directory("Controllers");
    }

    for (auto &file : fs::directory_iterator("Controllers")) {
        if(file.path().extension() == ".json") {
            ControllerGraph *cg = new ControllerGraph();
            cg->Load(file.path().c_str());
            controllers[cg->classname].reset(cg);
        }
    }
    if(controllers.find("Default") != controllers.end()) {
        currentController = controllers["Default"].get();
    } else {
        ControllerGraph *cg = new ControllerGraph();
        cg->Clear();
        cg->filename = "Controllers/Default.json";
        cg->classname = "Default";
        ed::SetCurrentEditor(cg->m_Context);
        cg->Save();
        ed::SetCurrentEditor(nullptr);
        controllers["Default"].reset(cg);
        currentController = cg;
    }
}
void InputController::ProcessInput(int ctrl[15], int af[6]) {
    if(currentController)
        currentController->Execute(ctrl, af);
}
void InputController::JoystickCallback(int jid, int event) {
    if(event == GLFW_CONNECTED) {
        const char *jname = glfwGetJoystickName(jid);
        oapiAddNotification(OAPINOTIF_INFO, "Joystick connected", jname);
    } else if(event == GLFW_DISCONNECTED) {
        for(auto &n: currentController->nodes) {
            if(n->is_joystick) {
                Joystick *joy = (Joystick *)n;
                if(joy->joy_id == jid) {
                    const char *jname = joy->name.c_str();
                    oapiAddNotification(OAPINOTIF_WARNING, "Joystick disconnected", jname);
                }
            }
        }
    }
    for(auto &c: controllers) {
        c.second->SynchronizeJoysticks();
    }
}

void InputController::SwitchProfile(const char *profile) {
    auto it = controllers.find(profile);
    if(it != controllers.end()) {
        ControllerGraph *newController = controllers["Default"].get();
        if(!it->second->disabled) {
            newController = it->second.get();
        }
        if(currentController != newController) {
            currentController = newController;
            oapiAddNotification(OAPINOTIF_INFO, "Controller profile changed", currentController->classname.c_str());
            defaultTab = profile;
        }
    } else {
        oapiAddNotification(OAPINOTIF_INFO, "New controller profile added", profile);
        ControllerGraph *cg = new ControllerGraph();
        cg->Load("Controllers/Default.json");
        char buf[256];
        sprintf(buf, "Controllers/%s.json", profile);
        for(int i=strlen("Controllers/");i<strlen(buf);i++) {
            if(buf[i]=='/'||buf[i]=='\\') {
                buf[i]='-';
            }
        }
        cg->filename = buf;
        cg->classname = profile;
        ed::SetCurrentEditor(cg->m_Context);
        cg->Save();
        ed::SetCurrentEditor(nullptr);
        controllers[profile].reset(cg);
        currentController = cg;
        defaultTab = profile;
    }
}
void InputController::DrawEditor(bool ingame) {
    //const ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
    const ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyMask_;
    if (ImGui::BeginTabBar("ClassTabs", tab_bar_flags))
    {
        static ControllerGraph *td = nullptr;
        for(auto &ctrl: controllers) {
            auto graph = ctrl.second.get();
            if(graph->disabled)
                continue;
            bool open = true;
            bool *popen = &open;
            
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if(graph->unsaved) flags |= ImGuiTabItemFlags_UnsavedDocument;
            if(ctrl.first == "Default") {
                popen = nullptr;
                flags |= ImGuiTabItemFlags_Leading;
            }
            if(defaultTab == ctrl.first) {
                flags |= ImGuiTabItemFlags_SetSelected;
                defaultTab.clear();
            }
            if (ImGui::BeginTabItem(ctrl.first.c_str(), popen, flags))
            {
                if(graph != currentController || !ingame)
                    graph->Simulate();
                graph->Editor();
                graph->Refresh();
                ImGui::EndTabItem();
            }
            if(!open) {
                td = ctrl.second.get();
                ImGui::OpenPopup("Confirm");
            }
        }

        bool unused_open = true;
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Confirm", &unused_open))
        {
            ImGui::TextUnformatted("Are you sure you want to remove this vessel class profile?");
            ImGui::TextUnformatted("From now on it will use the Default profile.");
            ImGui::Text("You'll need to delete the %s file to reactivate it", td->filename.c_str());
            if (ImGui::Button("Confirm")) {
                oapiAddNotification(OAPINOTIF_INFO, "Controller profile disabled", td->classname.c_str());
                td->Disable();
                if(td == currentController) {
                    SwitchProfile("Default");
                }

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        
        ImGui::EndTabBar();
    }
}
