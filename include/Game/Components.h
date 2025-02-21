#pragma once

#include "Core/Timer.h"
#include "Physics/Types.h"

#include "Vultron/SceneRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace Vultron;

namespace mk
{
    struct Transform
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 scale = glm::vec3(1.0f);

        glm::mat4 GetMatrix() const
        {
            glm::mat4 matrix = glm::mat4(1.0f);
            matrix = glm::translate(matrix, position);
            matrix = glm::rotate(matrix, glm::angle(rotation), glm::axis(rotation));
            matrix = glm::scale(matrix, scale);
            return matrix;
        }

        void SetMatrix(const glm::mat4 &matrix)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(matrix, scale, rotation, position, skew, perspective);
        }
    };

    struct AABB
    {
        glm::vec3 min = glm::vec3(0.0f);
        glm::vec3 max = glm::vec3(0.0f);
    };

    struct CameraSocket
    {
        float fov = 70.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;
    };

    struct Renderable
    {
        RenderHandle mesh = c_invalidHandle;
        RenderHandle material = c_invalidHandle;
        glm::mat4 renderMatrix = glm::mat4(1.0f);
        glm::vec4 color = glm::vec4(1.0f);
        glm::vec2 uvOffset = glm::vec2(0.0f);
        glm::vec2 uvScale = glm::vec2(1.0f);
    };

    struct PhysicsProxy
    {
        BodyID bodyID = c_invalidBodyID;
        RigidBodyState currentState;
        RigidBodyState previousState;
    };

    struct PlayerMovement
    {
        glm::vec3 dashDirection = glm::vec3(0.0f);
        DynamicTimer dashTimer = DynamicTimer(false);
        float dashSpeed = 0.0f;
        float jumpSpeed = 0.0f;
        bool wantsToJump = false;
        float yawSpeed = 0.0f;
        float pitchSpeed = 0.0f;
        float stamina = 1.0f;
    };

    struct Animation
    {
        float time = 0.0f;
        float duration;
        bool loop = false;

        struct KeyFrame
        {
            glm::vec3 position;
            glm::quat rotation;
            float time;
        };

        std::vector<KeyFrame> keyframes = {};
    };

    struct PlayerAnimations
    {
        Animation shootAnimation;
    };

    struct Lifetime
    {
        DynamicTimer timer = DynamicTimer(false);
    };

    enum class EnemyType
    {
        Basic,

        Count,
        None
    };

    struct Health
    {
        float current = 100.0f;
        float max = 100.0f;
    };

    // One component that is the weapon trigger action, like full auto or semi auto, then another that is the bullet firing part
    struct WeaponFireAction
    {
        bool automatic = false;
        float fireRate = 600.0f;
        DynamicTimer fireTimer = DynamicTimer(false);

        bool fire = false;
    };

    // Reads the WeaponTrigger component and fires a bullet

    struct ProjectileBulletEmitter
    {
        float speed = 0.0f;
        float damage = 0.0f;
        float lifetime = 0.0f;
    };

    struct RaycastBulletEmitter
    {
        float distance = 0.0f;
        RaycastType type = RaycastType::Closest;
    };
}