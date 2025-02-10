#pragma once

#include "UI/Types.h"
#include "Vultron/SceneRenderer.h"

#include <glm/glm.hpp>

namespace mk::Layout
{
    template <typename T, typename F>
    std::vector<Container::UIElement> Map(const T &input, const F &mapper)
    {
        std::vector<Container::UIElement> output;
        output.reserve(input.size());
        for (const auto &element : input)
        {
            output.push_back(mapper(element));
        }
        return output;
    }

    template <typename F>
    std::vector<Container::UIElement> MapRange(const uint32_t start, const uint32_t end, const F &mapper)
    {
        std::vector<Container::UIElement> output;
        output.reserve(end - start);
        for (uint32_t i = start; i < end; i++)
        {
            output.push_back(mapper(i));
        }
        return output;
    }

    Container::UIElement Conditional(const bool condition, const Container::UIElement &element);
    glm::vec2 GetSize(const LayoutValue &size, const glm::vec2 &baseSize);
    glm::vec2 GetPosition(const LayoutValue &position, const glm::vec2 &basePosition, const glm::vec2 &baseSize);

    void Render(const SceneRenderer &renderer, UIContext &context, Container &layout, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent = std::nullopt, float scale = 1.0f, float zIndex = 0.0f, float opacity = 1.0f);
    void Render(const SceneRenderer &renderer, UIContext &context, Text &text, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent = std::nullopt, float scale = 1.0f, float zIndex = 0.0f, float opacity = 1.0f);
    void Render(const SceneRenderer &renderer, UIContext &context, Image &text, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent = std::nullopt, float scale = 1.0f, float zIndex = 0.0f, float opacity = 1.0f);
}
