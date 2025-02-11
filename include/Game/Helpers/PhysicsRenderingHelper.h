#pragma once

#include "Core/Core.h"
#include "Physics/PhysicsWorld.h"

#include "Vultron/SceneRenderer.h"

#include <glm/glm.hpp>

using namespace Vultron;

namespace mk::PhysicsRenderingHelper
{
    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const BoxShape &boxShape, const glm::vec4 &color);
    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const SphereShape &sphereShape, const glm::vec4 &color);
    void RenderShape(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const CapsuleShape &capsuleShape, const glm::vec4 &color);
    void RenderCollision(SceneRenderer &renderer, const glm::vec3 &position, const glm::quat &rotation, const CollisionData &collision);
}