#pragma once

// Undef play sound macro
#ifdef PlaySound
#undef PlaySound
#endif

#include "Audio/Sound.h"

#include "al.h"
#include "alc.h"
#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>
#include <iostream>

namespace mk
{
    using SoundHandle = uint32_t;

    class AudioPlayer
    {
    private:
        ALCdevice *m_device = nullptr;
        ALCcontext *m_context = nullptr;

        SoundHandle m_handleCounter = 0;
        std::unordered_map<SoundHandle, Sound> m_sounds;

        float m_globalVolume = 1.0f;
        float m_globalPitch = 1.0f;

    public:
        bool Initialize();
        void Shutdown();

        SoundHandle LoadSound(const std::string &filename);
        void PlaySoundAt(SoundHandle handle, const glm::vec3 &position, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        void PlayRandomSoundAt(const std::vector<SoundHandle> &sounds, const glm::vec3 &position, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        void PlaySound(SoundHandle handle, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        void PlayRandomSound(const std::vector<SoundHandle> &sounds, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        void StopSound(SoundHandle handle);
        void SetSoundPosition(SoundHandle handle, const glm::vec3 &position);

        void PauseAllSounds();
        void ResumeAllSounds();

        void SetGlobalPitch(float pitch);
        void SetListenerVolume(float volume);
        void SetListenerPosition(const glm::vec3 &position);

        inline float GetGlobalPitch() const { return m_globalPitch; }
    };
}