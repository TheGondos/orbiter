#include "ControllerNodes.h"
#include <imgui/imgui.h>
#include <imgui_node_editor.h>
#include "OrbiterAPI.h"
#include "Orbiter.h"
#include "Camera.h"
#include "Vessel.h"
#include "Pane.h"
#include "font_awesome_5.h"

extern Pane *g_pane;
extern Camera *g_camera;
extern TimeData td;
extern Vessel *g_focusobj;
extern Orbiter *g_pOrbiter;

class ControllerDB
{
    public:
    ControllerDB()
    {
        std::ifstream db("Config/gamecontrollerdb.txt");
        assert(db.is_open());

       	std::string line = "";
        while (!db.eof()) {
            oapiGetLine(db, line);
            if(line[0] == '#') continue;
            if(line[0] == '\0') continue;
//            printf("DB line: %s\n", line.c_str());

            size_t pos_start = 0, pos_end;
            std::string guid, name;
            if ((pos_end = line.find_first_of (",", pos_start)) != std::string::npos) {
                guid = line.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + 1;
            } else continue;

            if ((pos_end = line.find_first_of (",", pos_start)) != std::string::npos) {
                name = line.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + 1;
            } else continue;

            mappings[guid]["name"] = name;

            while ((pos_end = line.find_first_of (",", pos_start)) != std::string::npos) {
                std::string token = line.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + 1;

                {
                    size_t pos_start2 = 0, pos_end2;
                    if((pos_end2 = token.find_first_of (":", pos_start2)) != std::string::npos) {
                        std::string btn_name = token.substr (pos_start2, pos_end2 - pos_start2);
                        std::string name2 = token.substr(pos_end2 + 1);
//                        printf("b=%s n=%s\n", btn_name.c_str(), name2.c_str());

                        mappings[guid][name2] = btn_name;
                    }
                }
            }
        }
    }

    const char *GetButtonName(const char *guid, const char *btn) {
        if(mappings.find(guid) != mappings.end()) {
            const auto &js = mappings.at(guid);
            if(js.find(btn) != js.end()) {
                return js.at(btn).c_str();
            }
        }
        return btn;
    }
    std::map<std::string /*GUID*/, std::map<std::string /*btn*/, std::string /*name*/>> mappings;
};

Filter::Filter(ControllerGraph *cg):Node(cg, "Filter") {
    AddInput("Enable", Pin::Button);
    EnableAddPins(true, true);
}
Filter::Filter(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    if(json.contains("entries")) {
        for(auto &e: json["entries"].get<crude_json::array>()) {
            ed::PinId i = std::stoi(e["in"].get<std::string>());
            ed::PinId o = std::stoi(e["out"].get<std::string>());
            AddEntry(i, o);
        }
    }
    EnableAddPins(true, true);
}

ed::PinId Filter::LinkWithAddPin(enum Pin::kind k, enum Pin::type t) {
    AddEntry();
    auto &sp = entries.back();
    Pin &in = graph->PinFromId(sp.in);
    Pin &out = graph->PinFromId(sp.out);
    in.type = t;
    out.type = t;
    if(k == Pin::Input)
        return sp.in;
    else
        return sp.out;
}

crude_json::value Filter::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "Filter";
    for(auto &e: entries) {
        crude_json::value entry;
        entry["in"] = std::to_string((ptrdiff_t)e.in.AsPointer());
        entry["out"] = std::to_string((ptrdiff_t)e.out.AsPointer());
        ret["entries"].push_back(entry);
    }

    return ret;
}

void Filter::AddEntry(ed::PinId i, ed::PinId o) {
    Pin *in;
    Pin *out;
    graph->dirty = true;

    if(i != ed::PinId::Invalid) {
        in = &graph->PinFromId(i);
    } else {
        in = &AddInput("In", Pin::Axis);
    }

    if(o != ed::PinId::Invalid) {
        out = &graph->PinFromId(o);
    } else {
        out = &AddOutput("Out", Pin::Axis);
    }

    entries.emplace_back(in->id, out->id);

    auto cb = [this](Pin &pin){
        auto itpair = std::find_if(entries.begin(), entries.end(), [&pin](const PinPair &e) { return e.in == pin.id || e.out == pin.id; });
        auto itin   = std::find(inputs.begin(), inputs.end(), graph->PinFromId(itpair->in));
        auto itout  = std::find(outputs.begin(), outputs.end(), graph->PinFromId(itpair->out));
        bool clearlinks = false;
        if (ImGui::BeginMenu("Type"))
        {
            DrawIcon(Pin::Button, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Button")) {
                clearlinks = itin->type != Pin::Button;
                itin->type = Pin::Button;
                itout->type = Pin::Button;
            }
            DrawIcon(Pin::Axis, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Axis")) {
                clearlinks = itin->type != Pin::Axis;
                itin->type = Pin::Axis;
                itout->type = Pin::Axis;
            }
            DrawIcon(Pin::HalfAxis, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Half-Axis")) {
                clearlinks = itin->type != Pin::HalfAxis;
                itin->type = Pin::HalfAxis;
                itout->type = Pin::HalfAxis;
            }
            DrawIcon(Pin::Hat, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Hat")) {
                clearlinks = itin->type != Pin::Hat;
                itin->type = Pin::Hat;
                itout->type = Pin::Hat;
            }
            ImGui::EndMenu();
        }
        bool deletePair = false;
        if (ImGui::MenuItem("Delete")) {
            clearlinks = true;
            deletePair = true;
        }

        if(clearlinks) {
            graph->links.erase(std::remove_if(graph->links.begin(),
                            graph->links.end(),
                            [=](Link &x){return x.InputId == itout->id || x.OutputId == itin->id;}),
            graph->links.end());
            graph->dirty = true;
        }
        if(deletePair) {
            entries.erase(itpair);
            inputs.erase(itin);
            outputs.erase(itout);
            graph->dirty = true;
        }
    };

    in->cbContextMenu = cb;
    out->cbContextMenu = cb;
}
void Filter::SimulateOutputs() {
    UpdateOutputs();
}

void Filter::UpdateOutputs() {
    for(auto &pins: entries) {
        Pin &o = graph->PinFromId(pins.out);
        if(inputs[Enable].bVal) {
            Pin &i = graph->PinFromId(pins.in);
            o.bVal = i.bVal;
            o.fVal = i.fVal;
            o.hVal = i.hVal;
        } else {
            o.bVal = false;
            o.fVal = 0.0;
            o.hVal = 0;
        }
    }
}

void Filter::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    DrawInput(inputs[Enable], minimized);
    builder.EndHeader();

    for(auto &pins: entries) {
        Pin &i = graph->PinFromId(pins.in);
        DrawInput(i, minimized);
    }
    DrawInput(*inAdd, minimized);

    for(auto &pins: entries) {
        Pin &o = graph->PinFromId(pins.out);
        DrawOutput(o, minimized);
    }
    DrawOutput(*outAdd, minimized);
}

