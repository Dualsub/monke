#pragma once

#include "Physics/Helpers.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include <variant>
#include <optional>

namespace ACS
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

    using CollisionShape = std::variant<SphereShape, BoxShape, CapsuleShape>;
    using OptionalCollisionShape = std::optional<CollisionShape>;
}