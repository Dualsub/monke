#include "Audio/AudioPlayer.h"

#include "Core/Core.h"

#include <iostream>
#include <random>

namespace ACS
{
    static std::random_device s_randomDevice;
    static std::mt19937 s_randomEngine(s_randomDevice());
    static std::uniform_real_distribution<float> s_randomDistribution(0.0f, 1.0f);

    bool AudioPlayer::Initialize()
    {
        // Open device
        m_device = alcOpenDevice(nullptr);

        if (!m_device)
        {
            return false;
        }

        m_context = alcCreateContext(m_device, nullptr);
        if (!m_context)
        {
            return false;
        }

        alcMakeContextCurrent(m_context);

        alListenerf(AL_GAIN, m_globalVolume);

        return true;
    }

    SoundHandle AudioPlayer::LoadSound(const std::string &filename)
    {
        Sound sound = Sound::Create(filename);
        SoundHandle handle = GetHandle(filename);
        m_sounds.insert({handle, sound});
        return handle;
    }

    void AudioPlayer::PlaySoundAt(SoundHandle handle, const glm::vec3 &position, float volume, float pitch, bool loop)
    {
        auto sound = m_sounds.find(handle);

        if (sound != m_sounds.end())
        {
            sound->second.PlayAt(position, volume, pitch * m_globalPitch, loop);
        }
        else
        {
            std::cerr << "Sound not found: " << handle << std::endl;
        }
    }

    void AudioPlayer::PlayRandomSoundAt(const std::vector<SoundHandle> &sounds, const glm::vec3 &position, float volume, float pitch, bool loop)
    {
        if (sounds.empty())
        {
            return;
        }

        uint32_t index = static_cast<uint32_t>(s_randomDistribution(s_randomEngine) * sounds.size());
        PlaySoundAt(sounds[index], position, volume, pitch, loop);
    }

    void AudioPlayer::PlaySound(SoundHandle handle, float volume, float pitch, bool loop)
    {
        auto sound = m_sounds.find(handle);

        if (sound != m_sounds.end())
        {
            sound->second.Play(volume, pitch * m_globalPitch, loop);
        }
    }

    void AudioPlayer::PlayRandomSound(const std::vector<SoundHandle> &sounds, float volume, float pitch, bool loop)
    {
        if (sounds.empty())
        {
            return;
        }

        uint32_t index = static_cast<uint32_t>(s_randomDistribution(s_randomEngine) * sounds.size());
        PlaySound(sounds[index], volume, pitch * m_globalPitch, loop);
    }

    void AudioPlayer::StopSound(SoundHandle handle)
    {
        auto sound = m_sounds.find(handle);

        if (sound != m_sounds.end())
        {
            sound->second.Stop();
        }
    }

    void AudioPlayer::SetSoundPosition(SoundHandle handle, const glm::vec3 &position)
    {
        auto sound = m_sounds.find(handle);

        if (sound != m_sounds.end())
        {
            sound->second.SetPosition(position);
        }
    }

    void AudioPlayer::PauseAllSounds()
    {
        for (auto &sound : m_sounds)
        {
            sound.second.Pause();
        }
    }

    void AudioPlayer::ResumeAllSounds()
    {
        for (auto &sound : m_sounds)
        {
            sound.second.Resume();
        }
    }

    void AudioPlayer::Shutdown()
    {
        for (auto &sound : m_sounds)
        {
            sound.second.Destroy();
        }

        m_sounds.clear();

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
    }

    void AudioPlayer::SetGlobalPitch(float pitch)
    {
        const float currentGlobalPitch = m_globalPitch;
        // Find all playing sounds and adjust pitch
        for (auto &sound : m_sounds)
        {
            float currentSoundPitch = sound.second.GetPitch();
            sound.second.SetPitch(currentSoundPitch / currentGlobalPitch * pitch);
        }

        m_globalPitch = pitch;
    }

    void AudioPlayer::SetListenerVolume(float volume)
    {
        m_globalVolume = volume;
        alListenerf(AL_GAIN, volume);
        auto error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << "Error setting global pitch:" << error << std::endl;
            abort();
        }
    }

    void AudioPlayer::SetListenerPosition(const glm::vec3 &position)
    {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
    }
}