Memory::Memory(ControllerGraph *cg):Node(cg, "Memory") {
    AddInput("Keep values", Pin::Button);
    EnableAddPins(true, true);
}
Memory::Memory(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    if(json.contains("entries")) {
        for(auto &e: json["entries"].get<crude_json::array>()) {
            ed::PinId i = std::stoi(e["in"].get<std::string>());
            ed::PinId o = std::stoi(e["out"].get<std::string>());
            AddEntry(i, o);
        }
    }
    EnableAddPins(true, true);
}

crude_json::value Memory::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "Memory";
    for(auto &e: entries) {
        crude_json::value entry;
        entry["in"] = std::to_string((ptrdiff_t)e.in.AsPointer());
        entry["out"] = std::to_string((ptrdiff_t)e.out.AsPointer());
        ret["entries"].push_back(entry);
    }

    return ret;
}

ed::PinId Memory::LinkWithAddPin(enum Pin::kind k, enum Pin::type t) {
    AddEntry();
    auto &sp = entries.back();
    Pin &in = graph->PinFromId(sp.in);
    Pin &out = graph->PinFromId(sp.out);
    in.type = t;
    out.type = t;
    if(k == Pin::Input)
        return sp.in;
    else
        return sp.out;
}

void Memory::AddEntry(ed::PinId i, ed::PinId o) {
    Pin *in;
    Pin *out;
    graph->dirty = true;

    if(i != ed::PinId::Invalid) {
        in = &graph->PinFromId(i);
    } else {
        in = &AddInput("In", Pin::Axis);
    }

    if(o != ed::PinId::Invalid) {
        out = &graph->PinFromId(o);
    } else {
        out = &AddOutput("Out", Pin::Axis);
    }

    entries.emplace_back(in->id, out->id);

    auto cb = [this](Pin &pin){
        auto itpair = std::find_if(entries.begin(), entries.end(), [&pin](const PinPair &e) { return e.in == pin.id || e.out == pin.id; });
        auto itin   = std::find(inputs.begin(), inputs.end(), graph->PinFromId(itpair->in));
        auto itout  = std::find(outputs.begin(), outputs.end(), graph->PinFromId(itpair->out));
        bool clearlinks = false;
        if (ImGui::BeginMenu("Type"))
        {
            DrawIcon(Pin::Button, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Button")) {
                clearlinks = itin->type != Pin::Button;
                itin->type = Pin::Button;
                itout->type = Pin::Button;
            }
            DrawIcon(Pin::Axis, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Axis")) {
                clearlinks = itin->type != Pin::Axis;
                itin->type = Pin::Axis;
                itout->type = Pin::Axis;
            }
            DrawIcon(Pin::HalfAxis, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Half-Axis")) {
                clearlinks = itin->type != Pin::HalfAxis;
                itin->type = Pin::HalfAxis;
                itout->type = Pin::HalfAxis;
            }
            DrawIcon(Pin::Hat, true, 255); ImGui::SameLine();
            if (ImGui::MenuItem("Hat")) {
                clearlinks = itin->type != Pin::Hat;
                itin->type = Pin::Hat;
                itout->type = Pin::Hat;
            }
            ImGui::EndMenu();
        }
        bool deletePair = false;
        if (ImGui::MenuItem("Delete")) {
            clearlinks = true;
            deletePair = true;
        }

        if(clearlinks) {
            graph->links.erase(std::remove_if(graph->links.begin(),
                            graph->links.end(),
                            [=](Link &x){return x.InputId == itout->id || x.OutputId == itin->id;}),
            graph->links.end());
            graph->dirty = true;
        }
        if(deletePair) {
            entries.erase(itpair);
            inputs.erase(itin);
            outputs.erase(itout);
            graph->dirty = true;
        }
    };

    in->cbContextMenu = cb;
    out->cbContextMenu = cb;
}
void Memory::SimulateOutputs() {
    UpdateOutputs();
}
void Memory::UpdateOutputs() {
    for(auto &pins: entries) {
        Pin &o = graph->PinFromId(pins.out);
        if(!inputs[Keep].bVal) {
            Pin &i = graph->PinFromId(pins.in);
            o.bVal = i.bVal;
            o.fVal = i.fVal;
            o.hVal = i.hVal;
        }
    }
}
void Memory::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    DrawInput(inputs[Keep], minimized);
    builder.EndHeader();

    for(auto &pins: entries) {
        Pin &i = graph->PinFromId(pins.in);
        DrawInput(i, minimized);
    }
    DrawInput(*inAdd, minimized);

    for(auto &pins: entries) {
        Pin &o = graph->PinFromId(pins.out);
        DrawOutput(o, minimized);
    }
    DrawOutput(*outAdd, minimized);
}

Selector::Selector(ControllerGraph *cg):Node(cg, "Selector") {
    AddInput("Next", Pin::Trigger);
    AddInput("Prev", Pin::Trigger);
    AddEntry();
    EnableAddPins(false, true, Pin::Add_Button);
    selected = 0;
}
Selector::Selector(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    selected = 0;
    for(auto &e: outputs) {
        AddEntry(e.id);
    }
    EnableAddPins(false, true, Pin::Add_Button);
}

crude_json::value Selector::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Selector";
    return ret;
}
ed::PinId Selector::LinkWithAddPin(enum Pin::kind k, enum Pin::type t) {
    assert(k == Pin::Output);
    assert(t == Pin::Button);
    AddEntry();
    auto &pin = outputs.back();
    pin.type = t;
    return pin.id;
}

