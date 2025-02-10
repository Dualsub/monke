#pragma once

#include <fstream>
#include <cstdint>

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