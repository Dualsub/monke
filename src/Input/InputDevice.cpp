#include "Input/InputDevice.h"

#include <glm/gtc/constants.hpp>

#include <iostream>

namespace ACS
{
    bool InputDevice::Initialize(Window &window)
    {
        glfwSetWindowUserPointer(window.GetWindowHandle(), this);
        glfwSetScrollCallback(
            window.GetWindowHandle(),
            [](GLFWwindow *window, double xOffset, double yOffset)
            {
                InputDevice *inputDevice = static_cast<InputDevice *>(glfwGetWindowUserPointer(window));
                inputDevice->m_scrollValue = static_cast<float>(yOffset);
            });

        m_keyMapping[InputActionType::Aim] = GLFW_KEY_RIGHT;
        m_keyMapping[InputActionType::Attack] = GLFW_KEY_LEFT;
        m_keyMapping[InputActionType::Dash] = GLFW_KEY_SPACE;
        m_keyMapping[InputActionType::Reload] = GLFW_KEY_R;
        m_keyMapping[InputActionType::Pause] = GLFW_KEY_ESCAPE;
        m_keyMapping[InputActionType::OpenShop] = GLFW_KEY_B;
        m_keyMapping[InputActionType::OpenLevelUp] = GLFW_KEY_I;
        m_keyMapping[InputActionType::ToggleAutoAim] = GLFW_KEY_TAB;

        m_keyMapping[InputActionType::Ability1] = GLFW_KEY_Q;
        m_keyMapping[InputActionType::Ability2] = GLFW_KEY_E;
        m_keyMapping[InputActionType::Ability3] = GLFW_KEY_F;
        m_keyMapping[InputActionType::Ability4] = GLFW_KEY_G;

        return true;
    }

    void InputDevice::Shutdown()
    {
        glfwSetScrollCallback(nullptr, nullptr);
        glfwSetWindowUserPointer(nullptr, nullptr);
    }

    void InputDevice::QueryInputState(Window &window, float dt)
    {
        InputState state = {};
        state.lookAxis = m_inputState.lookAxis;
        state.usingGamepad = m_inputState.usingGamepad;
        GLFWwindow *glfwWindow = window.GetWindowHandle();

        QueryMouseState(glfwWindow, state);
        QueryKeyboardState(glfwWindow, state);
        QueryGamepadState(glfwWindow, dt, state);

        state.beginActions = state.actions & ~m_inputState.actions;
        state.endActions = ~state.actions & m_inputState.actions;

        m_inputState = state;
        m_scrollValue = 0.0f;
    }

    void InputDevice::QueryMouseState(GLFWwindow *window, InputState &state)
    {
        bool action = false;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const glm::vec2 mousePosition = {static_cast<float>(mouseX), static_cast<float>(mouseY)};
        const glm::vec2 mouseDelta = mousePosition - m_previousMousePosition;
        if (glm::length(mouseDelta) > glm::epsilon<float>())
        {
            action = true;
        }

        m_previousMousePosition = mousePosition;

        auto delta = mouseDelta * m_mouseSensitivity;
        state.lookAxis = -delta;

        // Get mouse button state
        bool value = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        state.OrSet(value, InputActionType::Aim);
        action |= value;

        value = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        state.OrSet(value, InputActionType::Attack);
        action |= value;

        if (m_scrollValue != 0.0f)
        {
            state.Set(true, m_scrollValue > 0.0f ? InputActionType::NextOption : InputActionType::PreviousOption);
            action = true;
        }

        // If we have any action, we are using the mouse and not the gamepad
        if (action)
        {
            state.usingGamepad = false;
        }
    }