void Selector::AddEntry(ed::PinId id) {
    Pin *out;
    if(id != ed::PinId::Invalid) {
        out = &graph->PinFromId(id);
    } else {
        out = &AddOutput("Selection", Pin::Button);
    }

    auto cb = [this](Pin &pin) {
        ImGui::TextUnformatted("Mode");
        ImGui::Separator();

        auto it = std::find(outputs.begin(), outputs.end(), pin);

        if (outputs.size() > 1 && ImGui::MenuItem("Delete")) {
            outputs.erase(it);
            graph->dirty = true;
        }
        if (it != outputs.begin() && ImGui::MenuItem("Up")) {
            std::swap(*it, *(it - 1));
        }
        if (it != outputs.end() - 1 && ImGui::MenuItem("Down")) {
            std::swap(*it, *(it + 1));
        }
        if (ImGui::BeginMenu("Rename")) {
            if(ImGui::InputText("##new label", pin.name, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }
    };

    out->cbContextMenu = cb;
}
void Selector::SimulateOutputs() {
    UpdateOutputs();
}
void Selector::UpdateOutputs() {
    if(inputs[Next].bVal) {
        selected = (selected + 1) % outputs.size();
    }
    if(inputs[Prev].bVal) {
        selected--;
        if(selected < 0)
            selected = outputs.size() - 1;
    }
    int i = 0;
    for(auto &o: outputs) {
        o.bVal = selected == i;
        i++;
    }
}

void Selector::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    builder.EndHeader();
    DrawInput(inputs[Next], minimized);
    DrawInput(inputs[Prev], minimized);

    for(auto &o: outputs) {
        DrawOutput(o, minimized);
    }
    DrawOutput(*outAdd, minimized);
}

void Joystick::Draw() {
    if(!connected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.2);
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    builder.EndHeader();

    for(auto &o : outputs) {
        DrawOutput(o, minimized);
    }
    if(!connected) ImGui::PopStyleVar();
}

void Joystick::AddContextMenus() {
    int i = 0;
    const float *a = glfwGetJoystickAxes(joy_id, &nbAxis);

    auto cb = [this](Pin &pin) {
        ImGui::PushItemWidth(60.f);
        if(ImGui::DragFloat("Deadzone", &pin.deadzone, 0.01f, 0.0f, 0.9f, "%0.02f"))
            pin.node->graph->dirty = true;
        ImGui::PopItemWidth();
    };

    for(int n = 0; n < nbAxis; n++) {
        outputs[i].cbContextMenu = cb;
        i++;
    }
}

Joystick::Joystick(ControllerGraph *cg, int joy_id):Node(cg, glfwGetJoystickName(joy_id)),joy_id(joy_id) {
    visible = false;
    deletable = false;
    is_joystick = true;
    connected = true;
    static ControllerDB db;
    guid = glfwGetJoystickGUID(joy_id);
    glfwGetJoystickAxes(joy_id, &nbAxis);

    for(int i = 0; i < nbAxis; i++) {
        char buf[32];
        sprintf(buf, "a%d", i);
        const char *axisName = db.GetButtonName(guid.c_str(), buf);
        AddOutput(axisName, Pin::Axis);
    }

    glfwGetJoystickButtons(joy_id, &nbButtons);
    for(int i = 0; i < nbButtons; i++) {
        char buf[32];
        sprintf(buf, "b%d", i);
        const char *buttonName = db.GetButtonName(guid.c_str(), buf);
        AddOutput(buttonName, Pin::Button);
    }

    glfwGetJoystickHats(joy_id, &nbHats);
    for(int i = 0; i < nbHats; i++) {
        char buf[32];
        sprintf(buf, "h%d.1", i);
        std::string hatName = db.GetButtonName(guid.c_str(), buf);
        if(hatName == "dpup") {
            hatName = "DPad";
        } else {
            hatName = std::string("Hat") + std::to_string(i);
        }
        AddOutput(hatName.c_str(), Pin::Hat);
    }
    AddContextMenus();
}
Joystick::Joystick(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    guid = json["guid"].get<std::string>();
    joy_id = -1;
    connected = false;
}

crude_json::value Joystick::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Joystick";
    ret["guid"] = guid;

    return ret;
}

void Joystick::UpdateOutputs() {
    if(!connected) return;
    AddContextMenus();
    int i = 0;
    const float *a = glfwGetJoystickAxes(joy_id, &nbAxis);
    for(int n = 0; n < nbAxis; n++) {
        float f = a[n];
        float deadzone = outputs[i].deadzone;
        if(fabs(f) < deadzone) {
            f = 0.0f;
        } else if(f<0.0) {
            f = (f + deadzone) / (1.0f-deadzone);
        } else {
            f = (f - deadzone) / (1.0f-deadzone);
        }
        outputs[i].fVal = f;
        i++;
    }

    const unsigned char *b = glfwGetJoystickButtons(joy_id, &nbButtons);
    for(int n = 0; n < nbButtons; n++) {
        outputs[i].bVal = b[n] == GLFW_PRESS;
        i++;
    }

    const unsigned char * h = glfwGetJoystickHats(joy_id, &nbHats);
    for(int n = 0; n < nbHats; n++) {
        outputs[i].hVal = h[n];
        i++;
    }
}

Toggle::Toggle(ControllerGraph *cg):Node(cg, "Toggle") {
    AddInput("in", Pin::Trigger);
    AddOutput("A", Pin::Button);
    AddOutput("B", Pin::Button);
    outputs[OutA].bVal = true;
    minimized = true;
}
Toggle::Toggle(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    outputs[OutA].bVal = true;
}

crude_json::value Toggle::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Toggle";
    return ret;
}

void Toggle::UpdateOutputs() {
    if(inputs[In].bVal) {
        outputs[OutB].bVal = outputs[OutA].bVal;
        outputs[OutA].bVal = !outputs[OutB].bVal;
    }
}

Splitter::Splitter(ControllerGraph *cg):Node(cg, "Splitter") {
    AddInput("in", Pin::Axis);
    AddOutput("+", Pin::HalfAxis);
    AddOutput("-", Pin::HalfAxis);
}
Splitter::Splitter(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Splitter::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Splitter";
    return ret;
}

void Splitter::UpdateOutputs() {
    float f = inputs[In].fVal;
    if(f < 0.0) {
        outputs[Plus].fVal = 0.0;
        outputs[Minus].fVal = -f;
    } else {
        outputs[Plus].fVal = f;
        outputs[Minus].fVal = 0.0;
    }
}

Negate::Negate(ControllerGraph *cg):Node(cg, "Negate") {
    AddInput("in", Pin::Axis);
    AddOutput("out", Pin::Axis);
    minimized = true;
}
Negate::Negate(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Negate::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Negate";
    return ret;
}

void Negate::UpdateOutputs() {
    outputs[Out].fVal = -inputs[In].fVal;
}

Decoder::Decoder(ControllerGraph *cg):Node(cg, "Decoder")
{
    AddInput("A", Pin::Button);
    AddInput("B", Pin::Button);
    AddOutput("1", Pin::Button);
    AddOutput("2", Pin::Button);
    AddOutput("3", Pin::Button);
    AddOutput("4", Pin::Button);
}
Decoder::Decoder(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Decoder::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Decoder";
    return ret;
}

void Decoder::UpdateOutputs() {
    outputs[0].bVal = !inputs[A].bVal && !inputs[B].bVal;
    outputs[1].bVal =  inputs[A].bVal && !inputs[B].bVal;
    outputs[2].bVal = !inputs[A].bVal &&  inputs[B].bVal;
    outputs[3].bVal =  inputs[A].bVal &&  inputs[B].bVal;
}

