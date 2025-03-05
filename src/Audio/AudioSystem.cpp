#include "Audio/AudioSystem.h"

#include <glm/gtc/quaternion.hpp>

#include <iostream>

namespace mk
{
    bool AudioSystem::Initialize()
    {
        FMOD_RESULT result = FMOD::Studio::System::create(&m_system);
        if (result != FMOD_OK)
        {
            return false;
        }

        result = m_system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK)
        {
            return false;
        }

        return true;
    }

    void AudioSystem::LoadBank(const std::string &path, BankType type)
    {
        if (m_banks[type] != nullptr)
        {
            std::cerr << "Bank already loaded: " << path << std::endl;
            return;
        }

        FMOD::Studio::Bank *bank = nullptr;
        FMOD_RESULT result = m_system->loadBankFile(path.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to load bank: " << path << std::endl;
            return;
        }

        m_banks[type] = bank;
    }

    void AudioSystem::Update()
    {
        m_system->update();

        // Remove released events
        for (auto it = m_events.begin(); it != m_events.end();)
        {
            FMOD_STUDIO_PLAYBACK_STATE state;
            it->second->getPlaybackState(&state);
            if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
            {
                it->second->release();
                it = m_events.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void AudioSystem::Shutdown()
    {
        m_system->release();
    }

    void AudioSystem::PlayEvent(const std::string &eventPath)
    {
        FMOD::Studio::EventDescription *eventDescription = nullptr;
        FMOD_RESULT result = m_system->getEvent(eventPath.c_str(), &eventDescription);
        if (result != FMOD_OK || eventDescription == nullptr)
        {
            std::cerr << "Failed to get event description for event: " << eventPath << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = nullptr;
        result = eventDescription->createInstance(&eventInstance);
        if (result != FMOD_OK || eventInstance == nullptr)
        {
            std::cerr << "Failed to create event instance for event: " << eventPath << std::endl;
            return;
        }

        result = eventInstance->start();
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to start event: " << eventPath << std::endl;
        }

        eventInstance->release();
    }

    EventHandle AudioSystem::CreateEvent(const std::string &eventPath)
    {
        FMOD::Studio::EventDescription *eventDescription = nullptr;
        FMOD_RESULT result = m_system->getEvent(eventPath.c_str(), &eventDescription);
        if (result != FMOD_OK || eventDescription == nullptr)
        {
            std::cerr << "Failed to get event description for event: " << eventPath << std::endl;
            return c_invalidEventHandle;
        }

        FMOD::Studio::EventInstance *eventInstance = nullptr;
        result = eventDescription->createInstance(&eventInstance);
        if (result != FMOD_OK || eventInstance == nullptr)
        {
            std::cerr << "Failed to create event instance for event: " << eventPath << std::endl;
            return c_invalidEventHandle;
        }

        m_events[s_eventHandle] = eventInstance;
        return s_eventHandle++;
    }

    void AudioSystem::PlayEvent(EventHandle event)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        FMOD_RESULT result = eventInstanceIt->second->start();
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to start event: " << event << std::endl;
        }
    }

    void AudioSystem::PlayEventAtPosition(const std::string &eventPath, const glm::vec3 &position, const glm::vec3 &velocity)
    {
        FMOD::Studio::EventDescription *eventDescription = nullptr;
        FMOD_RESULT result = m_system->getEvent(eventPath.c_str(), &eventDescription);
        if (result != FMOD_OK || eventDescription == nullptr)
        {
            std::cerr << "Failed to get event description for event: " << eventPath << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = nullptr;
        result = eventDescription->createInstance(&eventInstance);
        if (result != FMOD_OK || eventInstance == nullptr)
        {
            std::cerr << "Failed to create event instance for event: " << eventPath << std::endl;
            return;
        }

        FMOD_3D_ATTRIBUTES attributes = {};
        glm::vec3 convertedPosition = position * c_unitConversion;
        glm::vec3 convertedVelocity = velocity * c_unitConversion;
        attributes.position = {convertedPosition.x, convertedPosition.y, convertedPosition.z};
        attributes.velocity = {convertedVelocity.x, convertedVelocity.y, convertedVelocity.z};
        attributes.forward = {0.0f, 0.0f, -1.0f};
        attributes.up = {0.0f, 1.0f, 0.0f};

        result = eventInstance->set3DAttributes(&attributes);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to set 3D attributes for event: " << eventPath << std::endl;
        }

        result = eventInstance->start();
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to start event: " << eventPath << std::endl;
        }

        eventInstance->release();
    }

    void AudioSystem::PlayEventAtPosition(EventHandle event, const glm::vec3 &position, const glm::vec3 &velocity)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = eventInstanceIt->second;

        FMOD_3D_ATTRIBUTES attributes = {};
        glm::vec3 convertedPosition = position * c_unitConversion;
        attributes.position = {convertedPosition.x, convertedPosition.y, convertedPosition.z};

        glm::vec3 convertedVelocity = velocity * c_unitConversion;
        attributes.velocity = {convertedVelocity.x, convertedVelocity.y, convertedVelocity.z};

        attributes.forward = {0.0f, 0.0f, -1.0f};
        attributes.up = {0.0f, 1.0f, 0.0f};

        FMOD_RESULT result = eventInstance->set3DAttributes(&attributes);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to set 3D attributes for event: " << event << std::endl;
        }

        result = eventInstance->start();
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to start event: " << event << std::endl;
        }
    }

    void AudioSystem::SetEventPosition(EventHandle event, const glm::vec3 &position, const glm::vec3 &velocity)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = eventInstanceIt->second;

        FMOD_3D_ATTRIBUTES attributes = {};
        glm::vec3 convertedPosition = position * c_unitConversion;
        attributes.position = {convertedPosition.x, convertedPosition.y, convertedPosition.z};

        glm::vec3 convertedVelocity = velocity * c_unitConversion;
        attributes.velocity = {convertedVelocity.x, convertedVelocity.y, convertedVelocity.z};

        attributes.forward = {0.0f, 0.0f, -1.0f};
        attributes.up = {0.0f, 1.0f, 0.0f};

        FMOD_RESULT result = eventInstance->set3DAttributes(&attributes);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to set 3D attributes for event: " << event << std::endl;
        }
    }

    void AudioSystem::SetEventParameter(EventHandle event, const std::string &parameter, float value)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = eventInstanceIt->second;

        FMOD_RESULT result = eventInstance->setParameterByName(parameter.c_str(), value);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to set parameter: " << parameter << " for event: " << event << std::endl;
        }
    }

    void AudioSystem::StopEvent(EventHandle event, bool allowFadeOut)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        FMOD::Studio::EventInstance *eventInstance = eventInstanceIt->second;

        FMOD_STUDIO_STOP_MODE stopMode = allowFadeOut ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE;
        FMOD_RESULT result = eventInstance->stop(stopMode);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to stop event: " << event << std::endl;
        }
    }

    void AudioSystem::ReleaseEvent(EventHandle event)
    {
        auto eventInstanceIt = m_events.find(event);
        if (eventInstanceIt == m_events.end())
        {
            std::cerr << "Event not found: " << event << std::endl;
            return;
        }

        eventInstanceIt->second->release();
        m_events.erase(eventInstanceIt);
    }

    void AudioSystem::StopAllEvents(bool allowFadeOut)
    {
        FMOD_STUDIO_STOP_MODE stopMode = allowFadeOut ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE;
        for (const auto &eventInstance : m_events)
        {
            eventInstance.second->stop(stopMode);
        }
    }

    void AudioSystem::ReleaseAllEvents()
    {
        for (const auto &eventInstance : m_events)
        {
            eventInstance.second->release();
        }

        m_events.clear();
    }

    void AudioSystem::SetListenerState(const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &velocity)
    {
        FMOD_3D_ATTRIBUTES attributes = {};

        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 convertedPosition = position * c_unitConversion;
        glm::vec3 convertedVelocity = velocity * c_unitConversion;

        attributes.forward = {forward.x, forward.y, forward.z};
        attributes.up = {up.x, up.y, up.z};
        attributes.position = {convertedPosition.x, convertedPosition.y, convertedPosition.z};
        attributes.velocity = {convertedVelocity.x, convertedVelocity.y, convertedVelocity.z};

        FMOD_RESULT result = m_system->setListenerAttributes(0, &attributes);
        if (result != FMOD_OK)
        {
            std::cerr << "Failed to set listener attributes" << std::endl;
        }
    }
}