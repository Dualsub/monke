#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>

namespace ACS
{
    using BodyID = uint32_t;
    constexpr BodyID c_invalidBodyID = -1;

    enum class BodyType : uint8_t
    {
        Rigidbody = 0,
        Character = 1,
    };

    struct RaycastResult
    {
        glm::vec3 position;
        float distance;
        BodyID hitBody;
        uint32_t data;
    };

    enum class RaycastType
    {
        Closest,
        All,

        Count,
        None,
    };

    struct UserData
    {
        BodyID id;
        uint32_t data;
    };

    struct RigidBodyState
    {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 linearVelocity;
        glm::vec3 angularVelocity;
    };
}