AndGate::AndGate(ControllerGraph *cg):Node(cg, "And")
{
    AddInput("A", Pin::Button);
    AddInput("B", Pin::Button);
    AddOutput("Out", Pin::Button);
}
AndGate::AndGate(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value AndGate::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "AndGate";
    return ret;
}

void AndGate::UpdateOutputs() {
    outputs[0].bVal = inputs[A].bVal && inputs[B].bVal;
}

Inverter::Inverter(ControllerGraph *cg):Node(cg, "Inverter") {
    AddInput("in", Pin::Button);
    AddOutput("out", Pin::Button);
    minimized = true;
}
Inverter::Inverter(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Inverter::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Inverter";
    return ret;
}

void Inverter::UpdateOutputs() {
    outputs[Out].bVal = !inputs[In].bVal;
}

Scaler::Scaler(ControllerGraph *cg):Node(cg, "Scaler") {
    AddInput("in", Pin::Axis);
    AddOutput("out",Pin::HalfAxis);
    minimized = true;
}
Scaler::Scaler(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Scaler::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Scaler";
    return ret;
}

void Scaler::UpdateOutputs() {
    outputs[Out].fVal = (inputs[In].fVal + 1.0) / 2.0;
}

Hat2Axis::Hat2Axis(ControllerGraph *cg):Node(cg, "Hat2Axis") {
    AddInput("in", Pin::Hat);
    AddOutput("x", Pin::Axis);
    AddOutput("y", Pin::Axis);
}
Hat2Axis::Hat2Axis(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}

crude_json::value Hat2Axis::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Hat2Axis";
    return ret;
}

void Hat2Axis::UpdateOutputs() {
    outputs[OutX].fVal = 0;
    outputs[OutY].fVal = 0;
    if(inputs[In].hVal & GLFW_HAT_UP) {
        outputs[OutY].fVal+=1.0;
    }
    if(inputs[In].hVal & GLFW_HAT_RIGHT) {
        outputs[OutX].fVal+=1.0;
    }
    if(inputs[In].hVal & GLFW_HAT_DOWN) {
        outputs[OutY].fVal-=1.0;
    }
    if(inputs[In].hVal & GLFW_HAT_LEFT) {
        outputs[OutX].fVal-=1.0;
    }
}

Hat2Btn::Hat2Btn(ControllerGraph *cg):Node(cg, "Hat2Btn") {
    AddInput("in", Pin::Hat);
    AddOutput("U", Pin::Button);
    AddOutput("D", Pin::Button);
    AddOutput("R", Pin::Button);
    AddOutput("L", Pin::Button);
}
Hat2Btn::Hat2Btn(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {}
crude_json::value Hat2Btn::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Hat2Btn";
    return ret;
}

void Hat2Btn::UpdateOutputs() {
    outputs[U].bVal = inputs[In].hVal & GLFW_HAT_UP;
    outputs[D].bVal = inputs[In].hVal & GLFW_HAT_DOWN;
    outputs[R].bVal = inputs[In].hVal & GLFW_HAT_RIGHT;
    outputs[L].bVal = inputs[In].hVal & GLFW_HAT_LEFT;
}

Btn2Axis::Btn2Axis(ControllerGraph *cg):Node(cg, "Btn2Axis") {
    AddInput("+", Pin::Button);
    AddInput("-", Pin::Button);
    AddInput("reset", Pin::Button);
    AddOutput("out", Pin::Axis);
    decay = 1.0;
    rampup = 1.0;
}
Btn2Axis::Btn2Axis(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    decay  = json["decay"].get<crude_json::number>();
    rampup = json["rampup"].get<crude_json::number>();
}

crude_json::value Btn2Axis::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"]  = "Btn2Axis";
    ret["decay"]  = decay;
    ret["rampup"] = rampup;
    return ret;
}

void Btn2Axis::UpdateOutputs() {
    if(inputs[Reset].bVal ) {
        outputs[Out].fVal = 0;
    } else {
        if(inputs[In_P].bVal) {
            outputs[Out].fVal += rampup;
        }
        if(inputs[In_N].bVal) {
            outputs[Out].fVal -= rampup;
        }
        if(!inputs[In_P].bVal && !inputs[In_N].bVal) {
            if(outputs[Out].fVal > 0.0) {
                outputs[Out].fVal = std::max(0.0f, outputs[Out].fVal - decay);
            } else {
                outputs[Out].fVal = std::min(0.0f, outputs[Out].fVal + decay);
            }
        } else {
            outputs[Out].fVal = std::clamp(outputs[Out].fVal, -1.0f, 1.0f);
        }
    }
}

void Btn2Axis::Draw() {
    if(minimized) {
        Node::Draw();
    } else {
        builder.Header();
        ImGui::TextUnformatted(name.c_str());
        ImGui::PushItemWidth(60.f);
        ImGui::DragFloat("Decay", &decay, 0.01f, 0.01f, 1.0f, "%0.02f");
        ImGui::DragFloat("Ramp Up", &rampup, 0.01f, 0.01f, 1.0f, "%0.02f");
        ImGui::PopItemWidth();
        builder.EndHeader();

        DrawInput(inputs[Reset]);
        DrawInput(inputs[In_P]);
        DrawInput(inputs[In_N]);

        DrawOutput(outputs[Out]);
    }
}

Btn2HalfAxis::Btn2HalfAxis(ControllerGraph *cg):Node(cg, "Btn2HalfAxis") {
    AddInput("+", Pin::Button);
    AddInput("reset", Pin::Button);
    AddOutput("out", Pin::HalfAxis);
    decay = 1.0;
    rampup = 1.0;
}
Btn2HalfAxis::Btn2HalfAxis(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    decay  = json["decay"].get<crude_json::number>();
    rampup = json["rampup"].get<crude_json::number>();
}

crude_json::value Btn2HalfAxis::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "Btn2HalfAxis";
    ret["decay"]  = decay;
    ret["rampup"] = rampup;
    return ret;
}

void Btn2HalfAxis::UpdateOutputs() {
    if(inputs[Reset].bVal ) {
        outputs[Out].fVal = 0;
    } else {
        if(inputs[In_P].bVal) {
            outputs[Out].fVal = std::min(1.0f, outputs[Out].fVal + rampup);
        } else {
            outputs[Out].fVal = std::max(0.0f, outputs[Out].fVal - decay);
        }
    }
}

void Btn2HalfAxis::Draw() {
    if(minimized) {
        Node::Draw();
    } else {
        builder.Header();
        ImGui::TextUnformatted(name.c_str());
        builder.EndHeader();

        DrawInput(inputs[In_P]);
        DrawInput(inputs[Reset]);

        builder.Middle();
        ImGui::PushItemWidth(60.f);
        ImGui::DragFloat("Decay", &decay, 0.01f, 0.01f, 1.0f, "%0.02f");
        ImGui::DragFloat("Ramp Up", &rampup, 0.01f, 0.01f, 1.0f, "%0.02f");
        ImGui::PopItemWidth();

        DrawOutput(outputs[Out]);
    }
}


