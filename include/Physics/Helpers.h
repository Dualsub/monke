#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Jolt/Jolt.h"

#include <cstdint>

namespace ACS::JoltHelpers
{

    const float spaceScale = 100.0f;
    const float spaceScaleInv = 1 / spaceScale;

    inline glm::vec3 ConvertWithUnits(const JPH::Vec3 &inVec)
    {
        return glm::vec3(inVec.GetX(), inVec.GetY(), inVec.GetZ()) * spaceScale;
    }

    inline JPH::Vec3 ConvertWithUnits(const glm::vec3 &inVec)
    {
        return JPH::Vec3(inVec.x, inVec.y, inVec.z) * spaceScaleInv;
    }

    inline glm::vec3 Convert(const JPH::Vec3 &inVec)
    {
        return glm::vec3(inVec.GetX(), inVec.GetY(), inVec.GetZ());
    }

    inline JPH::Vec3 Convert(const glm::vec3 &inVec)
    {
        return JPH::Vec3(inVec.x, inVec.y, inVec.z);
    }

    inline glm::quat Convert(const JPH::Quat &inQuat)
    {
        return glm::quat(inQuat.GetW(), inQuat.GetX(), inQuat.GetY(), inQuat.GetZ());
    }

    inline JPH::Quat Convert(const glm::quat &inQuat)
    {
        return JPH::Quat(inQuat.x, inQuat.y, inQuat.z, inQuat.w);
    }

    inline float ToJolt(float inValue)
    {
        return inValue * spaceScaleInv;
    }

    inline float FromJolt(float inValue)
    {
        return inValue * spaceScale;
    }

}