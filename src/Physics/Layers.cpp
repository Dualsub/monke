#include "Physics/Layers.h"

namespace mk
{
    bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            return inObject2 != Layers::NON_MOVING; // Non-moving does not collide with other non-moving
        case Layers::MOVING:
            return true; // Moving collides with everything
        case Layers::PLAYER:
            return inObject2 != Layers::PLAYER_PROJECTILE; // Player does not collide with player projectiles
        case Layers::ENEMY:
            return inObject2 != Layers::ENEMY_PROJECTILE; // Enemy does not collide with enemy projectiles
        case Layers::PLAYER_PROJECTILE:
            return inObject2 != Layers::PLAYER && inObject2 != Layers::PLAYER_PROJECTILE; // Player projectile does not collide with player or other player projectiles
        case Layers::ENEMY_PROJECTILE:
            return inObject2 != Layers::ENEMY && inObject2 != Layers::ENEMY_PROJECTILE; // Enemy projectile does not collide with enemy or other enemy projectiles
        default:
            JPH_ASSERT(false);
            return false;
        }
    }

    bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 != BroadPhaseLayers::NON_MOVING; // Non-moving does not collide with other non-moving
        case Layers::MOVING:
        case Layers::PLAYER:
        case Layers::ENEMY:
        case Layers::PLAYER_PROJECTILE:
        case Layers::ENEMY_PROJECTILE:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    };
}