CursorCtl::CursorCtl(ControllerGraph *cg):Node(cg, "Virtual Mouse") {
    AddInput("Enable",  Pin::Button);
    AddInput("dX",  Pin::Axis);
    AddInput("dY",  Pin::Axis);
    AddInput("Left Click",  Pin::Button);
    AddInput("Right Click",  Pin::Button);
    
    deletable = false;
    is_controller = true;
    oldLClick = false;
    oldRClick = false;

    maxspeed = 10.0f;
    acceleration = 100.0f;
    timepressed = 0.0;
}
CursorCtl::CursorCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
    is_controller = true;
    oldLClick = false;
    oldRClick = false;

    maxspeed  = json["maxspeed"].get<crude_json::number>();
    acceleration = json["acceleration"].get<crude_json::number>();
    timepressed = 0.0;
}
void CursorCtl::Draw() {
    if(minimized) {
        Node::Draw();
    } else {
        builder.Header();
        ImGui::TextUnformatted(name.c_str());
        builder.EndHeader();
        DrawInput(inputs[Enable]);
        DrawInput(inputs[dX]);
        DrawInput(inputs[dY]);
        DrawInput(inputs[LClick]);
        DrawInput(inputs[RClick]);

        builder.Middle();
        ImGui::PushItemWidth(60.f);
        ImGui::DragFloat("Max Speed", &maxspeed, 10.0f, 10.0f, 100.0f, "%0.02f");
        ImGui::DragFloat("Acceleration", &acceleration, 1.0f, 10.0f, 100.0f, "%0.02f");
        ImGui::PopItemWidth();
    }
}

crude_json::value CursorCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "CursorCtl";
    ret["maxspeed"]  = maxspeed;
    ret["acceleration"] = acceleration;

    return ret;
}
void CursorCtl::SimulateOutputs() { 
    td.SysDT = 1.0/60.0;
    UpdateOutputs();
}
void CursorCtl::UpdateOutputs() {
    if (inputs[Enable].bVal) {
        if(inputs[dX].fVal != 0.0 || inputs[dY].fVal != 0.0) {
            
            float dx = inputs[dX].fVal * td.SysDT * 300.0;
            float dy = inputs[dY].fVal * td.SysDT * 300.0;
            if(inputs[dX].fVal * inputs[dX].fVal + inputs[dY].fVal * inputs[dY].fVal > 0.7) {
                timepressed += td.SysDT;
                dx += 0.5f * timepressed * timepressed * acceleration * inputs[dX].fVal;
                dy += 0.5f * timepressed * timepressed * acceleration * inputs[dY].fVal;
            }

            dx = std::clamp(dx, -maxspeed, maxspeed);
            dy = std::clamp(dy, -maxspeed, maxspeed);
            g_pOrbiter->m_pGUIManager->UpdateCursor(dx, dy);
        } else {
            timepressed = 0.0;
        }
    }

    if (inputs[LClick].bVal) {
        if(!oldLClick) {
            oldLClick = true;
            g_pOrbiter->m_pGUIManager->LeftClick(true);
        }
    } else {
        if(oldLClick) {
            oldLClick = false;
            g_pOrbiter->m_pGUIManager->LeftClick(false);
        }
    }

    if (inputs[RClick].bVal) {
        if(!oldRClick) {
            oldRClick = true;
            g_pOrbiter->m_pGUIManager->RightClick(true);
        }
    } else {
        if(oldRClick) {
            oldRClick = false;
            g_pOrbiter->m_pGUIManager->RightClick(false);
        }
    }
}

CameraCtl::CameraCtl(ControllerGraph *cg):Node(cg, "Camera") {
    AddInput("RotX",  Pin::Axis);
    AddInput("RotY",  Pin::Axis);
    AddInput("RotZ",  Pin::Axis);
    AddInput("Reset", Pin::Button);
    AddInput("Toggle View", Pin::Trigger);
    
    deletable = false;
    is_controller = true;
}
CameraCtl::CameraCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
    is_controller = true;
}

crude_json::value CameraCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "CameraCtl";
    return ret;
}
void CameraCtl::SimulateOutputs() {
}
void CameraCtl::UpdateOutputs() {
/*
    enum {
        RotX = 0,
        RotY = 1,
        RotZ = 2,
        Reset = 3,
        ToggleView = 4
    };
*/

    if (g_camera->IsExternal()) {  // use the joystick's coolie hat to rotate external camera
		g_camera->AddPhi   (td.SysDT * inputs[RotX].fVal);
		g_camera->AddTheta (td.SysDT * inputs[RotY].fVal);
	} else { // internal view
		if(inputs[Reset].bVal) {
			g_camera->ResetCockpitDir();
		} else {
			g_camera->Rotate (-td.SysDT * inputs[RotX].fVal,  -td.SysDT * inputs[RotY].fVal, true);
		}
	}
    if(inputs[ToggleView].bVal) {
        g_pOrbiter->SetView (g_focusobj, !g_camera->IsExternal());
    }
}

Thrusters::Thrusters(ControllerGraph *cg):Node(cg, "Thrusters") {
    AddInput("Main",Pin::HalfAxis);
    AddInput("Retro",Pin::HalfAxis);
    AddInput("Hover",Pin::HalfAxis);
    AddInput("Pitch Up", Pin::HalfAxis);
    AddInput("Pitch Down", Pin::HalfAxis);
    AddInput("Yaw Left", Pin::HalfAxis);
    AddInput("Yaw Right", Pin::HalfAxis);
    AddInput("Bank Left", Pin::HalfAxis);
    AddInput("Bank Right", Pin::HalfAxis);
    AddInput("Right", Pin::HalfAxis);
    AddInput("Left", Pin::HalfAxis);
    AddInput("Up", Pin::HalfAxis);
    AddInput("Down", Pin::HalfAxis);
    AddInput("Forward", Pin::HalfAxis);
    AddInput("Backward", Pin::HalfAxis);
    deletable = false;
    is_controller = true;
}
Thrusters::Thrusters(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}
void Thrusters::GetOutputs(int thrusters[15]) {
    for(int i = 0; i < 15; i++) {
        thrusters[i] = inputs[i].fVal * 1000;
    }
}
crude_json::value Thrusters::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "Thrusters";
    return ret;
}
void Thrusters::SimulateOutputs() {
}
void Thrusters::UpdateOutputs() {
}

