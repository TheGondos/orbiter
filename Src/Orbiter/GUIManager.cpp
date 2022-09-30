#include "OrbiterAPI.h"
#include <imgui/imgui.h>
#include "imgui_notify.h"

ImGuiContext*   GImGui = NULL;

GUIManager::GUIManager()
{
}

void GUIManager::Notify(enum NotifType type, const char *title, const char *content)
{
    ImGuiToastType toasttype = ImGuiToastType_Info;
    switch(type) {
        case Success:
            toasttype = ImGuiToastType_Success;
            break;
        case Warning:
            toasttype = ImGuiToastType_Warning;
            break;
        case Error:
            toasttype = ImGuiToastType_Error;
            break;
        case Info:
            toasttype = ImGuiToastType_Info;
            break;
    }
  	ImGui::InsertNotification({ toasttype, title, content });
}