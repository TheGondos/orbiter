#include "Gamepad.h"
#include <stdio.h>

bool GamepadController::m_js[GLFW_JOYSTICK_LAST];
double GamepadController::m_Thrusters[THGROUP_ATT_BACK + 1];
double GamepadController::m_CameraRotX;
double GamepadController::m_CameraRotY;
bool GamepadController::m_ResetCamera;
bool GamepadController::m_CurrentState[GamepadController::NBUTTON];
bool GamepadController::m_actions[GamepadController::Action::LAST];
int GamepadController::m_NavMode;

void GamepadController::GlobalInit() {
    for(int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
        int present = glfwJoystickPresent(i);
        if(present) {
            const char* name = glfwGetGamepadName(i);
            printf("GamepadController: Joystick %d detected as '%s'\n", i, name);
            m_js[i] = true;
        } else {
            m_js[i] = false;
        }
    }

    for(int i = 0; i < THGROUP_ATT_BACK; i++) {
        m_Thrusters[i] = 0.0;
    }

    for(int i = 0; i < GLFW_GAMEPAD_BUTTON_DPAD_LEFT; i++) {
        m_CurrentState[i] = false;
    }

    m_CameraRotX = 0;
    m_CameraRotY = 0;
    m_ResetCamera = false;
    m_NavMode = 0;
}

void GamepadController::JoystickCallback(int jid, int event) {
    const char* name = glfwGetGamepadName(jid);
    if (event == GLFW_CONNECTED)
    {
        printf("GamepadController: Joystick %d connected as '%s'\n", jid, name);
        m_js[jid] = true;
    }
    else if (event == GLFW_DISCONNECTED)
    {
        printf("GamepadController: Joystick %d disconnected\n", jid);
        m_js[jid] = false;
    }
}

/*
enum THGROUP_TYPE {
	THGROUP_MAIN,            ///< main thrusters
	THGROUP_RETRO,           ///< retro thrusters
	THGROUP_HOVER,           ///< hover thrusters
	THGROUP_ATT_PITCHUP,     ///< rotation: pitch up
	THGROUP_ATT_PITCHDOWN,   ///< rotation: pitch down
	THGROUP_ATT_YAWLEFT,     ///< rotation: yaw left
	THGROUP_ATT_YAWRIGHT,    ///< rotation: yaw right
	THGROUP_ATT_BANKLEFT,    ///< rotation: bank left
	THGROUP_ATT_BANKRIGHT,   ///< rotation: bank right
	THGROUP_ATT_RIGHT,       ///< translation: move right
	THGROUP_ATT_LEFT,        ///< translation: move left
	THGROUP_ATT_UP,          ///< translation: move up
	THGROUP_ATT_DOWN,        ///< translation: move down
	THGROUP_ATT_FORWARD,     ///< translation: move forward
	THGROUP_ATT_BACK,        ///< translation: move back
	THGROUP_USER = 0x40      ///< user-defined group
};*/

/*
#define 	GLFW_GAMEPAD_AXIS_LEFT_X   0
#define 	GLFW_GAMEPAD_AXIS_LEFT_Y   1
#define 	GLFW_GAMEPAD_AXIS_RIGHT_X   2
#define 	GLFW_GAMEPAD_AXIS_RIGHT_Y   3
#define 	GLFW_GAMEPAD_AXIS_LEFT_TRIGGER   4
#define 	GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER   5
*/

const float deadzone = 0.2f;
float dz(float f) {
    if(fabs(f) < deadzone) return 0.0f;
    if(f<0.0) {
        return (f + deadzone) / (1.0f-deadzone);
    } else {
        return (f - deadzone) / (1.0f-deadzone);
    }
}