AirCtl::AirCtl(ControllerGraph *cg):Node(cg, "Airfoils") {
    AddInput("Elevator", Pin::Axis);
    AddInput("Rudder", Pin::Axis);
    AddInput("Aileron", Pin::Axis);
    AddInput("Flap", Pin::Axis);
    AddInput("ElevatorTrim", Pin::Axis);
    AddInput("RudderTrim", Pin::Axis);
    deletable = false;
    is_controller = true;
}
AirCtl::AirCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}

crude_json::value AirCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "AirCtl";
    return ret;
}
void AirCtl::GetOutputs(int af[6]) {
    for(int i = 0; i < 6; i++) {
        af[i] = inputs[i].fVal * 1000;
    }
}
void AirCtl::SimulateOutputs() {
}
void AirCtl::UpdateOutputs() {
}

AFCtl::AFCtl(ControllerGraph *cg):Node(cg, "Air Surfaces Mode") {
    AddInput("Toggle", Pin::Trigger);
    AddInput("Elevator", Pin::Trigger);
    AddInput("Rudder", Pin::Trigger);
    AddInput("Aileron", Pin::Trigger);
    AddOutput("Elevator", Pin::Button);
    AddOutput("Rudder", Pin::Button);
    AddOutput("Aileron", Pin::Button);
    deletable = false;
    is_controller = true;
    ctrlsurfmode = 0;
}

AFCtl::AFCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
    ctrlsurfmode = 0;
}

crude_json::value AFCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "AFCtl";
    return ret;
}
void AFCtl::SimulateOutputs() {
    if(inputs[In_Toggle].bVal) {
        ctrlsurfmode = (ctrlsurfmode ? 0 : 7);
    }

    if(inputs[In_Elevator].bVal) {
        ctrlsurfmode^=1;
    }
    if(inputs[In_Rudder].bVal) {
        ctrlsurfmode^=2;
    }
    if(inputs[In_Aileron].bVal) {
        ctrlsurfmode^=4;
    }

    outputs[Out_Elevator].bVal = (ctrlsurfmode & 1) !=0;
    outputs[Out_Rudder].bVal   = (ctrlsurfmode & 2) !=0;
    outputs[Out_Aileron].bVal  = (ctrlsurfmode & 4) !=0;
}
void AFCtl::UpdateOutputs() {
    /*
    enum {
        In_Toggle = 0,
        In_Elevator = 1,
        In_Rudder = 2,
        In_Aileron = 3,
        Out_Elevator = 0,
        Out_Rudder = 1,
        Out_Aileron = 2,
    };
   	int ToggleADCtrlMode ();
	void SetADCtrlMode (int mode, bool fromstream = false);
    int VESSEL::GetADCtrlMode () const

    */

    if(inputs[In_Toggle].bVal) {
        g_focusobj->ToggleADCtrlMode();
    }

    ctrlsurfmode = g_focusobj->GetADCtrlMode();
    if(inputs[In_Elevator].bVal) {
        ctrlsurfmode^=1;
    }
    if(inputs[In_Rudder].bVal) {
        ctrlsurfmode^=2;
    }
    if(inputs[In_Aileron].bVal) {
        ctrlsurfmode^=4;
    }
    g_focusobj->SetADCtrlMode(ctrlsurfmode);

    outputs[Out_Elevator].bVal = (ctrlsurfmode & 1) !=0;
    outputs[Out_Rudder].bVal   = (ctrlsurfmode & 2) !=0;
    outputs[Out_Aileron].bVal  = (ctrlsurfmode & 4) !=0;

}


RCSCtl::RCSCtl(ControllerGraph *cg):Node(cg, "RCS Mode") {
    AddInput("Disable", Pin::Trigger);
    AddInput("Translation", Pin::Trigger);
    AddInput("Rotation", Pin::Trigger);
    AddInput("Toggle", Pin::Trigger);
    AddOutput("Translation", Pin::Button);
    AddOutput("Rotation", Pin::Button);
    deletable = false;
    is_controller = true;
}

RCSCtl::RCSCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}

crude_json::value RCSCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "RCSCtl";
    return ret;
}

void RCSCtl::SimulateOutputs() {
        /*
        Disable = 0,
        Translation = 1,
        Rotation = 2,
        Toggle = 3,
        Translation = 0,
        Rotation = 1
    */

    if(inputs[In_Disable].bVal ) {
        outputs[Out_Translation].bVal = false;
        outputs[Out_Rotation].bVal = false;
    }
    if(inputs[In_Translation].bVal ) {
        outputs[Out_Translation].bVal = true;
        outputs[Out_Rotation].bVal = false;
    }
    if(inputs[In_Rotation].bVal ) {
        outputs[Out_Translation].bVal = false;
        outputs[Out_Rotation].bVal = true;
    }
    if(inputs[In_Toggle].bVal ) {
        outputs[Out_Translation].bVal = !outputs[Out_Translation].bVal;
        outputs[Out_Rotation].bVal = !outputs[Out_Translation].bVal;
    }
}
void RCSCtl::UpdateOutputs() {
/*
    enum {
        Enable = 0,
        Toggle = 1,
        Translation = 0,
        Rotation = 1
    };
    	int ToggleAttMode ();
	bool SetAttMode (int mode, bool fromstream = false);

*/
    if(inputs[In_Disable].bVal) {
        g_focusobj->SetAttMode(0);
    }
    if(inputs[In_Translation].bVal) {
        g_focusobj->SetAttMode(2);
    }
    if(inputs[In_Rotation].bVal) {
        g_focusobj->SetAttMode(1);
    }
    if(inputs[In_Toggle].bVal) {
        g_focusobj->ToggleAttMode();
    }
    int mode = g_focusobj->AttMode();
    outputs[Out_Rotation].bVal = (mode & 1) !=0;
    outputs[Out_Translation].bVal = (mode & 2) !=0;
}

NavMode::NavMode(ControllerGraph *cg):Node(cg, "Autopilots") {
    AddInput("Kill Rotation", Pin::Trigger);
    AddInput("Horizontal Level", Pin::Trigger);
    AddInput("Prograde", Pin::Trigger);
    AddInput("Retrograde", Pin::Trigger);
    AddInput("Normal", Pin::Trigger);
    AddInput("Antinormal", Pin::Trigger);
    AddInput("Hold Altitude", Pin::Trigger);
    AddInput("Clear", Pin::Trigger);
    deletable = false;
    is_controller = true;
}
NavMode::NavMode(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}

