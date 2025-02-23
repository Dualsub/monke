#include "Game/Helpers/ParticleHelper.h"

#include <random>

namespace mk::ParticleHelper
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    constexpr glm::vec3 c_fireColor = glm::vec3(255.0f, 150.0f, 30.0f) / 255.0f * 10.0f;
    constexpr glm::vec3 c_iceColor = glm::vec3(0.0f, 0.5f, 1.0f) * 2.0f;
    constexpr glm::vec3 c_plasmaColor = glm::vec3(1.0f, 0.0f, 1.0f) * 2.0f;

    void SpawnExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position)
    {
        constexpr float scale = 2.0f;
        constexpr glm::vec4 flameColor = glm::vec4(glm::vec3(255.0f, 150.0f, 30.0f) / 255.0f * 10.0f, 1.0f);
        constexpr glm::vec4 dustColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        // Flare
        if (true)
        {
            constexpr float lifeTime = 0.05f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position - glm::vec3(0.0f, 20.0f, 0.0f),
                .size = glm::vec2(400.0f),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = lifeTime,
                .initialVelocity = glm::vec3(0.0f),
                .velocitySpan = 0.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = flameColor * glm::vec4(glm::vec3(1.0f), 1.0f),
                .endColor = flameColor * glm::vec4(glm::vec3(0.1f), 0.0f),
                .numParticles = 1,
                .scaleIn = lifeTime,
                .opacityIn = 0.0f,
                .opacityOut = 0.5f * lifeTime,
            });
        }

        // Smoke out
        if (true)
        {
            constexpr float smokeOutLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(20.0f);
            constexpr float maxVelocity = 60.0f;
            constexpr float minVelocity = 30.0f;
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), dis(gen), dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(75.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = smokeOutLifeTime,
                    .initialVelocity = glm::normalize(offset) * (minVelocity + maxVelocity) * 0.5f,
                    .velocitySpan = (maxVelocity - minVelocity) * 0.5f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 4),
                    .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
                    .texCoordSpan = glm::vec2(0.0f, 0.0f),
                    .startColor = glm::vec4(glm::vec3(dustColor), 0.8f),
                    // .endColor = dustColor,
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / smokeOutLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.6f * smokeOutLifeTime,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.8f * smokeOutLifeTime,
                });
            }
        }

        // Fire ball
        if (false)
        {
            constexpr float fireBallLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(40.0f);
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), dis(gen), dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(50.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = fireBallLifeTime,
                    .initialVelocity = glm::normalize(offset) * 60.0f,
                    .velocitySpan = 10.0f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 3),
                    .texSize = ToAtlasSize(glm::vec2(0.125f)),
                    .texCoordSpan = glm::vec2(0.0f, 8.0f),
                    .startColor = flameColor,
                    .endColor = flameColor,
                    .numFrames = 8,
                    .framesPerSecond = 8.0f / fireBallLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.32f,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.4f,
                });
            }
        }

        // Embers
        if (true)
        {
            constexpr glm::vec2 sizeMin = glm::vec2(0.5f);
            constexpr glm::vec2 sizeMax = glm::vec2(1.5f);
            constexpr uint32_t minParticles = 128 / 2;
            constexpr uint32_t maxParticles = 256 / 2;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (sizeMin + sizeMax) / 2.0f,
                .sizeSpan = (sizeMax - sizeMin) / 2.0f,
                .lifetime = 0.75f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 200.0f,
                .velocitySpan = 200.0f,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = flameColor,
                .numParticles = uint32_t(minParticles + (dis(gen) + 1.0f) / 2.0f * (maxParticles - minParticles) * scale),
                .opacityOut = 1.0f,
            });
        }

        // Smoke
        if (true)
        {
            constexpr float smokeLifeTime = 1.5f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position + glm::vec3(0.0f, 30.0f, 0.0f),
                .size = glm::vec2(200.0f * scale),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = smokeLifeTime,
                .initialVelocity = glm::vec3(0.0f, 60.0f, 0.0f),
                .velocitySpan = 20.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 2),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f),
                .numFrames = 8,
                .framesPerSecond = 64.0f / smokeLifeTime,
                .numParticles = 1,
                .scaleIn = 0.4f * smokeLifeTime,
                .opacityIn = 0.4f,
                .opacityOut = 0.8f * smokeLifeTime,
            });
        }

        // Dust
        if (true)
        {
            constexpr glm::vec2 dustSizeMin = glm::vec2(3.0f);
            constexpr glm::vec2 dustSizeMax = glm::vec2(5.0f);
            constexpr uint32_t dustMinParticles = 32;
            constexpr uint32_t dustMaxParticles = 128;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (dustSizeMin + dustSizeMax) / 2.0f,
                .sizeSpan = (dustSizeMax - dustSizeMin) / 2.0f,
                .lifetime = 2.0f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 400.0f,
                .velocitySpan = 300.0f,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 6 * 0.125f), 0),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(5.0f, 0.0f),
                .startColor = dustColor,
                .numParticles = uint32_t(dustMinParticles + (dis(gen) + 1.0f) / 2.0f * (dustMaxParticles - dustMinParticles) * scale),
                .opacityOut = 0.3f * 2.0f,
            });
        }
    }

    void SpawnIceExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position)
    {
        constexpr float scale = 1.0f;
        constexpr glm::vec4 dustColor = glm::vec4(2.0f, 2.0f, 2.0f, 1.0f);

        // Flare
        if (true)
        {
            constexpr float lifeTime = 0.05f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position - glm::vec3(0.0f, 20.0f, 0.0f),
                .size = glm::vec2(400.0f),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = lifeTime,
                .initialVelocity = glm::vec3(0.0f),
                .velocitySpan = 0.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = glm::vec4(c_plasmaColor, 1.0f),
                .numParticles = 1,
                .scaleIn = lifeTime,
                .opacityIn = 0.0f,
                .opacityOut = 0.5f * lifeTime,
            });
        }

        // Smoke out
        if (true)
        {
            constexpr float smokeOutLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(20.0f, 5.0f, 20.0f);
            constexpr float maxVelocity = 500.0f;
            constexpr float minVelocity = 250.0f;
            for (size_t i = 0; i < 12; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), glm::abs(dis(gen)), dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(100.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = smokeOutLifeTime,
                    .initialVelocity = glm::normalize(offset) * (minVelocity + maxVelocity) * 0.5f,
                    .velocitySpan = (maxVelocity - minVelocity) * 0.5f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 4),
                    .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
                    .texCoordSpan = glm::vec2(0.0f, 0.0f),
                    .startColor = glm::vec4(glm::vec3(dustColor), 0.65f),
                    .endColor = glm::vec4(glm::vec3(dustColor), 0.65f),
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / smokeOutLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.8f * smokeOutLifeTime,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.8f * smokeOutLifeTime,
                });
            }
        }

        // Dust
        if (true)
        {
            constexpr glm::vec2 dustSizeMin = glm::vec2(3.0f);
            constexpr glm::vec2 dustSizeMax = glm::vec2(5.0f);
            constexpr uint32_t dustMinParticles = 32;
            constexpr uint32_t dustMaxParticles = 96;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (dustSizeMin + dustSizeMax) / 2.0f,
                .sizeSpan = (dustSizeMax - dustSizeMin) / 2.0f,
                .lifetime = 2.0f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 400.0f,
                .velocitySpan = 300.0f,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 6 * 0.125f), 0),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(5.0f, 0.0f),
                .startColor = glm::vec4(c_iceColor, 1.0f),
                .endColor = dustColor,
                .numParticles = uint32_t(dustMinParticles + (dis(gen) + 1.0f) / 2.0f * (dustMaxParticles - dustMinParticles) * scale),
                .opacityOut = 0.3f * 2.0f,
            });
        }
    }

    void SpawnFireExplosionEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position)
    {
        constexpr float scale = 2.0f;
        constexpr glm::vec4 flameColor = glm::vec4(c_fireColor * 2.0f, 1.0f);
        constexpr glm::vec4 dustColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        // Flare
        if constexpr (true)
        {
            constexpr float lifeTime = 0.05f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position - glm::vec3(0.0f, 20.0f, 0.0f),
                .size = glm::vec2(400.0f),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = lifeTime,
                .initialVelocity = glm::vec3(0.0f),
                .velocitySpan = 0.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = flameColor,
                .numParticles = 1,
                .scaleIn = lifeTime,
                .opacityIn = 0.0f,
                .opacityOut = 0.5f * lifeTime,
            });
        }

        // Fire out
        if constexpr (true)
        {
            constexpr float smokeOutLifeTime = 0.8f;
            constexpr glm::vec3 maxOffset = glm::vec3(30.0f);
            constexpr float maxVelocity = 300.0f;
            constexpr float minVelocity = 150.0f;
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), glm::abs(dis(gen)) * 0.5f, dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(50.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = smokeOutLifeTime,
                    .initialVelocity = glm::normalize(offset) * (minVelocity + maxVelocity) * 0.5f,
                    .velocitySpan = (maxVelocity - minVelocity) * 0.5f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 5),
                    .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
                    .texCoordSpan = glm::vec2(0.0f, 0.0f),
                    .startColor = flameColor,
                    // .endColor = dustColor,
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / smokeOutLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.6f * smokeOutLifeTime,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.8f * smokeOutLifeTime,
                });
            }
        }

        // Smoke out
        if constexpr (true)
        {
            constexpr float smokeOutLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(20.0f, 5.0f, 20.0f);
            constexpr float maxVelocity = 500.0f;
            constexpr float minVelocity = 250.0f;
            for (size_t i = 0; i < 12; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), glm::abs(dis(gen)), dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(100.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = smokeOutLifeTime,
                    .initialVelocity = glm::normalize(offset) * (minVelocity + maxVelocity) * 0.5f,
                    .velocitySpan = (maxVelocity - minVelocity) * 0.5f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 4),
                    .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
                    .texCoordSpan = glm::vec2(0.0f, 0.0f),
                    .startColor = glm::vec4(glm::vec3(dustColor), 0.65f),
                    // .endColor = dustColor,
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / smokeOutLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.8f * smokeOutLifeTime,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.8f * smokeOutLifeTime,
                });
            }
        }

        // Fire ball
        if constexpr (true)
        {
            constexpr float fireBallLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(20.0f);
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), glm::abs(dis(gen)) * 2.0f, dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(50.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = fireBallLifeTime,
                    .initialVelocity = glm::normalize(offset) * 120.0f,
                    .velocitySpan = 10.0f,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 5),
                    .texSize = ToAtlasSize(glm::vec2(0.125f)),
                    .startColor = flameColor,
                    .endColor = flameColor,
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / fireBallLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.32f,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.4f,
                });
            }
        }

        // Embers
        if constexpr (true)
        {
            constexpr glm::vec2 sizeMin = glm::vec2(0.9f);
            constexpr glm::vec2 sizeMax = glm::vec2(1.8f);
            constexpr uint32_t minParticles = 128;
            constexpr uint32_t maxParticles = 256;
            constexpr float lifeTime = 0.6f;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (sizeMin + sizeMax) / 2.0f,
                .sizeSpan = (sizeMax - sizeMin) / 2.0f,
                .lifetime = 0.75f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 200.0f,
                .velocitySpan = 200.0f,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = flameColor,
                .endColor = dustColor,
                .numParticles = uint32_t(minParticles + (dis(gen) + 1.0f) / 2.0f * (maxParticles - minParticles) * scale),
                .opacityOut = lifeTime * 0.75f,
            });
        }

        // Smoke
        if constexpr (true)
        {
            constexpr float smokeLifeTime = 1.5f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position + glm::vec3(0.0f, 30.0f, 0.0f),
                .size = glm::vec2(200.0f * scale),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = smokeLifeTime,
                .initialVelocity = glm::vec3(0.0f, 60.0f, 0.0f),
                .velocitySpan = 20.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 2),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.2f),
                .numFrames = 8,
                .framesPerSecond = 64.0f / smokeLifeTime,
                .numParticles = 1,
                .scaleIn = 0.4f * smokeLifeTime,
                .opacityIn = 0.4f,
                .opacityOut = 0.8f * smokeLifeTime,
            });
        }

        // Dust
        if constexpr (false)
        {
            constexpr glm::vec2 dustSizeMin = glm::vec2(3.0f);
            constexpr glm::vec2 dustSizeMax = glm::vec2(5.0f);
            constexpr uint32_t dustMinParticles = 32;
            constexpr uint32_t dustMaxParticles = 128;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (dustSizeMin + dustSizeMax) / 2.0f,
                .sizeSpan = (dustSizeMax - dustSizeMin) / 2.0f,
                .lifetime = 2.0f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 400.0f,
                .velocitySpan = 300.0f,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 6 * 0.125f), 0),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(5.0f, 0.0f),
                .startColor = dustColor,
                .numParticles = uint32_t(dustMinParticles + (dis(gen) + 1.0f) / 2.0f * (dustMaxParticles - dustMinParticles) * scale),
                .opacityOut = 0.3f * 2.0f,
            });
        }
    }

    void SpawnSmokeTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale)
    {
        // Smoke out
        constexpr float smokeOutLifeTime = 1.0f;
        constexpr float minVelocity = 10.0f;
        constexpr float maxVelocity = 40.0f;
        glm::vec3 direction = glm::vec3(dis(gen), dis(gen), dis(gen));
        SpawnSmoke(particleJobs, position, direction, minVelocity, maxVelocity, smokeOutLifeTime, glm::vec4(0.5f), glm::vec4(0.5f), scale);
    }

    void SpawnIceSmokeTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale)
    {
        // Smoke out
        constexpr float smokeOutLifeTime = 1.0f;
        constexpr float minVelocity = 10.0f;
        constexpr float maxVelocity = 40.0f;
        glm::vec3 direction = glm::vec3(dis(gen), dis(gen), dis(gen));
        SpawnSmoke(particleJobs, position, direction, minVelocity, maxVelocity, smokeOutLifeTime, glm::vec4(glm::vec3(1.0f), 0.5f), glm::vec4(glm::vec3(1.0f), 0.5f), scale);
    }

    void SpawnFireTrail(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale)
    {
        // Smoke out
        constexpr float smokeOutLifeTime = 0.1f;
        constexpr float minVelocity = 10.0f;
        constexpr float maxVelocity = 40.0f;
        glm::vec3 direction = glm::vec3(dis(gen), dis(gen), dis(gen));
        SpawnFire(particleJobs, position, direction, minVelocity, maxVelocity, 0.0f, 10.0f, 50.0f, smokeOutLifeTime, scale);
    }

    void SpawnSmoke(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, const glm::vec4 &startColor, const glm::vec4 &endColor, float scale)
    {
        particleJobs.push_back(ParticleEmitJob{
            .position = position,
            .size = glm::vec2(50.0f * scale),
            .rotation = glm::radians(180.0f) * dis(gen),
            .sizeSpan = glm::vec2(0.0f),
            .lifetime = lifeTime,
            .initialVelocity = glm::normalize(direction) * (minVelocity + maxVelocity) * 0.5f * scale,
            .velocitySpan = (maxVelocity - minVelocity) * 0.5f * scale,
            .gravityFactor = 0.0f,
            // .phiSpan = glm::radians(60.0f),
            // .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 4),
            .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = startColor,
            .endColor = endColor,
            .numFrames = 8,
            .framesPerSecond = 64.0f / lifeTime,
            .numParticles = 1,
            .scaleIn = 0.6f * lifeTime,
            // .opacityIn = 0.0f * fireBallLifeTime,
            .opacityOut = 0.8f * lifeTime,
        });
    }

    void SpawnFire(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale)
    {
        // Fire ball
        constexpr float fireLifeTime = 1.0f;
        constexpr float minVelocity = 0.0f;
        constexpr float maxVelocity = 10.0f;
        constexpr float maxSize = 50.0f;
        constexpr float minSize = 10.0f;

        glm::vec3 direction = glm::vec3(dis(gen), 2.0f * glm::abs(dis(gen)), dis(gen));
        SpawnFire(particleJobs, position, direction, minVelocity, maxVelocity, 0.0f, minSize, maxSize, fireLifeTime, scale);
    }

    void SpawnFire(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float deceleration, float minSize, float maxSize, float lifeTime, float scale)
    {
        particleJobs.push_back(ParticleEmitJob{
            .position = position,
            .size = glm::vec2((minSize + maxSize) / 2.0f * scale),
            .rotation = glm::radians(180.0f) * dis(gen),
            .sizeSpan = glm::vec2((maxSize - minSize) / 2.0f * scale),
            .lifetime = lifeTime,
            .initialVelocity = glm::normalize(direction) * (minVelocity + maxVelocity) * 0.5f * scale,
            .velocitySpan = (maxVelocity - minVelocity) * 0.5f * scale,
            .acceleration = -glm::normalize(direction) * deceleration * scale,
            .gravityFactor = 0.0f,
            // .phiSpan = glm::radians(60.0f),
            // .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 5),
            .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = glm::vec4(c_fireColor * 5.0f, 1.0f),
            .numFrames = 8,
            .framesPerSecond = 64.0f / lifeTime,
            .numParticles = 1,
            .scaleIn = 0.05f * lifeTime,
            // .opacityIn = 0.0f * fireBallLifeTime,
            .opacityOut = 0.8f * lifeTime,
        });
    }

    void SpawnGroundImpact(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, float scale)
    {
        constexpr glm::vec4 dustColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        // Smoke out
        if (true)
        {
            constexpr float smokeOutLifeTime = 0.6f;
            constexpr glm::vec3 maxOffset = glm::vec3(20.0f, 5.0f, 20.0f);
            constexpr float maxVelocity = 500.0f;
            constexpr float minVelocity = 250.0f;
            for (size_t i = 0; i < 12; i++)
            {
                glm::vec3 offset = glm::vec3(dis(gen), glm::abs(dis(gen)), dis(gen)) * maxOffset;
                particleJobs.push_back(ParticleEmitJob{
                    .position = position + offset,
                    .size = glm::vec2(100.0f * scale),
                    .rotation = glm::radians(180.0f) * dis(gen),
                    .sizeSpan = glm::vec2(0.0f),
                    .lifetime = smokeOutLifeTime,
                    .initialVelocity = glm::normalize(offset) * (minVelocity + maxVelocity) * 0.5f * scale,
                    .velocitySpan = (maxVelocity - minVelocity) * 0.5f * scale,
                    .gravityFactor = 0.0f,
                    // .phiSpan = glm::radians(60.0f),
                    // .thetaSpan = glm::radians(180.0f),
                    .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 4),
                    .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
                    .texCoordSpan = glm::vec2(0.0f, 0.0f),
                    .startColor = glm::vec4(glm::vec3(dustColor), 0.65f),
                    // .endColor = dustColor,
                    .numFrames = 8,
                    .framesPerSecond = 64.0f / smokeOutLifeTime,
                    .numParticles = 1,
                    .scaleIn = 0.8f * smokeOutLifeTime,
                    // .opacityIn = 0.0f * fireBallLifeTime,
                    .opacityOut = 0.8f * smokeOutLifeTime,
                });
            }
        }

        // Dust
        if (true)
        {
            constexpr glm::vec2 dustSizeMin = glm::vec2(3.0f);
            constexpr glm::vec2 dustSizeMax = glm::vec2(5.0f);
            constexpr uint32_t dustMinParticles = 32;
            constexpr uint32_t dustMaxParticles = 96;

            particleJobs.push_back(ParticleEmitJob{
                .position = position,
                .size = (dustSizeMin + dustSizeMax) / 2.0f,
                .sizeSpan = (dustSizeMax - dustSizeMin) / 2.0f,
                .lifetime = 2.0f,
                .initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f) * 400.0f * scale,
                .velocitySpan = 300.0f * scale,
                .gravityFactor = 1.0f,
                .phiSpan = glm::radians(90.0f),
                .thetaSpan = glm::radians(180.0f),
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 6 * 0.125f), 0),
                .texSize = ToAtlasSize(glm::vec2(0.125f)),
                .texCoordSpan = glm::vec2(5.0f, 0.0f),
                .startColor = dustColor,
                .numParticles = uint32_t(dustMinParticles + (dis(gen) + 1.0f) / 2.0f * (dustMaxParticles - dustMinParticles) * scale),
                .opacityOut = 0.3f * 2.0f,
            });
        }
    }

    void SpawnImpactEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, const glm::vec4 &color)
    {
        constexpr float scale = 1.0f;
        constexpr float lifeTime = 0.4f;

        constexpr uint32_t minParticles = 3;
        constexpr uint32_t maxParticles = 6;

        const glm::vec2 sizeMin = glm::vec2(1.0f) * scale;
        const glm::vec2 sizeMax = glm::vec2(2.0f) * scale;

        particleJobs.push_back(ParticleEmitJob{
            .position = position,
            .size = (sizeMin + sizeMax) / 2.0f,
            .sizeSpan = (sizeMax - sizeMin) / 2.0f,
            .lifetime = lifeTime,
            .initialVelocity = direction * 150.0f,
            .velocitySpan = 50.0f,
            .gravityFactor = 0.5f,
            .phiSpan = glm::radians(90.0f),
            .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
            .texSize = ToAtlasSize(glm::vec2(0.5f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = color,
            .numParticles = uint32_t(minParticles + (dis(gen) + 1.0f) / 2.0f * (maxParticles - minParticles)),
            .scaleOut = lifeTime,
            .opacityOut = lifeTime,
        });

        if (true)
        {
            constexpr float lifeTime = 0.02f;
            particleJobs.push_back(ParticleEmitJob{
                .position = position - glm::vec3(0.0f, 20.0f, 0.0f),
                .size = glm::vec2(80.0f * scale),
                .rotation = glm::radians(180.0f) * dis(gen),
                .sizeSpan = glm::vec2(0.0f),
                .lifetime = lifeTime,
                .initialVelocity = glm::vec3(0.0f),
                .velocitySpan = 0.0f,
                .gravityFactor = 0.0f,
                .phiSpan = 0.0f,
                .thetaSpan = 0.0f,
                .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 1),
                .texSize = ToAtlasSize(glm::vec2(0.5f)),
                .texCoordSpan = glm::vec2(0.0f, 0.0f),
                .startColor = color * glm::vec4(glm::vec3(1.0f), 1.0f),
                .endColor = color * glm::vec4(glm::vec3(0.1f), 0.0f),
                .numParticles = 1,
                .scaleIn = lifeTime,
                .opacityIn = 0.0f,
                .opacityOut = 0.5f * lifeTime,
            });
        }
    }

    void SpawnBloodEffect(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction)
    {
        constexpr glm::vec4 color = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);

        constexpr glm::vec2 sizeMin = glm::vec2(2.0f);
        constexpr glm::vec2 sizeMax = glm::vec2(4.0f);
        constexpr uint32_t minParticles = 8;
        constexpr uint32_t maxParticles = 32;

        particleJobs.push_back(ParticleEmitJob{
            .position = position + glm::vec3(dis(rd) * 10.0f, 44.0f, dis(rd) * 10.0f),
            .size = (sizeMin + sizeMax) / 4.0f,
            .sizeSpan = (sizeMax - sizeMin) / 4.0f,
            .lifetime = 2.0f,
            .initialVelocity = direction * 150.0f * (dis(gen) > 0.0f ? 1.0f : -1.0f),
            .velocitySpan = 150.0f,
            .gravityFactor = 1.0f,
            .phiSpan = glm::mix(glm::radians(30.0f), glm::radians(90.0f), (dis(gen) + 1.0f) / 2.0f),
            .thetaSpan = glm::mix(glm::radians(30.0f), glm::radians(90.0f), (dis(gen) + 1.0f) / 2.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
            .texSize = ToAtlasSize(glm::vec2(0.5f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = color,
            .numParticles = uint32_t(minParticles + (dis(gen) + 1.0f) / 2.0f * (maxParticles - minParticles)),
        });

        constexpr float lifeTime = 0.4f;

        particleJobs.push_back(ParticleEmitJob{
            .position = position,
            .size = glm::vec2(60.0f),
            .sizeSpan = glm::vec2(0.0f),
            .lifetime = lifeTime,
            .initialVelocity = glm::vec4(0.0f),
            .velocitySpan = 0.0f,
            .gravityFactor = 0.0f,
            .phiSpan = 0.0f,
            .thetaSpan = 0.0f,
            .texCoord = ToAtlasPos(glm::vec2(0.0f, 0.0f), 0),
            .texSize = ToAtlasSize(glm::vec2(0.125f)),
            .texCoordSpan = glm::vec2(0.0f, 8.0f),
            .startColor = color,
            .numFrames = 8,
            .framesPerSecond = 8.0f / lifeTime,
            .numParticles = 1,
            .scaleIn = 0.6f * lifeTime,
            .opacityIn = 0.0f,
            .opacityOut = 0.4f * lifeTime,
        });

        constexpr float groundSplashLifeTime = 10.0f;

        const glm::vec3 randomOffset = glm::vec3(dis(rd) * 50.0f, 0.0f, dis(rd) * 50.0f);
        float angle = glm::atan(randomOffset.x, randomOffset.z) + glm::radians(180.0f);

        particleJobs.push_back(ParticleEmitJob{
            .position = glm::vec3(position.x, 0.0f, position.z) + randomOffset,
            .size = glm::vec2(40.0f),
            .rotation = angle,
            .sizeSpan = glm::vec2(0.0f),
            .lifetime = groundSplashLifeTime,
            .initialVelocity = glm::vec4(0.0f),
            .velocitySpan = 0.0f,
            .gravityFactor = 0.0f,
            .phiSpan = 0.0f,
            .thetaSpan = 0.0f,
            .texCoord = ToAtlasPos(glm::floor(glm::vec2(4.0f + (dis(gen) + 1.0f) / 2.0f * 4.0f, (dis(gen) + 1.0f) / 2.0f * 8.0f)) * (1.0f / 8.0f), 0),
            .texSize = ToAtlasSize(glm::vec2(1.0f / 8.0f)),
            .startColor = color,
            .opacityOut = 0.5f,
        });
    }

    ParticleEmitJob SpawnPickupParticles(const glm::vec3 &position, const glm::vec4 &color, float scale, uint32_t minParticles, uint32_t maxParticles)
    {
        // Purple
        constexpr float lifeTime = 0.75f;

        constexpr glm::vec2 sizeMin = glm::vec2(0.5f);
        constexpr glm::vec2 sizeMax = glm::vec2(1.0f);

        return ParticleEmitJob{
            .position = position + glm::vec3(dis(rd) * 10.0f, 44.0f, dis(rd) * 10.0f),
            .size = (sizeMin + sizeMax) / 2.0f,
            .sizeSpan = (sizeMax - sizeMin) / 2.0f,
            .lifetime = lifeTime,
            .initialVelocity = glm::vec4(0.0f, 150.0f, 0.0f, 0.0f),
            .velocitySpan = 150.0f,
            .gravityFactor = 0.5f,
            .phiSpan = glm::radians(90.0f),
            .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
            .texSize = ToAtlasSize(glm::vec2(0.5f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = color,
            .numParticles = uint32_t(minParticles + (dis(gen) + 1.0f) / 2.0f * (maxParticles - minParticles)),
            .scaleOut = lifeTime,
            .opacityOut = lifeTime,
        };
    }

    void SpawnPickupParticles(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec4 &color)
    {
        particleJobs.push_back(SpawnPickupParticles(position, color));
    }

    void SpawnSpark(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec4 &startColor, const glm::vec4 &endColor)
    {
        // Smoke out
        constexpr float smokeOutLifeTime = 1.2f;
        constexpr float minVelocity = 40.0f;
        constexpr float maxVelocity = 100.0f;

        constexpr glm::vec2 sizeMin = glm::vec2(0.5f);
        constexpr glm::vec2 sizeMax = glm::vec2(1.0f);

        glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
        particleJobs.push_back(ParticleEmitJob{
            .position = position + glm::vec3(dis(gen), dis(gen), dis(gen)) * 10.0f,
            .size = (sizeMin + sizeMax) / 2.0f,
            .rotation = glm::radians(180.0f) * dis(gen),
            .sizeSpan = (sizeMax - sizeMin) / 2.0f,
            .lifetime = smokeOutLifeTime,
            .initialVelocity = glm::normalize(direction) * (minVelocity + maxVelocity) * 0.5f,
            .velocitySpan = (maxVelocity - minVelocity) * 0.5f,
            .gravityFactor = 0.0f,
            .phiSpan = glm::radians(60.0f),
            .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
            .texSize = ToAtlasSize(glm::vec2(0.5f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = startColor,
            .endColor = endColor,
            .numParticles = 1,
            .scaleIn = 0.3f * smokeOutLifeTime,
            // .opacityIn = 0.0f * fireBallLifeTime,
            .opacityOut = 0.5f * smokeOutLifeTime,
        });
    }

    void SpawnEmbers(std::vector<ParticleEmitJob> &particleJobs, const glm::vec3 &position, const glm::vec3 &direction, float minVelocity, float maxVelocity, float lifeTime, float scale)
    {
        constexpr glm::vec2 sizeMin = glm::vec2(0.5f);
        constexpr glm::vec2 sizeMax = glm::vec2(1.0f);
        constexpr uint32_t minParticles = 4;
        constexpr uint32_t maxParticles = 8;

        particleJobs.push_back(ParticleEmitJob{
            .position = position,
            .size = (sizeMin + sizeMax) / 2.0f,
            .sizeSpan = (sizeMax - sizeMin) / 2.0f,
            .lifetime = lifeTime,
            .initialVelocity = glm::normalize(direction) * (minVelocity + maxVelocity) * 0.5f * scale,
            .velocitySpan = (maxVelocity - minVelocity) * 0.5f * scale,
            .gravityFactor = 0.0f,
            .phiSpan = glm::radians(90.0f),
            .thetaSpan = glm::radians(180.0f),
            .texCoord = ToAtlasPos(glm::vec2(0.5f, 0.0f), 1),
            .texSize = ToAtlasSize(glm::vec2(0.5f)),
            .texCoordSpan = glm::vec2(0.0f, 0.0f),
            .startColor = glm::vec4(c_fireColor * 15.0f, 1.0f),
            .endColor = glm::vec4(0.1f * c_fireColor, 1.0f),
            .numParticles = 1,
            .scaleOut = lifeTime,
            .opacityOut = lifeTime,
        });
    }
}