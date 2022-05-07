#pragma once
#include <list>
#include <memory>
#include <string>
#undef min
#undef max
/*
class GUIElement {
    public:
        GUIElement(const std::string &n, const std::string type):name(n) ,type(type) {}
        virtual ~GUIElement() {}
        virtual void Show() = 0;
        const std::string name;
        bool show = false;
        const std::string type;
        bool IsVisible() { return show; }
};

class GUIManager {
    public:
        GUIManager();
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
*/
