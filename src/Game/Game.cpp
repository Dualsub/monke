#include "Game/Game.h"

#include "Application.h"
#include "Vultron/SceneRenderer.h"
#include "Audio/AudioSystem.h"
#include "Input/InputDevice.h"
#include "Physics/PhysicsWorld.h"

namespace mk
{
    void Game::OnInitialize()
    {
        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        renderer.CreateMaterial<FontSpriteMaterial>(
            "FontMaterial",
            {
                .fontAtlas = renderer.LoadFontAtlas(MK_ASSET_PATH("ui/font_msdf.dat")),
            });

        m_stateMachine.Visit([](auto &state)
                             { GameStateImpl::OnEnter(state); });
    }

    void Game::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld)
    {
        m_stateMachine.Visit([&](auto &state)
                             { GameStateImpl::OnFixedUpdate(dt, numSteps, physicsWorld, state); });
    }

    void Game::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState)
    {
        m_stateMachine.Transition<GameStateImpl>();
        m_stateMachine.Visit([&](auto &state)
                             { GameStateImpl::OnUpdate(dt, audioSystem, physicsWorld, inputState, state); });

        if (m_queuedState.has_value())
        {
            std::visit(
                [&](auto &&state)
                {
                    m_stateMachine.SetState<GameStateImpl>(std::move(state));
                },
                m_queuedState.value());
            m_queuedState = std::nullopt;
        }
    }

    void Game::OnRender(SceneRenderer &renderer)
    {
        m_stateMachine.Visit([&](auto &state)
                             { GameStateImpl::OnRender(renderer, state); });
    }

    void Game::OnShutdown()
    {
        m_stateMachine.Visit([](auto &state)
                             { GameStateImpl::OnExit(state); });
    }

    void Game::GoToMainMenu()
    {
    }

    void Game::RestartGame()
    {
    }
}
