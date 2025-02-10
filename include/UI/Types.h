#pragma once

#include "Vultron/Types.h"
#include "UI/Constants.h"
#include "Input/InputDevice.h"
#include "Audio/AudioSystem.h"

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>

namespace mk
{
    using namespace Vultron;

    enum class TextAlignment
    {
        Left,
        Center,
        Right
    };

    struct RelativeValue
    {
        glm::vec2 value;
        RelativeValue(const glm::vec2 &v) : value(v) {}
        template <typename... Args>
        RelativeValue(Args &&...args) : value(std::forward<Args>(args)...) {}
    };

    struct AbsoluteValue
    {
        glm::vec2 value;
        AbsoluteValue(const glm::vec2 &v) : value(v) {}
        template <typename... Args>
        AbsoluteValue(Args &&...args) : value(std::forward<Args>(args)...) {}
    };

    using LayoutValue = std::variant<RelativeValue, AbsoluteValue>;

    struct TextButton
    {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 size = glm::vec2(0.0f);
        float fontSize = 1.0f;
        float scale = 1.0f;
        float hoverScale = 1.1f;
        glm::vec4 backgroundColor = c_defaultBackgroundColor;
        glm::vec4 textColor = c_defaultTextColor;
        glm::vec4 hoverTextColor = c_defaultHoverTextColor;
        glm::vec4 hoverBackgroundColor = c_defaultHoverBackgroundColor;
        bool isHovered = false;
        bool isPressed = false;
        std::string text = "";
        TextAlignment alignment = TextAlignment::Center;
        float padding = 0.01f;
    };

    using UIRenderJob = std::variant<SpriteRenderJob, FontRenderJob>;

    struct UIState
    {
        float scale = 1.0f;
        bool isHovered = false;
    };

    struct UIContext
    {
        RenderHandle fontAtlas = c_invalidHandle;
        RenderHandle fontMaterial = c_invalidHandle;
        RenderHandle whiteSpriteMaterial = c_invalidHandle;
        InputState inputState = {};
        glm::vec2 cursorPosition = glm::vec2(0.0f);
        glm::vec2 aspectRatio = glm::vec2(1.0f);

        std::vector<UIRenderJob> renderJobs;
        std::unordered_map<std::string, UIState> uiStates;

        template <typename T>
        void AddRenderJob(const T &job)
        {
            renderJobs.push_back(job);
        }
    };

#define UI_ELEMENT_BASE_FIELDS(defaultId, defaultColor) \
    std::string id = defaultId;                         \
    LayoutValue position = RelativeValue(0.0f);         \
    LayoutValue size = RelativeValue(1.0f);             \
    float scale = 1.0f;                                 \
    glm::vec4 color = defaultColor;                     \
    glm::vec4 hoverColor = defaultColor;                \
    float opacity = 1.0f;                               \
    LayoutValue padding = RelativeValue(0.0f);          \
    LayoutValue margin = RelativeValue(0.0f);

    struct Text
    {
        UI_ELEMENT_BASE_FIELDS("Text", c_defaultTextColor)

        std::string text = "";
        float fontSize = 1.0f;
        TextAlignment alignment = TextAlignment::Left;
        float lineSpacing = 0.0f;
        uint32_t maxLineLength = 0;
        bool centerVertically = true;

        glm::vec4 backgroundColor = glm::vec4(0.0f);
        glm::vec4 backgroundBorderRadius = glm::vec4(0.0f);
    };

    struct Image
    {
        UI_ELEMENT_BASE_FIELDS("Image", glm::vec4(1.0f))

        RenderHandle texture = c_invalidHandle;
        glm::vec2 texCoord = glm::vec2(0.0f);
        glm::vec2 texSize = glm::vec2(1.0f);
        glm::vec2 texScale = glm::vec2(1.0f);

        glm::vec4 backgroundColor = glm::vec4(0.0f);
    };

    struct NullElement
    {
    };

    enum class LayoutDirection
    {
        Horizontal,
        Vertical,
    };

    enum class LayoutAlignment
    {
        Start,
        Center,
        End,
        Equal,
    };

    struct Container
    {
        UI_ELEMENT_BASE_FIELDS("Container", glm::vec4(0.0f))

        using UIElement = std::variant<Container, Text, Image, NullElement>;

        glm::vec4 borderRadius = glm::vec4(0.0f);
        LayoutDirection direction = LayoutDirection::Vertical;
        LayoutAlignment alignment = LayoutAlignment::Start;
        bool isHovered = false;
        bool propagateHover = false;
        bool ignoreInput = true;
        std::optional<std::function<void(Container &)>> onClicked;
        std::optional<std::function<void(Container &)>> onHoverEnter;
        std::optional<std::function<void(Container &)>> onHoverExit;
        std::optional<std::function<void(Container &)>> onLayout;
        std::vector<UIElement> children;

        float zIndex = 0.0f;

        void UpdateState(const UIState &state)
        {
            scale = state.scale;
            isHovered = state.isHovered;
        }
    };
}