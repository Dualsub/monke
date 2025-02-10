#pragma once

#ifndef AL_LIBTYPE_STATIC
#define AL_LIBTYPE_STATIC
#endif

#include "AL/al.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace mk
{
    class Sound
    {
    private:
        uint32_t m_bufferId = 0;
        uint32_t m_sourceId = 0;

        float m_pitch = 1.0f;
        float m_volume = 1.0f;

    public:
        Sound() = default;
        Sound(uint32_t bufferId, uint32_t sourceId)
            : m_bufferId(bufferId), m_sourceId(sourceId) {}
        ~Sound() = default;

        void Play(float volume, float pitch, bool loop = false);
        void PlayAt(const glm::vec3 &position, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        void Stop();

        void Pause();
        void Resume();

        void SetPosition(const glm::vec3 &position);
        void SetVolume(float volume);
        void SetPitch(float pitch);
        float GetVolume() const { return m_volume; }
        float GetPitch() const { return m_pitch; }

        static Sound Create(const std::string &path);
        static std::unique_ptr<Sound> CreatePtr(const std::string &path);

        void Destroy();
    };
}