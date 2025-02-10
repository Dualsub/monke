#pragma once

#include "Core/CmdArgs.h"
#include "Core/EventBus.h"
#include "Game/Game.h"
#include "Audio/AudioSystem.h"
#include "Input/InputDevice.h"
#include "Physics/PhysicsWorld.h"

#include "Vultron/Vultron.h"
#include "Vultron/SceneRenderer.h"
#include "Vultron/Window.h"

#include <cstdint>
#include <variant>
#include <vector>
#include <random>

using namespace Vultron;

namespace ACS
{

    constexpr float c_fixedUpdateInterval = 1.0f / 60.0f;
    constexpr uint32_t c_maxSubSteps = 6;

    constexpr float c_debugInfoUpdateInterval = 1.0f;

    constexpr const char *c_saveFileName = "save.dat";

    struct DebugSample
    {
        float physicsTime;
        float updateTime;
        float renderTime;
        float totalTime;
    };

    struct DebugInfo
    {
        float physicsTime;
        float updateTime;
        float renderTime;
        float totalTime;
    };

    class Application
    {
    private:
        static Application *s_instance;

        struct
        {
            std::random_device rd;
            std::mt19937 gen;
            std::uniform_real_distribution<float> dis;
        } m_random = {};

        SceneRenderer m_renderer;
        Window m_window;
        PhysicsWorld m_physicsWorld;
        InputDevice m_inputDevice;
        AudioSystem m_audioSystem;
        Game m_game;
        std::vector<Game::EventType> m_eventQueue;
        EventBus m_eventBus;
        Game::PersistentDataType m_persistentData;

        CmdArgs m_cmdArgs;
        DebugInfo m_debugInfo;

        float m_minUpdateRate = 1.0f / 20.0f;
        double m_maxUpdateRate = 1.0 / 144.0;
        float m_timeSincePhysics = 0.0f;
        float m_timeScale = 1.0f;
        float m_deltaTime = 0.0f;

        bool Initialize();
        void Shutdown();
        void FixedUpdate(float dt, uint32_t numSubSteps);
        void Update(float dt);
        void Render();

    public:
        Application() { s_instance = this; }
        ~Application() { s_instance = nullptr; }

        static const SceneRenderer &GetRenderer() { return s_instance->m_renderer; }
        static Window &GetWindow() { return s_instance->m_window; }
        static PhysicsWorld &GetPhysicsWorld() { return s_instance->m_physicsWorld; }
        static InputDevice &GetInputDevice() { return s_instance->m_inputDevice; }
        static AudioSystem &GetAudioSystem() { return s_instance->m_audioSystem; }
        static EventBus &GetEventBus() { return s_instance->m_eventBus; }
        static Game &GetGame() { return s_instance->m_game; }
        static const CmdArgs &GetCmdArgs() { return s_instance->m_cmdArgs; }
        static DebugInfo &GetDebugInfo() { return s_instance->m_debugInfo; }
        static float GetTimeSincePhysics() { return s_instance->m_timeSincePhysics; }
        static float GetTimeScale() { return s_instance->m_timeScale; }
        static float GetDeltaTime() { return s_instance->m_deltaTime; }

        template <typename T>
        static void DispatchEvent(const T &event)
        {
            static_assert(!std::is_same_v<T, Game::EventType>, "Old event type used");
            s_instance->m_eventBus.Dispatch(event);
        }

        template <typename T>
        static void QueueEvent(const T &event)
        {
            static_assert(!std::is_same_v<T, Game::EventType>, "Old event type used");
            s_instance->m_eventBus.QueueEvent(event);
        }

        static void SetPersistentData(const Game::PersistentDataType &data)
        {
            s_instance->m_persistentData = data;
        }

        static const Game::PersistentDataType &GetPersistentData()
        {
            return s_instance->m_persistentData;
        }

        static void ReadPersistentData()
        {
            std::fstream file(c_saveFileName, std::ios::in | std::ios::binary);
            if (file.is_open())
            {
                s_instance->m_persistentData.Read(file);
                file.close();
            }
            else
            {
                s_instance->m_persistentData = {};
            }
        }

        static std::future<void> WritePersistentData()
        {
            return std::async(
                std::launch::async,
                []()
                {
                    std::fstream file(c_saveFileName, std::ios::out | std::ios::binary);
                    if (file.is_open())
                    {
                        s_instance->m_persistentData.Write(file);
                        file.close();
                    }
                });
        }

        static void Quit() { s_instance->m_window.SetShouldClose(true); }
        static void SetTimeScale(float timeScale) { s_instance->m_timeScale = timeScale; }

        static float GetRandomFloat() { return s_instance->m_random.dis(s_instance->m_random.gen); }

        uint32_t Run(int argc, char **argv);
    };
}