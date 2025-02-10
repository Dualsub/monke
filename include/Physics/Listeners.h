#pragma once

#include "Physics/Types.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"

#include <set>
#include <unordered_map>
#include <vector>

namespace mk
{

    struct Contact
    {
        BodyID body;
        uint32_t data;
        glm::vec3 position;
        glm::vec3 normal;
        float penetration;
    };

    struct PairContact
    {
        BodyID body1;
        BodyID body2;
        glm::vec3 position;
        glm::vec3 normal;
        float penetration;
    };

    class ContactListener : public JPH::ContactListener
    {
    private:
        // Bodies we listen to contact events for:
        std::set<BodyID> m_listeningBodies;
        std::unordered_map<BodyID, std::vector<Contact>> m_contacts = {};
        std::mutex m_mutex;

    public:
        // See: ContactListener
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
        {
            // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
            // Get the user data from the bodies, it is stored in a uint64_t
            uint64_t bodyRawData1 = inBody1.GetUserData();
            uint64_t bodyRawData2 = inBody2.GetUserData();

            UserData body1Data = *reinterpret_cast<UserData *>(&bodyRawData1);
            UserData body2Data = *reinterpret_cast<UserData *>(&bodyRawData2);

            BodyID bodyId1 = body1Data.id;
            BodyID bodyId2 = body2Data.id;

            if (m_listeningBodies.find(bodyId1) == m_listeningBodies.end() && m_listeningBodies.find(bodyId2) == m_listeningBodies.end())
            {
                return;
            }

            auto position = JoltHelpers::ConvertWithUnits(inManifold.mBaseOffset);
            auto normal = JoltHelpers::Convert(inManifold.mWorldSpaceNormal);
            auto penetration = JoltHelpers::FromJolt(inManifold.mPenetrationDepth);

            m_mutex.lock();

            if (m_contacts.find(bodyId1) == m_contacts.end())
            {
                m_contacts[bodyId1] = std::vector<Contact>();
            }

            if (m_contacts.find(bodyId2) == m_contacts.end())
            {
                m_contacts[bodyId2] = std::vector<Contact>();
            }

            m_contacts[bodyId1].emplace_back(Contact{bodyId2, body2Data.data, position, normal, penetration});
            m_contacts[bodyId2].emplace_back(Contact{bodyId1, body1Data.data, position, -normal, penetration});

            m_mutex.unlock();
        }

        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
        {
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
        {
        }

        const void Register(BodyID bodyId)
        {
            m_listeningBodies.insert(bodyId);
        }

        const void Unregister(BodyID bodyId)
        {
            m_listeningBodies.erase(bodyId);
        }

        const std::vector<Contact> &GetContacts(BodyID bodyId) const
        {
            static std::vector<Contact> empty = {};
            auto it = m_contacts.find(bodyId);
            if (it != m_contacts.end())
            {
                return it->second;
            }
            else
            {
                return empty;
            }
        }

        void ClearContacts()
        {
            m_contacts.clear();
        }
    };

    // An example activation listener
    class BodyActivationListener : public JPH::BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
        {
        }

        virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
        {
        }
    };

}