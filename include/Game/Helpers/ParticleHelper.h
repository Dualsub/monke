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

    void SpawnExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position);
    void SpawnIceExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position);
    void SpawnFireExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position);
    void SpawnGroundImpact(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale = 1.0f);
    void SpawnImpactEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, const glm::vec4 &color);
    void SpawnSmokeTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale = 1.0f);
    void SpawnIceSmokeTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale = 1.0f);
    void SpawnFireTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale = 1.0f);
    void SpawnSmoke(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, const glm::vec4 &startColor, const glm::vec4 &endColor, float scale = 1.0f);
    void SpawnFire(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float deceleration, float minSize, float maxSize, float lifeTime, float scale = 1.0f);
    void SpawnFire(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale = 1.0f);
    void SpawnBloodEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction);
    void SpawnPickupParticles(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec4 &color);
    ParticleEmitJob SpawnPickupParticles(const glm::vec3 &position, const glm::vec4 &color, float scale = 1.0f, uint32_t minParticles = 8, uint32_t maxParticles = 12);
    void SpawnSpark(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec4 &startColor, const glm::vec4 &endColor);
    void SpawnEmbers(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, float scale = 1.0f);
}