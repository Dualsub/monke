#pragma once

#include "Physics/Helpers.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"

#include <variant>
#include <optional>
#include <cassert>
#include <iostream>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace mk
{
    class SphereShape
    {
    private:
        float m_radius;

    public:
        SphereShape(float radius) : m_radius(radius) {}
        SphereShape() = default;
        ~SphereShape() = default;

        float GetRadius() const { return m_radius; }

        JPH::ShapeSettings::ShapeResult GetShapeSettings() const
        {
            JPH::SphereShapeSettings settings;
            settings.mRadius = JoltHelpers::ToJolt(m_radius);
            return settings.Create();
        }
    };

    class BoxShape
    {
    private:
        glm::vec3 m_halfExtents;

    public:
        BoxShape(glm::vec3 halfExtents) : m_halfExtents(halfExtents) {}
        BoxShape() = default;
        ~BoxShape() = default;

        glm::vec3 GetHalfExtents() const { return m_halfExtents; }

        JPH::ShapeSettings::ShapeResult GetShapeSettings() const
        {
            JPH::BoxShapeSettings settings;
            settings.mHalfExtent = JoltHelpers::ConvertWithUnits(m_halfExtents);
            return settings.Create();
        }
    };

    class CapsuleShape
    {
    private:
        float m_radius;
        float m_halfHeight;

    public:
        CapsuleShape(float radius, float halfHeight) : m_radius(radius), m_halfHeight(halfHeight) {}
        CapsuleShape() = default;
        ~CapsuleShape() = default;

        float GetRadius() const { return m_radius; }
        float GetHalfHeight() const { return m_halfHeight; }

        JPH::ShapeSettings::ShapeResult GetShapeSettings() const
        {
            JPH::CapsuleShapeSettings settings;
            settings.mRadius = JoltHelpers::ToJolt(m_radius);
            settings.mHalfHeightOfCylinder = JoltHelpers::ToJolt(m_halfHeight);
            return settings.Create();
        }
    };

    class MeshShape
    {
    private:
        // Not the best to copy here but we need to store them for debug rendering
        std::vector<glm::vec3> m_vertices;
        std::vector<uint32_t> m_indices;

    public:
        MeshShape(const std::vector<glm::vec3> &vertices, const std::vector<uint32_t> &indices, const glm::mat4 &transform)
            : m_indices(indices)
        {
            m_vertices.reserve(vertices.size());
            for (const auto &vertex : vertices)
            {
                glm::vec3 newVertex = glm::vec3(transform * glm::vec4(vertex, 1.0f));
                m_vertices.push_back(newVertex);
            }
        }
        MeshShape(const std::vector<glm::vec3> &vertices, const std::vector<uint32_t> &indices)
            : m_vertices(vertices), m_indices(indices) {}
        MeshShape() = default;
        ~MeshShape() = default;

        const std::vector<glm::vec3> &GetVertices() const { return m_vertices; }
        const std::vector<uint32_t> &GetIndices() const { return m_indices; }

        JPH::ShapeSettings::ShapeResult GetShapeSettings() const
        {
            JPH::ConvexHullShapeSettings settings;
            settings.mPoints.reserve(m_vertices.size());
            for (const auto &vertex : m_vertices)
            {
                settings.mPoints.push_back(JoltHelpers::ConvertWithUnits(vertex));
            }
            auto result = settings.Create();
            if (!result.IsValid())
            {
                std::cerr << result.GetError() << std::endl;
                abort();
            }
            return result;
        }
    };

    using CollisionShape = std::variant<SphereShape, BoxShape, CapsuleShape, MeshShape>;
    using OptionalCollisionShape = std::optional<CollisionShape>;
}