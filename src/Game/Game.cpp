#include "Game/Game.h"

#include "Vultron/SceneRenderer.h"
#include "Audio/AudioSystem.h"
#include "Input/InputDevice.h"
#include "Physics/PhysicsWorld.h"

namespace mk
{
    void Game::OnInitialize()
    {
    }

    void Game::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld)
    {
    }

    void Game::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState)
    {
    }

    void Game::OnRender(SceneRenderer &renderer)
    {
    }

    void Game::OnShutdown()
    {
    }

    void Game::GoToMainMenu()
    {
    }

    void Game::RestartGame()
    {
    }
}
