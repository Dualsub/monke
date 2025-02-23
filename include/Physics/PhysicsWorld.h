#pragma once

#include "Physics/Types.h"
#include "Physics/CollisionShapes.h"
#include "Physics/Layers.h"
#include "Physics/Listeners.h"

#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Character/Character.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>

#define SL_MAX_PHYSICS_BODIES 512
#define INVALID_BODY_ID -1

namespace mk
{
    enum class ObjectLayer : uint32_t
    {
        NonMoving = Layers::NON_MOVING,
        Moving = Layers::MOVING,
        Player = Layers::PLAYER,
        Enemy = Layers::ENEMY,
        PlayerProjectile = Layers::PLAYER_PROJECTILE,
        EnemyProjectile = Layers::ENEMY_PROJECTILE,

        Count = Layers::NUM_LAYERS,
        None,
    };

    enum class CharacterGroundState : uint8_t
    {
        // On ground is 0 so that we can use it as a bool
        OnGround = static_cast<uint8_t>(JPH::Character::EGroundState::OnGround),
        OnSteepGround = static_cast<uint8_t>(JPH::Character::EGroundState::OnSteepGround),
        NotSupported = static_cast<uint8_t>(JPH::Character::EGroundState::NotSupported),
        InAir = static_cast<uint8_t>(JPH::Character::EGroundState::InAir),
        Unknown
    };

    // Info for creating a rigid body, not runtime data
    struct RigidBodySettings
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 initialVelocity = glm::vec3(0.0f);

        float mass = 1.0f;
        float friction = 0.5f;
        bool continuousCollision = false;
        float gravityFactor = 1.0f;
        bool isSensor = false;

        CollisionShape shape;
        ObjectLayer layer = ObjectLayer::None;
        uint32_t data = 0;
    };

    struct CollisionData
    {
        CollisionShape shape;
        ObjectLayer layer;
    };

    using OptionalCollisionData = std::optional<CollisionData>;

    static_assert(sizeof(UserData) == sizeof(JPH::uint64), "UserData must be 64 bits");

    class PhysicsWorld
    {
    private:
        BodyID m_nextBodyID = 0;

        std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem;

        BPLayerInterfaceImpl m_broadPhaseLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl m_objectVsBroadphaseLayerFilter;
        ObjectLayerPairFilterImpl m_objectLayerPairFilter;
        BodyActivationListener m_bodyActivationListener;
        ContactListener m_contactListener;

        static std::unique_ptr<JPH::TempAllocatorImpl> s_tempAllocator;
        static std::unique_ptr<JPH::JobSystemThreadPool> s_jobSystem;

        std::unordered_map<uint32_t, JPH::BodyID> m_bodyIDs;
        std::unordered_map<uint32_t, std::unique_ptr<JPH::Character>> m_characters;
        std::unordered_map<uint32_t, CollisionData> m_collisions;

    public:
        PhysicsWorld() = default;
        ~PhysicsWorld() = default;

        void Initialize();
        void Shutdown();

        void StepSimulation(float dt, uint32_t numSubSteps = 1);
        RigidBodyState GetRigidBodyState(BodyID id);
        glm::vec3 GetPosition(BodyID id);
        glm::vec3 GetLinearVelocity(BodyID id);
        OptionalCollisionData GetCollisionData(BodyID id) const { return m_collisions.find(id) != m_collisions.end() ? std::make_optional(m_collisions.at(id)) : std::nullopt; }
        bool IsBodyValid(BodyID id) const { return m_bodyIDs.find(id) != m_bodyIDs.end(); }
        ObjectLayer GetObjectLayer(BodyID id) const;

        BodyID CreateRigidBody(const RigidBodySettings &info, BodyType type);
        void UpdateRigidBody(BodyID id, RigidBodySettings &info);
        void RemoveRigidBody(BodyID id);
        void RemoveAllRigidBodies();

        void SetPosition(BodyID id, glm::vec3 position);
        void SetRotation(BodyID id, glm::quat rotation);
        void SetLinearVelocity(BodyID id, glm::vec3 velocity);
        void SetAngularVelocity(BodyID id, glm::vec3 velocity);
        void ApplyImpulse(BodyID id, const glm::vec3 &impulse);
        void SetGravityFactor(BodyID id, float factor);

        CharacterGroundState GetCharacterGroundState(BodyID id);
        void SetCharacterRotation(BodyID id, glm::quat rotation);

        void RegisterContactListener(BodyID id);
        void UnregisterContactListener(BodyID id);
        const std::vector<Contact> &GetContacts(BodyID id) const;
        const std::vector<PairContact> &GetPairContacts() const;
        void ResetContacts();

        std::vector<RaycastResult> Raycast(const glm::vec3 &from, const glm::vec3 &direction, float distance, RaycastType type = RaycastType::Closest) const;
        std::vector<BodyID> CastSphere(const glm::vec3 &center, float radius) const;
    };
}