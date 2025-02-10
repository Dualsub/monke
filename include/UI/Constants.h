#pragma once

#include <glm/glm.hpp>

namespace mk
{
    constexpr glm::vec4 c_defaultBackgroundColor = glm::vec4(0.08f, 0.08f, 0.08f, 0.75f);
    constexpr glm::vec4 c_dimBackgroundColor = glm::vec4(0.08f, 0.08f, 0.08f, 0.25f);
    constexpr glm::vec4 c_defaultTextColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr glm::vec4 c_defaultHoverTextColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    constexpr glm::vec4 c_defaultHoverBackgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr glm::vec4 c_yellowColor = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f);
    constexpr glm::vec4 c_redColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    constexpr glm::vec4 c_greenColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    constexpr glm::vec4 c_blueColor = glm::vec4(glm::vec3(0, 125, 255) / 255.0f, 1.0f);
    constexpr glm::vec4 c_pinkColor = glm::vec4(glm::vec3(0.4f, 0.0f, 0.2f) * 2.0f, 1.0f);
    constexpr glm::vec4 c_rareItemBlueBackgroundColor = glm::vec4(glm::vec3(0.0f, 0.2f, 0.4f), 1.0f);
    constexpr glm::vec4 c_legendaryItemPurpleBackgroundColor = glm::vec4(glm::vec3(0.4f, 0.0f, 0.2f), 1.0f);
    constexpr glm::vec4 c_epicItemGreenBackgroundColor = glm::vec4(glm::vec3(0.0f, 0.4f, 0.2f), 1.0f);

    constexpr float c_smallBorderRadius = 0.01f;
    constexpr float c_normalBorderRadius = 0.02f;
    constexpr float c_largeBorderRadius = 0.04f;
}