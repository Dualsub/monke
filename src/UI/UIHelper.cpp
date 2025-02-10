#include "UI/UIHelper.h"

#include "Application.h"

namespace mk::UIHelper
{
    glm::vec2 GetScreenPositionFromWorldPosition(const glm::vec3 &worldPosition, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix)
    {
        glm::vec4 clipSpacePos = projectionMatrix * viewMatrix * glm::vec4(worldPosition, 1.0f);
        glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
        return glm::vec2(ndcSpacePos.x, ndcSpacePos.y);
    }

    glm::vec3 GetWorldPositionFromScreenPosition(const glm::vec2 &screenPosition, const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix, float yPlane)
    {
        glm::vec4 clipPos = glm::vec4(screenPosition * glm::vec2(1.0, 1.0), 0.0, 1.0);

        glm::vec4 eyePos = inverse(projectionMatrix) * clipPos;

        eyePos /= eyePos.w;

        glm::vec4 worldPos = inverse(viewMatrix) * eyePos;

        glm::vec3 rayDirection = glm::normalize(glm::vec3(worldPos) - glm::vec3(inverse(viewMatrix) * glm::vec4(0, 0, 0, 1)));
        glm::vec3 cameraPos = glm::vec3(inverse(viewMatrix) * glm::vec4(0, 0, 0, 1));

        float t = (yPlane - cameraPos.y) / rayDirection.y;

        glm::vec3 crosshairWorldPosition = cameraPos + t * rayDirection;

        // If nan, return camera position
        if (crosshairWorldPosition != crosshairWorldPosition)
        {
            return cameraPos;
        }

        return crosshairWorldPosition;
    }

    std::string GetTimeString(float time)
    {
        uint32_t minutes = static_cast<uint32_t>(time / 60.0f);
        uint32_t seconds = static_cast<uint32_t>(time) % 60;

        std::string timeString = (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
        return timeString;
    }

    size_t GetLineHash(const std::string &text, const glm::vec2 &position, float size, float aspectRatio, TextAlignment alignment, bool centerVertically)
    {
        size_t hash = 0;
        hash = std::hash<std::string>{}(text);
        hash ^= std::hash<float>{}(position.x);
        hash ^= std::hash<float>{}(position.y);
        hash ^= std::hash<float>{}(size);
        hash ^= std::hash<float>{}(aspectRatio);
        hash ^= std::hash<int>{}(static_cast<int>(alignment));
        hash ^= std::hash<bool>{}(centerVertically);
        return hash;
    }

    glm::vec4 RenderLine(const SceneRenderer &renderer, UIContext &context, const std::string &text, const glm::vec2 &position, float size, const glm::vec4 &color, float aspectRatio, TextAlignment alignment, bool centerVertically)
    {
        std::vector<FontGlyph> glyphs = renderer.GetTextGlyphs(context.fontAtlas, text);

        const float c_scale = 1.0f / 5.0f;
        float heightOffset = centerVertically ? renderer.GetGlyph(context.fontAtlas, "A").uvExtent.y / 2.0f * size * c_scale * aspectRatio : 0.0f;

        float totalWidth = 0.0f;
        for (const auto &glyph : glyphs)
        {
            totalWidth += glyph.uvExtent.x * size * c_scale;
        }

        glm::vec2 totalMin = glm::vec2(std::numeric_limits<float>::max());
        glm::vec2 totalMax = glm::vec2(std::numeric_limits<float>::min());
        glm::vec4 currentColor = color;

        std::vector<FontRenderJob> renderJobs;
        float currentX = 0.0f;
        for (uint32_t i = 0; i < glyphs.size(); i++)
        {
            const auto &glyph = glyphs[i];
            const float width = size * glyph.uvExtent.x * c_scale;
            const float height = size * glyph.uvExtent.y * c_scale * aspectRatio;
            float dx = 0.0f;

            switch (alignment)
            {
            case TextAlignment::Center:
                currentX += width / 2;
                dx = -totalWidth / 2 + currentX;
                break;
            case TextAlignment::Left:
                dx = currentX + width / 2;
                break;
            case TextAlignment::Right:
                dx = currentX - totalWidth + width / 2;
                break;
            }

            const glm::vec2 glyphPosition = glm::vec2(position.x + dx, position.y - (glyph.baselineOffset * height) + heightOffset);
            const glm::vec2 glyphSize = glm::vec2(width, height);

            renderJobs.push_back(FontRenderJob{
                .material = context.fontMaterial,
                .position = glyphPosition,
                .size = glyphSize,
                .texCoord = glyph.uvOffset,
                .texSize = glyph.uvExtent,
                .color = currentColor,
            });

            glm::vec2 min = glyphPosition - glyphSize / 2.0f;
            glm::vec2 max = glyphPosition + glyphSize / 2.0f;

            totalMin = glm::min(totalMin, min);
            totalMax = glm::max(totalMax, max);

            switch (alignment)
            {
            case TextAlignment::Center:
                currentX += width / 2;
                break;
            case TextAlignment::Left:
                currentX += width;
                break;
            case TextAlignment::Right:
                currentX += width;
                break;
            }
        }

        context.renderJobs.insert(context.renderJobs.end(), renderJobs.begin(), renderJobs.end());

        return glm::vec4(totalMin, totalMax);
    }

    std::vector<std::string> SplitToLines(const std::string &text, uint32_t maxLineChars)
    {
        std::vector<std::string> lines;
        size_t start = 0;

        while (start < text.size())
        {
            size_t end = std::min(start + maxLineChars, text.size());
            if (end < text.size() && text[end] != ' ')
            {
                size_t lastSpace = text.rfind(' ', end);
                if (lastSpace != std::string::npos && lastSpace > start)
                {
                    end = lastSpace;
                }
            }
            lines.push_back(text.substr(start, end - start));
            start = end;
            if (start < text.size() && text[start] == ' ')
            {
                ++start; // Skip the space at the start of the next line
            }
        }

        return lines;
    }

    glm::vec4 RenderText(const SceneRenderer &renderer, UIContext &context, const std::string &text, const glm::vec2 &position, float size, const glm::vec4 &color, float aspectRatio, TextAlignment alignment, bool centerVertically, uint32_t maxLineLength, float lineSpacing)
    {
        std::vector<std::string> lines;

        if (maxLineLength == 0)
        {
            lines.push_back(text);
        }
        else
        {
            lines = SplitToLines(text, maxLineLength);
        }

        glm::vec2 min = glm::vec2(std::numeric_limits<float>::max());
        glm::vec2 max = glm::vec2(std::numeric_limits<float>::min());

        const glm::vec2 startPosition = centerVertically ? position - glm::vec2(0.0f, (lines.size() - 1) * lineSpacing * size / 2.0f) : position;

        for (uint32_t i = 0; i < lines.size(); i++)
        {
            glm::vec4 minMax = RenderLine(renderer, context, lines[i], startPosition + glm::vec2(0.0f, i * lineSpacing * size), size, color, aspectRatio, alignment, centerVertically);
            min = glm::min(min, glm::vec2(minMax.x, minMax.y));
            max = glm::max(max, glm::vec2(minMax.z, minMax.w));
        }

        return glm::vec4(min, max);
    }

    glm::vec4 RenderText(SceneRenderer &renderer, RenderHandle fontAtlas, RenderHandle fontMaterial, const std::string &text, const glm::vec2 &position, float size, const glm::vec4 &color, float aspectRatio, TextAlignment alignment, bool centerVertically, uint32_t maxLineLength, float lineSpacing)
    {
        UIContext context{
            .fontAtlas = fontAtlas,
            .fontMaterial = fontMaterial,
        };

        glm::vec4 bounds = RenderText(renderer, context, text, position, size, color, aspectRatio, alignment, centerVertically, maxLineLength, lineSpacing);

        for (const auto &job : context.renderJobs)
        {
            std::visit(
                [&](const auto &job)
                {
                    renderer.SubmitRenderJob(job);
                },
                job);
        }

        return bounds;
    }

    void RenderProgressBar(SceneRenderer &renderer, RenderHandle material, const glm::vec2 &position, const glm::vec2 &size, const glm::vec2 &texCoord, const glm::vec2 &texSize, float percent, const glm::vec4 &fillColor, const glm::vec4 &backgroundColor, float aspectRatio)
    {
        const float x = position.x;
        const float y = position.y;
        const float width = size.x;
        const float height = size.y * aspectRatio;

        if (percent > 0.0f)
        {
            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = material,
                .position = glm::vec2(x + width * percent / 2.0f, y + height / 2.0f),
                .size = glm::vec2(width * percent, height),
                .texCoord = texCoord,
                .texSize = texSize * glm::vec2(percent, 1.0f),
                .color = fillColor,
            });
        }