void GamepadController::ProcessInput() {
    GLFWgamepadstate gpState;
    bool gpStateValid = false;
    bool ButtonDown[GamepadController::NBUTTON];
    m_NavMode = 0;
    m_CameraRotX = 0;
    m_CameraRotY = 0;
    m_ResetCamera = false;

    for(int i = 0; i < GamepadController::NBUTTON; i++) {
        ButtonDown[i] = false;
    }

    for(int i = 0; i < Action::LAST; i++) {
        m_actions[i] = false;
    }

    for(int i = 0; i < GLFW_JOYSTICK_LAST; i++) {
        if(m_js[i] && i!=0) {
            if(glfwGetGamepadState(i, &gpState) == GLFW_FALSE)
                continue;

            gpStateValid = true;

            m_Thrusters[THGROUP_MAIN] = dz((gpState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0)/2.0);
            m_Thrusters[THGROUP_HOVER] = dz((gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0)/2.0);
            m_Thrusters[THGROUP_ATT_PITCHUP] = dz(gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.0?gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]:0.0);
            m_Thrusters[THGROUP_ATT_PITCHDOWN] = dz(gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.0?0.0:-gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
            m_Thrusters[THGROUP_ATT_BANKLEFT] = dz(gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.0?0.0:-gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
            m_Thrusters[THGROUP_ATT_BANKRIGHT] = dz(gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.0?gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_X]:0.0);

            //printf("pu=%f pd=%f m=%f h=%f bl=%f br=%f\n", m_Thrusters[THGROUP_ATT_PITCHUP], m_Thrusters[THGROUP_ATT_PITCHDOWN], 
            //    m_Thrusters[THGROUP_MAIN], m_Thrusters[THGROUP_HOVER], m_Thrusters[THGROUP_ATT_BANKLEFT], m_Thrusters[THGROUP_ATT_BANKRIGHT]);

            for(int i = 0; i < GamepadController::NBUTTON; i++) {
                if(gpState.buttons[i] && !m_CurrentState[i]) {
                    printf("pressed %d\n", i);
                    //button pressed this frame
                    ButtonDown[i] = true;
                } else if(!gpState.buttons[i] && m_CurrentState[i]) {
                    printf("released %d\n", i);
                    //button released this frame
                }
                m_CurrentState[i] = gpState.buttons[i] == GLFW_PRESS;
            }
        }
    }
    if(!gpStateValid)
        return;
/*
	if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_RCSEnable))      SetAttMode (attmode >= 1 ? 0 : 1);
	if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_RCSMode))        ToggleAttMode ();
	if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_Undock))         UndockInteractive ();

	// HUD control (internal view only)
	if (g_camera->IsInternal()) {
		if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_HUD))          g_pane->ToggleHUD();
		if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_HUDMode))      g_pane->SwitchHUDMode();
		if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_HUDReference))
			if (g_pane->GetHUD())                                      g_pane->GetHUD()->SelectReference();
		if (keymap.IsLogicalKey (key, kstate, OAPI_LKEY_HUDTarget))
			if (g_pane->GetHUD() && g_pane->GetHUD()->Mode() == HUD_DOCKING)    ((HUD_Docking*)g_pane->GetHUD())->SelectReferenceOld();
	}

		case OAPI_KEY_C:        // landing/takeoff clearance request
			IssueClearanceRequest ();
			return 1;
		}

		case OAPI_KEY_DIVIDE:   // connect/disconnect user input to aerodynamic control surfaces
			ToggleADCtrlMode ();
			return 1;

	}*/
    if(!m_CurrentState[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] && !m_CurrentState[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]) {
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_START]) m_actions[TOGGLE_PAUSE] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_A]) m_actions[TOGGLE_LANDING_GEAR] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_BACK]) m_actions[TOGGLE_PANEL_MODE] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB]) m_actions[RESET_CAMERA] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_UP]) m_actions[TRIM_DOWN] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) m_actions[TRIM_UP] = true;

        if(ButtonDown[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB]) {
            m_ResetCamera = true;
        } else {
            m_CameraRotX = dz(gpState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
            m_CameraRotY = dz(gpState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
        }

    } else if(m_CurrentState[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] && !m_CurrentState[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]) {
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_BACK]) m_actions[TOGGLE_CAMERA_INT_EXT] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]) m_actions[PANEL_SWITCH_LEFT] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]) m_actions[PANEL_SWITCH_RIGHT] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_UP]) m_actions[PANEL_SWITCH_UP] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) m_actions[PANEL_SWITCH_DOWN] = true;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_START]) m_actions[TOGGLE_HUD_COLOR] = true;
    } else if(!m_CurrentState[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] && m_CurrentState[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]) {
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_UP]) m_NavMode = NAVMODE_PROGRADE;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]) m_NavMode = NAVMODE_RETROGRADE;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]) m_NavMode = NAVMODE_NORMAL;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]) m_NavMode = NAVMODE_ANTINORMAL;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_A]) m_NavMode = NAVMODE_KILLROT;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_Y]) m_NavMode = NAVMODE_HOLDALT;
        if(ButtonDown[GLFW_GAMEPAD_BUTTON_X]) m_NavMode = NAVMODE_HLEVEL;
        
    } else if(m_CurrentState[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] && m_CurrentState[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]) {
    }
}
