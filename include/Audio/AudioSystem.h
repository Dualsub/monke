#pragma once

#include "Core/EnumArray.h"

#include <glm/glm.hpp>
#include "fmod_studio.hpp"

#include <string>
#include <map>

// Undefine Windows API macros
#ifdef CreateEvent
#undef CreateEvent
#endif

namespace mk
{
    enum class BankType
    {
        Master,
        Strings,

        Count,
        None,
    };

    using EventHandle = uint32_t;

    class AudioSystem
    {
    private:
        inline static constexpr float c_unitConversion = 0.01f;
        inline static constexpr EventHandle c_invalidEventHandle = static_cast<EventHandle>(-1);

        inline static EventHandle s_eventHandle = 0;

        FMOD::Studio::System *m_system = nullptr;

        EnumArray<BankType, FMOD::Studio::Bank *> m_banks = EnumArray<BankType, FMOD::Studio::Bank *>(nullptr);
        std::map<EventHandle, FMOD::Studio::EventInstance *> m_events;

    public:
        AudioSystem() = default;
        ~AudioSystem() = default;

        bool Initialize();
        void Shutdown();

        void Update();

        void LoadBank(const std::string &path, BankType type);
        EventHandle CreateEvent(const std::string &eventPath);
        void PlayEvent(const std::string &eventPath);
        void PlayEvent(EventHandle event);
        void PlayEventAtPosition(const std::string &eventPath, const glm::vec3 &position, const glm::vec3 &velocity = glm::vec3(0.0f));
        void SetEventParameter(EventHandle event, const std::string &parameter, float value);
        void StopEvent(EventHandle event, bool allowFadeOut = true);
        void ReleaseEvent(EventHandle event);

        void SetListenerState(const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &velocity);
    };
}