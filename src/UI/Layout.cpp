#include "UI/Layout.h"

#include "UI/UIHelper.h"

namespace mk::Layout
{
    Container::UIElement Conditional(const bool condition, const Container::UIElement &element)
    {
        if (condition)
        {
            return element;
        }

        return NullElement{};
    }

    glm::vec2 GetSize(const LayoutValue &size, const glm::vec2 &baseSize)
    {
        return std::visit(
            [&](const auto &value) -> glm::vec2
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, RelativeValue>)
                {
                    return value.value * baseSize;
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, AbsoluteValue>)
                {
                    return value.value;
                }

                return glm::vec2(1.0f);
            },
            size);
    }

    glm::vec2 GetPosition(const LayoutValue &position, const glm::vec2 &basePosition, const glm::vec2 &baseSize)
    {
        return std::visit(
            [&](const auto &value) -> glm::vec2
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, RelativeValue>)
                {
                    return basePosition + value.value * baseSize;
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, AbsoluteValue>)
                {
                    return basePosition + value.value;
                }

                return glm::vec2(0.0f);
            },
            position);
    }

    void Render(const SceneRenderer &renderer, UIContext &context, Container &container, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent, float scale, float zIndex, float opacity)
    {
        const std::string currIdString = idString + container.id;
        UIState &state = context.uiStates[currIdString];
        container.UpdateState(state);

        if (container.onLayout.has_value())
        {
            container.onLayout.value()(container);
        }

        state.scale = container.scale;

        // Calculate the position and size of the layout
        const glm::vec2 position = GetPosition(container.position, basePosition, baseSize);
        const float combinedScale = container.scale * scale;
        const glm::vec2 size = GetSize(container.size, baseSize) * container.scale;
        const float combinedOpacity = container.opacity * opacity;

        const uint32_t mainAxisIndex = container.direction == LayoutDirection::Horizontal ? 0 : 1;
        const uint32_t crossAxisIndex = container.direction == LayoutDirection::Horizontal ? 1 : 0;
        const glm::vec2 direction = container.direction == LayoutDirection::Horizontal ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f);

        // Calculate the start position
        glm::vec2 startPosition = position;

        float childCombinedSize = 0.0f;
        for (auto &child : container.children)
        {
            std::visit(
                [&](auto &child)
                {
                    if constexpr (!std::is_same_v<std::decay_t<decltype(child)>, NullElement>)
                    {
                        const glm::vec2 childSize = GetSize(child.size, size);
                        const glm::vec2 margin = GetSize(child.margin, size);
                        childCombinedSize += childSize[mainAxisIndex] + 2.0f * margin[mainAxisIndex];
                    }
                },
                child);
        }

        switch (container.alignment)
        {
        default:
        case LayoutAlignment::Start:
            startPosition -= (size * 0.5f * direction);
            break;
        case LayoutAlignment::Center:
            startPosition -= (childCombinedSize * 0.5f * direction);
            break;
        case LayoutAlignment::End:
            startPosition += (size * 0.5f * direction) - (childCombinedSize * direction);
            break;
        case LayoutAlignment::Equal:
            break;
        }

        // Render children
        float mainAxisOffset = 0.0f;
        float crossAxisOffset = 0.0f;

        bool wasHovered = state.isHovered;
        bool isHovered = false;
        if (!container.ignoreInput)
        {
            isHovered = UIHelper::IsPointInRect(context.cursorPosition, position, size);
            container.isHovered = isHovered;
            state.isHovered = isHovered;
        }
        else if (container.propagateHover)
        {
            isHovered = parent.has_value() && parent.value().isHovered;
            container.isHovered = isHovered;
            state.isHovered = isHovered;
        }

        for (auto &child : container.children)
        {
            std::visit(
                [&](auto &child)
                {
                    if constexpr (!std::is_same_v<std::decay_t<decltype(child)>, NullElement>)
                    {
                        const glm::vec2 childSize = GetSize(child.size, size);
                        const glm::vec2 margin = GetSize(child.margin, size);
                        const float halfSize = childSize[mainAxisIndex] / 2.0f + margin[mainAxisIndex];
                        mainAxisOffset += halfSize;
                        const glm::vec2 childPosition = startPosition + mainAxisOffset * direction;
                        mainAxisOffset += halfSize;

                        crossAxisOffset = glm::max(crossAxisOffset, childSize[crossAxisIndex]);

                        Render(renderer, context, child, childPosition, size, currIdString, aspectRatio, container, combinedScale, zIndex + 1.0f, combinedOpacity);
                    }
                },
                child);
        }

        if (!container.ignoreInput)
        {
            if (isHovered && context.inputState.Pressed(UIHelper::InputActions::Select) && container.onClicked.has_value())
            {
                if (container.onClicked.has_value())
                {
                    container.onClicked.value()(container);
                }
                UIHelper::PlayClickSound();
            }

            if (isHovered && !wasHovered)
            {
                if (container.onHoverEnter.has_value())
                {
                    container.onHoverEnter.value()(container);
                }
                UIHelper::PlayHoverSound();
            }
            else if (!isHovered && wasHovered)
            {
                if (container.onHoverExit.has_value())
                {
                    container.onHoverExit.value()(container);
                }
            }
        }

        // Render surrounding box
        const glm::vec4 color = (isHovered ? container.hoverColor : container.color) * glm::vec4(1.0f, 1.0f, 1.0f, combinedOpacity);
        if (color.a > 0.0f)
        {
            context.AddRenderJob(SpriteRenderJob{
                .material = context.whiteSpriteMaterial,
                .position = position,
                .size = size,
                .color = color,
                .borderRadius = container.borderRadius * scale,
                .zOrder = zIndex,
            });
        }
    }

    void Render(const SceneRenderer &renderer, UIContext &context, Text &text, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent, float scale, float zIndex, float opacity)
    {
        const glm::vec2 position = GetPosition(text.position, basePosition, baseSize);
        const glm::vec2 size = GetSize(text.size, baseSize);

        glm::vec4 color = text.color;
        if (parent.has_value() && parent.value().isHovered)
        {
            color = text.hoverColor;
        }

        UIHelper::RenderText(
            renderer,
            context,
            text.text,
            position,
            text.fontSize * scale,
            color * glm::vec4(1.0f, 1.0f, 1.0f, opacity),
            text.alignment,
            text.centerVertically,
            text.maxLineLength,
            text.lineSpacing);

        if (text.backgroundColor.a > 0.0f)
        {
            context.AddRenderJob(SpriteRenderJob{
                .material = context.whiteSpriteMaterial,
                .position = position,
                .size = size,
                .color = text.backgroundColor * glm::vec4(1.0f, 1.0f, 1.0f, opacity),
                .borderRadius = text.backgroundBorderRadius * scale,
                .zOrder = zIndex - 0.1f,
            });
        }
    }

    void Render(const SceneRenderer &renderer, UIContext &context, Image &image, const glm::vec2 &basePosition, const glm::vec2 &baseSize, const std::string &idString, const glm::vec2 &aspectRatio, const std::optional<Container> &parent, float scale, float zIndex, float opacity)
    {
        const glm::vec2 position = GetPosition(image.position, basePosition, baseSize);
        const glm::vec2 size = GetSize(image.size, baseSize);

        glm::vec4 color = image.color;
        if (parent.has_value() && parent.value().isHovered)
        {
            color = image.hoverColor;
        }

        context.AddRenderJob(SpriteRenderJob{
            .material = image.texture,
            .position = position,
            .size = image.texScale * aspectRatio * scale,
            .texCoord = image.texCoord,
            .texSize = image.texSize,
            .color = color * glm::vec4(1.0f, 1.0f, 1.0f, opacity),
            .zOrder = zIndex,
        });

        if (image.backgroundColor.a > 0.0f)
        {
            context.AddRenderJob(SpriteRenderJob{
                .material = context.whiteSpriteMaterial,
                .position = position,
                .size = size,
                .color = image.backgroundColor * glm::vec4(1.0f, 1.0f, 1.0f, opacity),
                .zOrder = zIndex - 0.1f,
            });
        }
    }
}