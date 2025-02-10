#include "Application.h"

#include "Core/Logger.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <random>

using namespace Vultron;

namespace ACS
{
    Application *Application::s_instance = nullptr;

    bool Application::Initialize()
    {
        m_random.gen = std::mt19937(m_random.rd());
        m_random.dis = std::uniform_real_distribution<float>(0.0f, 1.0f);

        Window::WindowCreateInfo windowCreateInfo = {
            .title = "Arcane Siege",
            .mode = Window::WindowMode::Fullscreen,
        };

        if (m_cmdArgs.HasFlag("--windowed"))
        {
            windowCreateInfo.mode = Window::WindowMode::Windowed;
            windowCreateInfo.width = 1920;
            windowCreateInfo.height = 1080;
        }

        // #ifdef DEBUG
        //         windowCreateInfo.mode = Window::WindowMode::Windowed;
        //         windowCreateInfo.width = 2304;
        //         windowCreateInfo.height = 1296;
        // #endif

        if (!m_window.Initialize(windowCreateInfo))
        {
            std::cerr << "Window failed to initialize" << std::endl;
            return false;
        }

        if (!m_renderer.Initialize(m_window))
        {
            std::cerr << "Renderer failed to initialize" << std::endl;
            return false;
        }

        // #ifdef DEBUG
        // m_renderer.SetDebugCallback([](const std::string &message)
        //                             { Logger::Print(message); });
        // #endif

        if (!m_audioSystem.Initialize())
        {
            std::cerr << "AudioSystem failed to initialize" << std::endl;
            return false;
        }

        if (!m_inputDevice.Initialize(m_window))
        {
            std::cerr << "InputDevice failed to initialize" << std::endl;
            return false;
        }

        m_physicsWorld.Initialize();

        m_game.OnInitialize();

        return true;
    }

    void Application::Update(float dt)
    {
        Logger::GetInstance().Update(dt);
        const InputState &inputState = m_inputDevice.GetInputState();
        m_game.OnUpdate(dt, m_audioSystem, m_physicsWorld, inputState);

        m_eventBus.Update();
        m_audioSystem.Update();
    }

    void Application::FixedUpdate(float dt, uint32_t numSubSteps)
    {
        m_game.OnFixedUpdate(dt, numSubSteps, m_physicsWorld);
    }

    void Application::Render()
    {
        m_game.OnRender(m_renderer);
    }

    void Application::Shutdown()
    {
        m_game.OnShutdown();
        m_physicsWorld.Shutdown();
        m_audioSystem.Shutdown();
        m_renderer.Shutdown();
        m_window.Shutdown();
    }

    uint32_t Application::Run(int argc, char **argv)
    {
        m_cmdArgs = CmdArgs::Parse(argc, argv);

        if (!Initialize())
        {
            return EXIT_FAILURE;
        }

        std::vector<DebugSample> debugSamples;
        debugSamples.reserve(1000);
        std::chrono::high_resolution_clock debugClock;
        auto debugInfoLastUpdate = debugClock.now();

        std::chrono::high_resolution_clock clock;
        auto lastTime = clock.now();
        float timeSincePhysics = 0.0f;

        auto currentTime = clock.now();
        while (!m_window.ShouldShutdown())
        {
            const auto maxUpdateDuration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(m_maxUpdateRate));
            const auto timeTaken = std::chrono::duration_cast<std::chrono::microseconds>(clock.now() - currentTime);

            if (timeTaken < maxUpdateDuration)
            {
                auto nextFrame = currentTime + maxUpdateDuration;
                while (clock.now() < nextFrame)
                {
                    std::this_thread::yield();
                }
            }

            currentTime = clock.now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            deltaTime = glm::min(deltaTime, m_minUpdateRate);
            lastTime = currentTime;
            m_deltaTime = deltaTime;

            m_window.PollEvents();
            if (m_window.IsMinimized())
            {
                continue;
            }

            m_inputDevice.QueryInputState(m_window, deltaTime);

            auto start = clock.now();

            auto physicsStart = debugClock.now();

            m_physicsWorld.ResetContacts();
            timeSincePhysics += deltaTime;
            if (timeSincePhysics >= c_fixedUpdateInterval)
            {
                const uint32_t numSubSteps = static_cast<uint32_t>(timeSincePhysics / c_fixedUpdateInterval);
                const float physicsDeltaTime = numSubSteps * c_fixedUpdateInterval;
                timeSincePhysics -= physicsDeltaTime;
                FixedUpdate(physicsDeltaTime * m_timeScale, glm::min(numSubSteps, c_maxSubSteps));
            }

            m_timeSincePhysics = timeSincePhysics;

            auto physicsEnd = debugClock.now();

            auto updateStart = debugClock.now();

            Update(deltaTime * m_timeScale);

            auto updateEnd = debugClock.now();

            auto renderStart = debugClock.now();

            m_renderer.SetFramebufferResized(m_window.IsResized());

            m_renderer.BeginFrame();

            Render();

            m_renderer.EndFrame();

            auto renderEnd = debugClock.now();

            auto end = clock.now();

            debugSamples.push_back(DebugSample{
                .physicsTime = std::chrono::duration<float, std::chrono::milliseconds::period>(physicsEnd - physicsStart).count(),
                .updateTime = std::chrono::duration<float, std::chrono::milliseconds::period>(updateEnd - updateStart).count(),
                .renderTime = std::chrono::duration<float, std::chrono::milliseconds::period>(renderEnd - renderStart).count(),
                .totalTime = std::chrono::duration<float, std::chrono::milliseconds::period>(end - start).count(),
            });

            if (std::chrono::duration<float>(debugClock.now() - debugInfoLastUpdate).count() > c_debugInfoUpdateInterval)
            {
                float physicsTime = 0.0f;
                float updateTime = 0.0f;
                float renderTime = 0.0f;
                float totalTime = 0.0f;
                for (const auto &sample : debugSamples)
                {
                    physicsTime += sample.physicsTime;
                    updateTime += sample.updateTime;
                    renderTime += sample.renderTime;
                    totalTime += sample.totalTime;
                }

                m_debugInfo.physicsTime = physicsTime / debugSamples.size();
                m_debugInfo.updateTime = updateTime / debugSamples.size();
                m_debugInfo.renderTime = renderTime / debugSamples.size();
                m_debugInfo.totalTime = totalTime / debugSamples.size();

                debugSamples.clear();
                debugInfoLastUpdate = debugClock.now();
            }
        }

        Shutdown();

        return EXIT_SUCCESS;
    }
}