        if (percent < 1.0f)
        {
            // Background, should be growing from the opposite side
            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = material,
                .position = glm::vec2(x + width * percent + (1.0f - percent) * width / 2.0f, y + height / 2.0f),
                .size = glm::vec2((1.0f - percent) * width, height),
                .texCoord = texCoord + glm::vec2(texSize.x * percent, 0.0f),
                .texSize = texSize * glm::vec2(1.0f - percent, 1.0f),
                .color = backgroundColor,
            });
        }
    }

    bool IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectPosition, const glm::vec2 &rectSize)
    {
        return point.x >= rectPosition.x - rectSize.x / 2.0f &&
               point.x <= rectPosition.x + rectSize.x / 2.0f &&
               point.y >= rectPosition.y - rectSize.y / 2.0f &&
               point.y <= rectPosition.y + rectSize.y / 2.0f;
    }

    void PlayHoverSound()
    {
        Application::GetAudioSystem().PlayEvent("event:/ui/hover");
    }

    void PlayClickSound()
    {
        Application::GetAudioSystem().PlayEvent("event:/ui/click");
    }

    Container RenderButton(const std::string &text, const LayoutValue &size, const LayoutValue &margin, const std::function<void(Container &)> &onClicked)
    {
        return Container{
            .id = text,
            .size = size,
            .color = glm::vec4(glm::vec3(c_defaultBackgroundColor), 0.75f),
            .hoverColor = c_defaultHoverBackgroundColor,
            .margin = margin,
            .borderRadius = glm::vec4(c_smallBorderRadius),
            .ignoreInput = false,
            .onClicked = onClicked,
            .children = {
                Text{
                    .size = RelativeValue(1.0f, 1.0f),
                    .color = c_defaultTextColor,
                    .hoverColor = c_defaultHoverTextColor,
                    .text = text,
                    .fontSize = 1.0f,
                    .alignment = TextAlignment::Center,
                },
            },
        };
    }
}