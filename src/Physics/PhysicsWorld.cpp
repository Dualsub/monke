#include "Physics/PhysicsWorld.h"
#include "Physics/Helpers.h"
#include "Physics/Debug.h"

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

#include <cstdarg>
#include <thread>

namespace mk
{

    using namespace JPH::literals;

    std::unique_ptr<JPH::TempAllocatorImpl> PhysicsWorld::s_tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> PhysicsWorld::s_jobSystem;

    // Callback for traces, connect this to your own trace function if you have one
    static void TraceImpl(const char *inFMT, ...)
    {
        // Format the message
        std::va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // Print to the TTY
        std::cout << buffer << std::endl;
    }

    void PhysicsWorld::Initialize()
    {
        JPH::RegisterDefaultAllocator();

        JPH::Trace = TraceImpl;

        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();

        s_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
        s_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1);

        const uint32_t maxBodies = 1024;
        const uint32_t numBodyMutexes = 0;
        const uint32_t maxBodyPairs = 65536;
        const uint32_t maxContactConstraints = 10240;

        m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();

        m_physicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, m_broadPhaseLayerInterface, m_objectVsBroadphaseLayerFilter, m_objectLayerPairFilter);

        m_physicsSystem->SetBodyActivationListener(&m_bodyActivationListener);

        m_physicsSystem->SetContactListener(&m_contactListener);

        JPH::PhysicsSettings settings;
        settings.mSpeculativeContactDistance = 0.0f;

        m_physicsSystem->SetPhysicsSettings(settings);
    }

    void PhysicsWorld::Shutdown()
    {
        // Unregisters all types with the factory and cleans up the default material
        JPH::UnregisterTypes();

        // Destroy the factory
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsWorld::StepSimulation(float dt, uint32_t numSubSteps)
    {
        m_physicsSystem->Update(dt, numSubSteps, s_tempAllocator.get(), s_jobSystem.get());

        const float collisionTolerance = 0.05f;
        for (auto &[id, character] : m_characters)
        {
            character->PostSimulation(collisionTolerance);
        }
    }

    RigidBodyState PhysicsWorld::GetRigidBodyState(BodyID id)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];

        JPH::Vec3 position = interface.GetCenterOfMassPosition(bodyId);
        JPH::Quat rotation = interface.GetRotation(bodyId);
        JPH::Vec3 linearVelocity = interface.GetLinearVelocity(bodyId);
        JPH::Vec3 angularVelocity = interface.GetAngularVelocity(bodyId);

        return {
            JoltHelpers::ConvertWithUnits(position),
            JoltHelpers::Convert(rotation),
            JoltHelpers::ConvertWithUnits(linearVelocity),
            JoltHelpers::Convert(angularVelocity)};
    }

    glm::vec3 PhysicsWorld::GetPosition(BodyID id)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        JPH::Vec3 position = interface.GetCenterOfMassPosition(bodyId);
        return JoltHelpers::ConvertWithUnits(position);
    }

    glm::vec3 PhysicsWorld::GetLinearVelocity(BodyID id)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        JPH::Vec3 linearVelocity = interface.GetLinearVelocity(bodyId);
        return JoltHelpers::ConvertWithUnits(linearVelocity);
    }

    BodyID PhysicsWorld::CreateRigidBody(const RigidBodySettings &info, BodyType type)
    {
        assert(m_nextBodyID < std::numeric_limits<BodyID>::max() && "BodyID overflow");

        BodyID id = m_nextBodyID++;
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();

        static_assert(sizeof(UserData) == sizeof(JPH::uint64), "UserData must be 64 bits");
        UserData userData{id, info.data};
        uint64_t userDataBits = *reinterpret_cast<uint64_t *>(&userData); // :) I know

        // Creating shape
        JPH::ShapeSettings::ShapeResult shapeResult = std::visit(
            [&](const auto &shape)
            { return shape.GetShapeSettings(); },
            info.shape);
        JPH::ShapeRefC shape = shapeResult.Get();

        m_collisions[id] = {info.shape, info.layer};

        if (type == BodyType::Rigidbody)
        {
            JPH::ObjectLayer layer = info.layer != ObjectLayer::None ? static_cast<JPH::ObjectLayer>(info.layer) : info.mass > 0.0f ? Layers::MOVING
                                                                                                                                    : Layers::NON_MOVING;
            JPH::EMotionType motionType = info.mass > 0.0f ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;

            JPH::BodyCreationSettings settings(shape, JoltHelpers::ConvertWithUnits(info.position), JoltHelpers::Convert(info.rotation), motionType, layer);
            settings.mGravityFactor = info.gravityFactor;
            settings.mMotionQuality = info.continuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
            settings.mUserData = userDataBits;
            settings.mIsSensor = info.isSensor;
            JPH::BodyID bodyId = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
            interface.SetLinearVelocity(bodyId, JoltHelpers::ConvertWithUnits(info.initialVelocity));
            m_bodyIDs[id] = bodyId;
        }
        else if (type == BodyType::Character)
        {
            JPH::CharacterSettings settings;
            settings.mShape = shape;
            settings.mLayer = info.layer != ObjectLayer::None ? static_cast<JPH::ObjectLayer>(info.layer) : Layers::MOVING;
            settings.mUp = JPH::Vec3::sAxisY();
            settings.mGravityFactor = info.gravityFactor;
            settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -shape->GetLocalBounds().GetExtent().GetY());
            settings.mFriction = info.friction;
            // Hardcoded for now
            settings.mMaxSlopeAngle = glm::radians(45.0f);

            std::unique_ptr<JPH::Character> character = std::make_unique<JPH::Character>(&settings, JoltHelpers::ConvertWithUnits(info.position), JoltHelpers::Convert(info.rotation), 0, m_physicsSystem.get());
            character->AddToPhysicsSystem(JPH::EActivation::Activate);
            JPH::BodyID bodyId = character->GetBodyID();
            m_characters[id] = std::move(character);
            m_bodyIDs[id] = bodyId;
            interface.SetUserData(bodyId, userDataBits);
        }
        else
        {
            assert(false && "Unknown body type");
        }

        return id;
    }

    void PhysicsWorld::RemoveRigidBody(BodyID id)
    {
        if (m_bodyIDs.find(id) == m_bodyIDs.end())
            return;

        if (m_characters.find(id) != m_characters.end())
        {
            m_characters[id]->RemoveFromPhysicsSystem();
            m_characters.erase(id);
        }
        else
        {
            JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
            JPH::BodyID &bodyId = m_bodyIDs[id];

            if (!bodyId.IsInvalid() && interface.IsAdded(bodyId))
            {
                interface.RemoveBody(bodyId);
                interface.DestroyBody(bodyId);
            }
        }

        m_bodyIDs.erase(id);
        m_collisions.erase(id);
    }

    void PhysicsWorld::RemoveAllRigidBodies()
    {
        std::vector<BodyID> idsToRemove;
        for (auto &[id, bodyId] : m_bodyIDs)
        {
            idsToRemove.push_back(id);
        }

        for (auto id : idsToRemove)
        {
            RemoveRigidBody(id);
        }

        m_nextBodyID = 0;
        m_bodyIDs.clear();
    }

    void PhysicsWorld::SetPosition(BodyID id, glm::vec3 position)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        interface.SetPosition(bodyId, JoltHelpers::ConvertWithUnits(position), JPH::EActivation::Activate);
    }

    void PhysicsWorld::SetRotation(BodyID id, glm::quat rotation)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        interface.SetRotation(bodyId, JoltHelpers::Convert(rotation), JPH::EActivation::Activate);
    }

    void PhysicsWorld::SetLinearVelocity(BodyID id, glm::vec3 velocity)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        interface.SetLinearVelocity(bodyId, JoltHelpers::ConvertWithUnits(velocity));
    }

    void PhysicsWorld::SetAngularVelocity(BodyID id, glm::vec3 velocity)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        interface.SetAngularVelocity(bodyId, JoltHelpers::Convert(velocity));
    }

    void PhysicsWorld::ApplyImpulse(BodyID id, const glm::vec3 &impulse)
    {
        JPH::BodyInterface &interface = m_physicsSystem->GetBodyInterfaceNoLock();
        JPH::BodyID bodyId = m_bodyIDs[id];
        interface.AddImpulse(bodyId, JoltHelpers::ConvertWithUnits(impulse));
    }

    CharacterGroundState PhysicsWorld::GetCharacterGroundState(BodyID id)
    {
        if (m_characters.find(id) == m_characters.end())
            return CharacterGroundState::Unknown;

        JPH::Character *character = m_characters[id].get();
        JPH::Character::EGroundState state = character->GetGroundState();
        return static_cast<CharacterGroundState>(state);
    }

    void PhysicsWorld::SetCharacterRotation(BodyID id, glm::quat rotation)
    {
        if (m_characters.find(id) == m_characters.end())
            return;

        JPH::Character *character = m_characters[id].get();
        character->SetRotation(JoltHelpers::Convert(rotation));
    }

    void PhysicsWorld::RegisterContactListener(BodyID id)
    {
        m_contactListener.Register(id);
    }

    void PhysicsWorld::UnregisterContactListener(BodyID id)
    {
        m_contactListener.Unregister(id);
    }

    const std::vector<Contact> &PhysicsWorld::GetContacts(BodyID id) const
    {
        return m_contactListener.GetContacts(id);
    }

    void PhysicsWorld::ResetContacts()
    {
        m_contactListener.ClearContacts();
    }

    std::vector<RaycastResult> PhysicsWorld::Raycast(const glm::vec3 &from, const glm::vec3 &direction, float distance, RaycastType type) const
    {
        std::vector<RaycastResult> raycastResults;
        const auto &query = m_physicsSystem->GetNarrowPhaseQuery();
        auto &interface = m_physicsSystem->GetBodyInterfaceNoLock();

        JPH::RRayCast ray;
        ray.mOrigin = JoltHelpers::ConvertWithUnits(from);
        ray.mDirection = JoltHelpers::ConvertWithUnits(direction * distance);

        JPH::RayCastSettings settings;
        switch (type)
        {
        case RaycastType::Closest:
        {
            JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
            query.CastRay(ray, settings, collector);
            if (collector.HadHit())
            {
                const auto &hit = collector.mHit;
                RaycastResult raycastResult;
                raycastResult.position = JoltHelpers::ConvertWithUnits(ray.GetPointOnRay(hit.mFraction));
                raycastResult.distance = hit.mFraction * distance;
                auto userDataBits = interface.GetUserData(hit.mBodyID);
                UserData userData = *reinterpret_cast<UserData *>(&userDataBits);
                raycastResult.hitBody = userData.id;
                raycastResult.data = userData.data;
                raycastResults.push_back(raycastResult);
            }
        }
        break;
        case RaycastType::All:
        {
            JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
            query.CastRay(ray, settings, collector);
            if (collector.HadHit())
            {
                // Append all hits
                raycastResults.reserve(collector.mHits.size());
                for (auto &hit : collector.mHits)
                {
                    RaycastResult raycastResult;
                    raycastResult.position = JoltHelpers::ConvertWithUnits(ray.GetPointOnRay(hit.mFraction));
                    auto userDataBits = interface.GetUserData(hit.mBodyID);
                    UserData userData = *reinterpret_cast<UserData *>(&userDataBits);
                    raycastResult.hitBody = userData.id;
                    raycastResult.data = userData.data;
                    raycastResults.push_back(raycastResult);
                }
            }
        }
        break;
        }

        return raycastResults;
    }
}