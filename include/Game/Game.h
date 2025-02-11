#pragma once

#include "Game/StateMachines/GameStateMachine.h"

#include <fstream>
#include <cstdint>

#define MK_ASSET_PATH(path) (MK_ASSETS_DIR "/" path)

namespace Vultron
{
    class SceneRenderer;
}

namespace mk
{
    class AudioSystem;
    class InputState;
    class PhysicsWorld;

    struct PersistentData
    {
        // Add persistent data here

        void Read(std::fstream &file)
        {
            // Read persistent data from file
        }

        void Write(std::fstream &file)
        {
            // Write persistent data to file
        }
    };

    class Game
    {
    public:
        using PersistentDataType = PersistentData;

    private:
        GameStateMachine m_stateMachine;
        GameStateMachine::OptionalState m_queuedState;

    public:
        Game() = default;
        ~Game() = default;

        void OnInitialize();
        void OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld);
        void OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState);
        void OnRender(Vultron::SceneRenderer &renderer);
        void OnShutdown();

        void GoToMainMenu();
        void RestartGame();
    };

}