crude_json::value NavMode::ToJSON() {
    crude_json::value ret = Node::ToJSON();

    ret["class"] = "NavMode";
    return ret;
}
void NavMode::SimulateOutputs() {
}
void NavMode::UpdateOutputs() {
    if(inputs[KillRot].bVal) {
        g_focusobj->TglNavMode(NAVMODE_KILLROT);
    }
    if(inputs[HLevel].bVal) {
        g_focusobj->TglNavMode(NAVMODE_HLEVEL);
    }
    if(inputs[Prograde].bVal) {
        g_focusobj->TglNavMode(NAVMODE_PROGRADE);
    }
    if(inputs[Retrograde].bVal) {
        g_focusobj->TglNavMode(NAVMODE_RETROGRADE);
    }
    if(inputs[Normal].bVal) {
        g_focusobj->TglNavMode(NAVMODE_NORMAL);
    }
    if(inputs[Antinormal].bVal) {
        g_focusobj->TglNavMode(NAVMODE_ANTINORMAL);
    }
    if(inputs[HAltitude].bVal) {
        g_focusobj->TglNavMode(NAVMODE_HOLDALT);
    }
    if(inputs[Clear].bVal) {
        g_focusobj->SetNavMode(0);
    }
}

PanelCtl::PanelCtl(ControllerGraph *cg):Node(cg, "Panel") {
    AddInput("Switch Left", Pin::Trigger);
    AddInput("Switch Right", Pin::Trigger);
    AddInput("Switch Up", Pin::Trigger);
    AddInput("Switch Down", Pin::Trigger);

    AddInput("ShiftX", Pin::Axis);
    AddInput("ShiftY", Pin::Axis);

    AddInput("No Panel", Pin::Trigger);
    AddInput("MFDs Only", Pin::Trigger);
    AddInput("2D Panel View", Pin::Trigger);
    AddInput("Virtual Cockpit", Pin::Trigger);
    AddInput("Toggle Panel Mode", Pin::Trigger);
    deletable = false;
    is_controller = true;
}
PanelCtl::PanelCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}

crude_json::value PanelCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "PanelCtl";
    return ret;
}
void PanelCtl::SimulateOutputs() {
        td.SysDT = 1.0/60.0;
}
void PanelCtl::UpdateOutputs() {
    if(inputs[ShiftX].fVal != 0.0 || inputs[ShiftY].fVal != 0.0) {
    	g_pane->ShiftPanel (inputs[ShiftX].fVal * td.SysDT * 600.0, inputs[ShiftY].fVal * td.SysDT * 600.0);
    }

    if(inputs[SwitchLeft].bVal) {
        g_pane->SwitchPanel (0);
    }
    if(inputs[SwitchRight].bVal) {
        g_pane->SwitchPanel (1);
    }
    if(inputs[SwitchUp].bVal) {
        g_pane->SwitchPanel (2);
    }
    if(inputs[SwitchDown].bVal) {
        g_pane->SwitchPanel (3);
    }
    if(inputs[NoPanel].bVal) {
        g_pane->SetPanelMode(0);
    }
    if(inputs[MFDOnly].bVal) {
        g_pane->SetPanelMode(1);
    }
    if(inputs[Panel2D].bVal) {
        g_pane->SetPanelMode(2);
    }
    if(inputs[VCockpit].bVal) {
        g_pane->SetPanelMode(3);
    }
    if(inputs[ToggleMode].bVal) {
        g_pane->TogglePanelMode();
    }
}

TimeCtl::TimeCtl(ControllerGraph *cg):Node(cg, "Time") {
    AddInput("Toggle Pause", Pin::Trigger);
    AddInput("Warp x0.1", Pin::Trigger);
    AddInput("Warp x1", Pin::Trigger);
    AddInput("Warp x10", Pin::Trigger);
    AddInput("Warp x100", Pin::Trigger);
    AddInput("Warp x1000", Pin::Trigger);
    AddInput("Warp x10000", Pin::Trigger);
    is_controller = true;
    deletable = false;
}
void TimeCtl::UpdateOutputs() {
    if(inputs[TogglePause].bVal) {
        g_pOrbiter->TogglePause();
    }
    if(inputs[Warp_01].bVal) {
        g_pOrbiter->SetWarpFactor (0.1);
    }
    if(inputs[Warp_1].bVal) {
        g_pOrbiter->SetWarpFactor (1.0);
    }
    if(inputs[Warp_10].bVal) {
        g_pOrbiter->SetWarpFactor (10.0);
    }
    if(inputs[Warp_100].bVal) {
        g_pOrbiter->SetWarpFactor (100.0);
    }
    if(inputs[Warp_1000].bVal) {
        g_pOrbiter->SetWarpFactor (1000.0);
    }
    if(inputs[Warp_10000].bVal) {
        g_pOrbiter->SetWarpFactor (10000.0);
    }
}
TimeCtl::TimeCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}

crude_json::value TimeCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "TimeCtl";
    return ret;
}
void TimeCtl::SimulateOutputs() {
}

HUDCtl::HUDCtl(ControllerGraph *cg):Node(cg, "HUD") {
    AddInput("Show", Pin::Button);
    AddInput("Toggle On/Off", Pin::Trigger);
    AddInput("Switch Mode", Pin::Trigger);
    AddInput("Orbit", Pin::Trigger);
    AddInput("Surface", Pin::Trigger);
    AddInput("Docking", Pin::Trigger);
    AddInput("Change Color", Pin::Trigger);
    AddInput("Brightness", Pin::HalfAxis);
    is_controller = true;
    deletable = false;
}
void HUDCtl::UpdateOutputs() {
    if(inputs[Show].hasInputs()) {
        g_pane->ShowHUD(inputs[Show].bVal);
    }
    if(inputs[Toggle].bVal) {
        g_pane->ToggleHUD();
    }
    if(inputs[Switch].bVal) {
        g_pane->SwitchHUDMode();
    }
    if(inputs[Orbit].bVal) {
        g_pane->SetHUDMode(HUD_ORBIT);
    }
    if(inputs[Surface].bVal) {
        g_pane->SetHUDMode(HUD_SURFACE);
    }
    if(inputs[Docking].bVal) {
        g_pane->SetHUDMode(HUD_DOCKING);
    }
    if(inputs[ToggleColour].bVal) {
        g_pane->ToggleHUDColour();
    }
    if(inputs[Intensity].hasInputs()) {
        double currentIntensity = g_pane->HudIntens();
        float newIntensity = inputs[Intensity].fVal;
        if(fabs(newIntensity - currentIntensity) > 0.01) {
            g_pane->SetHUDIntens(newIntensity);
        }
    }
}

