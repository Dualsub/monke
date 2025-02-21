#pragma once

#include "Vultron/SceneRenderer.h"

#include <glm/glm.hpp>

using namespace Vultron;

namespace mk::ParticleHelper
{
    constexpr uint32_t numSubAtlas = 6;

    constexpr glm::vec2 ToAtlasPos(const glm::vec2 &coord, uint32_t subAtlasIndex)
    {
        return glm::vec2(coord.x * (1.0f / float(numSubAtlas)), coord.y) + glm::vec2(float(subAtlasIndex) / float(numSubAtlas), 0.0f);
    }

    constexpr glm::vec2 ToAtlasSize(const glm::vec2 &size)
    {
        return {size.x * (1.0f / float(numSubAtlas)), size.y};
    }

    void SpawnExplosionEffect(SceneRenderer &renderer, const glm::vec3 &position);
    void SpawnIceExplosionEffect(SceneRenderer &renderer, const glm::vec3 &position);
    void SpawnFireExplosionEffect(SceneRenderer &renderer, const glm::vec3 &position);
    void SpawnGroundImpact(SceneRenderer &renderer, const glm::vec3 &position, float scale = 1.0f);
    ParticleEmitJob SpawnImpactEffect(const glm::vec3 &position, const glm::vec3 &direction, const glm::vec4 &color);
    void SpawnSmokeTrail(SceneRenderer &renderer, const glm::vec3 &position, float scale = 1.0f);
    void SpawnIceSmokeTrail(SceneRenderer &renderer, const glm::vec3 &position, float scale = 1.0f);
    void SpawnFireTrail(SceneRenderer &renderer, const glm::vec3 &position, float scale = 1.0f);
    void SpawnSmoke(SceneRenderer &renderer, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, const glm::vec4 &startColor, const glm::vec4 &endColor, float scale = 1.0f);
    void SpawnFire(SceneRenderer &renderer, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float deceleration, float minSize, float maxSize, float lifeTime, float scale = 1.0f);
    void SpawnFire(SceneRenderer &renderer, const glm::vec3 &position, float scale = 1.0f);
    void SpawnBloodEffect(SceneRenderer &renderer, const glm::vec3 &position, const glm::vec3 &direction);
    void SpawnPickupParticles(SceneRenderer &renderer, const glm::vec3 &position, const glm::vec4 &color);
    ParticleEmitJob SpawnPickupParticles(const glm::vec3 &position, const glm::vec4 &color, float scale = 1.0f, uint32_t minParticles = 8, uint32_t maxParticles = 12);
    void SpawnSpark(SceneRenderer &renderer, const glm::vec3 &position);
    void SpawnEmbers(SceneRenderer &renderer, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, float scale = 1.0f);
}