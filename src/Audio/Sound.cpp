#include "Audio/Sound.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

namespace mk
{
    struct WavHeader
    {
        char riff[4]; // "RIFF"
        uint32_t size;
        char wave[4]; // "WAVE"
    };

    struct ChunkHeader
    {
        char id[4];    // Chunk ID: "fmt " or "data"
        uint32_t size; // Chunk size
    };

    struct FmtChunk
    {
        uint16_t format;        // Audio format
        uint16_t channels;      // Number of channels
        uint32_t sampleRate;    // Sample rate
        uint32_t byteRate;      // Byte rate
        uint16_t blockAlign;    // Block align
        uint16_t bitsPerSample; // Bits per sample
    };

    void ConvertEndianness16Bit(std::vector<char> &data)
    {
        for (size_t i = 0; i < data.size(); i += 2)
        {
            std::swap(data[i], data[i + 1]);
        }
    }

    Sound Sound::Create(const std::string &path)
    {
        WavHeader header;
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Failed to open .wav-file: " << path << std::endl;
            abort();
        }

        WavHeader wavHeader;
        file.read(reinterpret_cast<char *>(&wavHeader), sizeof(WavHeader));
        if (std::strncmp(wavHeader.riff, "RIFF", 4) || std::strncmp(wavHeader.wave, "WAVE", 4))
        {
            std::cerr << "Invalid .wav-file: " << path << std::endl;
            abort();
        }

        ChunkHeader chunkHeader;
        FmtChunk fmtChunk;
        std::vector<char> audioData;
        bool fmtFound = false, dataFound = false;
        while (file.read(reinterpret_cast<char *>(&chunkHeader), sizeof(ChunkHeader)))
        {
            if (std::strncmp(chunkHeader.id, "fmt ", 4) == 0)
            {
                if (chunkHeader.size > sizeof(FmtChunk) || chunkHeader.size < 16)
                {
                    std::cerr << "Invalid fmt chunk size: " << chunkHeader.size << std::endl;
                    abort();
                }
                file.read(reinterpret_cast<char *>(&fmtChunk), sizeof(FmtChunk));
                fmtFound = true;
                if (chunkHeader.size > 16)
                { // Skip extra fmt subchunk data if present
                    file.seekg(chunkHeader.size - 16, std::ios_base::cur);
                }
            }
            else if (std::strncmp(chunkHeader.id, "data", 4) == 0)
            {
                audioData.resize(chunkHeader.size);
                file.read(audioData.data(), chunkHeader.size);
                dataFound = true;
                break; // Assuming data chunk is the last we need to read
            }
            else
            {
                // Skip unknown chunk
                file.seekg(chunkHeader.size, std::ios_base::cur);
            }
        }

        if (!fmtFound || !dataFound)
        {
            std::cerr << "Failed to find necessary chunk (fmt or data)." << std::endl;
            abort();
        }

        // Determine the OpenAL format
        ALenum format = AL_NONE;
        if (fmtChunk.bitsPerSample == 16)
        {
            format = (fmtChunk.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        }
        else if (fmtChunk.bitsPerSample == 8)
        {
            format = (fmtChunk.channels == 2) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        }
        else
        {
            std::cerr << "Unsupported bit depth: " << fmtChunk.bitsPerSample << std::endl;
            abort();
        }

        uint32_t bufId = 0;
        alGenBuffers(1, &bufId);
        alBufferData(bufId, format, audioData.data(), audioData.size(), fmtChunk.sampleRate);

        uint32_t srcId = 0;
        alGenSources(1, &srcId);
        alSourceQueueBuffers(srcId, 1, &bufId);
        alSourcef(srcId, AL_GAIN, 1.0f);
        alSourcef(srcId, AL_PITCH, 1.0f);
        alSourcei(srcId, AL_LOOPING, AL_FALSE);

        return Sound(bufId, srcId);
    }

    std::unique_ptr<Sound> Sound::CreatePtr(const std::string &path)
    {
        return std::make_unique<Sound>(Create(path));
    }

    void Sound::Destroy()
    {
        alDeleteSources(1, &m_sourceId);
        alDeleteBuffers(1, &m_bufferId);
    }

    void Sound::Play(float volume, float pitch, bool loop)
    {
        alSourcef(m_sourceId, AL_PITCH, pitch);
        alSourcef(m_sourceId, AL_GAIN, volume);
        alSourcei(m_sourceId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(m_sourceId);

        m_volume = volume;
        m_pitch = pitch;
    }

    void Sound::PlayAt(const glm::vec3 &position, float volume, float pitch, bool loop)
    {
        alSourcef(m_sourceId, AL_PITCH, pitch);
        alSourcef(m_sourceId, AL_GAIN, volume);
        alSource3f(m_sourceId, AL_POSITION, position.x, position.y, position.z);
        alSourcei(m_sourceId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(m_sourceId);

        m_volume = volume;
        m_pitch = pitch;
    }

    void Sound::Stop()
    {
        alSourceStop(m_sourceId);
    }

    void Sound::Pause()
    {
        alSourcePause(m_sourceId);
    }

    void Sound::Resume()
    {
        ALint state;
        alGetSourcei(m_sourceId, AL_SOURCE_STATE, &state);
        if (state == AL_PAUSED)
        {
            alSourcePlay(m_sourceId);
        }
    }

    void Sound::SetPosition(const glm::vec3 &position)
    {
        alSource3f(m_sourceId, AL_POSITION, position.x, position.y, position.z);
    }

    void Sound::SetVolume(float volume)
    {
        alSourcef(m_sourceId, AL_GAIN, volume);
        m_volume = volume;
    }

    void Sound::SetPitch(float pitch)
    {
        alSourcef(m_sourceId, AL_PITCH, pitch);
        m_pitch = pitch;
    }
}