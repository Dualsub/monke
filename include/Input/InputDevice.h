#pragma once

#include "Core/EnumArray.h"
#include "Vultron/Window.h"

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <bitset>

using namespace Vultron;

namespace mk
{
    enum class InputActionType
    {
        Aim,
        Attack,
        Dash,
        Reload,
        Pause,
        OpenShop,
        OpenLevelUp,
        ToggleAutoAim,

        Ability1,
        Ability2,
        Ability3,
        Ability4,

        Option1,
        Option2,
        Option3,
        Option4,
        Option5,
        Option6,

        NextOption,
        PreviousOption,

        DebugOption1,
        DebugOption2,
        DebugOption3,
        DebugOption4,
        DebugOption5,
        DebugOption6,
        DebugOption7,

        Count,
        None
    };

    constexpr uint32_t c_numInputActions = static_cast<uint32_t>(InputActionType::Count);
    constexpr uint32_t c_numOptions = static_cast<uint32_t>(InputActionType::Option6) - static_cast<uint32_t>(InputActionType::Option1) + 1;
    constexpr uint32_t c_numDebugOptions = static_cast<uint32_t>(InputActionType::DebugOption7) - static_cast<uint32_t>(InputActionType::DebugOption1) + 1;
    constexpr uint32_t c_numAbilities = static_cast<uint32_t>(InputActionType::Ability4) - static_cast<uint32_t>(InputActionType::Ability1) + 1;

    struct InputState
    {
        glm::vec2 movementAxis = {};
        glm::vec2 lookAxis = {};
        std::bitset<c_numInputActions> actions = {};
        std::bitset<c_numInputActions> beginActions = {};
        std::bitset<c_numInputActions> endActions = {};

        bool usingGamepad = false;

        void Reset()
        {
            movementAxis = {};
            lookAxis = {};
            actions.reset();
            beginActions.reset();
            endActions.reset();
            usingGamepad = false;
        }

        template <typename T>
        bool Pressed(T action, uint32_t index = 0) const
        {
            const uint32_t actionIndex = static_cast<uint32_t>(action) + index;
            return beginActions[actionIndex];
        }

        template <typename T>
        bool Released(T action, uint32_t index = 0) const
        {
            const uint32_t actionIndex = static_cast<uint32_t>(action) + index;
            return endActions[actionIndex];
        }

        template <typename T>
        bool Down(T action, uint32_t index = 0) const
        {
            const uint32_t actionIndex = static_cast<uint32_t>(action) + index;
            return actions[actionIndex];
        }

        template <typename T>
        void Set(bool value, T action, uint32_t index = 0)
        {
            const uint32_t actionIndex = static_cast<uint32_t>(action) + index;
            actions.set(actionIndex, value);
        }

        template <typename T>
        void OrSet(bool value, T action, uint32_t index = 0)
        {
            const uint32_t actionIndex = static_cast<uint32_t>(action) + index;
            actions.set(actionIndex, actions[actionIndex] || value);
        }
    };

    class InputDevice
    {
    private:
        glm::vec2 m_previousMousePosition = {};
        InputState m_inputState = {};
        EnumArray<InputActionType, uint32_t> m_keyMapping = {};

        float m_mouseSensitivity = glm::radians(0.05f);
        float m_gamepadDeadzone = 0.2f;
        float m_gamepadSensitivity = glm::radians(50.0f);
        uint32_t m_gamepadIndex = 0;

        float m_scrollValue = 0.0f;

        void QueryMouseState(GLFWwindow *window, InputState &state);
        void QueryKeyboardState(GLFWwindow *window, InputState &state);
        void QueryGamepadState(GLFWwindow *window, float dt, InputState &state);

    public:
        InputDevice() = default;
        ~InputDevice() = default;

        bool Initialize(Window &window);
        void Shutdown();

        void QueryInputState(Window &window, float dt);
        const InputState &GetInputState() const { return m_inputState; }
    };
}