HUDCtl::HUDCtl(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    deletable = false;
}
crude_json::value HUDCtl::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "HUDCtl";
    return ret;
}
void HUDCtl::SimulateOutputs() {
    float brightness = 1.0f;
    if(inputs[0].inputs.size() != 0) {
        brightness = inputs[0].fVal;
    }
}
GraphNote::GraphNote(ControllerGraph *cg):Node(cg, "Note") {
    note[0] = '\0';
}
GraphNote::GraphNote(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    strcpy(note, json["note"].get<crude_json::string>().c_str());
}
crude_json::value GraphNote::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "GraphNote";
    ret["note"] = note;
    return ret;
}
void GraphNote::Draw() {
    ImGui::SetNextItemWidth(80);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImVec4 col = {0,0,0,0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, col);
    if(ImGui::InputText("##new label", note, MAX_NOTE_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
        graph->dirty = true;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

GraphNotification::GraphNotification(ControllerGraph *cg):Node(cg, "Notification") {
    AddInput("Show", Pin::Trigger);
    type = GUIManager::Info;
    title[0] = '\0';
    content[0] = '\0';
}
GraphNotification::GraphNotification(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    strcpy(title, json["title"].get<crude_json::string>().c_str());
    strcpy(content, json["content"].get<crude_json::string>().c_str());
}
crude_json::value GraphNotification::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "GraphNotification";
    ret["title"] = title;
    ret["content"] = content;
    return ret;
}

void GraphNotification::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    builder.EndHeader();

    DrawInput(inputs[0], minimized);

    const char* items[] = { ICON_FA_CHECK_CIRCLE, ICON_FA_EXCLAMATION_TRIANGLE, ICON_FA_TIMES_CIRCLE, ICON_FA_INFO_CIRCLE };
    int idx = (int)type;
    for(int i = 0; i < 4; i++) {
        if(i == idx) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.4f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.3f, 0.4f, 1.0f});
            ImGui::Button(items[i]); ImGui::SameLine();
            ImGui::PopStyleColor(2);
        } else {
            if(ImGui::Button(items[i])) {
                type = (enum GUIManager::NotifType)i;
            }
            ImGui::SameLine();
        }
    }
    ImGui::NewLine();
    ImGui::PushItemWidth(100);
    
    if(ImGui::InputText("Title", title, MAX_NOTE_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
        graph->dirty = true;
    }
    if(ImGui::InputText("Content", content, MAX_NOTE_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
        graph->dirty = true;
    }
    ImGui::PopItemWidth();
}
void GraphNotification::UpdateOutputs() {
    if(inputs[0].bVal) {
        oapiAddNotification(type, title, content);
    }
}

static char kstate[256] = {0};

KeyBinds::KeyBinds(ControllerGraph *cg):Node(cg, "KeyBinds") {
    EnableAddPins(true, false, Pin::Add_Trigger);
}
KeyBinds::KeyBinds(ControllerGraph *cg, const crude_json::value &json):Node(cg, json) {
    if(json.contains("bindings")) {
        for(auto &i: json["bindings"].get<crude_json::array>()) {
            bool ctrl = i["ctrl"].get<bool>();
            bool alt = i["alt"].get<bool>();
            bool shift = i["shift"].get<bool>();

            AddEntry(stoi(i["id"].get<std::string>()), stoi(i["key"].get<std::string>()), ctrl, alt, shift);
        }
    }
    EnableAddPins(true, false, Pin::Add_Trigger);
}
crude_json::value KeyBinds::ToJSON() {
    crude_json::value ret = Node::ToJSON();
    ret["class"] = "KeyBinds";

    for(auto &e: bindings) {
        crude_json::value entry;
        entry["id"] = std::to_string((ptrdiff_t)e.id.AsPointer());
        entry["key"] = std::to_string(e.key);
        entry["ctrl"] = e.ctrl;
        entry["alt"] = e.alt;
        entry["shift"] = e.shift;

        ret["bindings"].push_back(entry);
    }

    return ret;
}
ed::PinId KeyBinds::LinkWithAddPin(enum Pin::kind k, enum Pin::type t) {
    assert(k == Pin::Input);
    assert(t == Pin::Trigger);
    AddEntry();
    auto &pin = inputs.back();
    pin.type = t;
    return pin.id;
}
void KeyBinds::AddEntry(ed::PinId e, int key, bool ctrl, bool alt, bool shift) {
    Pin *in;
    if(e != ed::PinId::Invalid) {
        in = &graph->PinFromId(e);
    } else {
        in = &AddInput("In", Pin::Trigger);
    }

    bindings.emplace_back(in->id, key, ctrl, alt, shift);

    auto cb = [this](Pin &pin) {
        if (ImGui::MenuItem("Delete")) {
            auto itbind = std::find_if(bindings.begin(), bindings.end(), [&pin](const KeyBind &e) { return e.id == pin.id; });
            auto itpin  = std::find(inputs.begin(), inputs.end(), graph->PinFromId(itbind->id));

            graph->links.erase(std::remove_if(graph->links.begin(),
                graph->links.end(),
                [=](Link &x){return x.InputId == itpin->id || x.OutputId == itpin->id;}),
                graph->links.end());
            inputs.erase(itpin);
            bindings.erase(itbind);
            graph->dirty = true;
        }
    };

    in->cbContextMenu = cb;
}

void KeyBinds::UpdateOutputs() {
    for(auto &kb: bindings) {
        Pin &pin = graph->PinFromId(kb.id);
        if(pin.bVal) {
            kstate[OAPI_KEY_LCONTROL] = 0;
            kstate[OAPI_KEY_LALT] = 0;
            kstate[OAPI_KEY_LSHIFT] = 0;
            if(kb.ctrl)  kstate[OAPI_KEY_LCONTROL] = 0x80;
            if(kb.alt)   kstate[OAPI_KEY_LALT]     = 0x80;
            if(kb.shift) kstate[OAPI_KEY_LSHIFT]   = 0x80;
            g_focusobj->GetModuleInterface()->SendBufferedKey(kb.key, true, kstate);
        }
    }
}
void KeyBinds::Draw() {
    builder.Header();
    ImGui::TextUnformatted(name.c_str());
    builder.EndHeader();

    int k=0;
    const Keymap &keymap = g_pOrbiter->keymap;
    for(auto &i : inputs) {
        DrawInput(i, minimized);
        int key = bindings[k].key;
        ImGui::SameLine();
        char buf[32];
        keymap.PrintStr(buf, (uint16_t)key);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImVec2 nopadding{0,0};
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, nopadding);

        ImGui::SetNextItemWidth(60);
        ImGui::PushID(k);
        if(ImGui::InputText("##keybind",buf, 32)) {
            uint16_t val;
            if(keymap.ScanStr(buf, val)) {
                bindings[k].key = (int)val;
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Ctrl", &bindings[k].ctrl);
        ImGui::SameLine();
        ImGui::Checkbox("Alt", &bindings[k].alt);
        ImGui::SameLine();
        ImGui::Checkbox("Shift", &bindings[k].shift);
        ImGui::SameLine();
        ImGui::PopID();
        ImGui::PopStyleVar(2);
        k++;
    }
    DrawInput(*inAdd, minimized);
}
