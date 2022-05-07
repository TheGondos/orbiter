#pragma once

#include <GLFW/glfw3.h>
#include "Orbiter.h"

class Orbiter;
class GamepadController final {
public:
    enum Action{
        TOGGLE_PAUSE,
        TOGGLE_LANDING_GEAR,
        TOGGLE_PANEL_MODE,
        TOGGLE_CAMERA_INT_EXT,
        RESET_CAMERA,
        PANEL_SWITCH_LEFT,
        PANEL_SWITCH_RIGHT,
        PANEL_SWITCH_UP,
        PANEL_SWITCH_DOWN,
        TOGGLE_HUD_COLOR,
        TRIM_UP,
        TRIM_DOWN,
        LAST
    };
    static const int NBUTTON = GLFW_GAMEPAD_BUTTON_DPAD_LEFT + 1;
    static void GlobalInit();
    static void JoystickCallback(int jid, int event);
    static void ProcessInput();
    static double GetThrusterLevel(THGROUP_TYPE t) {return m_Thrusters[t] * 1000.0;}
    static bool GetCameraRotation(double *x, double *y) { *x = m_CameraRotX; *y = m_CameraRotY; return m_ResetCamera;}
    static bool ActionRequired(Action id) { return m_actions[id]; }
    static int GetNavMode() { return m_NavMode; };

    static bool m_actions[Action::LAST];

    static bool m_js[GLFW_JOYSTICK_LAST];
    static double m_Thrusters[THGROUP_ATT_BACK + 1];
    static double m_CameraRotX;
    static double m_CameraRotY;
    static bool m_ResetCamera;
    static bool m_CurrentState[NBUTTON];
    static int m_NavMode;
};
