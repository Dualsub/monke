#pragma once

#include "UI/Types.h"

#include "Input/InputDevice.h"
#include "Vultron/SceneRenderer.h"

#include <glm/glm.hpp>
#include <string>

namespace mk
{
    struct Container;
}

namespace mk::UIHelper
{
    enum class InputActions
    {
        Select = static_cast<uint32_t>(mk::InputActionType::Attack),
    };

    glm::vec2 GetScreenPositionFromWorldPosition(const glm::vec3 &worldPosition, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix);
    glm::vec3 GetWorldPositionFromScreenPosition(const glm::vec2 &screenPosition, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix, float yPlane);
    std::string GetTimeString(float time);
    glm::vec4 RenderText(SceneRenderer &renderer, RenderHandle fontAtlas, RenderHandle fontMaterial, const std::string &text, const glm::vec2 &position, float size, const glm::vec4 &color, float aspectRatio, TextAlignment alignment = TextAlignment::Center, bool centerVertically = true, uint32_t maxLineLength = 0, float lineSpacing = 0.0f);
    glm::vec4 RenderText(const SceneRenderer &renderer, UIContext &context, const std::string &text, const glm::vec2 &position, float size, const glm::vec4 &color, float aspectRatio, TextAlignment alignment = TextAlignment::Center, bool centerVertically = true, uint32_t maxLineLength = 0, float lineSpacing = 0.0f);
    void RenderProgressBar(SceneRenderer &renderer, RenderHandle material, const glm::vec2 &position, const glm::vec2 &size, const glm::vec2 &texCoord, const glm::vec2 &texSize, float percent, const glm::vec4 &fillColor, const glm::vec4 &backgroundColor, float aspectRatio);
    bool IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectPosition, const glm::vec2 &rectSize);
    void PlayHoverSound();
    void PlayClickSound();
    Container RenderButton(const std::string &text, const LayoutValue &size, const LayoutValue &margin, const std::function<void(Container &)> &onClicked);
}