    void InputDevice::QueryKeyboardState(GLFWwindow *window, InputState &state)
    {
        bool action = false;

        glm::vec2 movementAxis = {};
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            movementAxis.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            movementAxis.y -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movementAxis.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movementAxis.x += 1.0f;

        state.movementAxis = glm::clamp(state.movementAxis + movementAxis, glm::vec2(-1.0f), glm::vec2(1.0f));

        if (glm::length(state.movementAxis) > 0.0f)
        {
            action = true;
            state.movementAxis = glm::normalize(state.movementAxis);
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(InputActionType::Count); i++)
        {
            InputActionType actionType = static_cast<InputActionType>(i);
            bool value = (glfwGetKey(window, m_keyMapping[actionType]) == GLFW_PRESS);
            state.OrSet(value, actionType);
            action |= value;
        }

        for (uint32_t i = 0; i < c_numDebugOptions; i++)
        {
            bool value = (glfwGetKey(window, GLFW_KEY_F10 - i) == GLFW_PRESS);
            state.OrSet(value, InputActionType::DebugOption1, i);
            action |= value;
        }

        for (uint32_t i = 0; i < c_numOptions; i++)
        {
            bool value = (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS);
            state.OrSet(value, InputActionType::Option1, i);
            action |= value;
        }

        // If we have any action, we are using the keyboard and not the gamepad
        if (action)
        {
            state.usingGamepad = false;
        }
    }

    void InputDevice::QueryGamepadState(GLFWwindow *window, float dt, InputState &state)
    {
        bool action = false;

        const int gamepad = std::min(static_cast<int>(GLFW_JOYSTICK_1 + m_gamepadIndex), GLFW_JOYSTICK_LAST);
        int gamepadPresent = glfwJoystickPresent(gamepad);
        if (gamepadPresent == GLFW_TRUE)
        {
            int axesCount;
            const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);

            if (axesCount >= 2)
            {
                auto movementAxis = glm::vec2(axes[0], axes[1] * -1.0f);
                if (glm::length(movementAxis) >= m_gamepadDeadzone)
                {
                    state.movementAxis = glm::clamp(state.movementAxis + movementAxis, glm::vec2(-1.0f), glm::vec2(1.0f));
                    action = true;
                }
            }

            GLFWgamepadstate gamepadState;
            if (glfwGetGamepadState(gamepad, &gamepadState))
            {
                // Get D-Pad state
                for (uint32_t i = 0; i < 4; i++)
                {
                    bool value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP + i]);
                    state.OrSet(value, InputActionType::Option1, i);
                    action |= value;
                }

                // Get Pause button state
                bool value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_START]);
                state.OrSet(value, InputActionType::Pause);
                action |= value;

                // Get Reload button state
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_X]);
                state.OrSet(value, InputActionType::Reload);
                action |= value;

                // Get Cycle weapon state
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_Y]);
                state.OrSet(value, InputActionType::NextOption);
                action |= value;

                // Get Dash button state
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_B]);
                state.OrSet(value, InputActionType::Dash);
                action |= value;

                // Get AutoAim button state
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB]);
                state.OrSet(value, InputActionType::ToggleAutoAim);
                action |= value;

                // Ability1
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]);
                state.OrSet(value, InputActionType::Ability1);
                action |= value;

                // Shop
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_A]);
                state.OrSet(value, InputActionType::OpenShop);
                action |= value;

                // LevelUp
                value = static_cast<bool>(gamepadState.buttons[GLFW_GAMEPAD_BUTTON_BACK]);
                state.OrSet(value, InputActionType::OpenLevelUp);
                action |= value;
            }

            if (axesCount >= 4)
            {
                const auto lookDelta = glm::vec2(axes[2], axes[3]);
                if (glm::length(lookDelta) >= m_gamepadDeadzone)
                {
                    state.lookAxis = -lookDelta * m_gamepadSensitivity * dt;
                    action = true;
                }
            }

            // Trigger axes for aiming and attacking
            if (axesCount >= 6)
            {
                const auto aim = axes[4];
                state.OrSet(aim > m_gamepadDeadzone, InputActionType::Aim);

                const auto attack = axes[5];
                state.OrSet(attack > m_gamepadDeadzone, InputActionType::Attack);

                if (attack > m_gamepadDeadzone || aim > m_gamepadDeadzone)
                {
                    action = true;
                }
            }
        }

        // If we have any action, we are using the gamepad
        if (action)
        {
            state.usingGamepad = true;
        }
    }

}