#pragma once
#include <list>
#include <GLFW/glfw3.h>
#include "OrbiterAPI.h"

class GUIElement;
class ImFont;
class GUIManager {
    public:
		enum NotifType {
			Success,
			Warning,
			Error,
			Info
		};
		ImFont *fontDefault;
		ImFont *fontH1;
		ImFont *fontH2;
		ImFont *fontH3;
		ImFont *fontBold;
		void UpdateCursor(float dx, float dy);
        void GetCursorPos(double *x, double *y);
		void LeftClick(bool click);
		void RightClick(bool click);
 		void (*prev_scroll_callback)(GLFWwindow* window, double xoffset, double yoffset);
		void (*prev_mouse_button_callback)(GLFWwindow* window, int button, int action, int mods);
		void (*prev_cursor_position_callback)(GLFWwindow* window, double xpos, double ypos);
		void (*prev_key_callback)(GLFWwindow* window, int gkey, int scancode, int action, int mods);
		void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
		void key_callback(GLFWwindow* window, int gkey, int scancode, int action, int mods);
		void framebuffer_size_callback(GLFWwindow *window, int width, int height);
		GLFWwindow *hRenderWnd; // render window handle
		void CloseWindow() { glfwSetWindowShouldClose(hRenderWnd, GL_TRUE); }
		bool ShouldCloseWindow() { return glfwWindowShouldClose(hRenderWnd); }
		void DisableMouseCursor() { glfwSetInputMode(hRenderWnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
		void EnableMouseCursor() { glfwSetInputMode(hRenderWnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
		void SetupCallbacks();


        GUIManager();
		void RenderGUI();
		void Notify(enum NotifType, const char *title, const char *content);
        void RegisterCtrl(GUIElement *ctrl) {
            for(auto &e: m_GUICtrls) {
                if(e == ctrl) {
                    return;
                }
            }
            m_GUICtrls.push_back(ctrl);
        }
        void UnregisterCtrl(GUIElement *e) {
            for (auto it = m_GUICtrls.begin(); it != m_GUICtrls.end(); ) {
                if (*it == e) {
                    it = m_GUICtrls.erase(it);
                    return;
                } else {
                    ++it;
                }
            }
        }

        std::list<GUIElement *> m_GUICtrls;
        
        template<class T>
        T *GetCtrl() {
            for(auto &e: m_GUICtrls) {
                if(e->type == T::etype) {
                    return (T *)e;
                }
            }
            return nullptr;
        }
        template<class T>
        void ShowCtrl() { auto e = GetCtrl<T>(); if(e) e->show = true; }
        template<class T>
        void HideCtrl() { auto e = GetCtrl<T>(); if(e) e->show = false; }
        template<class T>
        void ToggleCtrl() { auto e = GetCtrl<T>(); if(e) e->show = !e->show; }
};
