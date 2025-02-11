#include "Game/Helpers/PhysicsRenderingHelper.h"

#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace mk::PhysicsRenderingHelper
{
    constexpr uint32_t c_numSegments = 16;

    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const BoxShape &boxShape, const glm::vec4 &color)
    {
        const glm::vec3 halfExtents = boxShape.GetHalfExtents();
        // Define the eight vertices of the box
        glm::vec3 vertices[8];
        vertices[0] = position + glm::rotate(rotation, glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z));
        vertices[1] = position + glm::rotate(rotation, glm::vec3(-halfExtents.x, -halfExtents.y, halfExtents.z));
        vertices[2] = position + glm::rotate(rotation, glm::vec3(-halfExtents.x, halfExtents.y, -halfExtents.z));
        vertices[3] = position + glm::rotate(rotation, glm::vec3(-halfExtents.x, halfExtents.y, halfExtents.z));
        vertices[4] = position + glm::rotate(rotation, glm::vec3(halfExtents.x, -halfExtents.y, -halfExtents.z));
        vertices[5] = position + glm::rotate(rotation, glm::vec3(halfExtents.x, -halfExtents.y, halfExtents.z));
        vertices[6] = position + glm::rotate(rotation, glm::vec3(halfExtents.x, halfExtents.y, -halfExtents.z));
        vertices[7] = position + glm::rotate(rotation, glm::vec3(halfExtents.x, halfExtents.y, halfExtents.z));

        // Define the indices for the edges of the box
        constexpr int edgeIndices[12][2] = {
            {0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

        for (uint32_t i = 0; i < 12; i++)
        {
            uint32_t startIndex = edgeIndices[i][0];
            uint32_t endIndex = edgeIndices[i][1];
            glm::vec3 start = vertices[startIndex];
            glm::vec3 end = vertices[endIndex];
            renderer.SubmitRenderJob({start, end, color});
        }
    }

    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const SphereShape &sphereShape, const glm::vec4 &color)
    {
        const float angleIncrement = glm::radians(360.0f / c_numSegments);
        float radius = sphereShape.GetRadius();
        glm::vec3 center = position;

        for (int i = 0; i < c_numSegments; i++)
        {
            float theta1 = i * angleIncrement;
            float theta2 = (i + 1) * angleIncrement;

            glm::vec3 point1 = glm::vec3(radius * glm::cos(theta1), 0.0f, radius * glm::sin(theta1));
            glm::vec3 point2 = glm::vec3(radius * glm::cos(theta2), 0.0f, radius * glm::sin(theta2));

            // Rotate the points based on the transform's rotation
            point1 = glm::rotate(rotation, point1) + center;
            point2 = glm::rotate(rotation, point2) + center;

            renderer.SubmitRenderJob({point1, point2, color});
        }

        for (int i = 0; i < c_numSegments / 2; i++)
        {
            float phi1 = i * angleIncrement;
            float phi2 = (i + 1) * angleIncrement;
            float phi3 = i * angleIncrement - glm::radians(180.0f);
            float phi4 = (i + 1) * angleIncrement - glm::radians(180.0f);

            glm::vec3 point1 = glm::vec3(0.0f, radius * glm::sin(phi1), radius * glm::cos(phi1));
            glm::vec3 point2 = glm::vec3(0.0f, radius * glm::sin(phi2), radius * glm::cos(phi2));
            glm::vec3 point3 = glm::vec3(0.0f, radius * glm::sin(phi3), radius * glm::cos(phi3));
            glm::vec3 point4 = glm::vec3(0.0f, radius * glm::sin(phi4), radius * glm::cos(phi4));

            glm::vec3 point5 = glm::vec3(radius * glm::cos(phi1), radius * glm::sin(phi1), 0.0f);
            glm::vec3 point6 = glm::vec3(radius * glm::cos(phi2), radius * glm::sin(phi2), 0.0f);
            glm::vec3 point7 = glm::vec3(radius * glm::cos(phi3), radius * glm::sin(phi3), 0.0f);
            glm::vec3 point8 = glm::vec3(radius * glm::cos(phi4), radius * glm::sin(phi4), 0.0f);

            // Rotate the points based on the transform's rotation
            point1 = glm::rotate(rotation, point1) + center;
            point2 = glm::rotate(rotation, point2) + center;
            point3 = glm::rotate(rotation, point3) + center;
            point4 = glm::rotate(rotation, point4) + center;
            point5 = glm::rotate(rotation, point5) + center;
            point6 = glm::rotate(rotation, point6) + center;
            point7 = glm::rotate(rotation, point7) + center;
            point8 = glm::rotate(rotation, point8) + center;

            renderer.SubmitRenderJob({point1, point2, color});
            renderer.SubmitRenderJob({point3, point4, color});
            renderer.SubmitRenderJob({point5, point6, color});
            renderer.SubmitRenderJob({point7, point8, color});
        }
    }

    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const CapsuleShape &capsuleShape, const glm::vec4 &color)
    {
        const float angleIncrement = glm::radians(360.0f / c_numSegments);
        glm::vec3 center = position;
        const float radius = capsuleShape.GetRadius();
        const float halfHeight = capsuleShape.GetHalfHeight();

        // Draw the top rings
        for (int i = 0; i < c_numSegments; i++)
        {
            float theta1 = i * angleIncrement;
            float theta2 = (i + 1) * angleIncrement;

            glm::vec3 point1 = glm::vec3(radius * glm::cos(theta1), 0.0f, radius * glm::sin(theta1)) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point2 = glm::vec3(radius * glm::cos(theta2), 0.0f, radius * glm::sin(theta2)) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point3 = glm::vec3(radius * glm::cos(theta1), 0.0f, radius * glm::sin(theta1)) + glm::vec3(0.0f, -halfHeight, 0.0f);
            glm::vec3 point4 = glm::vec3(radius * glm::cos(theta2), 0.0f, radius * glm::sin(theta2)) + glm::vec3(0.0f, -halfHeight, 0.0f);

            // Rotate the points based on the transform's rotation
            point1 = glm::rotate(rotation, point1) + center;
            point2 = glm::rotate(rotation, point2) + center;
            point3 = glm::rotate(rotation, point3) + center;
            point4 = glm::rotate(rotation, point4) + center;

            renderer.SubmitRenderJob({point1, point2, color});
            renderer.SubmitRenderJob({point3, point4, color});
        }

        // Add lines to connect the poles to the equator

        for (int i = 0; i < c_numSegments / 2; i++)
        {
            float phi1 = i * angleIncrement;
            float phi2 = (i + 1) * angleIncrement;
            float phi3 = i * angleIncrement - glm::radians(180.0f);
            float phi4 = (i + 1) * angleIncrement - glm::radians(180.0f);

            glm::vec3 point1 = glm::vec3(0.0f, radius * glm::sin(phi1), radius * glm::cos(phi1)) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point2 = glm::vec3(0.0f, radius * glm::sin(phi2), radius * glm::cos(phi2)) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point3 = glm::vec3(0.0f, radius * glm::sin(phi3), radius * glm::cos(phi3)) + glm::vec3(0.0f, -halfHeight, 0.0f);
            glm::vec3 point4 = glm::vec3(0.0f, radius * glm::sin(phi4), radius * glm::cos(phi4)) + glm::vec3(0.0f, -halfHeight, 0.0f);

            glm::vec3 point5 = glm::vec3(radius * glm::cos(phi1), radius * glm::sin(phi1), 0.0f) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point6 = glm::vec3(radius * glm::cos(phi2), radius * glm::sin(phi2), 0.0f) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point7 = glm::vec3(radius * glm::cos(phi3), radius * glm::sin(phi3), 0.0f) + glm::vec3(0.0f, -halfHeight, 0.0f);
            glm::vec3 point8 = glm::vec3(radius * glm::cos(phi4), radius * glm::sin(phi4), 0.0f) + glm::vec3(0.0f, -halfHeight, 0.0f);

            // Rotate the points based on the transform's rotation
            point1 = glm::rotate(rotation, point1) + center;
            point2 = glm::rotate(rotation, point2) + center;
            point3 = glm::rotate(rotation, point3) + center;
            point4 = glm::rotate(rotation, point4) + center;
            point5 = glm::rotate(rotation, point5) + center;
            point6 = glm::rotate(rotation, point6) + center;
            point7 = glm::rotate(rotation, point7) + center;
            point8 = glm::rotate(rotation, point8) + center;

            renderer.SubmitRenderJob({point1, point2, color});
            renderer.SubmitRenderJob({point3, point4, color});
            renderer.SubmitRenderJob({point5, point6, color});
            renderer.SubmitRenderJob({point7, point8, color});
        }

        // Draw the 4 lines connecting the top and bottom rings
        for (int i = 0; i < 4; i++)
        {
            float theta = glm::radians(i * 90.0f);

            glm::vec3 point1 = glm::vec3(radius * glm::cos(theta), 0.0f, radius * glm::sin(theta)) + glm::vec3(0.0f, halfHeight, 0.0f);
            glm::vec3 point2 = glm::vec3(radius * glm::cos(theta), 0.0f, radius * glm::sin(theta)) + glm::vec3(0.0f, -halfHeight, 0.0f);

            // Rotate the points based on the transform's rotation
            point1 = glm::rotate(rotation, point1);
            point2 = glm::rotate(rotation, point2);

            // Offset the points by the center of the sphere
            point1 += center;
            point2 += center;

            renderer.SubmitRenderJob({point1, point2, color});
        }
    }

    void RenderCollision(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const CollisionData &collision)
    {
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        switch (collision.layer)
        {
        case ObjectLayer::Player:
        case ObjectLayer::PlayerProjectile:
            color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case ObjectLayer::Enemy:
        case ObjectLayer::EnemyProjectile:
            color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        case ObjectLayer::NonMoving:
            color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            break;
        default:
            break;
        }

        std::visit([&](const auto &arg)
                   { RenderShape(renderer, position, rotation, arg, color); },
                   collision.shape);
    }
}